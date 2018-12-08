# Mace Module

A kernel module to measure and report kernel network stack latency with a focus on latencies induced by virtual network devices and namespaces.

## Interface

Active Measurement Protocol Generalization:
1) send packets
2) wait for packets to return
3) repeat

Standard events:
`sendto / recvmsg` system calls in userspace.
`net_dev_xmit / netif_recv_skb` events on target device in kernel space.


Need consistent representation of 1) userspace measurement process (pid? pgroup? masks? namespaces?) 2) if devices (device name? device id? namespace?)
For this initial version let's just do things simple: user space by proc id, device by device id.  Any later specifications schemes should be able to map
specific specifications into theses two, well-defined parameter spaces.

General report is a stream of either send or receive latencies as filtered
by the given flow specification.


### Grammar

```
<Protocol Filter> ::= dev-id: <Num> proc-id: <Num> in:<Flow Spec> out:<Flow Spec>

<Flow Spec> ::= corr:<Fields> info:<Fields>

<Fields> ::= [<L2 Fields>] [<L3 Fields>] [<L4 Fields>]

<L2 Fields> ::= (eth_src_addr | eth_dst_addr | eth_proto)+

<L3 Fields> ::= (ip_src_addr | ip_dst_addr | ip_prot0)+

<L4 Fields> ::= (tcp_seq | icmp_seq | icmp_id)+

```

In English, a Protocol Filter contains four items:
  (dev-id) the device id of the outer (physical) device in the requested device path;
  (proc-id) the process, as seen from the root namespace, of the measurement process;
  (in) a flow specification for inbound events
  (out) a flow specification for outbound events

Flow specifications contain two items:
  (corr) a list of fields which should be used to correlate events between userspace and the device layer;
  (info) a list of fields which should be reported with in / out latencies in to general output for this protocol specification.

While the info field list may be empty (in which case only latencies are reported) the corr fields must be carefully chosen to prevent
over sampling of events on the path. Probably we will need some safety catches to prevent requests from slowing down traffic on the host
by requesting overtly general correlations, which when combined with exhaustive info reporting might have significant performance impact.













## Design


The protocol specs on events can be a little complex.
What goes in the queue should remain small, simple and standardized:

timestamp, packet id (formed some how from all flow filter fields?)


