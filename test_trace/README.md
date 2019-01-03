Implementation:

ping event record struct:
  `egress_rx_ts`
  `egress_tx_ts`
  `egress_dt`
  `ingress_rx_ts`
  `ingress_tx_ts`
  `ingress_dt`
  `pid`
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


