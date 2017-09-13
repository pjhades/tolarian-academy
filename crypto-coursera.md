Notes on Coursera Course Cryptography
=========================================

# Week 1 - Stream Ciphers

**Perfect secrecy** (Shannon) is based on information theory states that,
for a given key `k` and a cipher text `c`, the probability that
`c` is encrypted from a plain text `m1` using `k` is equal to the probability
that `c` is encrypted from a plain text `m2` using `k`. That is,
from the attacker's perspective, there's no bias on the plain text
so that having a key `k` and a cipher text `c` do not tell the attacker
anything new about the plain text.

If a cypher has perfect secrecy,
its key space must be larger than or equal to the message space (Shannon).
So in terms of bitstrings, the key length should be longer than or equal
to the message. 

**One-Time Pad (OTP)** is a cipher that XORs a key and the plain text
to encrypt and decrypt. It has perfect secrecy. But it's not practical as you have
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

The security of the PRNG of a stream ciphers doesn't has perfect secrecy
because the requirement on key length can not be met.
But its security can be defined as to compare the output of a PRNG with a uniform distribution
and see if we can distinguish from one another. Here the concept of **statistical test**
is introduced as a means to do this comparison: if a PRNG is secure, any efficient algorithm
cannot distinguish its output from a uniformly randomly generated bit string.
The general conclusion is, according to Chi-Chih Yao,
a PRNG should be unpredictable to be secure and be secure if unpredictable.

The **semantic security** of a stream cipher is similar to perfect secrecy.
The idea behind this definition is also that, any efficient algorithm cannot
distinguish its cipher text from one that is encrypted with a uniformly randomly
generated key. A conclusion: if the PRNG used by a stream cipher is secure, then
the stream cipher is semantically secure.

A possible way to attack multi-time pad is **crib dragging**, where the attacker
first gets the XOR of two plain texts `m1 ^ m2`, then she may make a reasonable guess
about a piece of the plain text, say `p` from `m2`,
like `" the "`, which is XOR-ed with `m1 ^ m2`. If the attacker obtains anything
readable, then her guess should probably correct and she can have `p ^ c2` to
obtain the corresponding piece of the key. Some means can be introduced to automate this
procedure, like a dictionary of useful words and phrases, a measure to estimate the
"readability" of a piece of text.

# Week 2 - Block Ciphers

To summarize a block cipher, it has n-bit plain text and cipher text, and has
a **key expansion** procedure in which the key is expanded into `n` pieces,
and each piece is used in a **round function** that is piped up one after another
and is consecutively applied to the previous output, starting from the plain text:

```
k -> Key Expansion -> k[0], k[1], ..., k[n-1] --+
                                                |
                                                V
m -> r[0] = R(k[0], m) -> r[1] = R(k[1], r[0]) -> ... -> r[n-1] = R(k[n-1], r[n-2]) -> c
```

A block cipher is secure if it uses a secure **Pseudo Random Permutation (PRP)**.
A PRP is basically a efficient **one-to-one** (thus invertible) function `PRP: K, X -> X`. By the name you can infer
that it permutes the input randomly so its security, based on the experience obtained
before, should be that it must not be, with reasonable computation ability, indistinguishable
from a real uniformly random permutation. This security definition also applies to
**Pseudo Random Functions (PRF)**, which are special cases of PRPs where the output
set is any set: `PRF: K, X -> Y`.

In **DES (Data Encryption Standard)**, the iterative part above is a 16-round **Feistel network**,
which iteratively apply a series of functions to two equal-length inputs in an interleaved
way. It's simply invertible, and the decrypting algorithm has the same structure as the encrypting
one, with the only difference being that we apply the series of functions in reverse order.

Here comes a theorem (**Luby-Rackoff**): based on a secure PRF, a 3-round Feistel network is a secure PRP.

In DES, the serious of functions are all bit manipulations, with a key component called
the **S-boxes**. An S-box is a 6-bit-to-4-bit lookup table. It must not be linear, otherwise
the attacker would XOR three cipher texts to get the cipher text of the XOR of those plain texts.

DES has a property that given a pair of `(m, c)`, with close-to-1 probability there's only
one key that's used in the encryption. So this makes DES vulnerable to **exhaustive search attacks**
in which an attacker tries all `2^56` keys in a brute-force manner. To strengthen DES,
**3DES** (triple-DES) was invented where 3 keys `(k1, k2, k3)` are used to encrypt the message
as `E(k1, D(k2, E(k3, m)))`. The interleaving of encrypting and decrypting enables users
to set the 3DES system work as orignal DES by setting `k1 == k2 == k3`.
3DES improved the security, but is 3 times slowers than DES. Another DES enhancement
is **DESX**, where, also 3 keys are used, but in a way like `k1 XOR E(k2, m XOR k3)`.

2DES is dangerous as it suffers **meet-in-the-middle attacks**. Generally this attack
is a kind of searching but the attacker tries to search from both ends so that it
can check if the results computed from both ends match in the middle, in order to
reduce the overall computing time.

There are some other elaborated attacks on block ciphers, like **side-channel attacks**,
where an attacker measures some metrics demonstrated by the implementation of the cipher
like the circuit properties, **fault attacks**, where an attacker tries to make
the cipher system malfunction to obtain some leaked information, and **quantum attacks**,
where a quantum computer is used to magically solve certain search problems more efficiently
than normal computers.

In **AES (Advanced Encryption Standard)**, the iterative part is a 10-round **Substitution-Permutation Network**,
where the output from the previous round is XORed, part-by-part substituted and permutated.
These procedures are all invertible and are easy-to-implement table lookups.
So AES can be implemented efficiently by pre-compute the lookup tables.
Intel CPUs since Westmere provides `aesenc` and `aesenclast` instructions working
together with 128-bit `xmm` registers to quickly compute AES cryptos.
gcc/g++ also provides compiler intrinsics via `wmmintrin.h` header.

PRF can be built from PRNG by **Goldreich-Goldwasser-Micali Construction**,
where, iteratively, we run a PRNG twice for the input, and according to each bit
of the input we choose either the first or the second part as the input for
the next iteration. This is theoretically secure but not practical because of
its poor performance. By using Luby-Rackoff theorem, we can even build a PRP from a PRNG.

For a PRF to be secure, the adversary should not be able to distinguish
the output from a PRF `F(k, ?)` from a randomly chosen function from
the same domain to the range. Following the same framework we can compute
the **advantage** of an adversary over a function. This also extends
to PRPs.

(Note that, for a domain `X` and a range `Y`, no matter what key
is in use, there are at most `X.size ^ Y.size` functions on them.
(Choosing an image in `Y` for each `x` in `X`.)
But for a certain key space `K`, we can fix the key to determine a certain
PRF, so there are `K.size` possible PRFs.)

**PRF Switching Lemma** says any secure PRP is also a secure PRF if
the domain `X` is sufficiently large.

Electronic Code Book (ECB) is NOT a correct use of PRP, since for the same
plain text it will generate the same cipher text, which lets the attacker
learn information about the plain text. One secure construction would be
**Deterministic Counter Mode**, where, using the same key, we encode a
series of counters `0, 1, 2, ...`, as many times as the number of blocks
in the plain text. Then we use these cipher texts as a stream cipher key
to XOR it with the plain text. This is basically a stream cipher built
from a PRF, and its security can be guaranteed by the security of the PRF
itself.
