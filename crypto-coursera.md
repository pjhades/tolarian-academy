Notes on Coursera Course Cryptography
=========================================

Security (Shannon) based on information theory states that,
for a given key `k` and a crypto text `c`, the probability that
`c` is encrypted from a plain text `m1` using `k` is equal to the probability
that `c` is encrypted from a plain text `m2` using `k`. That is,
from the attacker's perspective, there's no bias on the plain text
so that having a key `k` and a crypto text `c` do not tell the attacker
anything new about the plain text.

If a cypher is secure based on information theory definition,
its key space must be larger than or equal to the message space (Shannon).
So in terms of bitstrings, the key length should be longer than or equal
to the message. 

*One-Time Pad (OTP)* is a cipher that XORs a key and the plain text
to encrypt and decryption. It's secure based on the information
theory definition of security. But it's not practical as you have
to send a key whose length is at least the same as your message.
