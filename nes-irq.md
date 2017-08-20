#NES interrupt

## NMI

NMI input is connected to an edge detector, which detects NMI signal
high-to-low change during the second half of each CPU cycle, and raises
an internal signal during the next cycle if a change is detected.
This internal signal is kept high until the NMI is handle.

```
                                                            NMI handled
                                                                 |
                                                                 V
internal signal       ---------- ---------- ++++++++++       +++++-----

polled NMI input           +++++      -----

CPU cycle             1111122222|1111122222|1111122222| ... |1111122222
                      ^    ^            ^
                      |    |            |
                     first |            |
                          second    high-to-low
                                    detected
```


##IRQ
IRQ input is connected to a level detector, which detects IRQ signal
high-to-low change in the same manner as NMI, but the internal signal
is kept high only during the next cycle if a change is detected.

```
                                          only remains high
                                          during the next cycle
                                                |
                                                V
internal signal       ---------- ---------- ++++++++++ ----------       ----------

polled IRQ input           +++++      -----

CPU cycle             1111122222|1111122222|1111122222|1111122222| ... |1111122222
                      ^    ^            ^
                      |    |            |
                     first |            |
                          second   high-to-low
                                   detected
```

##Polling
The output of the edge/level detectors will be polled during the last cycle
of an instruction in order to determine if there is any pending interrupt.
If NMI and IRQ are both pending, only NMI is handled and the IRQ forgotten.

Once an interrupt is detected, the interrupt sequence (jump to a certain address
containing the entry of the handler) will be executed rather than the next instruction.
The sequence itself does not do any interrupt polling. That's why at least
one instruction from the handler will be executed, because polling will be done during
the last cycle of the instructions of the handler.
