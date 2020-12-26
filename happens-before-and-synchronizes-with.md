# Happens-Before

Happens-before relation is:
- formal: it has nothing to do with actual runtime scenarios
- optional: it exists only when the language standard says it exists 

Since it's formal:
- happens-before does *not* imply actually happening before
Example: compiler reordering. Things at runtime may not be
what they look like in the code.

- actually happening before does *not* imply happens-before
Example: a runtime synchronizes-with between two threads without enforced ordering.
One access just happens before the other but they don't have a happens-before relation.


# Synchronizes-With

Synchronizes-with relation is:
- at runtime
- a relation involving a *payload* and a *guard variable*
- a relation formed not only by read-acquire and write-release
although they are very common. It can also be formed by
for example thread creation and join, mutex lock and unlock.
- a relation that implies happens-before
