# Reclamation

## EBR

EBR is a deferred reclamation mechanism for memory management,
which can be used as a workaround of the ABA problem.

- Every thread updates local epoch to global epoch upon entering critical section.
  - It seems that _local_ does not mean _thread-local_ but a snapshot
    which is globally visible because write thread needs it.
- Every thread sets a flag to indicate that it is in critical section.
- Logically deleted nodes are put on a limbo list
  belonging to a certain epoch.
- The GC operation can be performed by any thread at any time.
  - This is the main source of complexity and confusion.
- Only in GC can the global epoch be updated.
- GC thread checks if all other threads have witnessed the current epoch.
  If so, it tries to increment the current epoch.
  Here’s the key: we use 3 epochs, so now we know that
  no thread could be in the third epoch, we can push
  the epoch forward because no other thread falls behind.
- If the global epoch is updated successfully from `E` to `E + 1`...
  - The deleted node is now on the limbo list of epoch `E`.
  - Threads that start accessing the data structure after
    logical deletion can never see the deleted node.
  - Threads that started accessing the data structure before
    logical deletion may still see the deleted node.
  - Since all threads have witnessed epoch `E`...
    - There may be threads that are still accessing the
      logically deleted data because.
      they entered the critical section in epoch `E`.
    - There may be threads that have already witnesses
      the new epoch `E + 1`.
    - As `E + 1` is the latest epoch, no thread is still in epoch `E - 1`.
      Thus objects on the limbo list of epoch `E - 1` can be
      physically deleted safely.

Here's a good article explaining the mechanism in a clear way:

https://aturon.github.io/blog/2015/08/27/epoch/

Papers on this topic tend to be brief,
which may indicate academic discrimination:
they assume that readers are smart enough to
understood it. Or, put in another way, people
who do not understand are not expected to
be the target readers.
