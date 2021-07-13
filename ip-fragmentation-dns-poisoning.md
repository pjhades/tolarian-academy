The attack exploits IP fragmentation and UDP to poison a caching DNS resolver.

# Participants
- Attacker
- Authoritative server
- Caching resolver

# The attack steps
1. Attacker obtains a full and correct response from authoritative server.
2. Attacker forges an ICMP fragmentation needed packets and sends it to the authoritative server.
    - This is to spoof authoritative server to make it believe that the MTU of the path to the caching resolver is reduced.
3. Attacker splits the full response carefully and forges a 2nd fragment as if it was fragmented.
    - The 2nd fragment usually contains the resource record that the attacker would like to poison.
4. Attacker sends a query to the caching resolver to start the attack.
5. The caching resolver recursively sends a query to the authoritative server.
6. Attacker sends the crafted 2nd fragment to the caching resolver.
    - With careful timing, the caching resolve would receive the crafted 2nd fragment before the real one and would be waiting
      for the missing 1st fragment
7. The authoritative server sends the 1st fragment to the caching resolver.
8. The caching resolver reassembles the fragments and regards it as the real response from the authoritative server.
    - But the 2nd fragment is crafted, now its cache is poisoned.

# Reference
- https://blog.apnic.net/2019/07/12/its-time-to-consider-avoiding-ip-fragmentation-in-the-dns/
- https://ripe67.ripe.net/presentations/240-ipfragattack.pdf
