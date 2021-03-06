# Tips
把 label 赋值给寄存器时，要注意 `movl label, %eax` 和 `movl $label, %eax` 的差别，前者赋值 label 下的内容，后者赋值 label 本身。

# x86\_64
    long int simple_l(long int *xp, long int y)
    {
		long int t = *xp + y;
		*xp = t;
		return t;
    }

分别用 `gcc -O2 -m32 -S` 和 `gcc -O2 -m64 -S` 编译后可见差别。

* IA32 从右到左 push 参数入栈
* x86\_64 64bit 寄存器 rax，rcx，rdx，rbx，rsp，rbp，rsi，rdi，r8 ~ r15，
  * 原有寄存器取不同长度：`rax`、`eax`、`ax`、`ah`、`al`，还有 `sil`、`dil`、`spl`、`bpl`
  * 新寄存器取不同长度：`r8`、`r8d`、`r8w`、`r8b`
* x86\_64 返回值保存在 rax
* x86\_64 栈按 16 字节对齐
* 寄存器使用约定
  * `rax`：return value
  * `rbx`：callee saved
  * `rcx`：4th argument （传参顺序：`rdi`、`rsi`、`rdx`、`rcx`、`r8`、`r9`）
  * `rdx`：3rd argument
  * `rsi`：2nd argument
  * `rdi`：1st argument
  * `rbp`：callee saved
  * `rsp`：stack pointer
  * `r8` ：5th argument
  * `r9` ：6th argument
  * `r10`：callee saved
  * `r11`：used for linking
  * `r12`：unused for C
  * `r13`：callee saved
  * `r14`：callee saved
  * `r15`：callee saved

`movq` 对 32bit 立即数进行符号扩展。

`?:` 操作符不能被编译为 conditional move 指令，因为 condition move 指令会导致操作数被求值，
所以类似 `value = ptr ? *ptr : 0;` 的语句，无论 `ptr` 是否为 `NULL` 都会被解引用。
同样，当 `?:` 中有副作用时，例如 `return x < y ? (counter++, y) : x;`， 其中 `counter++`
必须在条件判断为真时执行，所以也只能编译为分支。如果真假两个分支动作有大量运算，conditional move
会求值两个，影响效率。

# Red zone
Red zone 即 `rsp` 开始往下走 128 字节的区域。程序可以假设这个区域不会被中断和信号处理修改，
从而用来传递数据并通过 rsp 和偏移量来访问，不用减 rsp 再加回来。当然这个区域会被函数调用修改，
所以 red zone 一般都是在 leaf functions 里用，即不再调用其他函数的函数使用。

# `rbp`
`rbp` 用来维护 stack frame，也让调试器可以找到函数的调用关系。但编译器可以只用 `rsp`，而且 DWARF 调试格式
也支持不用 `rbp`。GCC 可以通过 `-fomit-frame-pointer` 选项让编译器不使用 `rbp`。

## Recall x86
* `eax edx ecx` caller saved，其他 callee saved
* 参数传递
  * C declaration：都传到栈上，`eax` 存返回值，若返回 64bit 则高 32bit 存 `edx`
