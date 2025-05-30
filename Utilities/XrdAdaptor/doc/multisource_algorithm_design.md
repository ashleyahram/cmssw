# Multi-source algorithm design

## Introduction

The existing AAA infrastructure has a relatively high penalty for poor redirection decisions.  A client which is redirected to a "poor" server in terms of network locality (in terms of high latency or low bandwidth) will continue to use that server as long as it doesn't fail outright, even if there is a much better source available.

Improving network locality argues for improving the redirector's redirection logic.  This is certainly needed, however network locality is not the only concern.  A site's storage can degrade in the middle of a file transfer (due to a device loss in the RAID, or sudden spurt of new transfers), making a "reasonable choice" at redirection time perform poorly overall.

To reduce the penalty for poor redirection decisions, we aim to upgrade the CMSSW XrdAdaptor to be multi-source capable.  By actively using multiple sources and probing for additional ones, the client will be able to discover the fastest sources, even if the designation of "fastest" changes through the job's lifetime.

## Design Goals

Design goals of the multi-source Xrootd client:
0. Determine a metric for quality of the source server.
1. Actively balance transfers over multiple links in order to determine several high-quality sources of the file.
2. Recover from transient IO errors at a single source.
3. Probe for additional possible sources during the file transfer.
4. Minimize the impact on the source site versus a single-source client. Understand both average case and the worst case scenarios.
   - In particular, actively utilizing too many sources can cause TCP windows to stay small and OS read-ahead to be inefficient.
   - Any speculative probes for additional sources should be a small percentage of the total traffic.
   - Any retry mechanisms must have reasonable delays prior to failure.
5. Have the number of requests per source be proportional to source quality.
   - Servers or sites experiencing a "soft failure" causing a degrade in quality will receive the least amount of traffic.

## Implementation

### Quality metric

A source's quality is to be defined, per-file, to be the average request response time over the last 5 time intervals. Each time interval is one minute long; time intervals with no data points are discarded.

If there is no previously recorded data for a given source, it is assumed to have an average of 260ms (assumes a 256KB request, 1MB/s server speed, and 10ms of latency) in one time interval. When a new file is opened on a source, the prior average for that source is used for the first time interval.

Notes:
- The request splitting algorithm outlined below will split all client requests into a series of requests at most 256KB (similar to what the Xrootd client does internally).  Since the request has a maximum size, it makes looking at the unweighted time per request more reasonable.
- It may seem strange to the reader to not differentiate between bandwidth and latency, or somehow factoring in request size to the quality metric.  I believe it is acceptable to ignore this as the distribution of small and large requests will remain approximately constant throughout the job lifetime.

### Source selection algorithm

The client will maintain a set of up to two "active servers" and an arbitrary number of inactive servers. When the client opens a file, the initial data server it receives from the redirector becomes the first active server.

For a 5-second grace period, this initial server remains the only active one. (The use of "5 seconds" as the grace period is motivated by the redirector's implementation; any internal file location requests triggered by the initial file open should be finished after 5 seconds.) After the grace period, the client enters source search mode.

When in source search mode, the client will try trigger a new file open at most once every 5 seconds. When trying a new file-open, it explicitly requests the redirector avoid any previously-used sources for that file. Only one file open operation may be in progress at a time. When a file-open is complete, that server is added to the set of active servers.  When a "file not found" is returned by the redirector, a new file open will not be performed for at least 2 minutes.

When an active source's quality goes above 5130 (256kb request, 50kb/s bandwidth, 10ms latency), the source is moved to the inactive set if it is not the only active server.

If an active source's quality is a factor 10 worse than the other active source, it is moved to the inactive set.

When there is only one source in the active set, a source is promoted from the inactive set if its quality is below 5130 and is not factor 10 worse than the other active source. If no server in the inactive set is eligible, the client re-enters search mode for additional sources.

If a source encounters an error (either a file IO error or a disconnect), then it is marked as disabled and removed from both active and inactive sets.

If an inactive source's quality metric is better than an active source's metric, the two are swapped. This swap is not performed if the inactive source itself has been removed from the active set in the last two minutes. The "Active probe algorithm" section below describes one mechanism for updating an inactive source's quality metric.

### Request splitting algorithm

When a client performs a new request, the request is balanced amongst the active servers using the following algorithm:

1. Source A removes 256KB from the beginning of the request and places it at the end of its request queue.
2. Source B removes 256KB from the end 256KB of the request and places it at the beginning of its request queue.
3. Steps (1) and (2) are repeated until the original request has been completely split into the two queues.

After each client request has been fully split, the labels "A" and "B" are swapped. This allows a series of small client requests to be load-balanced between the two sources.

Examples:

For example, suppose a client requests to read 1024KB starting at offset 0. The client request and queues look like:
```
A: []
B: []
client request: [1024KB @ 0]
```

After steps 1 and 2, the sources have the following queues:
```
A: [256KB @ 0]
B: [256KB @ 768KB]
client request: [512KB @ 256KB]
```

After steps 1 and 2 are applied again, we have the following queues:
```
A: [256KB @ 0, 256KB @ 256KB]
B: [256KB @ 512KB, 256KB @ 768KB]
client request: []
```

Consider the following example for a vectored read request:
Start:
```
A: []
B: []
client request: [192KB @ 0, 128KB @ 256KB, 128KB @ 512KB, 192KB @ 768KB]
```

Iteration 1:
```
A: [192KB @ 0, 64KB @ 256KB]
B: [64KB @ 576KB, 192KB @ 768KB]
client request: [64KB @ 320KB, 64KB @ 512KB]
```

Iteration 2:
```
A: [192KB @ 0, 64KB @ 256KB, 64KB @ 320KB, 64KB @ 512KB]
B: [64KB @ 576KB, 192KB @ 768KB]
client request: []
```

Notes:
- The algorithm halts because, after each iteration, the client request is reduced by 512KB and the algorithm terminates once the client request is empty.
- The request is split between the two clients; the difference in number of bytes assigned is at max 256KB.
- 256KB is a natural unit of size as the Xrootd client break requests into this size by default.
- If the client request, when performed in order, results in the server performing non-overlapping reads with monotonically-increasing offsets, then the split requests will also have this property.
- TODO: We should have the initial assignment done with respect to the quality metric.  When the two servers are heavily out-of-balance, one may steal a lot of work from the other.  When work is stolen, it is performed via backward reads, which won't cause ideal filesystem performance.

### Load-balance algorithm

When a client request is made, it is split into two sets of requests as described previously if there are two active servers. Each source performs the IO operations in its queue in order to completion.

If two sources are active and one source has already finished its queue, it may steal work from the end of the other source's queue if the other source has not already started on that IO operation.

If one source has not completed its request in more than 4 times the quality metric and the other source is idle, then the other source may speculatively start the same IO operation. The results of this "speculative read" are stored in a separate, statically-allocated 256KB buffer; this means only one speculative read at a time is allowed. The first request to complete is returned to the client.

If an IO error occurs on one active source, the same IO operation is inserted into the other source's queue. If the IO operation fails in the other active source, it is repeated immediately on all inactive source. The first inactive source to successfully complete the IO is swapped into the active set, removing the currently worst-performing active source.

Notes:
- Due to the way the quality metric is calculated, it could take a few minutes for an active source that starts moving data at 1 byte / sec to become inactive. Should we re-weight the time intervals to give more weight to recent results.
  - TODO: calculate the worst case over-read for very slow sources if they get their work stolen all the time.
  - The over-read is probably not as bad as having to take quite some time to give up on the source. We can probably introduce a penalty for sources that are the "victim" of a successful speculative read.

### Active probe algorithm

If the client is not in search mode, for every 1024KB of data read, generate a random number such that there will be a .25% chance the request will also be issued as a speculative read to an inactive source. If it has been at least 2 minutes since the last file-open request to the redirector, there is also a .25% chance the request will trigger a file-open and a speculative read in the redirector.

The results of the speculative read are kept in the same statically-allocated buffer as used by the load-balance algorithm. Only one speculative reads can be issued at a time, including reads from the load-balance algorithm. The timing results of the read are used to update the quality metric for the inactive or new sources. If the request is finished prior to the active source, the results of the speculative read will be used to fulfill the client request.

Notes:
- Assuming a client request is 256KB, the active probe algorithm should be no more than 2% of the total traffic.

