epoll 各种事件的触发条件。

# OOB
TCP 只支持 1 byte 的 OOB，发送时 send 指定 `MSG_OOB` 即可。若发送长度大于 1 的内容，则最后一个字节当作 OOB 发送。
接收端收到 OOB 时，若 epoll 监视了 `EPOLLPRI`，则会触发 `EPOLLIN + EPOLLPRI`，此时用 `recv` 带 `MSG_OOB` 可以读到 OOB，
但若调用 `recv` 没有 OOB，`recv` 会返回 -1。普通数据只触发 `EPOLLIN`。 也可以设置收到 OOB 时以信号通知进程，
或用 inline 方式接收 OOB。见 APUE 和 UNP。

# 关闭连接
正常 `close`，`shutdown` 写端，或带正常 linger 的 `close` 都会被识别为对端关闭连接，主动关闭方发送 FIN，完成报文交换。
对端关闭连接时，`EPOLLIN` 和 `EPOLLRDHUP` 触发，`read` 返回 0。

若设置了 linger：`l_onoff = 1` 且 `l_linger = 0`，此时 `close` 将发送 RST，
接收端触发 `EPOLLIN + EPOLLERR + EPOLLHUP + EPOLLRDHUP`，有错误标志，因此 `read` 返回 -1，错误为 Connection closed by peer。
接下来会触发 `EPOLLIN + EPOLLHUP + EPOLLRDHUP`，`read` 返回 0。


# 例子
server:

```C
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <netdb.h>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <sys/types.h>

# define MAXFIRED 1024

int main()
{
    int i, j, ret, srv, cli = -1, lp, nfd, len, flag;
    struct sockaddr addr;
    struct addrinfo hint, *res;
    struct epoll_event ev, fired[MAXFIRED];
    char buf[INET_ADDRSTRLEN];

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;

# if 0
    struct addrinfo {
        int              ai_flags;
        int              ai_family;
        int              ai_socktype;
        int              ai_protocol;
        socklen_t        ai_addrlen;
        struct sockaddr *ai_addr;
        char            *ai_canonname;
        struct addrinfo *ai_next;
    };
# endif

    if ((ret = getaddrinfo(NULL, "9999", &hint, &res)) != 0) {
        fprintf(stderr, "%s\n", gai_strerror(ret));
        exit(1);
    }

    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == -1) {
        fprintf(stderr, "socket failed\n");
        exit(1);
    }

    if (bind(srv, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "bind failed\n");
        exit(1);
    }

    if (listen(srv, SOMAXCONN) == -1) {
        fprintf(stderr, "listen failed\n");
        exit(1);
    }

    lp = epoll_create(1);
    if (lp == -1) {
        fprintf(stderr, "cannot create epoll\n");
        exit(1);
    }

    ev.data.fd = srv;
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    epoll_ctl(lp, EPOLL_CTL_ADD, srv, &ev);

    while (1) {
        printf("\nlegend:\nin=%d  pri=%d  err=%d  hup=%d  rdhup=%d\n",
                EPOLLIN, EPOLLPRI, EPOLLERR, EPOLLHUP, EPOLLRDHUP);

        nfd = epoll_wait(lp, fired, MAXFIRED, -1);
        for (i = 0; i < nfd; i++) {
            if (fired[i].data.fd == srv) {
                cli = accept(srv, &addr, &len);
                ev.data.fd = cli;
                ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
                epoll_ctl(lp, EPOLL_CTL_ADD, cli, &ev);
                inet_ntop(AF_INET, &((struct sockaddr_in *)&addr)->sin_addr, buf, INET_ADDRSTRLEN);
                printf("got conn from %s:%d, fd=%d\n", buf, ntohs(((struct sockaddr_in *)&addr)->sin_port), cli);
            }
            else if (fired[i].data.fd == cli) {
                flag = (fired[i].events & EPOLLPRI) ? MSG_OOB : 0;

                len = recv(cli, buf, INET_ADDRSTRLEN, flag);

                if (len == 0) {
                    printf("close conn\n");
                    close(cli);
                    goto print;
                }
                else if (len == -1) {
                    fprintf(stderr, "read error\n");
                    goto print;
                }

                printf("receive %d bytes:", len);
                for (j = 0; j < len; j++)
                    printf(" %c", buf[j]);
                printf("\n");
            }
print:
            printf("fd=%d  event=%d\n", fired[i].data.fd, fired[i].events);
        }
    }

    close(srv);
    close(lp);
    freeaddrinfo(res);

    return 0;
}
```

client:

```C
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <netdb.h>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <sys/types.h>

int main()
{
    int srv, ret;
    struct addrinfo hint, *res;
    struct sockaddr addr;
    struct linger lin;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    if ((ret = getaddrinfo(NULL, "9999", &hint, &res)) != 0) {
        fprintf(stderr, "%s\n", gai_strerror(ret));
        exit(1);
    }

    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == -1) {
        fprintf(stderr, "socket failed\n");
        exit(1);
    }

    if (connect(srv, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "connect failed\n");
        exit(1);
    }

    send(srv, "abcd", 4, MSG_OOB);
    //send(srv, "123", 3, MSG_OOB);
    sleep(3);

    lin.l_onoff = 1;
    lin.l_linger = 0;
    if (setsockopt(srv, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin)) == -1) {
        fprintf(stderr, "setsockopt failed\n");
        exit(1);
    }

    close(srv);
    return 0;
}
```

