Crash course on linux sockets

These structures are used as the entry point from user space:
```
struct socket     // things related to a socket from the user space view
struct proto_ops  // operations corresponding to whatever a system call would perform
```

These structures are used for the specific implementations:
```
struct sock  // a specific socket type, e.g. an IPv4 + UDP socket
struct proto // operations related to a specific socket type
```

Now if you call `recvmmsg(2)`, the following call path would be followed:

```
recvmmsg
__sys_recvmmsg
___sys_recvmsg                   // for each message
sock_recvmsg_nosec
```

In `sock_recvmsg_nosec`, a call would be made to `sock->ops->recvmsg`
where the variable types are:
```
sock: struct socket
ops: struct proto_ops
```

This `proto_ops` is defined as `inet_dgram_ops`, which, as the name
indicates, is for IPv4 + UDP sockets.

The `recvmsg` is set to `inet_recvmsg`, where a call would be made to the
specific socket type `sk->sk_prot->recvmsg` where the variable types are:
```
sk: struct sock
sk_prot: struct proto
```

And the recvmsg is set to `udp_recvmsg`.

Finally `__skb_recv_udp` would be called.

The timeout parameter of `recvmmsg` is checked in the call itself, after
receiving each message, which is exactly as stated by the manpage. The
socket timeout, however, is checked in the `__skb_recv_udp`.

If we set `MSG_DONTWAIT`, the timeout would be treated as 0 since this
is basically setting the socket as non-blocking. Otherwise, if there
is socket timeout set, and we have to block, we put the socket on the
wait queue and yield. Later the thread may either be waken by the
arrival of new packets, or be waken by the expiration of the timer.
