# MTU

The concept is not tied to any layer. (like PDU)

To calculate the MTU of a medium one needs to subtract
the overhead like header from the maximum frame size.

## Path MTU Discovery (PMTUD)

PMTU is defined by IP as the minimum MTU supported by any
hop between a source and a destination.

This is used by a sender to detect the largest possible
IP packet size to avoid IP fragmentation. The sender keeps
sending IP packets with Don't Fragment flag set. Hosts
not supporting that size will respond with Fragmentation Needed
ICMP packet.

## MSS

Don't confuse MSS with MTU. The 2nd S means segment so clearly
it's related to TCP segments. MSS is a TCP option declaring the
size, excluding TCP and IP headers, that an end would like
to receive. It is declared to the peer during connection establishment.

Note that MSS has nothing to do with IP fragmentation. But
usually we want to avoid fragmentation, so that we should set a 
roper MSS value. IPv4 requires that any host should be able to
receive 576 bytes without fragmentation. (Note that this says nothing
about the ability of link layer) Then we subtract 20
for IP header and another 20 for TCP header, resulting in
576 - 20 - 20 = 536 bytes. For IPv6 we have the similar computation:
1280 - 40 - 20 = 1220 bytes.
