# Parallel Cheat Sheet

Split stdin (file `g`) into blocks (each block has 3 bytes) and feed them into a command (`{%}` is the job ID):

```bash
t (master) $ cat g
123456789abcdefgh

t (master) $ cat g | parallel -j5 --spreadstdin --block 3 --recend '' 'echo {%} "$(cat)"'
5 def
4 abc
2 456
1 123
3 789
5 gh
```

Note that default block size is 1M and default record end delimiter is `\n`.
Since the default block size is big if it's not specified you won't see any effect.
Compare the following two runs:

```bash
t (master) $ cat f
one
two
three
four
five
six
seven
eight
nine
ten
eleven

t (master) $ cat f | parallel -j5 --spreadstdin --recend '\n' 'echo {%} "[$(cat)]"'
1 [one
two
three
four
five
six
seven
eight
nine
ten
eleven]

t (master) $ cat f | parallel -j5 --spreadstdin --block 10 --recend '\n' 'echo {%} "[$(cat)]"'
2 [three]
3 [four
five]
4 [six
seven]
5 [eight]
1 [one
two]
2 [nine
ten]
3 [eleven]
```

In the second run, each block is determined by either 10 bytes or an ending `\n`,
whichever comes first.
