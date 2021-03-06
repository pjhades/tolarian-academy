# Control
* `cr0.PG == 1` requires `cr0.PE == 1`
* `cr0.PG == 1 && cr4.PAE == 0`: 32-bit paging
  * 32bit linear address
  * up to 40bit physical address
  * 4K or 4M page size if cr4.PSE=1
  * do not support execute disable
* `cr0.PG == 1 && cr4.PAE == 1 && EFER.LME == 0`: PAE paging
  * 32bit linear address
  * up to 53bit physical address
  * 4K or 2M page size
  * support execute disable
* `cr4.PSE`: page size extension
  * `cr4.PSE == 1 && PDE.PS == 1`: PDE maps a 4M page, 40bit physical address
    * `PA[39:32] = PDE[20:13]`
    * `PA[31:22] = PDE[31:22]`
    * `PA[21:0]  = LA[21:0]`
  * `cr4.PSE == 0 && PDE.PS == 0`: naturally aliagned 4K page
    * `PA[39:32] = 0`
    * `PA[31:12] <- PDE`
    * `PA[11:2]  = LA[21:12]`
    * `PA[1:0]   = 0`
  * normal
    * `PA[39:32] = 0`
    * `PA[31:12] <- PTE`
    * `PA[11:0]  = LA[11:0]`

# PAE Paging
* 4K paging
  * PDPTER: page directory pointer entry register
  * `LA[31:30]` finds PDPTE value based on PDPTER
  * `LA[29:21]` finds PDE based on PDPTE value
  * `LA[20:12]` finds PTE based on PDE
  * `LA[11:0]`  finds data based on PTE
* 2M paging
  * `LA[31:30]` finds PDPTE value based on PDPTER
  * `LA[29:21]` finds PDE based on PDPTE value
  * `LA[20:0]` finds data based on PDE

# Memory Typing
* memory type is the type of caching used for that access
* bits in control registers and PDE/PTEs are used to control the caching of VM pages and regions of physical memroy


# TLB
TLB maps page numbers (upper bits of LA) to page frames (upper bits of PA).

`PTE.G == 1 && cr4.PGE == 1` makes a PTE global, i.e. not automatically invalidated in TLB
on task switches or load of cr3.

# Linux
* ver 2.6.10, 3-level paging
* ver >= 2.6.11, 4-level paging
  * page global directory > page upper directory > page middle directory > page table
  * only page global directory and page table are used on 32bit machines,
    the number of page upper/middle diretories are set to 1
 
