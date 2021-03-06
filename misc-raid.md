#  Standard RAID

##  RAID0
* **striping**
* parallel r/w, n times single disk
* no redundancy
* two parameters
   * stripe width, # disks
   * stripe size, data block size of each disk

```
     +--+--+--+
     |  |  |  |
     A  B  C  D
     E  F  G  H
     I  J  K  L
        ...
```


##  RAID1
* **mirroring**
* write to >= 2 disks redundantly
* no data check
* slow write, fast read
* low disk utility

```  
     +--+----+--+
     A  A    E  E
     B  B    F  F
     C  C    G  G
     D  D    H  H
      ...   ...
```


##  RAID2
* **RAID0 + Hamming Code**
* `2^P >= P + D + 1`, where `P` is # Hamming ECC bits, `D` is # data bits
* 1-bit data writes, with Hamming ECC
* each write involves all disks, so to maximize write performance,
  we'd better keep all heads at the same sector
* better than RAID0 since it takes advantage of disk parallelism
* fit for continuous I/O and large I/O blocks

```
            +--------------------+
            |                    |
      +---+-+-+---+     +-------+-------+
     A0  A1  A2  A3    ECC-A0  ECC-A1  ECC-A2
     B0  B1  B2  B3    ECC-B0  ECC-B1  ECC-B2
     C0  C1  C2  C3    ECC-C0  ECC-C1  ECC-C2
```


##  RAID3
* **RAID2-like striping + parity check**
* n data disks + 1 parity disk
* may restore single disk data according to other n disks
* each write will update the parity disk, so heavy parity disk load for large # writes
* fit for more read and fewer writes scenarios

```
           +-----------+
           |           |
     +---+-+-+---+     +
    A0  A1  A2  A3    A parity
    B0  B1  B2  B3    B parity
    C0  C1  C2  C3    C parity
```


##  RAID4
* **blockwise RAID3**
* r/w by blocks (sectors), small I/O only involves two disks (1 data, 1 parity), not all disks as in RAID3
* improve small I/O performance


##  RAID5
* **compromising RAID0 and RAID1**
* store data and parity in different disks, so that any n - 1 disks have complete data
* high reliability
* higher disk utility than mirroring

```
      +---+---+---+---+
      A0  B0  C0  D0  0P   // xP are parity blocks for level x
      A1  B1  C1  1P  E1
      A2  B2  2P  D2  E2
      A3  3P  C3  D3  E3
      4P  B4  C4  D4  E4
```


##  RAID6
* **RAID5 (level parity) + block parity**
* add another parity block for a blocks on another disk
* very high reliability
* poorer write performance than RAID5
* allow two disk failures
* may restore more data



#  Mixed RAID
* RAID01
  * RAID0 + RAID1, striping then mirroring
* RAID10
  * RAID1 + RAID0, mirroring then striping
  * higher reliability than RAID01



#  Implementation
* software
* firmware/driver
