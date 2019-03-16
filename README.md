# MACE

MACE (Measure the Added Container Expense) is a kernel network stack latency monitor geared towards measuring container networking overheads.
By hooking into common trace-events, MACE is able to dynamically report network stack latency on a per-packet basis.

## Install

Assuming the proper kernel headers are where they should be, just

```
# make
```
So far only tested on release 4.15.0.

## Run

```
# insmod ./mace.ko outer_dev=<ifindex of outer network interface>
```
The ifindex for any interface can be found with `ip l`

## Mace Interface

Mace uses the kernel's device model to communicate per-packet latencies to userspace and to allow control of mace internals from userspace.
The following files are created after module initilization:

### /dev/

* `mace`

Reads from this file return outstanding egress and ingress latencies for the current net namespace and remove them from the queue.


### /sys/class/mace/

* `mace_on`

Writing a non-zero value to this file enables mace for the current network
namespace. Writing a zero disables mace. Reading shows status of current
network namespace.

## Containers

Generally, containers will need explicit permission to access the mace interface.
In docker, user `--device /dev/mace:/dev/mace` and `-v /sys/class/mace:/sys/class/mace` to allow a container acces to both latencies and knobs.
