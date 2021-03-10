# Hazard Pointer

The purpose is to solve resource reclamation problem
in lock-free data structures.

The idea behind hazard pointer is to let
readers declare that they are still accessing some piece of data
in the data structure in some shared table called hazard pointer records.
The GC thread, when freeing resources, can know that
something is still in use so it won't free them.

The hard part is to make the mechanism lock-free.


# Reference
1. https://pvk.ca/Blog/2020/07/07/flatter-wait-free-hazard-pointers
