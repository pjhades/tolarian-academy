# 硬件终端设备结构

             HARDWARE                             SOFTWARE
    
    Terminal -- Physical -- UART ------------- UART ---- Line ----------- tty ----- User
                line       Universal           driver    discipline      driver     processes
                           Asynchronous         |                           |
                           Receiver and         |<------- kernel ---------> |
                           Transmitter

* Line discipline 可以对数据进行处理，常见的是行编辑（line editing），例如 xterm 中输错了命令
可以用退格键修改，等到按回车后才发送给 shell。一些软件如 shell、vim、nano 通常会关掉带行编辑的 canonical 模式，
而在 raw 模式下自己处理
* tty driver 提供了操作系统实现的会话管理功能，方便通过 terminal 进行多任务控制
* UART driver、line discipline、tty driver 合称一个 tty 设备，或直接叫 tty，可以通过 /dev 下对应的设备文件来操作。
这需要进程拥有该设备的写权限，通常在登录时由 login 程序来实现


# Linux

       HARDWARE                                     SOFTWARE
    
    Display <---------- VGA <---------
                       driver         \ Terminal <--> Line <-----------> tty <-------> User
                                      / emulator      discipline        driver         processes
    Keyboard -----> Keyboard -------->
                    driver
                     |                                                      |
                     |<-------------------- kernel ------------------------>|


伪终端：

                 <---------> tty driver <------> User
    Line                     (pty slave)         processes
    discipline   
                 <---------> pty master <------> xterm



#  manpage
伪终端（pesudoterminal，pty）就是一对提供双向通信的虚拟字符设备，分为 master 和 slave，slave 对外表现跟传统终端一样。
需要连接到终端的进程 open 伪终端的 slave 端，然后受打开了 master 端的另一个进程驱动。写入 master 的数据就像在真实
终端上输入一样发送给 slave 端的进程，写入 slave 端的数据可被 master 端的进程读到。

伪终端 API 分 BSD 和 SysV 两种。Linux 提供了两种 API 的支持，其中基于 SysV 的终端一般称为 UNIX 98 终端。
从 2.6.4 内核开始，BSD API 已过时，应统一使用 SysV API。

Linux 上 master 端的进程需要 open /dev/ptmx 并设置 slave 的访问权限，然后再 open slave。Slave 设备将出现在 /dev/pts/\*。
而 BSD 上的伪终端设备文件是预先创建好的，由 /dev/ptyXY （master）和 /dev/ttyXY （slave）构成一对。


#  实验
1. ssh 登录到 guest archlinux
2. `watch -n 0.1 "pstree -ap"` 观察进程树和命令参数。此时可见 sshd 的一串 fork，最后 fork 出了 bash
3. `lsof` 可见 sshd 打开了 /dev/ptmx，而 bash 打开了 /dev/pts/0
4. Guest 本身由 `init` fork 出了 `agetty`，在 /dev/tty1 上等待用户登录。直接登录 guest，可在 `pstree` 中看到 `init` fork 出 `login` 来处理登录，并 fork 出了 bash
5. 直接在 guest 上用 `lsof` 查看 bash 进程的打开文件，可以看到其描述符 0、1、2 均指向 /dev/tty1
6. 在 host 系统上用 ssh 另外发起一次登录，可见 sshd fork 出另一个 sshd 来处理本次登录
7. 再次 `lsof` 查看第二个 ssh 之后的 bash 打开文件，可见其打开了 /dev/pts/1

此时的情况是：

    ssh client -- network -- sshd -- /dev/ptmx ---- line discipline --- /dev/pts/*  ---- bash
                                     (pty master)                       (pty slave)


#  Refs
1. [what-are-the-responsibilities-of-each-pseudo-terminal-pty-component-software](http://unix.stackexchange.com/questions/117981/what-are-the-responsibilities-of-each-pseudo-terminal-pty-component-software)
2. [The TTY demystified](http://www.linusakesson.net/programming/tty/)
5. [Wiki - Pseudo terminal](http://en.wikipedia.org/wiki/Pseudo_terminal)
3. `man pty`
4. `man pts`
