# NES PPU Notes

I feel that I need to take some notes on NES PPU to understand it
better because:

* I find that information scatters around the Internet,
  some of the documents are even `.txt` files from the last century;
* I am too stupid to understand those that is intuitive and apparent
  to you and to these document authors.

However, I cannot produce anything new. What I am writing here is simply
a summary of what I have read so far.

# Reference

* [https://wiki.nesdev.com/w/index.php/PPU_scrolling](https://wiki.nesdev.com/w/index.php/PPU_scrolling)

# PPU Internal Registers

According to the layout of NES PPU, the range `$4000 - $FFFF` mirrors
the lower part of PPU memory. So memory access to PPU memory only needs
14-bit addresses. These addresses have their own formats according to the
different parts of PPU memory that they are going to reference,
and are constructed with the information kept in several internal registers.

The internal registers are (details will be covered later):
* `v`, 15 bits, which holds the address to access
* `t`, 15 bits, which holds a "temporary" address shared by `$2005` and `$2006`
* `x`, 3 bits, which holds the 3-bit fine X position, that is, the X position within a
       8x8-pixel tile
* `w`, a flag which serves as the "write toggle" of `$2005` and `$2006`

The `v` and `t` registers have the same format:

```
          11111
    bit   432109876543210
content   yyyNNYYYYYXXXXX
```

where the components are:
* `y`, the fine Y position, the counterpart of `x` mentioned above,
       holding the Y position within a 8x8-pixel tile
* `N`, the index for choosing a certain name table
* `Y`, the 5-bit coarse Y position, which can reference one of the 30 8x8 tiles
       on the screen in the vertical direction
* `X`, the 5-bit coarse X position, the counterpart of `Y`

Rendering the screen proceeds in a row-first manner done on scanlines
one after another, so that:

1. The fine X position `x` is incremented first.
2. After finishing each scanline, fine Y position `y` is incremented.
3. After finishing each 8-pixel "columns" of a tile, coarse X position `X` is incremented.
4. Finally after finishing each line of tiles, coarse Y position `Y` is incremented.

Normally each position will wrap to 0 when it reaches its boundary.

When coarse X or Y position wraps, the 2 bits in `N` will be
changed accordingly, switching the name tables back and forth
in the two directions. This mechanism accords with the starting
address of the 4 name tables.

# Name Table Access

Accessing name table and attribute table requires only the lower 12 bits extracted
from `v`. The highest 2 bits of the 14-bit PPU address is fixed so that the lower 12 bits
can be used as an offset from the first name table at `$2000`.

Therefore, we use only the `NNYYYYYXXXXX` part of `v` to reference a byte in a name table,
where `NN` selects one of the four name tables and `YYYYYXXXXX` corresponds to
the position of the tile on the screen. So if we want to access the tile at position
`(3, 5)` in the third name table, the address would be:

```
10NNYYYYYXXXXX
10100010100011  ==> $28a3
--~~-----~~~~~
^ ^ ^    ^
| | |    |
| | y=5  x=3
| |
| 3rd name table
|
fixed
```

Fetching the byte `$a3` starting from the third name table at `$2800`
