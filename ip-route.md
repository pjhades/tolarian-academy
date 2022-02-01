# iproute2

IP addresses are classified into 3 categories:
- Locally hosted: addresses that we own, like loopback or the addresses for our `enpXXX` interface
- Locally connected: connected via link-layer medium, like the address of the router in our LAN
- Everything else: addresses that we can't directly reach but have to go through routers

There are actually multiple routing tables on the system,
but the default one, which is accessed if no specific table
is speficied on the command line, is the `main` routing table.
Multiple tables exist to make policy-based routing possible.

Policy-based routing, as opposed to the common case, determines
a route not only base on the destination address of a packet,
but also a number of other criteria, like the source address,
the type of service.

Rules can be added to the routing policy databse (RPDB) in order to
configure such policies. A rule can match a certain key, like source address,
against the packet to be routed.  If a rule hits, routing is done based on
the routing table specified by the rule.

Pseudo-code of the highly simplified route selection process is

```
key = packet.key

if key in routing_cache:
    return routing_cache[key]

for rule in routing_rules:
    if rule.match(key):
        if key in rule.routing_table:
            return rule.routing_table[key]

return fail
```

To show rules:
```
$ ip rule
0:      from all lookup local
32766:  from all lookup main
32767:  from all lookup default
```
The kernel configures 3 rules at startup, ranging from highest priority (0) to lowest (32767).
These rules are scanned in order of descending priority.

To add a rule:
```
$ sudo ip rule add unreachable to 8.8.8.8
```
It asks to generate a host-is-unreachable error if the destination address is 8.8.8.8.
Note that `unreachable` is known as the rule type. If no type is specified it defaults
to `unicast`, which is the type to configure actual routes.

To show cache (removed since Linux 3.6):
```
$ ip route show cache
```

To show another table:
```
$ ip route show table local
```

To show a route that the kernel might use:
```
$ ip route get 1.1.1.1 from 192.168.0.105 oif myinterface
```

## Under the Hood
`ip route get` relies on rtnetlink (7) to add/delete/retrieve route information from the kernel.

## Ref
- [http://vger.kernel.org/~davem/columbia2012.pdf](http://vger.kernel.org/~davem/columbia2012.pdf)
