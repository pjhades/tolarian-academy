# LLVM's Implementation of C++ Exception Handling

The code is better read after [IA64 C++ ABI specification](https://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html).

If the program `throw`s, once an exception object is created, it calls the level-2 interface `__cxa_throw`,
which in turn calls the level-1 interface `_Unwind_RaiseException`.
There are two data structures used to store information about a call frame:

- `unw_context_t` that stores the values of the registers
- `unw_cursor_t` that serves as a buffer to construct a `UnwindCursor` in place. `UnwindCursor`, like a database cursor, points to a call frame and knows about the CFI

The cursor serves as the handle for searching in the call stack.
Its constructed from a context is done with `__unw_getcontext`, implemented directly in assembly in libunwind/src/UnwindRegistersSave.S.
The cursor is passed among various level-1 interfaces as the `_Unwind_Context` object.

`_Unwind_RaiseException` is easy to understand:

```C++
/// Called by __cxa_throw.  Only returns if there is a fatal error.
_LIBUNWIND_EXPORT _Unwind_Reason_Code
_Unwind_RaiseException(_Unwind_Exception *exception_object) {
  // ...
  unw_context_t uc;
  unw_cursor_t cursor;
  __unw_getcontext(&uc);

  // Mark that this is a non-forced unwind, so _Unwind_Resume()
  // can do the right thing.
  exception_object->private_1 = 0;
  exception_object->private_2 = 0;

  // phase 1: the search phase
  _Unwind_Reason_Code phase1 = unwind_phase1(&uc, &cursor, exception_object);
  if (phase1 != _URC_NO_REASON)
    return phase1;

  // phase 2: the clean up phase
  return unwind_phase2(&uc, &cursor, exception_object);
}
```

This is how a cursor is initialized with `__unw_init_local` at the beginning:

```C++
/// Create a cursor of a thread in this process given 'context' recorded by
/// __unw_getcontext().
_LIBUNWIND_HIDDEN int __unw_init_local(unw_cursor_t *cursor,
                                       unw_context_t *context) {
  // ...
# define REGISTER_KIND Registers_x86_64
  // ...
  // Use "placement new" to allocate UnwindCursor in the cursor buffer.
  new (reinterpret_cast<UnwindCursor<LocalAddressSpace, REGISTER_KIND> *>(cursor))
      UnwindCursor<LocalAddressSpace, REGISTER_KIND>(
          context, LocalAddressSpace::sThisAddressSpace);
#undef REGISTER_KIND
  AbstractUnwindCursor *co = (AbstractUnwindCursor *)cursor;
  co->setInfoBasedOnIPRegister();

  return UNW_ESUCCESS;
}
```

Note the call to `setInfoBasedOnIPRegister` at the end, where DWARF CFI is consulted,
and the information from the CIE and FDE is stored in `cursor._info`, a struct of type
`unw_proc_info_t`.

```C++
template <typename A, typename R>
void UnwindCursor<A, R>::setInfoBasedOnIPRegister(bool isReturnAddress) {
  // ...
  // This PC points to the instruction next to `__unw_getcontext`
  pint_t pc = static_cast<pint_t>(this->getReg(UNW_REG_IP));
  // ...

  // Ask address space object to find unwind sections for this pc.
  UnwindInfoSections sects;
  if (_addressSpace.findUnwindSections(pc, sects)) {
    // ...
#if defined(_LIBUNWIND_SUPPORT_DWARF_UNWIND)
    // If there is dwarf unwind info, look there next.
    if (sects.dwarf_section != 0) {
      if (this->getInfoFromDwarfSection(pc, sects)) {
        // found info in dwarf, done
        return;
      }
    }
#endif
    // ...
  }

#if defined(_LIBUNWIND_SUPPORT_DWARF_UNWIND)
  // There is no static unwind info for this pc. Look to see if an FDE was
  // dynamically registered for it.
  pint_t cachedFDE = DwarfFDECache<A>::findFDE(DwarfFDECache<A>::kSearchAll,
                                               pc);
  if (cachedFDE != 0) {
    typename CFI_Parser<A>::FDE_Info fdeInfo;
    typename CFI_Parser<A>::CIE_Info cieInfo;
    if (!CFI_Parser<A>::decodeFDE(_addressSpace, cachedFDE, &fdeInfo, &cieInfo))
      if (getInfoFromFdeCie(fdeInfo, cieInfo, pc, 0))
        return;
  }

  // Lastly, ask AddressSpace object about platform specific ways to locate
  // other FDEs.
  pint_t fde;
  if (_addressSpace.findOtherFDE(pc, fde)) {
    typename CFI_Parser<A>::FDE_Info fdeInfo;
    typename CFI_Parser<A>::CIE_Info cieInfo;
    if (!CFI_Parser<A>::decodeFDE(_addressSpace, fde, &fdeInfo, &cieInfo)) {
      // Double check this FDE is for a function that includes the pc.
      if ((fdeInfo.pcStart <= pc) && (pc < fdeInfo.pcEnd))
        if (getInfoFromFdeCie(fdeInfo, cieInfo, pc, 0))
          return;
    }
  }
#endif // #if defined(_LIBUNWIND_SUPPORT_DWARF_UNWIND)
  // ...
  // no unwind info, flag that we can't reliably unwind
  _unwindInfoMissing = true;
}
```

Now we have prepared our cursor, so we have knowledge about the call frame, it's time to find the landing pad.
Note that the very first call frame, which was stored in the context, corresponds to `_Unwind_RaiseException`, which we can safely skip.
During the search, we keep getting the next frame as the CFI instructs and updating the cursor.
This is done by `__unw_step`, which is similar to the initialization of the cursor: the CFI is first consulted and then a call to `setInfoBasedOnIPRegister` is made to update it.

For each frame, the personality routine `__gxx_personality_v0` is called to determine if the search should stop at it.
The stack pointer is saved in the level-1 exception header for use in phase 2.

```C++
static _Unwind_Reason_Code
unwind_phase1(unw_context_t *uc, unw_cursor_t *cursor, _Unwind_Exception *exception_object) {
  __unw_init_local(cursor, uc);

  // Walk each frame looking for a place to stop.
  while (true) {
    // Ask libunwind to get next frame (skip over first which is
    // _Unwind_RaiseException).
    int stepResult = __unw_step(cursor);
    if (stepResult == 0) {
      // ...
      return _URC_END_OF_STACK;
    } else if (stepResult < 0) {
      // ...
      return _URC_FATAL_PHASE1_ERROR;
    }

    // See if frame has code to run (has personality routine).
    unw_proc_info_t frameInfo;
    unw_word_t sp;
    if (__unw_get_proc_info(cursor, &frameInfo) != UNW_ESUCCESS) {
      // ...
      return _URC_FATAL_PHASE1_ERROR;
    }
    // ...
    // If there is a personality routine, ask it if it will want to stop at
    // this frame.
    if (frameInfo.handler != 0) {
      _Unwind_Personality_Fn p =
          (_Unwind_Personality_Fn)(uintptr_t)(frameInfo.handler);
      // ...
      _Unwind_Reason_Code personalityResult =
          (*p)(1, _UA_SEARCH_PHASE, exception_object->exception_class,
               exception_object, (struct _Unwind_Context *)(cursor));
      switch (personalityResult) {
      case _URC_HANDLER_FOUND:
        // found a catch clause or locals that need destructing in this frame
        // stop search and remember stack pointer at the frame
        __unw_get_reg(cursor, UNW_REG_SP, &sp);
        exception_object->private_2 = (uintptr_t)sp;
        // ...
        return _URC_NO_REASON;

      case _URC_CONTINUE_UNWIND:
        // ...
        // continue unwinding
        break;

      default:
        // something went wrong
        // ...
        return _URC_FATAL_PHASE1_ERROR;
      }
    }
  }
  return _URC_NO_REASON;
}
```

How does the personality routine makes the decision?
Such decisions are language-dependent, so it needs some context.
This information is encoded in the binary as well, called the language-specific data area (LSDA).
The LSDA is embedded in DWARF as the augmentation field of CIE.
Once the information is read, it is saved on the level-2 exception header `__cxa_exception`,
to be used later in phase 2.

```C++
// ...
_LIBCXXABI_FUNC_VIS _Unwind_Reason_Code
// ...
__gxx_personality_v0
// ...
                    (int version, _Unwind_Action actions, uint64_t exceptionClass,
                     _Unwind_Exception* unwind_exception, _Unwind_Context* context)
{
    if (version != 1 || unwind_exception == 0 || context == 0)
        return _URC_FATAL_PHASE1_ERROR;

    bool native_exception = (exceptionClass     & get_vendor_and_language) ==
                            (kOurExceptionClass & get_vendor_and_language);
    scan_results results;
    // ...
    // In other cases we need to scan LSDA.
    scan_eh_tab(results, actions, native_exception, unwind_exception, context);
    if (results.reason == _URC_CONTINUE_UNWIND ||
        results.reason == _URC_FATAL_PHASE1_ERROR)
        return results.reason;

    if (actions & _UA_SEARCH_PHASE)
    {
        // Phase 1 search:  All we're looking for in phase 1 is a handler that
        //   halts unwinding
        assert(results.reason == _URC_HANDLER_FOUND);
        if (native_exception) {
            // For a native exception, cache the LSDA result.
            __cxa_exception* exc = (__cxa_exception*)(unwind_exception + 1) - 1;
            exc->handlerSwitchValue = static_cast<int>(results.ttypeIndex);
            exc->actionRecord = results.actionRecord;
            exc->languageSpecificData = results.languageSpecificData;
            exc->catchTemp = reinterpret_cast<void*>(results.landingPad);
            exc->adjustedPtr = results.adjustedPtr;
            // ...
        }
        return _URC_HANDLER_FOUND;
    }
    // ...
}
```

Now phase 1 is done, we start phase 2. Again, a cursor is constructed. We use the cursor to visit each frame,
and ask the personality routine to do something depending on the actions passed to it. If the saved stack pointer
is reached, the personality routine is passed an additional `_UA_HANDLER_FRAME` action so that it knows it should
set up the program counter to prepare for the jump to the landing pad:

```C++
// ...
_LIBCXXABI_FUNC_VIS _Unwind_Reason_Code
// ...
__gxx_personality_v0
// ...
                    (int version, _Unwind_Action actions, uint64_t exceptionClass,
                     _Unwind_Exception* unwind_exception, _Unwind_Context* context)
{
    // ...
    bool native_exception = (exceptionClass     & get_vendor_and_language) ==
                            (kOurExceptionClass & get_vendor_and_language);
    scan_results results;
    // Process a catch handler for a native exception first.
    if (actions == (_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME) &&
        native_exception) {
        // Reload the results from the phase 1 cache.
        __cxa_exception* exception_header =
            (__cxa_exception*)(unwind_exception + 1) - 1;
        results.ttypeIndex = exception_header->handlerSwitchValue;
        results.actionRecord = exception_header->actionRecord;
        results.languageSpecificData = exception_header->languageSpecificData;
        results.landingPad =
            reinterpret_cast<uintptr_t>(exception_header->catchTemp);
        results.adjustedPtr = exception_header->adjustedPtr;

        // Jump to the handler.
        set_registers(unwind_exception, context, results);
        // Cache base for calculating the address of ttype in
        // __cxa_call_unexpected.
        if (results.ttypeIndex < 0) {
          // ...
          exception_header->catchTemp = 0;
          // ...
        }
        return _URC_INSTALL_CONTEXT;
    }
    // ...
}
```

Once the personality routine returns `_URC_INSTALL_CONTEXT`, phase 2 transfers control to the landing pad
by calling `__unw_phase2_resume`. On x86-64, it ultimately calls `__libunwind_Registers_x86_64_jumpto`, which is defined in libunwind/src/UnwindRegistersRestore.S.

```C++
static _Unwind_Reason_Code
unwind_phase2(unw_context_t *uc, unw_cursor_t *cursor, _Unwind_Exception *exception_object) {
  // ...
  // Walk each frame until we reach where search phase said to stop.
  while (true) {
    // ...

    // If there is a personality routine, tell it we are unwinding.
    if (frameInfo.handler != 0) {
      _Unwind_Personality_Fn p =
          (_Unwind_Personality_Fn)(uintptr_t)(frameInfo.handler);
      _Unwind_Action action = _UA_CLEANUP_PHASE;
      if (sp == exception_object->private_2) {
        // Tell personality this was the frame it marked in phase 1.
        action = (_Unwind_Action)(_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME);
      }
       _Unwind_Reason_Code personalityResult =
          (*p)(1, action, exception_object->exception_class, exception_object,
               (struct _Unwind_Context *)(cursor));
      switch (personalityResult) {
      case _URC_CONTINUE_UNWIND:
        // Continue unwinding
        // ...
        break;
      case _URC_INSTALL_CONTEXT:
        // ...
        // Personality routine says to transfer control to landing pad.
        // We may get control back if landing pad calls _Unwind_Resume().
        // ...

        __unw_phase2_resume(cursor, framesWalked);
        // __unw_phase2_resume() only returns if there was an error.
        return _URC_FATAL_PHASE2_ERROR;
      default:
        // Personality routine returned an unknown result code.
        _LIBUNWIND_DEBUG_LOG("personality function returned unknown result %d",
                             personalityResult);
        return _URC_FATAL_PHASE2_ERROR;
      }
    }
  }

  // Clean up phase did not resume at the frame that the search phase
  // said it would...
  return _URC_FATAL_PHASE2_ERROR;
}
```

# How GCC's `backtrace(3)` Is Affected By Command-line Arguments

With this test program:

```C
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <execinfo.h>

void trace() {
    void *bt[1024];
    int n = backtrace(bt, 1024);
    char **names = backtrace_symbols(bt, n);
    for (size_t i = 0; i < n; i++)
        printf("%s\n", names[i]);
    free(names);
}

void baz(jmp_buf jb) {
    trace();
    //longjmp(jb, 999);
    printf("baz\n");
}

void bar(jmp_buf jb) {
    baz(jb);
    printf("bar\n");
}

void foo(jmp_buf jb) {
    bar(jb);
    printf("foo\n");
}

int main() {
    jmp_buf jb;
    int x;
    if ((x = setjmp(jb)) != 0) {
        printf("come from longjmp, x=%d\n", x);
    } else {
        foo(jb);
        printf("main\n");
    }
    return 0;
}
```

Only when `-fno-asynchronous-unwind-tables` and `-fno-unwind-tables` are specified will `backtrace(3)` fail.
Specifying either solely won't work.

```
10:16:24:mitochondrion:t $ gcc -g -o test -rdynamic -fno-asynchronous-unwind-tables -fno-unwind-tables test.c
10:16:52:mitochondrion:t $ ./test
./test(trace+0x30) [0xaaaab13a0c04]
baz
bar
foo
main
```

# Helpful Reading Material
1. [Exception Handling in LLVM](https://llvm.org/docs/ExceptionHandling.html#introduction)
2. [C++ Exception Handling for IA-64](https://www.usenix.org/legacy/events/wiess2000/full_papers/dinechin/dinechin.pdf)
3. [Dwarf2 Exception Handler HOWTO](https://gcc.gnu.org/wiki/Dwarf2EHNewbiesHowto) - This can be read together with [the code](https://github.com/gcc-mirror/gcc/blob/master/libgcc/unwind.inc).
4. [An overview of unwinding](https://maskray.me/blog/2020-11-08-stack-unwinding)
5. [Unwinding the stack the hard way](https://lesenechal.fr/en/linux/unwinding-the-stack-the-hard-way)
