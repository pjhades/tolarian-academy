# ftrace

- If `CONFIG_DYNAMIC_FTRACE` is enabled, when compiling the kernel, `gcc -pg` inserts extra function call to `mcount()`
- Kernel reimplements `mcount()`
- The call sites of all `mcount()` calls are recorded
- At boot time the calls are converted to NOPs
- Either `mcount()` or NOP will be inserted into the recorded call sites
  depending on if function or function graph tracer is enabled
- `trace_printk()` writes to a ring buffer, avoiding down sides of `printk()`


# Tracepoint

Tracepoints are static hooks in the kernel code
to which a probe (user defined function) could be registered/unregistered.
A probe has to follow a predefined prototype. Like:

```c
void trace_block_touch_buffer(struct buffer_head * bh)
```

will be called from `touch_buffer()`.

New tracepoints and their associated probe prototypes could be defined in the kernel (recompilation needed).

See example tracepoint APIs [here](https://www.kernel.org/doc/html/latest/core-api/tracepoint.html).

You can list tracepoints with:

```bash
perf list
```

# kprobe

kprobes are dynamic probes, where a running kernel is patched
and a user-defined probe is run. The instrumenting code is compiled
into a kernel module, whose initializing function registers the probe.

A kprobe is registered with `register_kprobe()`, which does:

- copy the instruction at the probed address (`prepare_kprobe()`)
- insert the kprobe into a hash table called `kprobe_table`, keyed
  by the probed address
- modify the live kernel text with `arm_kprobe`, whose architecture-dependent
  version on x86\_64 writes `0xcc`, the opcode of an INT 3 software interrupt,
  to the probed address. Note that INT 3 has only 1-byte opcode which does not
  follow the usual 2-byte encoding of INT instructions. Since INT 3 is expected
  to be used for calling debug exception handler, this 1-byte opcode makes it
  easy to replace any instruction with a breakpoint. The normal 2-byte encoding
  of INT 3 (`0xcd 0x03`) does not have these special features.
  [More detail here.](https://x86.puri.sm/html/file_module_x86_id_142.html)

Thus when the INT 3 is hit, an interrupt handler will be called.
The execution is then passed to kprobe.

A kretprobe is registered with `register_kretprobe()`, which saves the real return
address and sets up a normal kprobe whose `pre_handler` is set to calling into a trampoline.
The trampoline calls the specified user probe function, and later finds the saved return
address in a hash table, restoring the real return address.


# uprobe

Similar to kprobe, but allows to register probes against user-space code.
The mechanism is similar to kprobe: replacing the instruction at the probed
address with a breakpoint (opcode `0xcc`).
