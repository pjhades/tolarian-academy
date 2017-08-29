Notes on Coursera Course Cryptography
=========================================

Downsides of this course:
- The professor speaks too fast.
- The videos are too many to fit in a week.
- Formulae are not written with mathematical symbols.
- Too much free handwriting.

# Week 1

Security (Shannon) based on information theory states that,
for a given key `k` and a cipher text `c`, the probability that
`c` is encrypted from a plain text `m1` using `k` is equal to the probability
that `c` is encrypted from a plain text `m2` using `k`. That is,
from the attacker's perspective, there's no bias on the plain text
so that having a key `k` and a cipher text `c` do not tell the attacker
anything new about the plain text.

If a cypher is secure based on information theory definition,
its key space must be larger than or equal to the message space (Shannon).
So in terms of bitstrings, the key length should be longer than or equal
to the message. 

**One-Time Pad (OTP)** is a cipher that XORs a key and the plain text
to encrypt and decryption. It's secure based on the information
theory definition of security. But it's not practical as you have
to send a key whose length is at least the same as your message.

As the name indicates, OTP can **never be used twice** as that will
enable the attacker to get the XOR of several messages which can later
be exploited to learn information about it.

Counterexample: WEP, as it's key is prepended a 24-bit number so after
`2^24` frames the same key will be used again.

To resolve the issue of key length, **stream ciphers** are used where
a pseudo random number generator (PRNG) is called on a short key to
obtain a large key. To guarantee the security, the PRNG must not be
predictable.

Typically disk encryption does not use stream ciphers because it will
tell the attacker the starting position where the owner has made any
change on a file, since the corresponding encrypted file system block
would change but other unchanged blocks will remain the same.

Counterexample: RC4, used also by WEP as the PRNG, generates 2048-bit keys
that are biased on some bytes.

Counterexample: content scrambling system (CSS) used in DVD, GSM encryption.
It uses the XOR of a 17-bit linear feedback shift register (LFSR) and a 25-bit LFSR
to map a 5-byte key to 40 bits. But if the attacker knows the file format
of the encrypted file, say MPEG videos in a DVD, we can XOR that with the cipher text
to get the beginning part of the key. Then by enumerating all `2^17` values and 
minus that value from the key, we can check if the result is a valid output
of a 25-bit LFSR. So in this way we can get the initial state of the LFSRs and
further infer the key.

The security of the PRNG of a stream ciphers is not based on the information
theory one because the requirement on key length can not be met.
The security is defined as to compare the output of a PRNG with a uniform distribution
and see if we can distinguish from one another. Here the concept of **statistical test**
is introduced as a means to do this comparison. The general conclusion is,
according to Chi-Chih Yao, a PRNG should be unpredictable to be secure and be secure
if unpredictable.

The security of a stream cipher is sort of combination of information theory definition
and statistical test. If a stream cipher is secure, an attacker, within reasonable
computation time, should not learn any information about two cipher text.
The conclusion is, the security of a PRNG can determine that of a stream cipher.
So a stream cipher with a secure PRNG is secure.
