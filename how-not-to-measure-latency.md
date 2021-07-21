This is a great talk.

https://www.youtube.com/watch?v=lJ8ydIuPFeU&t=11s


# About P99's lie

## The P99 case is not rare
Assume loading a site results in `n` HTTP requests, then we can calculate
the probability of loading the page experiences the P99 response time:
```
P(loading page experiences P99)
= P(at least one HTTP request experiences P99)
= 1 - P(all HTTP requests don't experience P99)
= 1 - P(one HTTP request doesn't experiences P99)^n
= 1 - 0.99^n
```
Plugging in a bunch of numbers (a few hundred for example) gives over 50% probablity,
which means the P99 case is not rare.

## The percentile you are watching does not describe the real thing
Suppose a user session involves 5 page loads with 40 requests per load,
then how many users will not experience a response time worse than an X-percentile?
It's the same as asking, what is the probability of a user falling within the X-percentile.
```
P(a user falls within PX)
= X^n
```
When X=95, i.e., if we use P95 as the measure, it's `0.95^200 ~= 0.03%`.
That means if we measure response with P95, about 0.03% users will fall within the "normal" case.

If we use P99.9 as the measure, `1 - 0.999^200 ~= 18%` of the users
will experience the P99.9 case, according to the calculation from above,
still not a good enough measure.

## Which percentile should be used

Going in the other direction, let's ask,
what percentile X will be experienced by more than Y percent (say, 95%) of users.
Reusing our calculation, we want
```
X^200 > 0.95
```
We get `X ~= 99.97%`, which means to monitor the experience of 95% of users
we have to watch P99.97 instead. Following the same way,
if we want to monitor the experience of more than 99% of users we have to watch P99.995 instead.

## Use correct histogram library to get things right

`hdrhistogram` makes precision configurable. Precision is represented by
the number of significant digits. For example if we use 3 significant digits,
any observed value X will be put in a bin/interval `[A, B)` such that

```
X - 1/1000*X <= A <= X < B <= X + 1/1000*X
```

That is, the more the number of significant digits, the thinner any bin is,
thus the more precise that any observed value falls within a bin.

The purpose of libraries like `hdrhistogram` is that, if we naively split the value range into bins,
it's very likely that the bins are not precise enough (resolution is low).
Since we may have to watch the 99.9999% percentile,
we may need to go into a certain bin, but the lack of resolution may lead to the fact that
we may not be able to distinguish 99.9999% percentile from 99.8888% percentile.
In other words, if we want to watch 99.9999% percentile, we would need a finer resolution.


# Coordinated omission

Coordinated omission sneaks in when bad measurements are thrown away
because the measurement tool happens to stop measuring when the system
behaves poorly. A typical example is

```
start = get_timestamp();
call_an_expensive_function();
emit_metric("time", get_timestamp() - start);
```
But if the system stalls for some reason, say, context switching,
you'll get one bad result because of that. But during the stall
the measurement stops as well so if this is a request processing system,
there might be thousands of other requests waiting in the queue but
when the system comes back from the stall everything will be good as if
the stall never happened and never affected the waiting requests.


# Service time vs response time

Service time is the time spent on preparing your tacos, while
response time is that time plus the time you spent on waiting in the queue.

When using a load generator to push the system beyond its limit,
response time will be experienced but coordinated omission may hide that
from the result because you may end up only getting the service time,
which stays the same.


# Finding out sustainable throughput

Sustainable throughput is the throughput achieved while safely
maintaining service levels. So the goal is to try to improve that
throughput, the throughput you can get before hitting an incident.

The goal of saturating a system to find its limit is to find out
how quickly we approach that limit, rather than looking into the limit
itself and try to tweak specifically for that point. When two systems hit
the limit, they will both provide bad service level, which is not acceptable
in any case. Thus improving latency should make the time that we hit the limit
as late as possible.


# Comparing latency of two systems

Given the above points, to compare the latency of two systems, A and B,
we can keep one system working at a fixed throughput, which gradually
lowering/raising the throughput of the other. Then we can draw conclusions like,

> If the goal is to make 99% percentile below 10ms, B delivers similar 99% percentile
while carrying 10x the throughput.

In this way, we are comparing the "speed" that the two systems approach the limit
and apparently we would choose the system that has more "capacity" before hitting the limit.

To draw that conclusions we need the goal. Otherwise comparing curves or numbers makes no sense.

Another way is to test the systems with different amount of throughput, say from
10K to 80K QPS, do the same 10K-80K tests for both systems and compare them.
