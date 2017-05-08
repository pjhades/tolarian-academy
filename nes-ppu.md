# NES PPU Notes

I feel that I need to take some notes on NES PPU to understand it
better because:

* I find that information scatters around the Internet,
  some of the documents are even `.txt` files from the last century;
* I am too stupid to understand those that is intuitive and apparent
  to you and to these document authors.

However, I cannot produce anything new. What I am writing here is simply
a summary of what I have read so far.

# PPU Internal Registers

According to the layout of NES PPU, the range `$4000 - $FFFF` mirrors
the lower part of PPU memory. So memory access to PPU memory only needs
14-bit addresses. These addresses have their own formats according to the
different parts of PPU memory that they are going to reference,
and are constructed with the information kept in several internal registers.

The internal registers are (details will be covered later):
* `v`, 15 bits, which holds the address of the pile the PPU is about to access.
* `t`, 15 bits, which holds a "temporary" address shared by `$2005` and `$2006`.
       This register is basicall a "buffer" for `v` as changes to this register
       will be copied to `v` at several points of time during the rendering.
* `x`, 3 bits, which holds the 3-bit fine X position, that is, the X position within a
       8x8-pixel tile.
* `w`, a flag which serves as the "write toggle" of `$2005` and `$2006`.

The `v` and `t` registers have the same format:

```
          11111
    bit   432109876543210
content   yyyNNYYYYYXXXXX
```

where the components are:
* `y`, the fine Y position, the counterpart of `x` mentioned above,
       holding the Y position within a 8x8-pixel tile.
* `N`, the index for choosing a certain name table.
* `Y`, the 5-bit coarse Y position, which can reference one of the 30 8x8 tiles
       on the screen in the vertical direction.
* `X`, the 5-bit coarse X position, the counterpart of `Y`.

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

# Name Table and Attribute Table Access

Accessing name table and attribute table requires only the lower 12 bits extracted
from `v`. The highest 2 bits of the 14-bit PPU address is fixed so that the lower 12 bits
can be used as an offset from the first name table at `$2000`.

Therefore, we use only the `NNYYYYYXXXXX` part of `v` to reference a byte in a name table,
where `NN` selects one of the four name tables and `YYYYYXXXXX` corresponds to
the position of the tile on the screen. So if we want to access the tile at position
`(3, 5)` in the third name table, the address would be:

```
10NNYYYYYXXXXX
10100010100011  ==> $28A3
--~~-----~~~~~
^ ^ ^    ^
| | |    |
| | y=5  x=3
| |
| 3rd name table
|
fixed
```

Fetching the byte `$A3` starting from the third name table at `$2800`

Accessing attribute table is similar except that a 4x4 tile group shares a byte
in an attribute table, so the coarse X and Y positions of a tile should be
shifted 2 bits to the right. Again, the tile at `(3, 5)` finds its attribute
within the third name table at address:

```
10NN1111YYYXXX
10101111001000
--~~----~~~---
^ ^ ^   ^  ^
| | |   |  |
| | |   |  x=3>>2
| | |   |
| | |   y=5>>2
| | |
| | offset of the attribute table from a name table
| | each name table has 960 bytes, i.e. $3C0 == 1111000000
| | that's why the address has 1111 in the middle
| |
| 3rd name table
|
fixed
```

# Pattern Table Access

Recall that the two pattern tables each can accommodate
256 tiles, and each tile is represented by two consecutive
8-byte chunks.

The address used to access a pattern table has the format:

```
          1111
    bit   32109876543210
content   0HRRRRCCCCPTTT
```

where the components of this 14-bit value are:

* `T`: 3 bits, which holds the fine Y offset, which can be used
       to reference a specific byte (a "row") within a 8-byte chunk.
* `P`: 1 bit, which chooses the bit plane, `0` for the lower plane
       and `1` for the higher.
* `C`, `R`: 4 bits each, representing the column and row number of
            the desired tile. According to the memory map of the two
            pattern tables, the 256 tiles in each pattern table can
            be arranged in a 16x16 table. Thus a 4-bit row/column number
            is needed to fetch a specific tile.
* `H`: 1 bit, representing the half of the sprite table, choosing one
       of the two pattern tables.

# Reference

* [https://wiki.nesdev.com/w/index.php/PPU_scrolling](https://wiki.nesdev.com/w/index.php/PPU_scrolling)
* [http://nesdev.com/NESDoc.pdf](http://nesdev.com/NESDoc.pdf)
* [https://wiki.nesdev.com/w/index.php/PPU_programmer_reference](https://wiki.nesdev.com/w/index.php/PPU_programmer_reference)
