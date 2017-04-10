格式：

    asm ( assembler template 
        : output operands                  /* optional */
        : input operands                   /* optional */
        : list of clobbered registers      /* optional */
   	);

* Template 用双引号包围，`\r\t` 分隔指令  `"...\r\t" "...\r\t"`
* C 相关的变量操作数用 `%0`、`%1` …… 获取
* Template 中的寄存器名前多加一个 % 以便和操作数序号区分
* Clobber list 指的是 GCC 不知道被修改过的寄存器列表
  其中不包括在 input 和 output 部分指定的寄存器，也不包括 GCC 自己分配的寄存器
* 如果修改了 conditional flags register，要在 clobber list 中指定 `cc`
* 指定 `memory` 将防止 GCC 在执行指令期间把内存数据缓存在寄存器中，没有在 input 和 output 中指定的内存读写需要指定 `memory`
* 为了保证内存数据的正确性，GCC 在执行 inline assembly 之前会将特定寄存器的值刷入内存，
  也不会假设已读入寄存器的值是正确的，因此会重新读。所以仅指定 `memory` 可作为编译器层面的 barrier。但不会影响 CPU
* 将整个汇编区域声明为 `volatile` 将防止 GCC 做优化。如果汇编指令仅包含简单计算，没有什么副作用，不必声明为 `volatile`。


Operand constraints：
* `r` 存于寄存器
* `a`、`b`、`c`、`d`、`S`、`D` 分别指定 eax、ebx、ecx、edx、esi、edi
* `m` 存于内存
* `0 - 9` 指定该操作数存于同序号指定的操作数一样的寄存器，可在数字前加 modifier
  * `%b0` al
  * `%h0` ah
  * `%w0` ax
  * `%k0` eax
  * `%q0` rax
* `g` 任意寄存器、内存或 immediate integer
* `= `该操作数 write-only，旧值丢弃
* `&` 指定 early-clobbered operand。如果没有 `&`，GCC 可能会给两个不相关的读、写变量分配同一寄存器，
  并假设输入读取之后才会输出到该寄存器。但如果指令中提前修改了输入寄存器，则会导致读取数据错误。所以当不允许输入和输出混淆时需指定 `&`。


`volatile`
> If our assembly statement must execute where we put it, (i.e. must not be moved out of a loop as an optimization), put the keyword volatile after asm and before the ()’s. So to keep it from moving, deleting and all, we declare it as

Clobber list
> If our instruction can alter the condition code register, we have to add "cc" to the list of clobbered registers.
> 
> If our instruction modifies memory in an unpredictable fashion, add "memory" to the list of clobbered registers. This will cause GCC to not keep memory values cached in registers across the assembler instruction. We also have to add the volatile keyword if the memory affected is not listed in the inputs or outputs of the asm.
