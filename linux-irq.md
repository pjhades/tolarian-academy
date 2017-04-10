### Entry and exit
- entry path
  - change to kernel stack
  - must save current register status for later restore
- exit path
  - check if we need schedule
  - check if signals need to be delivered


### ISRs should
- be implemented with little code
- not interfere with other running ISRs when triggered

### ISR types
- critical, must be executed immediately following an interrupt, other interrupts must be disabled
- noncritical, executed with enabled interrupts
- deferrable, can be executed later (tasklets)

### Data structure
- a global array with an entry for each IRQ number
  - `struct irq_desc irq_desc[NR_IRQS]`
  - 256 interrupts on IA-64
  - 16 interrupts on IA-32 with 8256A
  - 224 interrupts on IA-32 with IO-APIC

### Handler types
- high-level ISR, `irq_desc->action`, a chain of `struct irqaction` handlers, called by flow handlers
- flow handling, edge- and level-triggering, called by arch-depending code, `irq_desc->handle_irq`
- chip-level, talk to the specific hardware, `irq_desc->chip`

### `irq_desc->depth`
- >0, an IRQ line is disabled, maintain the number of times that an IRQ line has been disabled
- 0, an IRQ line is enabled

### IRQ status
- `IRQ_DISABLED` disabled IRQ line
- `IRQ_INPROGRESS` during execution of an IRQ handler
- `IRQ_PENDING` CPU has noticed but has not yet executed handler
- `IRQ_MASKED`
- `IRQ_REPLAY` IRQ has been disabled, but a previous interrupt has not yet been acknowledged
- can only be set by controller-specific functions

### Controller
- `struct irq_chip` or `hw_interrup_type`
- see status at `/proc/interrupts`

```
                  +-- irq_desc[i]->action -- irqaction
                  |
    irq_chip -----+-- irq_desc[i]->action -- irqaction
                  |
                  | ...
                  |
                  +-- irq_desc[i]->action -- irqaction
```

### Edge-triggerd interrupts
- do not mask IRQ when processing
  - CPU1 is processing
  - CPU2 received the same
  - same handler shouldn't be executed on two CPUs, so
  - CPU2 sees `IRQ_INPROGRESS` set by CPU1
  - CPU2 sets `IRQ_PENDING`, telling the kernel that there's another IRQ to serve
  - ack the PIC chip and go back to normal execution

### Level-triggered interrupts
- mask IRQ
- set state to in-progress
- call the handler
- unmask IRQ

### **Question**
In detail, how does kernel handle the case that the same IRQ comes multiple times ?

1. If we enter `handle_edge_irq` when the same IRQ is being processed,
we set the IRQ state to pending, mask/ack and leave.
2. If we are about to execute the handler, but find that the IRQ state
is pending, then that IRQ is triggered on another CPU, so the IRQ should be
masked currently, we unmask it, and execute the handler.
3. Mask/unmask require obtaining the lock `desc->lock`. If 1 happens,
the IRQ currently being handled should already entered its handler,
as in the handler the pending state would be cleared, in-progress state
would be set, and `desc->lock` would be released.

The timeline is like:
```
    xxx locked
    --- unlocked

          -pending     +pending
          +inprogress  masked
          |            |
    xxxxxxx------------xxxxxx------------xxxxxxxxxxxx------
          ^            ^                 ^
          |            |                 |
     handle_irq_event  another      handle_irq_event
     starts            IRQ comes    quits, re-aquire
                                    lock
```

```C
    /**
     *	handle_edge_irq - edge type IRQ handler
     *	@irq:	the interrupt number
     *	@desc:	the interrupt description structure for this irq
     *
     *	Interrupt occures on the falling and/or rising edge of a hardware
     *	signal. The occurrence is latched into the irq controller hardware
     *	and must be acked in order to be reenabled. After the ack another
     *	interrupt can happen on the same source even before the first one
     *	is handled by the associated event handler. If this happens it
     *	might be necessary to disable (mask) the interrupt depending on the
     *	controller hardware. This requires to reenable the interrupt inside
     *	of the loop which handles the interrupts which have arrived while
     *	the handler was running. If all pending interrupts are handled, the
     *	loop is left.
     */
     void
    handle_edge_irq(unsigned int irq, struct irq_desc *desc)
    {
    	raw_spin_lock(&desc->lock);
    
    	desc->istate &= ~(IRQS_REPLAY | IRQS_WAITING);
    	/*
    	 * If we're currently running this IRQ, or its disabled,
    	 * we shouldn't process the IRQ. Mark it pending, handle
    	 * the necessary masking and go out
    	 */
    	if (unlikely(irqd_irq_disabled(&desc->irq_data) ||
    		     irqd_irq_inprogress(&desc->irq_data) || !desc->action)) {
    		if (!irq_check_poll(desc)) {
    			desc->istate |= IRQS_PENDING;
    			mask_ack_irq(desc);
    			goto out_unlock;
    		}
    	}
    	kstat_incr_irqs_this_cpu(irq, desc);
    
    	/* Start handling the irq */
    	desc->irq_data.chip->irq_ack(&desc->irq_data);
    
    	do {
    		if (unlikely(!desc->action)) {
    			mask_irq(desc);
    			goto out_unlock;
    		}
    
    		/*
    		 * When another irq arrived while we were handling
    		 * one, we could have masked the irq.
    		 * Renable it, if it was not disabled in meantime.
    		 */
    		if (unlikely(desc->istate & IRQS_PENDING)) {
    			if (!irqd_irq_disabled(&desc->irq_data) &&
    			    irqd_irq_masked(&desc->irq_data))
    				unmask_irq(desc);
    		}
    
    		handle_irq_event(desc);
    
    	} while ((desc->istate & IRQS_PENDING) &&
    		 !irqd_irq_disabled(&desc->irq_data));
    
    out_unlock:
    	raw_spin_unlock(&desc->lock);
    }
```

### Register & free IRQs: chip-level, kernel data structures
- `request_irq` create `irqaction`
- `free_irq`

### Handling
- register state `struct pt_regs`
- interrupts can be handled with kernel stack or specific stacks
- if handle with kernel stack
  - `CONFIG_4KSTACKS` reduce kernel stack from 8KB to 4KB since 1 page is easier to find
  - but kernel stack may not be enough for both kernel and interrupts
    - so add two stacks
      - hardware IRQ stack 
      - software IRQ stack
    - access through
      - `static union irq_ctx *hardirq_ctx[NR_CPUS] __read_mostly`
      - `static union irq_ctx *softirq_ctx[NR_CPUS] __read_mostly`
    - the context holds the stack space and `struct thread_info` of thread running before interrupt comes
    - per-CPU allocate
- normal (8KB) kernel stack
  - per-process allocate

## AMD64
- `do_IRQ`
  - `set_irq_regs`, save state
  - `irq_enter`, maintain statistics, etc.
  - `generic_handle_irq`, call registered ISRs, activate flow handler by `irq_desc[irq]->handle_irq`
  - `irq_exit`, statistics bookkeeping, call `do_softirq` to process softirq
  - `set_irq_regs`

## IA32
- `do_IRQ`
  - `set_irq_regs`
  - `irq_enter`
  - check if need to switch to irq stack
    - no, `desc->handle_irq`
    - yes
      - switch
      - `desc->handle_irq`
      - switch back
  - `irq_exit`

### Interrupt context
- prohibit access to userspace, cannot move data between user/kernel spaces,
cannot forward data directly to the waiting application, it may not be running
after executing the interrupt handler
- scheduler must not be called in the interrupt context
- handler cannot sleep

### Software interrupt
- scarce resource
- service all pending softirq at the end of `do_IRQ`
- a table with 32 entries to hold elements of `struct softirq_action`
- register before kernel can execute them
- each softirq has a unique number, determining the execution order
- `raise_softirq` sets a bit in a per-CPU bitmap
- `do_softirq`
- not in interrupt context (thus time-uncritical)

### `ksoftirqd`
- asynchronously execute softirq
- each CPU has its own softirq
- `wakeup_softirqd` wakes up `ksoftirqd`
  - in `do_softirq`
  - in `raise_softirq_irqoff` called in `raise_softirq`
