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
**Deterministic Counter Mode (DCM)**, where, using the same key, we encode a
series of counters `0, 1, 2, ...`, as many times as the number of blocks
in the plain text. Then we use these cipher texts as a stream cipher key
to XOR it with the plain text. This is basically a stream cipher built
from a PRF, and its security can be guaranteed by the security of the PRF
itself.

Regarding many-time key encryption, it's necessary to distinguish two types of attacks,
**cipher-text-only attack**, which applies to one-time keys since each key is used
only once so that the attacker can only attack based on cipher text, and **chosen plain-text attack (CPA)**,
where the attacker can obtain the cipher text of a certain plain text since he/she
can test the system because the key is used multiple times. We would like
block ciphers to have **CPA security**. With this concept, the adversary can query
the cipher with a series of plain text pairs `(mi[0], mi[1])` and determine
if the cipher is encrypting all `mi[0]` or all `mi[1]`.

There are two solutions to grant block ciphers CPA security. One is **randomized encryption**,
where the cipher text is replaced by non-overlapping sets, either element in which
could be used to decrypt. The other is **nonce-based encryption**, where a nonce
is used in encryption/decryption, satisfying that the pair `(key, nonce)` is never used
twice. The nonce can be a random number or a counter. To discuss the security
of this kind of cipher, the adversary is allowed to specify whatever nonce
to the system, as long as each nonce is specified only once.

There are two constructions of nonce-based encryption systems. One is **cipher block chaining (CBC)**,
where an **initialization vector (IV)**, either generated randomly, or provided as a nonce
that is encrypted with a separate key. (If the nonce is not encrypted with a separate key,
the attacker can elaborate two sets of input to gain advantage near 1, see Quiz 2.)
If generated randomly, the IV needs to be sent to
the receiver as it will be used to decrypt. The IV should not be predictable. CBC needs
**padding** on plain texts to get a input whose length is the multiple of the block size.
The **PKCS#5 padding scheme** pads the plain text with `n` bytes of byte `n` if its length
is not a multiple of block size, otherwise it pads it with 16 bytes of 16.
Another construction is **random counter mode (CTR)**, where the counter starts from
an IV rather than 0, and, like in DCM, it XORs the plain text with the output of `PRF(k, IV+i)`.
Compared with CBC, CTR can be implemented parallely, and does not need padding so that
the cipher text would not be expanded.

# Week 3 - Message Authentication Code (MAC)

MAC is for providing integrity for messages by computing a short tag based on a key and
a message.

Note that CRC is designed to detect random errors rather than elaborated tampering,
since a man-in-the-middle can send false message with valid CRC to the peers.

A secure MAC does not allow attackers to succeed in **existential forgery** via
**chosen message attack**, where, after querying MAC with a number of messages,
he/she should be unable to provide a new message-tag pair `(m, t)` that is valid
under this MAC.

Secure MAC can be derived from secure PRF, as long as the output space is large enough,
precisely, the output set size `|Y|` should satisfy that `1/|Y|` is negligible.

Practical MACs are usually constructed by block ciphers to enable computing tags
from a large input.

**Encrypted CBC-MAC (ECBC)** is similar to the CBC mode, except that we only output the last value,
which is encrypted by a separate key by a PRP. Prior to the last round the construction is
called **raw CBC**.

**Nested MAC (NMAC)** is constructed by iteratively compute the PRF with the input
from last round as the key. The final output is padded and given into a PRF with
a separate key to produce the output tag. The construction prior to the last PRF
is called **cascade**.

The security (advantage) of ECBC and NMAC are bounded by the advantage of the
underlying PRP or PRF, and are guaranteed as long as the input space `X` for ECBC
and the key space `K` for NMAC are far larger than the number of attacker queries,
which is actually the number of times certain MACs would be used. Beyond that many
times, the MACs would suffer from **extension property**, where the output could
be predicted on messages with an extending suffix.

For padding, the ISO (perhaps ISO-9797) pads messagse with `100000...`.

NIST has another standard that do not need dummy padding, and uses
two different keys for the get XORed together in the last round,
for padded and non-padded messages. These last XORed keys also make
the attacker hard to get the raw CBC or cascade output thus prevent
extension attacks.

Either CBC-MAC or NMAC is sequential so they cannot fully make use of multiprocessors.
In **Parallel MAC (PMAC)**, each message block is first processed separately with a mask function
which enforces an order on the blocks to prevent attacks by swapping two blocks, and
then given to the PRF. The PRF output of each block is finally XORed together and
given to the last PRF. All PRFs use a different key as the mask function.

PMAC is **incremental**: once we use PRPs, changes on a certain block only
results in incrementally re-computing the tag. All the unchanged blocks do
not need to be processed again.

**One-time MAC** is a faster/randomized version of MAC without PRF, serving
as the analog of OTP. **Carter-Wegman MAC** is a construction that converts
a one-time MAC to a many-time MAC with a PRF:

```
Carter-Wegman(k1, k2, m) = (n, PRF(k1, n) XOR Sign(k2, m))
```

where `n` is a nonce with the same length as the tag.

**Collision resistance (CR)** means a hash function guarantees that an attacker
can find collisions with negligible probability. Since the input set is usually
much much larger than the output set, there should be a lot of collisions, but
CR hash function should make sure that they are difficult to find.

Note that hash functions convert large input into small output, this can be
combined with MAC, i.e., we can construct a MAC for large inputs with
a MAC for small inputs and a CR hash function, simply by first hashing the large
input and feed it input the MAC with small inputs. For example, we can use
CBC AES for 2-block messages and SHA-256 to do that.

Note also that the difference between MACs and hash functions:

* Hash functions allow **public verifiability**, but requires that hash values
  are stored in read-only spaces, i.e., hash functions cannot prevent tampering
  on hash values. For example, an attacker modifies `gcc` binary package and
  tampers the hash, then a user may download a false `gcc` package but still
  pass the verification, since the user doensn't know that the given hash value
  has been modified accordingly by the attacker.
* MACs can prevent such tampering but do not allow public verifiability. The
  secret key should be used in order to verify the tag. But no attacker can
  tamper the content and tag but still pass verification (existential forgery).

Like blocks ciphers are vulnerable to exhaustive search, CR hash functions are
vulnerable to **birthday attacks**. For a message space `M` with size `n`, it
only needs `O(2^(n/2))` times of hashes to find a collision with probability
larger than 1/2.

**Merkle-Damgard construction** is used to construct CR hash on large inputs
from CR hash on small inputs (compression functions).
It uses a padding block like `100...0 || message length`.

**Davies-Meyer construction** is used to construct CR hash on small inputs
from block ciphers like `h(Ci, m) = E(m, Ci) XOR Ci`, where `Ci` is the
chaining variable from the last round of the Merkle-Damgard chain, and
`E` is the encrypting algorithm of the block cipher.

There's another kind of way to construct compression function, namely
based on hard number theory problems, called **provable compression functions**,
which means if you find a collisions on these functions, you will also be
able to solve the corresponding number theory problem. These constructions
are not usually used in practice because of its bad performance compared
with block ciphers.

As an example SHA-256 uses SHACAL-2 as the block cipher, and is then constructed
with Davies-Meyer and finallyh Merkle-Damgard.

**Hashed MAC (HMAC)** is the MAC used in TLS, and can be proven to be
a secure PRF under the consumption that the underlying compression function
can be regarded as a PRF that can generate unrelated keys even if related
keys are given as input.

MAC implementations are vulnerable to **timing attacks**. Due to the bad implementation,
an attacker may try to query the system with a series of `(m, tag)` pairs where
`m` is fixed. If the comparison of tags are simply implemented as a loop and returns
the comparison result as long as the first mismatch is found, the attacker can
guess each byte of the tag, and measure the time that the system spends on doing this comparison,
in order to tell if he/she has made a successful guess.

# Week 4 - Authenticated Encryption

**Authentication encryption (AE)** protects the communication from **chosen cipher-text attacks**,
where an attacker can query the system with a certain cipher-text and get the result.

Ideally we could build AE systems with:

- a cipher, providing confidentiality
- a MAC, providing integrity

Some possible constructions with MAC and cipher:

- MAC-then-encrypt, used by SSL, providing AE when cipher works in randomized counter mode or randomized CBC mode
  for randomized counter mode, only one-time MAC is sufficient
- encrypt-then-MAC, used by IPsec, always providing AE
- MAC-and-encrypt, used by SSH, where the plain-text is both encrypted and MACed

Some proposed standards:

- GCM: CTR cipher then Carter-Wegman MAC (encrypt-then-MAC)
- CCM: CBC-MAC then CTR cipher (MAC-then-encrypt)
- EAX: CTR cipher then CMAC (encrypt-then-MAC)

A more practical concept is **authenticated encryption with associated data (AEAD)**,
where only part of the data is encrypted but all of them is authenticated. This is
practical because network protocols usually need to transmit some metadata like header
in plain-text.

**Key derivation function (KDF)** transforms a source key, maybe unevenly distributed, to an evenly
distributed key. If the source key is not uniform, we use the **extract-then-expand paradigm**,
where we add **salt**, which is non-secret randomly chosen string to the original key,
transforming the key distribution to a uniform one. Then we employ the usual
key expanding algorithm. An example of this construction would be **HKDF**, a KDF
from HMAC.

Passwords do not have enough entropy to be used with KDF.
We should use standard approaches like **PKCS#5** to do it, basically this is
accomplished by constructing a **slow hash function**, which makes it hard
to traverse the generated key space.

Normally, **deterministic encryption** is not CPA-secure as it leaks information
if the attacker sees two equal cipher texts. But in some practices, the system
almost never encrypt the same message twice, i.e., the data being encrypted is
not repeated very often. In this case we define **deterministic CPA security**,
in which the attacker is not allowed to submit a message that has been submitted
in the same message set.

CBC/CTR mode with fixed IV is not CPA secure.

Possible ways to construct deterministic CPA secure systems:

- Synthetic IV (SIV) for long messages, where two keys are used, one of which is used to generate a random value `r`
  used with the encrypting PRF. This also provides **deterministic authenticated encryption (DAE)**,
  as the `r` value can be used as a MAC to verify cipher text integrity.
- PRP for short messages.
- EME that constructs wide block PRP with a narrow block PRP.

**Tweakable block cipher**: construct many PRPs from a single key.
This adds more randomness to the encryption, so that the attacker should not be able to distinguish
from the tweaked message from a randomly permuted one.

**Format preserving encryption**: generating cipher texts that has the same format as the input.

# Week 5 - Basic Key Exchange

**Trusted 3rd party** is one way to do key exchange, which simply
encrypt the shared key using the two peers' own keys.

**Merkle puzzle** is a key exchange method using only symmetric ciphers.
The basic idea is, A keeps a database containing all puzzles and the
corresponding shared key, and then she sends all the puzzles to B.
B randomly chooses one puzzle and solve it, sending to A the answer.
Then A can find that A is using the corresponding key. This introduces
a **quadratic gap** between A/B and the attacker, where A/B either needs
`O(n)` computation but the attacker needs `O(n^2)`, which is so far
the best we can achieve.

The downside of Merkle puzzles is its inefficiency.

Here an important thing is, we are trying to make the computation cost
for the peers low but make that high for the attacker. To do this we
need some number theory and algebra instead of only symmetric ciphers.

**Diffie-Hellman protocol** is a protocol that uses hard algebraic problems.
Basically the attacker needs to compute `g^(a*b)` using only `g^a` and `g^b`,
where `a` and `b` are owned by A and B respectively.

**Public-key encryption** uses a key pair: a public key that is known to all
parties, and a secret key that is known only to the owner. To establish a
shared secret, we simply encrypt the secret with the peer's public key.

Note the difference between Diffie-Hellman and public-key encryption:
Diffie-Hellman does not require the order of communication -- if A knows
`g^b`, he/she can immediately compute `g^(a*b)` and so does B.
But for public-key encryption, one party must wait the peer to send
the encrypted shared secret before actual communication starts.

# Week 6 - Public Key Encryption

For the security of a public key encryption system, as the attacker
knows the public key, it's necessary to achieve CCA security, where
the attacker is allowed to decrypt any cipher text submitted to the system,
but she should be unable to tell which plain text she submits to the system
is decrypted.

A **trapdoor function** is a mapping from `X` to `Y` which is invertible
under a public key `pk` and a secret key `sk`. That is, `pk` determines
the function in the forward direction, whereas `sk` determines the function
in the backward direction. Trapdoor functions are **one-way**, which means
without `sk`, computing the inverse is hard.

A way to construct public key encryption from trapdoor functions `F: X->Y` is:

1. generate `x` from `X`
2. get `y = F(pk, x)`
3. get `k = Hash(x)`
4. encrypt with symmetric cipher `c = E(k, m)`
5. output `(y, c)` 

So here `x` is used to generate the symmetric cipher key and itself
is protected with the trapdoor function.

This construction is secure as long as `F` is a secure trapdoor function,
and the symmetric cipher provides authenticated encryption, and finally
the hash function is a random oracle.

The trapdoor function should never be directly used as the encryption/decryption algorithm.

The **RSA trapdoor permutation** is a trapdoor function that simply raises
the input `x` to `x^e`, and the inverse is to raise `y` to `y^d`, where
`e` and `d` are integers satisfying `e*d = 1 (mode phi(N))`, where `N = p*q`,
and `p` and `q` are large primes. The security relies on the RSA assumption that
the permutation is one-way.

Plugging RSA trapdoor permutation to the public key encryption construction above
gives us the ISO standardized RSA public key encryption.

PKCS#1 v1.5 is a counterexample of incorrectly using RSA function.
With the leakage of information about whether the padding format
is correct after RSA decryption, the attacker can tamper the cipher text
and verify if the cipher text is valid under PKCS#1, thus enabling her
to decrypt all the content of the cipher text.

**ElGamal** is a public key system built from Diffie-Hellman protocol.
The basic idea is similar to RSA above, but we choose `(g, g^a)` as
the public key:

1. `b <- Zn` randomly choose `b`
2. `u <- g^b`
3. `v <- h^b = g^(a*b)`
4. `k <- H(u, v) = H(g^b, g^(a*b))`
5. `c <= E(k, m)` encrypt with symmetric cipher
6. output `(u, c)`

To prove its security, we need to use stronger assumptions called
**interactive Diffie-Hellman assumption (IDH)**. This is stronger than
the usual **computational Diffie-Hellman assumption (CDH)** and
**hash Diffie-Hellman assumption (HDH)**, where the attacker is allowed
to submit pairs `(x, y)` to the system and to get answers whether
`x^a == y` holds. The attacker wins the game if she can still work out
`g^(a*b)` given `g^a` and `g^b`. Only under IDH, together with
the security of the symmetric cipher and hash function, ElGamal is CCA secure.

