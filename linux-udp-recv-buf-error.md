You can see this via

```
grep Udp /proc/net/snmp
```

This error is corresponding to `UDP_MIB_RCVBUFERRORS`, which is
associated to the MIB (management information base) item in
`snmp4_udp_list`

The first thing to know is, this error is only related to receiving
packets from IP layer. This call path starts from the function
`udp_rcv`, which is registered as a handler in `struct net_protocol`
`udp_protocol`. These protocol-specific handlers are responsible for
dealing with the IP layer.

In out case, the call path expands itself as:

```
udp_rcv
__udp4_lib_rcv                   // Here we check the checksum,
recording UDP_MIB_NOPORTS ...
udp_queue_rcv_skb
__udp_queue_rcv_skb
__udp_enqueue_schedule_skb
```

The last function has the signature

```
int __udp_enqueue_schedule_skb(struct sock *sk, struct sk_buff *skb);
```

where `sk` is the socket of our interest, and `skb` is the `skbuff` that
we'd like to receive from IP layer.

Here the kernel first check if the UDP receive queue is full. The size
of the current receive queue is record in `sk->sk_rmem_alloc`, so this
value is compared against the allowed maximum, `sk->sk_rcvbuf`. The
latter value can be tuned with `sysctl` via `net.core.rmem_max` and
`net.core.rmem_default`.

If the receive queue is already full, the kernel will directly return
`-ENOMEM`, which will then trigger the updating of the
`UDP_MIB_RCVBUFERRORS` counter. As the comment explains, this comparison
is for avoiding the expensive atomic add and then subtract operations.

Then this function obtains the size of the `skbuff`: `skb->truesize`, and
compare these two:
```
truesize + sk_rmem_alloc
truesize + sk_rcvbuf
```

If the former is larger, we cannot afford the new `skbuff`. Again,
`-ENOMEM` is returned and the corresponding MIB counter will be updated.
