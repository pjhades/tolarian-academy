* 打印
  * `:echo "xxx" "yyy"` 打印 `xxx yyy`
  * `:echon "xxx" "yyy"` 打印 `xxxyyy`
  * `:echom[sg] "xxx"` 打印后会出现在 `:messages` 中，相当于脚本日志便于调试
  * `:echohl WarningMsg | echo "warning!!!" | echohl None` 对中间的 `echo` 使用高亮 `WarningMsg`
  * `" xxxxx` 注释
* 选项
  * `:set number` `:set nonumber` 布尔值 `:set number!` toggle
  * `:set showmatch?` 获取当前选项的值
  * `:set matchtime?` 查询带值选项
  * `:set boolopt key1=value1 key2=value2` 同时指定多个值
  * `:setlocal wrap` 仅在本地设置 `wrap` 选项，仅部分选项支持本地设置
* 映射
  * `:map - dd` 映射 `-` 键为 `dd` 命令
  * `:map <space> viw` 特殊键名用尖括号，映射空格到选中当前词
  * `:map <c-d> dd` 组合键映射
  * `:nmap` `:vmap` `:imap` 指定 normal/visual/insert 模式下的映射
  * `:imap <C-d> <ESC>ddi` insert 模式下删除行
  * `:imap <C-u> <ESC>viwUi` insert 模式下大写当前词
  * `:unmap` `:nunmap` `:vunmap` `:iunmap` 解除映射
  * `:nmap dd O<ESC>jddk` 映射查询会递归进行，所以这里 `dd` 会死循环，安装了插件可能会和现有映射冲突
  * `:nnoremap \ x` 非递归映射 `\` 到 `x` 命令，但不会递归查询 `x`
  * `:noremap` `:nnoremap` `:inoremap` `:vnoremap` 各个模式的非递归映射
  * `:help unmap` 不会就查
  * `:let mapleader = ","` `:nnoremap <leader>d dd` 设置映射 leader，即自定义的 modifier key，按 `,d` 执行 `dd` 命令
  * `:let maplocalleader = "\\"` 设置 local leader，即对特定文件类型生效的 leader
  * `:nnoremap <leader>ev :vsplit $MYVIMRC<cr>` 快速打开 `.vimrc`
  * `:nnoremap <leader>sv :source $MYVIMRC<cr>` 快速加载 `.vimrc`
  * `:inoremap <esc> <nop>` 映射 ESC 为无效操作
  * `:nnoremap <buffer> <leader>d dd` 仅在本地缓冲区建立映射，插件要用此法或 `<localleader>` 建立本地映射，避免覆盖全局设置。本地映射将覆盖相同定义的其他映射，因为它更具体
* Abbreviation
  * `:iabbrev api application programming interface` 定义 insert mode 下的缩写
  * `:unabbreviate api` 取消缩写
  * `:set iskeyword?` 查询 keyword 字符。vim 在非 keyword 输入之后开始展开缩写
* 自动命令
  * `:autocmd BufNewFile *.txt :write` 三部分：监听事件（创建新文件，多个事件用逗号隔开）、事件过滤（仅针对 `.txt` 后缀）、动作（写入磁盘）
  * `:autocmd BufRead,BufWritePre *.html :normal gg=G` 在读写任何 `.html` 文件之前执行命令
  * `:autocmd BufRead,BufNewFile *.html setlocal nowrap` 无论打开的文件是否存在，都会执行命令
  * `:autocmd FileType javascript nnoremap <buffer> <localleader>c I//<esc>` `FileType` 事件在设置 buffer 的 `filetype` 时触发，此命令对 javascript 文件映射一个自动注释命令
  * 重复定义相同的 `autocmd` 不会覆盖之前的，vim 会认为定义了两个
  * `augroup xxxx ... augroup END` 定义自动命令组。同名的组重复定义不会覆盖，如果要清除之前定义的同名组，要在组里加入 `autocmd!`
* Operator-pending 映射
  * `onoremap p i(` 在某个命令等待移动操作时，把 `p` 映射为 `i(`，例如 `dp` 将映射为 `di(` 删除括号内的文本
  * `onoremap b /return<cr>` 把 `b` 映射为搜索 `return` 后光标的移动
  * `onoremap il( :<c-u>normal! F)vi(<cr>` 映射 `il(` 为向前找第一个 `)` 然后选中括号内容的光标移动
  * Operator-pending 映射操作文本的规则：如果选定了文本就操作它们，否则操作当前位置到移动目标位置之间的文本
* `:normal`
  * `:normal` 在 normal 模式下执行后面跟的操作，不能识别特殊字符如 `<cr>`
* `:excute`
  * `:excute` 将后面的字符串当做命令执行
  * `:onoremap ih :<c-u>execute "normal! ?^==\\+$\r:nohlsearch\rkvg_"<cr>` 对 markdown 文件，向后找到一个标题行做操作
* 状态行
  * `:set statusline=%f` 显示文件类型
  * `:set statusline+=%l/%L` 显示当前行数、总行数
  * 格式化字符串与 `printf` 一样
* Quickfix window
  * `:copen` 打开
* 语法
  * `let foo = "bar"` 变量
  * `echo &textwidth` `echo &number` 将选项当做变量，可以计算出某个选项的值，不受限于 `set` 命令只能指定常数
  * `let &l:number = 1` 本地选项
  * `let @a = "hello"` 寄存器变量，此后可以用 `"ap` 命令粘贴寄存器值
  * `echo @"` 打印未命名寄存器值，比如 `yiw` 命令复制的对象
  * `echo @/` 打印最近一次的正则匹配模式
  * `let b:hello = "world"` 限制变量作用域为当前缓冲区
  * `echo "a" | echo "B"` 管道符分割多行命令
  * `if "xxx" | echo "yes" | endif` 需要数字的时候 vim 会强制转换，若字符串不以数字开头则转换为 0
  * `elseif` 和 `else` 分句
  * `==` 的行为依赖于 `ignorecase` 选项，`==?` 始终大小写不敏感，`==#` 始终大小写敏感
  * `function GlobalFunc() endfunction` 没有作用域限制的函数要以大写开头
  * `call Func()` 或 `Func()` 调用之，但有返回值时 `call` 会将它丢弃，若函数不返回，隐式返回 0
  * 引用函数参数用 `a:name`，可变参数时，`a:0` 是参数个数，`a:000` 是参数列表
  * 参数不能赋值
  * `+` 仅用于数字，字符串连接用 `.`
  * 单引号中的字符串无视转义字符，连续两个单引号打印一个单引号
  * 字符串函数 `strlen` `len` `split` `join` `toupper` `tolower` 
  * `execute` 将字符串当做命令执行
  * `normal` 执行 normal 模式下的命令。`normal!` 无视映射
  * `normal` 不能识别 `<cr>` 之类的特殊字符，所以通常将 `execute` 和 `normal!` 配合使用，如 `execute "normal!
    mqA;\<esc>`q"`，注意字符串中的特殊字符要转义
  * `\v` 放在正则模式前与其他语言相同
  * `<cword>` 光标词小写，`<cWORD>` 大写
  * `expand("<cword>")` 强制扩展特殊变量，否则 vim 会在命令执行前扩展
  * `shellescape()` 转义为适合 shell 执行
  * `:nnoremap <leader>g :silent execute "grep! -R " . shellescape(expand("<cWORD>")) . " ."<cr>:copen<cr>` 映射：grep 在当前目录中递归查找光标下的词。`silent` 不显示 `grep` 的输出
  * 关于构造命令，总的来讲就是
    * vim 命令行 `:` 上的内容不是字符串，而是命令
    * 字符串要放到命令行执行只能用 `execute`
    * `execute` 的字符串里面如果有特殊按键必须转义
    * 涉及到 shell 命令的最好用 `shellescape` 转义一下避免执行出错
    * `<cword>` 这种东西只有 vim 命令行认识，所以会在 vim 命令执行前扩展，要用在字符串中需要 `expand()`
  * 
  







