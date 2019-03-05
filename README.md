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

## Mace Interface

On inserting the module, a network namespace aware device file is created at /dev/mace.
Reads from this device block, returning egress and ingress latencies in nanoseconds as they are computed by the module.

Generally containers will have to be granted access to this device. In docker, user `--device /dev/mace:/dev/mace` option.

### Knobs

* `mace_on`

Writing a non-zero value to this file enables mace for the current network
namespace. Writing a zero disables mace. Reading shows status of current
network namespace.

