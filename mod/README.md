# Mace Module

A kernel module to measure and report kernel network stack latency with a focus on latencies induced by virtual network devices and namespaces.

## Interface

Active Measurement Protocol Generalization:
1) send packets
2) wait for packets to return
3) repeat

General report is a stream of either send or receive latencies as filtered
by the given flow specification.

Flow specification fields:



### Grammar

```
<Protocol Filter> ::= in:<Flow Spec> out:<Flow Spec>

<Flow Spec> ::= corr:<Fields> info:<Fields>

<Fields> ::= [<L2 Fields>] [<L3 Fields>] [<L4 Fields>]

<L2 Fields> ::= (eth_src_addr | eth_dst_addr | eth_proto)+

<L3 Fields> ::= (ip_src_addr | ip_dst_addr | ip_prot0)+

<L4 Fields> ::= (tcp_seq | icmp_seq | icmp_id)+

```




## Design


The protocol specs on events can be a little complex.
What goes in the queue should remain small, simple and standardized:

timestamp, packet id (formed some how from all flow filter fields?)


