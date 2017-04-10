# 内存管理

# 内存管理的层次结构
* node
  * 一块连续内存
  * `struct pglist_data`/`pg_data_t`
  * 成员 `node_zonelists` 定义了一个 node 的 fallback list，即分配内存时各个 node 和 zone 的访问顺序
* zone
  * node 内部细分的区域
  * 数量 `MAX_NR_ZONES`
    * `ZONE_DMA` && `ZONE_DMA32` 设备 DMA 可用区域，<16MB
    * `ZONE_NORMAL` 非 DMA 直接映射，<896MB
    * `ZONE_HIGHMEM` 动态映射的页，>896MB
  * `struct zone_struct`/`zone_t`
* page
  * `struct page`
  * 2.6.11 版本以后采用四级分页，32bit 机器只用 1、4 两级
    * page global directory
    * page upper directory
    * page middle directory
    * page table


# Bootmem
目标是管理初始化期间的内存，重在简单而非性能和通用性。

bootmem 对每个 node 维护一个 `struct bootmem_data` 结构，
其中记录了需要管理的 page 范围，前一次分配的位置和一个 bitmap，
bitmap 记录每个 page 的状态是空闲还是保留。UMA 架构下，全局的
`contig_page_data` 即唯一的 node 保存了一个 `contig_bootmem_data`，
也就是该 node 的 bootmem 管理结构，这个结构肯定需要编译器来分配，
因为此时还不能用 bootmem。

bootmem 在 `setup_bootmem_allocator` 中初始化，bootmem 会根据
E820 调用的结果和具体的 CPU 来决定哪些 page 是需要保留的。
bootmem 自己要用的数据结构所在 page 也要保留。

接口形如 `alloc_bootmemxxxx` 和 `free_bootmemxxx`。bootmem 采用 first-fit 的分配策略，
且可以分配小于 page 的空间。

page allocator 初始化完成后，kernel 通过 `free_all_memoryxxx` 来将 bootmem
管理的 page 移交给 buddy system 并释放 bootmem 所用的内存。

此外，kernel 初始化期间所用的数据结构、函数等也需要清除，
这些是通过编译器属性如 `__attribute__((__section__(".init.text"))) __cold`
来实现，将初始化期间用的函数、数据结构放在单独的 section 里面，
并用 linker script 获得这些 section 的起止地址，在代码中即可直接回收。




# Zoned Buddy System
"The heart":

    __alloc_pages_nodemask(gfp_mask, order, zonelist, nodemask)

kernel 采用的是 anti-fragmentation 策略，即避免碎片。方法是将一个 zone 上每种 order 的 `struct free_area`
分成 `MIGRATE_TYPES` 种 migrate type，每一种 type 描述了一类 page 的可移动性。典型的有三种：
* `MIGRATE_UNMOVABLE` 不能移动的 page，占大多数
* `MIGRATE_RECLAIMABLE` 不能移动，但可回收，比如文件映射
* `MIGRATE_MOVABLE` 可移动，如用户数据

当 kernel 无法在某种 migrate type 上满足内存分配请求时，就在 `fallbacks` 数组里找到下一个 migrate type 重试。

## 分配过程
* 第一次尝试 `get_page_from_freelist`
* 若失败，进入 `__alloc_pages_slowpath`
  * 唤醒 `kswapd` 回收一些 page，并调整 mask，降低水位，第二次尝试分配
  * 若再次失败，尝试忽略水位（`ALLOC_NO_WATERMARKS`）再次分配
  * 若仍失败，后面的尝试比较耗时，且可能让当前线程 sleep，所以先调用 `cond_resched`，然后在 `__alloc_pages_direct_reclaim` 中调用 `try_to_free_pages` 开始同步的 reclaim，然后再尝试分配
  * 若失败，`drain_all_pages` 回收 per-CPU cache 中的 page 然后再次分配
  * 若 `try_to_free_pages` 回收 page 未果（`did_some_progress`），进入 `__alloc_pages_may_oom` 让 OOM killer 杀死一个进程以便回收内存
  * 若 `try_to_free_pages` 回收成功，则通过 `should_alloc_retry` 判断是否应重试分配请求
  * 若重试，则回到 label `rebalance` 处
  * 否则分配失败

找到足够 page 后，调用 `buffered_rmqueue` 完成剩下的工作：
* 判断 page 是否连续
* 从 buddy system 中取下来

如果是分配单页，则由 per-CPU cache 来满足，若 cache 空，则调用 `rmqueue_bulk` 将 page 从 buddy system 加入 per-CPU cache。
否则，调用 `__rmqueue` 检查 page 连续性，并将 page 从 buddy system 中取出来，page 不连续时返回 `NULL`（`__rmqueue_smallest`）。
若 `__rmqueue_smallest` 分配失败，再由 `__rmqueue_fallback` 根据 fallback list 来从其他 migrate type 中分配。

返回 page 之前要根据分配时指定的 mask 做一些初始化，例如 `__GFP_ZERO` 清零，`__GFP_COMP` 构造 compound pages。
compound pages 是将一系列连续的 page 组成一个 huge-TLB page，减少 TLB 中存储的信息，从而降低 TLB miss。

`__rmqueue_fallback` 对于 `MIGRATE_RECLAIMABLE` 类型的分配，会将某个 migrate list 中分配剩下的 page 全部移动到
reclaimable 类型的 migrate list 中（`move_freepages`），这样做可以减少此类型对其他 migrate list 造成的碎片。

若所有的 order 和 migrate type 都不能满足分配，就从 `MIGRATE_RESERVE` 中分配。


## 释放过程
释放的主要逻辑都在 `__free_pages`

* 单页的释放进入 `free_hot_cold_page`，设置其 migrate type 并将该页加入 per-CPU cache hot list。若 per-CPU cache 大小超过了 `pcp->high`，调用 `free_pcppages_bulk` 将 `pcp->batch` 个 page 还给 buddy system
* 多页的释放进入 `__free_pages_ok`，再由 `__free_one_page` 合并 buddy 并释放



#  Highmem 管理

内核地址空间中直接映射之外的部分即 highmem，主要分三块：

* 非物理连续的页的映射区域（VMALLOC 区）
* 永久内核映射（PKMAP 区）
* 固定内核映射（FIXMAP 区）

## 非连续页的分配
有时候需要很大一块内存，但就是找不到，可以和 userspace 一样通过不连续的页映射到连续的虚拟内存空间来解决。

这部分位于直接映射的 896MB 之后，与直接映射区域相隔 8MB 的安全区。起止分别为 `VMALLOC_START` 和 `VMALLOC_END`，
分为很多小块，小块之间隔一个页大小。每个小块由 `struct vm_struct` 描述，所有 `vm_struct` 构成全局的链表 `vmlist`。
隔出来的部分是虚拟内存，所以不影响物理页。

`vmalloc` 一般是用在 kernel module 的实现中，因为 module 可能在任意时间加载到系统中，
可能那时候已经没有足够大的连续物理页了。除 kernel 以外，`vmalloc` 大多被设备和声音驱动调用。

`vmalloc` 使用的物理页都是主动映射到内核的虚拟地址空间，所以一般都是用 `ZONE_HIGHMEM` 中的页。

接口：
* `get_vm_area` 创建 `vm_struct`，最终会调用 `__get_vm_area_node`，在 `vmlist` 中目前创建的所有 `vm_struct` 中找到一个坑，
把新建的 `vm_struct` 放进去
* `remove_vm_area` 从 `vmalloc` 地址空间中删除一个 `vm_struct`
* `vmalloc` 和 `vfree` 分配和释放，`vmalloc` 调用的典型例子是内核模块和设备驱动

`vmalloc` 最终调用 `__vmalloc_area_node`，对这个区域内的所有 page 依次调用 `alloc_page` 或 `alloc_pages_node` 得到 page，
然后由 `map_vm_area` 设置这些 page 的映射。`vfree` 最终调用 `__vunmap`。

`vmalloc` 分配 page 的方式是逐页分配，而非一次性分配一大块 page。因为如果有连续的 page，显然没有必要再用 `vmalloc`，
直接从 buddy system 即可得到。


## 状态查看
* `/proc/pagetypeinfo` 当前各种 migrate type 的 page 分布状态
* `/proc/buddyinfo` 当前 buddy system 状态


## Persistent kernel mappings

```C
// 持久映射区域的起始地址，总共在固定地址映射区下面保留了 LAST_PKMAP + 1 个页
# define PKMAP_BASE ((FIXADDR_START - PAGE_SIZE * (LAST_PKMAP + 1))    \
            & PMD_MASK)
```

* 接口 `kmap` 和 `kunmap`
* 范围 `PKMAP_BASE` 到 `FIXADDR_START`
* 总共可以映射 `LAST_PKMAP` 个 page
* 对每个 page 维护一个引用计数，所有的计数保存在 `pkmap_count` 数组中
  * 0：该 page 可以使用
  * 1：该 page 是 reserved 但不可使用，其原有 TLB 数据未更新
  * >1：记录该 page 的引用计数 +1
* page 的映射关系用 `struct page_address_map` 来描述
* 将所有 `page_address_map` 保存在哈西表 `page_address_htable`

`kmap` 对非 highmem 的 page 直接返回其虚拟地址，对 highmem 的 page 调用 `kmap_high` 完成映射：
* 调用 `page_address` 查哈西表找现有的映射
* 若找到，增加引用计数
* 若找不到，调用 `map_new_virtual`，从 `last_pkmap_nr` 位置开始遍历 `pkmap_count`，尝试找一个引用计数为 0 的可用位置来映射
* 若找到，设置引用计数为 1（之后到 `kmap_high` 中会认为找到并增加），修改页表，并将映射关系插入哈西表
* 若遍历回到了开头，调用 `flush_all_zero_pkmaps` 清掉引用计数为 1 的映射，并刷新 TLB
* 若遍历了整个数组还没找到，就 sleep 等待其他线程解除映射关系

```C
    void *kmap(struct page *page)
    {
        might_sleep();
        // 直接怼它
        if (!PageHighMem(page))
            return page_address(page);
        // 推卸责任
        return kmap_high(page);
    }

    void *kmap_high(struct page *page)
    {
        unsigned long vaddr;

        /*
         * For highmem pages, we can't trust "virtual" until
         * after we have the lock.
         */
        lock_kmap();
        // 看是否已经映射了
        vaddr = (unsigned long)page_address(page);
        // 没有的话就用这个搞一搞
        if (!vaddr)
            vaddr = map_new_virtual(page);
        // 引用计数+1，此时至少应该为2
        pkmap_count[PKMAP_NR(vaddr)]++;
        BUG_ON(pkmap_count[PKMAP_NR(vaddr)] < 2);
        unlock_kmap();
        return (void*) vaddr;
    }

    static inline unsigned long map_new_virtual(struct page *page)
    {
        unsigned long vaddr;
        int count;
        unsigned int last_pkmap_nr;
        // 忽略这个 color 即可
        unsigned int color = get_pkmap_color(page);

    start:
        // 返回 LAST_PKMAP，即总共要检查这么多个 PKMAP 的 slot 看是否
        // 可用。检查完如果没有可用的就睡觉
        count = get_pkmap_entries_count(color);
        /* Find an empty entry */
        for (;;) {
            // 这个函数里面定义了一个 static 变量，没初始化，
            // 也就是从 0 开始，每次返回下一个要检查的 slot 下标
            last_pkmap_nr = get_next_pkmap_nr(color);
            // 这个检查就是判断目前的下标是否绕回到了 0
            if (no_more_pkmaps(last_pkmap_nr, color)) {
                // 如果绕回到了 0，清除所有引用计数为 1 的 slot
                flush_all_zero_pkmaps();
                // 然后重置一下准备重头再来
                count = get_pkmap_entries_count(color);
            }
            // 如果引用计数为 0 则找到可用的 slot
            if (!pkmap_count[last_pkmap_nr])
                break;    /* Found a usable entry */
            // 如果尝试了 count 个 slot 还没找到能用的就 schedule
            // 等待其他线程释放
            if (--count)
                continue;

            /*
             * Sleep for somebody else to unmap their entries
             */
            {
                DECLARE_WAITQUEUE(wait, current);
                wait_queue_head_t *pkmap_map_wait =
                    get_pkmap_wait_queue_head(color);

                __set_current_state(TASK_UNINTERRUPTIBLE);
                add_wait_queue(pkmap_map_wait, &wait);
                unlock_kmap();
                schedule();
                remove_wait_queue(pkmap_map_wait, &wait);
                lock_kmap();

                /* Somebody else might have mapped it while we slept */
                if (page_address(page))
                    return (unsigned long)page_address(page);

                /* Re-start */
                goto start;
            }
        }
        // 根据 PKMAP_BASE 和 slot 的下标算出映射的虚拟地址
        vaddr = PKMAP_ADDR(last_pkmap_nr);
        // 改页表
        set_pte_at(&init_mm, vaddr,
               &(pkmap_page_table[last_pkmap_nr]), mk_pte(page, kmap_prot));

        // 现在状态为 1，因为映射了，但 TLB 还没有刷新
        pkmap_count[last_pkmap_nr] = 1;
        // 把虚拟地址设置进 page 结构
        set_page_address(page, (void *)vaddr);

        return vaddr;
    }
```

因为可能 sleep，所以 `kmap` 不能在中断上下文调用。

`kunmap` 取消 PKMAP 区域的映射。

```C
    void kunmap(struct page *page)
    {
        if (in_interrupt())
            BUG();
        if (!PageHighMem(page))
            return;
        kunmap_high(page);
    }

    /**
     * kunmap_high - unmap a highmem page into memory
     * @page: &struct page to unmap
     *
     * If ARCH_NEEDS_KMAP_HIGH_GET is not defined then this may be called
     * only from user context.
     */
    void kunmap_high(struct page *page)
    {
        unsigned long vaddr;
        unsigned long nr;
        unsigned long flags;
        int need_wakeup;
        unsigned int color = get_pkmap_color(page);
        wait_queue_head_t *pkmap_map_wait;

        lock_kmap_any(flags);
        // 拿到虚拟地址
        vaddr = (unsigned long)page_address(page);
        BUG_ON(!vaddr);
        // 拿到 PKMAP slot 下标
        nr = PKMAP_NR(vaddr);

        /*
         * A count must never go down to zero
         * without a TLB flush!
         */
        need_wakeup = 0;
        // 引用计数-1
        switch (--pkmap_count[nr]) {
        case 0:
            // 不允许减到 0
            BUG();
        case 1:
            /*
             * Avoid an unnecessary wake_up() function call.
             * The common case is pkmap_count[] == 1, but
             * no waiters.
             * The tasks queued in the wait-queue are guarded
             * by both the lock in the wait-queue-head and by
             * the kmap_lock.  As the kmap_lock is held here,
             * no need for the wait-queue-head's lock.  Simply
             * test if the queue is empty.
             */
            // 检查是否有其他线程在等待 slot
            pkmap_map_wait = get_pkmap_wait_queue_head(color);
            need_wakeup = waitqueue_active(pkmap_map_wait);
        }
        unlock_kmap_any(flags);

        /* do wake-up, if needed, race-free outside of the spin lock */
        if (need_wakeup)
            wake_up(pkmap_map_wait);
    }
```

清理引用计数为 1 的 page 的过程：

```C
    static void flush_all_zero_pkmaps(void)
    {
        int i;
        int need_flush = 0;

        // x86 上定义为空操作，见 Professional Linux Kernel Architecture Sec 3.7
        flush_cache_kmaps();

        for (i = 0; i < LAST_PKMAP; i++) {
            struct page *page;

            /*
             * zero means we don't have anything to do,
             * >1 means that it is still in use. Only
             * a count of 1 means that it is free but
             * needs to be unmapped
             */
            // 只搞计数为 1 的页
            if (pkmap_count[i] != 1)
                continue;
            pkmap_count[i] = 0;

            /* sanity check */
            BUG_ON(pte_none(pkmap_page_table[i]));

            /*
             * Don't need an atomic fetch-and-clear op here;
             * no-one has the page mapped, and cannot get at
             * its virtual address (and hence PTE) without first
             * getting the kmap_lock (which is held here).
             * So no dangers, even with speculative execution.
             */
            page = pte_page(pkmap_page_table[i]);
            pte_clear(&init_mm, PKMAP_ADDR(i), &pkmap_page_table[i]);

            set_page_address(page, NULL);
            // 只要有引用计数置 0 的操作，必须要刷 TLB
            need_flush = 1;
        }
        // 刷
        if (need_flush)
            flush_tlb_kernel_range(PKMAP_ADDR(0), PKMAP_ADDR(LAST_PKMAP));
    }
```

刷 TLB 的方法：

```C
// 读写 CR3 显式刷
# define __flush_tlb() __native_flush_tlb()
// 若开了 CR4 的 global page 特性，读写 CR4.PG 位隐式刷
# define __flush_tlb_global() __native_flush_tlb_global()
// 用 invlpg 指令使一条地址记录无效
# define __flush_tlb_single(addr) __native_flush_tlb_single(addr)
```




## Temporary kernel mappings
映射使用的每个范围都有一个标签（就是一个虚拟页的编号），定义在 `fixed_addresses` enum 中。

```C
    enum fixed_addresses {
    #ifdef CONFIG_X86_32
        FIX_HOLE,
    #else
    #ifdef CONFIG_X86_VSYSCALL_EMULATION
        VSYSCALL_PAGE = (FIXADDR_TOP - VSYSCALL_ADDR) >> PAGE_SHIFT,
    #endif
    #ifdef CONFIG_PARAVIRT_CLOCK
        PVCLOCK_FIXMAP_BEGIN,
        PVCLOCK_FIXMAP_END = PVCLOCK_FIXMAP_BEGIN+PVCLOCK_VSYSCALL_NR_PAGES-1,
    #endif
    #endif
        FIX_DBGP_BASE,
        FIX_EARLYCON_MEM_BASE,
    #ifdef CONFIG_PROVIDE_OHCI1394_DMA_INIT
        FIX_OHCI1394_BASE,
    #endif
    #ifdef CONFIG_X86_LOCAL_APIC
        FIX_APIC_BASE,    /* local (CPU) APIC) -- required for SMP or not */
    #endif
    #ifdef CONFIG_X86_IO_APIC
        FIX_IO_APIC_BASE_0,
        FIX_IO_APIC_BASE_END = FIX_IO_APIC_BASE_0 + MAX_IO_APICS - 1,
    #endif
        FIX_RO_IDT,    /* Virtual mapping for read-only IDT */
    #ifdef CONFIG_X86_32
        FIX_KMAP_BEGIN,    /* reserved pte's for temporary kernel mappings */
        FIX_KMAP_END = FIX_KMAP_BEGIN+(KM_TYPE_NR*NR_CPUS)-1,
    #ifdef CONFIG_PCI_MMCONFIG
        FIX_PCIE_MCFG,
    #endif
    #endif
    #ifdef CONFIG_PARAVIRT
        FIX_PARAVIRT_BOOTMAP,
    #endif
        FIX_TEXT_POKE1,    /* reserve 2 pages for text_poke() */
        FIX_TEXT_POKE0, /* first page is last, because allocation is backward */
    #ifdef    CONFIG_X86_INTEL_MID
        FIX_LNW_VRTC,
    #endif
        __end_of_permanent_fixed_addresses,

        // ....
    };
```

x86-32 上，定义为

```C
# ifdef CONFIG_X86_32
    FIX_KMAP_BEGIN,    /* reserved pte's for temporary kernel mappings */
    // 这里总共有 (CPU 数) * (KM 类型总数) 这么多个区域
    // 即在 FIXMAP 的区域中，为每个 CPU 都保留了 KM_TYPE_NR 这么多个 page
    // 这也是为什么 kmap_atomic 调用者不能 block，否则同 CPU 的其他线程可能也创建
    // 了同一个映射，把之前的覆盖掉了
    FIX_KMAP_END = FIX_KMAP_BEGIN+(KM_TYPE_NR*NR_CPUS)-1,
// ...
```

所以这个只是固定映射区的一部分。


```C
// 固定地址映射区总共有这么多个页
# define FIXADDR_SIZE    (__end_of_permanent_fixed_addresses << PAGE_SHIFT)
// 这个区域的起始地址
# define FIXADDR_START        (FIXADDR_TOP - FIXADDR_SIZE)
```

```C
/*
 * We can't declare FIXADDR_TOP as variable for x86_64 because vsyscall
 * uses fixmaps that relies on FIXADDR_TOP for proper address calculation.
 * Because of this, FIXADDR_TOP x86 integration was left as later work.
 */
# ifdef CONFIG_X86_32
/* used by vmalloc.c, vsyscall.lds.S.
 *
 * Leave one empty page between vmalloc'ed areas and
 * the start of the fixmap.
 */
extern unsigned long __FIXADDR_TOP;
// 固定地址映射区的最高地址，要么写死，要么算出来
# define FIXADDR_TOP    ((unsigned long)__FIXADDR_TOP)
# else
# define FIXADDR_TOP    (round_up(VSYSCALL_ADDR + PAGE_SIZE, 1<<PMD_SHIFT) - \
             PAGE_SIZE)
# endif
```

接口 `kmap_atomic` 和 `kunmap_atomic`。

```C
    void *kmap_atomic_prot(struct page *page, pgprot_t prot)
    {
        unsigned long vaddr;
        int idx, type;

        /* even !CONFIG_PREEMPT needs this, for in_atomic in do_page_fault */
        pagefault_disable();

        // 非高区页
        if (!PageHighMem(page))
            return page_address(page);

        // 得到映射类型
        type = kmap_atomic_idx_push();
        // 根据 CPU ID 和类型计算出在 FIXMAP 区域的偏移
        idx = type + KM_TYPE_NR*smp_processor_id();
        vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
        BUG_ON(!pte_none(*(kmap_pte-idx)));
        set_pte(kmap_pte-idx, mk_pte(page, prot));
        arch_flush_lazy_mmu_mode();

        return (void *)vaddr;
    }

    void *kmap_atomic(struct page *page)
    {
        return kmap_atomic_prot(page, kmap_prot);
    }
```


`__fix_to_virt` 获得映射的虚拟地址：

```C
// 这里 x、FIXADDR_TOP 都是虚拟页的页号
# define __fix_to_virt(x)    (FIXADDR_TOP - ((x) << PAGE_SHIFT))
# define __virt_to_fix(x)    ((FIXADDR_TOP - ((x)&PAGE_MASK)) >> PAGE_SHIFT)
```




## 鸡零狗碎
`PF_MEMALLOC` 指示当前线程的内存分配请求都是为了搜寻可用内存，
比如在一次分配的尝试过程中，可能需要调用一些函数本身也需要

Professional Linux Kernel Architecture 中说的是：

> ... set for the task to indicate to the remaining kernel code that all
> subsequent memory allocations are needed in the search for memory.
>
> ...
>
> The call is framed by code that sets the above `PF_MEMALLOC` flag.
> It may be necessary for `try_to_free_pages` to allocate new memory for its own work.
> As this additional memory is needed to obtain fresh memory (a rather paradoxical situation),
> the process should, of course,
> enjoy maximum priority in terms of memory management from this point on — this is achieved by setting the above flag.




# Slab Allocator
管理若干连续 page，加速小对象分配，提高缓存利用率。
## 结构与接口
### cache
数据结构 `struct kmem_cache`

接口
* `kmem_cache_init`
* `kmem_cache_create`
* `kmem_cache_destroy`
* `kmem_cache_alloc`
* `kmem_cache_free`

结构
* cache 在每个 node 维护三个 slab 链表，`struct kmem_list3 lists`：full/partial/empty
* 每个 cache 维护一个 per-CPU 的 `struct array_cache`，按 LIFO 保存最近释放的对象

### slab
  * `struct slab`，可以存在 slab 开头或 slab 外部（`CFLGS_OFF_SLAB`）
  * `struct slab` 后面保存 `kmem_bufctl_t`（即 `unsigned int`）的数组，从 `slabp->free` 开始构成一个空闲对象链表
  * 每个对象按机器字长或 L1 cache line 大小对齐

与 page allocator 交互
* `kmem_getpages`
* `kmem_freepages`

特殊 cache
* `cache_cache` 提供 cache descriptor
* `malloc_sizes` 给 `kmalloc` 用

通过 `cat /proc/slabinfo` 或 `sudo slabtop` 来查看 slab 的状态。


## cache 创建/删除
创建 cache 的主要操作

1. 参数检查
2. 计算 alignment
3. 分配 cache 结构
4. 确定 slab head 存放位置
5. `calculate_slab_order` 调用 `cache_estimate` 根据对象大小迭代地计算 slab 需要的 page 数量
6. 计算颜色
7. `enable_cpucache` 调用 `do_tune_cpucache` 创建 per-CPU cache
8. 将 cache 插入 `cache_chain` 链表

`kmem_cache_destroy` 删除空的 cache

1. 扫描 free slab 链表，调对象 dtor，将 page 还回 page allocator
2. 释放 per-CPU cache
3. 将 cache 从 `cache_chain` 链表删除


## 对象分配/释放
`kmalloc` 和 `kmem_cache_alloc` 最终都调用 `____cache_alloc`。

分配步骤

1. 看 per-CPU cache 是否有空闲对象，有就返回
2. 从 slab 中补充空闲对象到 per-CPU cache 再返回（`cache_alloc_refill`，每次补充 `ac->batchcount` 个对象）
3. slab 中对象也不够用了，增加 slab（`cache_grow`）

`cache_grow` 的主要操作

1. 计算颜色
2. 获取一坨 page（`kmem_getpages`）
3. 创建并初始化 slab head（`alloc_slabmgmt`）
4. 让 `page->lru.next` 和 `page->lru.prev` 分别指向 cache 和 slab 结构
5. 初始化对象，主要是调 ctor、初始化 `slabp->free` 链表（`cache_init_objs`）

`kmem_cache_free` 最终调用 `__cache_free`
释放步骤
1. per-CPU cache 若有空间就把对象放进去
2. 清除一些 per-CPU cache 的空间再释放对象（`cache_flusharray`、`free_block`）

`cache_flusharray` 一次清除 `ac->batchcount` 个对象，然后将 per-CPU cache 中剩下的对象移动到数组开头，即这些对象现在比较冷门。



## cache 管理
当一个对象从 cache 取走时，设置 `l3->free_touched = 1`，即该 cache 最近被用到。
若 `free_touched == 1`，cache reap 时设置为 0 让它下一次可以被 reap 掉。

`l3->next_reap`：下一次 cache reap 的时间。
`l3->free_limit`：cache 中空闲对象的上限

reap 操作周期性地清除各个 cache 的内存，对 `cache_chain` 上的每个 cache，调用
清除一部分 per-CPU cache 中的对象（`drain_array`）。若 `l3->free_touched == 0`，
再释放一部分 free slab（`drain_freelist`）

艹，shrink 是啥时候开始的


## `kmalloc`/`kfree`
`malloc_sizes` 上对 2^5 ~ 2^17 共 13 种大小各自维护一个 `cache_sizes`，即每种大小
分两组对象，分别为 ISA DMA 或常规使用。

使用 slab （slab/slub/slob）的情况下，若请求的大小超过了 `KMALLOC_MAX_CACHE_SIZE`，则调用 `kmalloc_large` 来分配。
进去之后，先根据请求的大小计算出 order，再调 `kmalloc_order_trace`，再到 `kmalloc_order`。在这个函数里面是用
`alloc_kmem_pages` 来分配一定数量的 page，通过 `page_address` 计算映射后的虚拟地址。

```C
    /**
     * page_address - get the mapped virtual address of a page
     * @page: &struct page to get the virtual address of
     *
     * Returns the page's virtual address.
     */
    void *page_address(const struct page *page)
    {
        unsigned long flags;
        void *ret;
        struct page_address_slot *pas;

        // 不是高区，直接根据 page 结构取地址
        if (!PageHighMem(page))
            return lowmem_page_address(page);

        // 哈希，得到桶，桶里都是 page_address_map 结构
        pas = page_slot(page);
        ret = NULL;
        spin_lock_irqsave(&pas->lock, flags);
        if (!list_empty(&pas->lh)) {
            struct page_address_map *pam;

            // 查 persistent kernel mapping，得到映射的地址
            list_for_each_entry(pam, &pas->lh, list) {
                if (pam->page == page) {
                    ret = pam->virtual;
                    goto done;
                }
            }
        }
    done:
        spin_unlock_irqrestore(&pas->lock, flags);
        return ret;
    }
```


## 一些优化和技巧
* slab coloring
  * 控制同一 cache 各 slab 中第一个对象的偏移，避免它们相互竞争 cache line
  * `cachep->colour` 颜色数量，取 0 ~ n
  * `cachep->colour_off` 颜色偏移，颜色 ID * 偏移 = 颜色
  * `l3->colour_next` 下一个颜色
* 让常用数据靠近 CPU
* 内存紧张的时候复用一些成员
* 结构体中经常访问的成员放在前面



# 参考
1. Professional Linux Kernel Architecture
2. Understanding the Linux Kernel
3. Linux Kernel Development
