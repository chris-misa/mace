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
# insmod ./mace.ko
```

## Mace sysfs Interface


The MACE interface is found at `/sys/kernel/mace/` assuming sysfs is mounted at `/sys`.
The knobs are described below.

Since containers tend to have sysfs mounted read-only, you might need to
bind mount the mace control directory into your container. Since mace
is actively namespace aware, calls from the container to these knobs will
still be interpreted relative to the container's network namespace.

### Knobs

* `mace_on`

Writing a non-zero value to this file enables mace for the current network
namespace. Writing a zero disables mace. Reading shows status of current
network namespace.

* `latencies_ns`

Reading from here shows all ingress and egress latencies found in the buffer for hte currnet network namespace.
Note that, due to buffer wrap around, these might not be in strict chronological order.
Writting any value to here clears the latency buffer of entries for the current network namespace.

