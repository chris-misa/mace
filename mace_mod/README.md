# MACE

MACE (Measure the Added Container Expense) is a kernel network stack latency monitor geared towards measuring container networking overheads.
By hooking into common trace-events, MACE is able to dynamically report network stack latency on a per-packet basis.

## Install

Assuming the proper kernel headers are where they should be, just

```
# make
```

## Run

```
# insmod mace.ko
```

Make sure sysfs is mounted. (Here we assume it is mounted at `/sys`).
The MACE interface is found at `/sys/kernel/mace/`. Each available file is described below.

* `latencies_ns`

Reading from here shows all ingress and egress latencies found in the buffer.
Writting to here clears the latency buffer.

