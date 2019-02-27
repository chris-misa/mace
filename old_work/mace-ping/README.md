Implementation:

Filter on pid and outer device for now.

ping event record struct:
  `egress_rx_ts`
  `egress_tx_ts`
  `egress_dt`
  `ingress_rx_ts`
  `ingress_tx_ts`
  `ingress_dt`
  `seq`

Lemma:
For any given echo request / reply event,
these fields are populated in the above order.

Proof:
By choice of tracepoints, semantics of packet processing in kernel,
and semantics of echo requests / replies.

Implementation:
Allocate block of such structs.


pointers into block:
  `egress_rx_head` // entry where new egress receive events will go
  `egress_tx_head` // entry to begin search for egress transmit events
  `ingress_rx_head` // entry to begin search for ingress receive events
  `ingress_tx_head` // entry to begin search for ingress transmit events
  `read_head`       // next entry to read out into userspace


# Notes

regs <-> arg mapping for sendto syscall:
```
(assuming x86_64, i.e. #define CONFIG_X86_64)
di  : 0 (socket) int
si  : 1 (message) char * (in userspace?)
dx  : 2 (message len) size_t
r10 : 3 (flags) int
r8  : 4 (dest_addr) struct sockaddr *
r9  : 5 (dest_len) socklen_t
```
