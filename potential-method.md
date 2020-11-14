# Potential Method

To use the potential method, we need to derive a potential function `Ø`,
which describes the complexity of a data structure by assigning a "score" to a certain state.

The key idea of potential method is that, if a time-consuming operation can bring
down the overall complexity of a data structure and make later operations fast,
we can amortize its complexity among the later operations.

Based on this idea, for one operation `i`, we can derive the relationship between amortized time `Ta` and
real time `Tr`:

```
Ta[i] = Tr[i] + C * (Ø[i] - Ø[i-1])
```

where `C` is constant, `Ø[i]` is the potential after the operation `i`, `Ø[i-1]` is the potential before.

From this, we can obtain the relationship between the total amortized time `Tta` and the total real time `Ttr`:

```
Tta = ∑ Ta[i]
Ttr = ∑ Tr[i]
```

Then

```
Tta = ∑ ( Tr[i] + C * (Ø[i] - Ø[i-1]) )
    = ∑ Tr[i] + C * ∑ (Ø[i] - Ø[i-1])
    = Ttr + C * (Ø[n] - Ø[0])
```

That is, total amortized time can be computed by adding the total real time and the potential difference
after and before a sequence of operations.


# Easy-to-Remember Example: Binary Counter

Consider a binary counter that supports operations:

- `init`: reset all bits to 0
- `inc`: add 1
- `read`: return current counter value

`inc` may result in flipping a series of 1 bits, but we want to show that its amortized complexity is _O(1)_.

Consider potential function

```
Ø = the number of 1 bits in the counter
```

`inc` will flip the lowest bit, set all the consecutive upper bits that are 1 to 0, and set the final 0 bit to 1.
If before `inc` there are `n` 1 bits, afterwards at most `n` 1 bits will become 0 and one 0 bit will become 1,
thus the potential difference is `1-n`. The actual time in this case will be `n+1`, thus the amortized time
of `inc` is `n+1 + C*(1-n)`, which is _O(1)_ if we take `C` as 1 in this case.
