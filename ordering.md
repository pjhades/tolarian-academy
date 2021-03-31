# Ordering

## Store Buffer (SB)
SB enables "speculative stores" to other CPU's cache lines:
storing CPU can proceed without having to wait for the acknowledgement of
read invalidates from other CPUs.

But, two things must be taken into consideration:

- For cache designers, loads should also check SB otherwise there will be inconsistency.
- For programmers, barriers should to be used in certain cases.
  Barriers mark the entries in SB and guarantee that
  stores AFTER the barrier will wait until the marked entires in SB are flushed,
  i.e. the stores will temporarily be invisible to other CPUs

## Invalidate Queue (IQ)
IQ enables "speculative acknowledgements" of other CPU's invalidate messages:
receiving CPU can send acknowledgement without having to actually invalidate
the cache line but just queue the invalidate messages.

But, for programmers, barriers should be used in certain cases.

Barriers mark the entries in IQ and guarantee that
loads AFTER the barrier will wait until the marked entries in IQ are flushed,
i.e. the pending invalidate messages are processed.

## Read and Write Barriers
Apparently SB and IQ are both speculative ways to improve
the delay of cache coherence messages. SB tries to improve
from stores while IQ tries to improve from loads.

In that sense, we may need weaker barriers to only
control either SB or IQ. That's how read and write
barriers come into play.


## Reference
- http://www.puppetmastertrading.com/images/hwViewForSwHackers.pdf
