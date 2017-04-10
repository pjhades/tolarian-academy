1. [http://fms.komkon.org/EMUL8/NES.html](http://fms.komkon.org/EMUL8/NES.html)
2. [http://nesdev.com/NESDoc.pdf](http://nesdev.com/NESDoc.pdf)
3. [http://nesdev.com](http://nesdev.com)

#  CPU
* 2A03, based on 6502
* 8-bit data, 8-bit control, 16-bit address (64K, $0000 ~ $ffff)
* registers
  * 16-bit PC, 8-bit A, 8-bit SP, 8-bit X and Y, 8-bit P (status)
  * status bits (high to low): 7-Neg, 6-Overflow, 5-, 4-Break, 3-Decimal, 2-Interrupt, 1-Zero, 0-Carry
* interrupt
  * 7 cycles latency
  * reset > NMI > IRQ
  * IVT at $fffa ~ $ffff
  * handling
      1. push PC and P
      2. disable interrupt
      3. load IVT entry to PC
      4. execute
      5. restore PC and P
      6. enable interrupt and continue
  * where to jump
      * reset: $fffc and $fffd
      * NMI: $fffa and $fffb (PPU V-blank at the end of frame)
      * IRQ: $fffe and $ffff (BRK instruction)
* addressing modes
  * see [http://www.emulator101.com.s3-website-us-east-1.amazonaws.com/6502-addressing-modes/](http://www.emulator101.com.s3-website-us-east-1.amazonaws.com/6502-addressing-modes/)
  * zero-page addressing should wrap back if overflow



#  Main memory map

```
+--------------------+ $10000
| PRG-ROM upper bank |
+--------------------+ $c000
| PRG-ROM lower bank |
+--------------------+ $8000
|   Cartridge RAM    |
|      may be        |
|  battery-backed    |
+--------------------+ $6000
|   Expansion ROM    |
+--------------------+ $5000
|   I/O registers    |
+--------------------+ $4000
|      Mirrors       |
|  $2000 ~ $2007     |
+--------------------+ $2008
|   I/O registers    |
+--------------------+ $2000
|      Mirrors       |
|  $0000 ~ $07ff     |
+--------------------+ $0800
|      RAM           |
+--------------------+ $0200
|     stack          |
+--------------------+ $0100
|     zero page      |
+--------------------+ $0000
```


# cartridges
* contain
  * PRG-ROM: game code/data, loaded to main memory $8000 - $ffff (32KB)
  * CHR-ROM: store pattern tables, loaded to PPU memory $0000 - $1fff
    * usually 8KB for 2 pattern tables
    * >8KB VROM mapped to PPU memory with mappers
  * additional CHR-RAM
* cartridges with small/large ROM
  * with 16KB ROM, only use $c000 - $ffff
  * with >32KB ROM, mappped to main memory with mappers
* some cartridges have non battery-backed RAM at $6000 - $7fff

# mappers
* aka, MMC, memory management chip
* large games occur, putting a challenge to the NES system limit
* circuitry on the cartridge to swap game data in and out of memory


# PPU
* chips
  * Ricoh RP2C02 (NTSC)
  * Ricoh RP2C07 (PAL)
* 2KB Video RAM (VRAM) on separate SRAM chip to store name tables
* 256-byte object attribute memory (OAM)
* 32-byte palette RAM, allowing selection of background and sprite colors
* support 8x8 and 8x16 sprites

## PPU I/O
* done during V-blank, I/O registers: $2006 (address) and $2007 (data)
* write
  * write high address to $2006
  * write low address to $2006
  * write data to $2007, after each write, the address will increment
    * by 1 if bit 2 of $2000 is 0
    * by 32 if bit 2 of $2000 is 1
* read
  * write high address to $2006
  * write low address to $2006
  * read data from $2007, the first read is invalid, after each read, the address will increment by 1

## name tables/attribute tables
### name tables (things on the screen)
* a matrix of numbers, pointing to tiles stored in pattern tables
* each byte controls one 8x8 pixel character cell like text mode screen buffer
* one name table has 30 rows x 32 tiles, 1 tile = 8x8 pixels, so the screen
  * NTSC TV: upper and lower 8 pixels are shown off, so 224x256 pixels
  * PAL TV: full 240x256 pixels

PPU supports 4 name tables: $2000, $2400, $2800, $2c00, but NES can only use 2, other 2 are mirrored.
The 4 name tables are arranged as:

         (0,0)     (256,0)     (511,0)
           +-----------+-----------+
           |           |           |
           |           |           |
           |   $2000   |   $2400   |
           |           |           |
           |           |           |
    (0,240)+-----------+-----------+(511,240)
           |           |           |
           |           |           |
           |   $2800   |   $2C00   |
           |           |           |
           |           |           |
           +-----------+-----------+
         (0,479)   (256,479)   (511,479)

Note that each name table represents 30x32 tiles which is 240x256 pixels.
NES has only 2KB VRAM (stored in a separate SRAM chip), enough for 2 name tables.
Hardware on the cartridges controls address bit 11 and bit 10 of the SRAM to perform mapping.

Name table mappings (bit 11 and bit 10):
* (x0) vertical: $2000 == $2800, $2400 == $2c00
* (0x) horizontal: $2000 == $2400, $2800 == $2c00
* (00) one-screen: all name table refer to the same memory
* (xx) four-screen: cartridge contains additional VRAM to use all name tables
* others

### attribute tables
Each byte (64 bytes in total) represents the upper 2 bits of the color numbers for the 2x2 tile groups
in a 4x4 tile group (16 tiles):

    (0,0)  (1,0) 0|  (2,0)  (3,0) 1
    (0,1)  (1,1)  |  (2,1)  (3,1)
    --------------+----------------
    (0,2)  (1,2) 2|  (2,2)  (3,2) 3
    (0,3)  (1,3)  |  (2,3)  (3,3)
     
    Bits   Function                        Tiles
    --------------------------------------------------------------
    7,6    Upper color bits for square 3   (2,2),(3,2),(2,3),(3,3)    
    5,4    Upper color bits for square 2   (0,2),(1,2),(0,3),(1,3)
    3,2    Upper color bits for square 1   (2,0),(3,0),(2,1),(3,1)
    1,0    Upper color bits for square 0   (0,0),(1,0),(0,1),(1,1)

## pattern tables (what can be drawn)
* contain tile images (pattern), represent 8x8 pixel tiles to be drawn on the screen
* PPU supports 2 pattern tables: $0000, $1000
* games uaually store pattern tables in CHR-ROM on the cartridge, or RAM if without CHR-ROM
* each table entry store the lower 2 bits of the 4-bit number to identify image/sprite palette entry
* some roms contain CHR-RAM, which are 8KB banks of pattern tables that can be swapped in/out by mapper

composition of 8x8 tiles on screen:
* background tile = pattern table + attribute table
* sprite tile = pattern table + SPR-RAM

pattern format is:

    Character   Colors      Contents of Pattern Table
                            low               high
    ...*....    00010000    00010000 $10  +-> 00000000 $00
    ..O.O...    00202000    00000000 $00  |   00101000 $28   10 -> 2
    .#...#..    03000300    01000100 $44  |   01000100 $44
    O.....O.    20000020    00000000 $00  |   10000010 $82
    *******. -> 11111110 -> 11111110 $FE  |   00000000 $00
    O.....O.    20000020    00000000 $00  |   10000010 $82
    #.....#.    30000030    10000010 $82  |   10000010 $82
    ........    00000000    00000000 $00  |   00000000 $00
                                +---------+

$0000 ~ $1000 has 0x1000 / 8 / 2 = 256 blocks, corresponding 2 bits in consecutive 2 blocks
are combined to form the lower 2 bits of the index into the sprite or image palette.
Note that only 2 bits for each pixel are stored in the pattern table. The other 2 bits are
obtained from attribute table. So there are 4 bits in total, producing 16 colors available.

## palettes
* NES has a 52-color system palette (actually it can hold 64)
* not all colors can be displayed at a given time
* PPU supports two 16-byte palettes
  * $3f00 image palette: for background tiles
  * $3f10 sprite palette: for sprites
* palettes do not store color values but index into the system palette
* $3f00 is used for transparency, mirrored every 4 bytes:
  $3f00, $3f04, $3f08, $3f0c, $3f10, $3f14, $3f18, $3f1c

## V-Blank flag
* scanline: a line of pixels
* H-Blank: the period for the electron emitter to move to the left for the next scanline
* V-Blank: the period for the electron emitter to move to the top-left of the screen for the next frame

Bit 7 of PPU status register $2002, indicating whether PPU is refreshing
screen, or generating V-Blank impulse. Set to 1 at the end of each frame until the next
screen refresh.

When V-Blank is set, PPU I/O can be done.

## Hit flag
Bit 6 of PPU status register $2002 is set if the first non-transparent pixel of sprite #0
is in the same position as a non-transparent pixel of the background..


## sprites
* 64 sprites, each 8x8 or 8x16 pixels
* stored in one pattern table in PPU memory
* sprite attributes are stored in a 256-byte sprite memroy (SPR-RAM, Object Attribute Memory, OAM), not in CPU or PPU address space
  * SPR-RAM can be written in one DMA operation
  * SPR-RAM can be accessed byte-by-byte by writing address to $2003
    then read/write $2004
* sprite patterns are fetch in the same way as background tile patterns
* sprites are given priority based on their positions in SPR-RAM. sprite #0 has higher priority
* sprites with lower priority on each line are drawn first
* 8 sprites are allowed per scanline, bit 5 of $2002 indicates whether this limit is reached

OAM structure:

    Sprite Attribute RAM:
    | Sprite#0 | Sprite#1 | ... | Sprite#62 | Sprite#63 |
         |
         +---- 4 bytes: 0: Y position of the left-top corner - 1
                        1: Sprite pattern number
                        2: Color and attributes:
                           bits 1,0: two upper bits of color
                           bits 2,3,4: Unknown (???)
                           bit 5: if 1, display sprite behind background
                           bit 6: if 1, flip sprite horizontally
                           bit 7: if 1, flip sprite vertically
                        3: X position of the left-top corner


# PPU graphics (太 tm 复杂了艹你大爷)
* PPU rate is 3 times than CPU
* each frame has 262 scanlines x 341 cycles
* frame rendering (from the last V-Blank)
  * 20 scanlines, no access to PPU memory
  * 1 dummy scanline, fetch data but no pixel is rendered, update hori/vert scroll counters at 256-th cycle
  * 240 scanlines, render graphic data
  * 1 dummy scanline before V-Blank is set
* PPU memory access takes 2 cycles, therefore 170 accesses in each scanline


#  PPU memory map

```
+--------------------+ $10000
|     Mirrors        |
|  $0000 ~ $3fff     |
+--------------------+ $4000
|     Mirrors        |
|  $3f00 ~ $3f1f     |
+--------------------+ $3f20
| sprite palette     |
+--------------------+ $3f10
|  image palette     |
+--------------------+ $3f00
|     Mirrors        |
|  $2000 ~ $2eff     |
+--------------------+ $3000
|  attribute table 3 |
+--------------------+ $2fc0
|      name  table 3 |
+--------------------+ $2c00
|  attribute table 2 |
+--------------------+ $2bc0
|      name  table 2 |
+--------------------+ $2800
|  attribute table 1 |
+--------------------+ $27c0
|      name  table 1 |
+--------------------+ $2400
|  attribute table 0 |
+--------------------+ $23c0
|      name  table 0 |
+--------------------+ $2000
|   pattern table 1  |
+--------------------+ $1000
|   pattern table 0  |
+--------------------+ $0000
```

# rom file format

    Byte     Contents
    ---------------------------------------------------------------------------
    0-3      String "NES^Z" used to recognize .NES files.
    4        Number of 16kB ROM banks.
    5        Number of 8kB VROM banks.
    6        bit 0     1 for vertical mirroring, 0 for horizontal mirroring.
             bit 1     1 for battery-backed RAM at $6000-$7FFF.
             bit 2     1 for a 512-byte trainer at $7000-$71FF.
             bit 3     1 for a four-screen VRAM layout. 
             bit 4-7   Four lower bits of ROM Mapper Type.
    7        bit 0     1 for VS-System cartridges.
             bit 1-3   Reserved, must be zeroes!
             bit 4-7   Four higher bits of ROM Mapper Type.
    8        Number of 8kB RAM banks. For compatibility with the previous
             versions of the .NES format, assume 1x8kB RAM page when this
             byte is zero.
    9        bit 0     1 for PAL cartridges, otherwise assume NTSC.
             bit 1-7   Reserved, must be zeroes!
    10-15    Reserved, must be zeroes!
    16-...   ROM banks, in ascending order. If a trainer is present, its
             512 bytes precede the ROM bank contents.
    ...-EOF  VROM banks, in ascending order.
    ---------------------------------------------------------------------------



我觉得这玩意变态之处有以下几点：
1. NES 硬件不行，所以才会有 mapper 这样的东西，但硬件不行可能是（疑）由于技术发展，也可能是由于 FC 主打家庭市场要控制成本
2. 网上各种资料称呼不同，看了半天发现 SPR-ROM == PPU OAM
