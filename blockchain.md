This is only a very basic and rough description.

A blockchain acts as a decentralized distributed ledger,
where a number of blocks are chained with hash.
Each block consists of a batch of operations or transactions,
encoded in a Merkle tree. Blocks are chained by
a reference to the hash of the previous block.

Blocks are created independently by each peer.
The determination of which block will be added to the globally
agreed chain will be done by a distributed concensus protocol.
Once upated, the chain will be broadcast to other peers
so that the change propagates in the entire P2P network.

There are generally two types of concensus protocols:

- Proof-of-Work (PoW): a peer must prove its work in order to add a new block.
The proof is usually done by performing cryptographic computations that
are hard to do but easy to verify. In this way all other peers in the network
can easily check if the required work is done.
- Proof-of-Stake (PoS): this favors peers that are "rich" in some sense,
like in the case of cryptocurrency, usually the peers that hold a certain
amount of assets meeting a certain criteria, like age.

The design of blockchain makes it hard to tamper since changing any
part of the history requires changing all the history afterwards.
Also the distributed concensus makes forging stuffs hard because
other peers will always act in their own interest.

Bitcoin uses blockchain as its ledger. A unit is created by mining.
If a miner successfully performed the required computation, a certain
amount of unit is created and she gets her reward. New units are
created in a diminishing way, making the total amount of bitcoins
in circulation fixed.

Bitcoin can be sent to or received through an address. Anyone can freely
create new addresses. Addresses are protected by asymmetric cryptography:
a private key is used to generate a new address but it's computationally
impossible to infer to the private key from an address. Tranferring units
only needs a public key.

Bitcoin uses a PoW concensus protocol.
