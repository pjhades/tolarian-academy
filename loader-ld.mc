#Loadable and Allocatable
* loadable: section contents should be loaded into memory when the output file is run
* allocatable: an memory area should be set aside but nothing should be loaded there
* neither: debugging information typically

#VMA and LMA
Each loadable or allocatable sections has two address
* VMA: virtual memory address, address when running
* LMA: load memory address, address to load

Most cases are the same.

Use `objdump -h` to display information about section headers.

#Symbols
Defined symbols have addresses.

Use `nm` or `objdump -t` to see symbols in an object file.

#Syntax
##Section output

    section [address] [(type)] : [AT(lma)]
      {
        output-section-command
        output-section-command
        …
      } [>region] [AT>lma_region] [:phdr :phdr ...] [=fillexp]

`address` 指定 VMA，`AT` 指定 LMA，默认跟随 VMA。`phdr` 指定该 section 所属的 segment。`=fillexp` 是个表达式，
指定该 section 的 fill pattern。section 中间可以用 `FILL(expression)` 命令来指定该 section 中 FILL 之后的 fill pattern。
与 `=fillexp` 不同点就在于 `FILL` 仅作用于该命令之后，而 `=fillexp` 作用于整个 section。`address` 可以对齐，如 `ALIGN(0x10)`
即按 16 字节对齐。

通配符模式
* `*` match any number of char
* `?` match single char
* `[a-z]` range
* `\` espace


##Program Headers

    PHDRS
    {
      name type [ FILEHDR ] [ PHDRS ] [ AT ( address ) ]
            [ FLAGS ( flags ) ] ;
    }

指定最终结果包含的 segments。`type` 就是 `PT_NULL`。`AT` 指定此 segment 的 LMA，`FLAGS` 指定该 segment 的标记 `p_flags` 字段。
`FILEHDR` 指定该 segment 包含 ELF file header，`PHDRS` 指定该 segment 包含 program headers 本身。


#关键字和内置函数
* `ADDR(section)` 返回 section 的 VMA
* `SIZEOF(section)` 返回 section 的大小
* `AT(lma)` 设置一个 section 的 LMA
* `OUTPUT` 输出文件名，同 `ld -o`
* `OUTPUT_FORMAT` 输出文件的格式（`ld` 支持多种输入输出文件格式），同 `ld -oformat`
* `TARGET` 输入文件的格式，同 `ld -format`
* `PROVIDE(etext = .);` 默认符号定义。如果源文件引用了 `etext` 但没定义，就用链接脚本的定义，否则使用源文件定义。注意这里的 `etext` 只是符号，
   不是变量，所以在 C 代码中最好是声明为 `extern char etext[]`，或者取它的地址。如果声明为其他形式，如 `extern void *etext`，那么使用时会将 `etext`
   当做指针变量来访问，即从它的地址处取 `sizeof(void *)` 这么多个字节，所以得到的是 garbage
