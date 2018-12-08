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
by the given flow specification. Again, leaving grouping of send / recv events into RTTs to later developments.


### Grammar

```
<Protocol Filter> ::= dev-id:<Num> proc-id:<Num> in:<Flow Spec> out:<Flow Spec>

<Flow Spec> ::= flow:<Fields> info:<Fields>

<Fields> ::= [<L2 Fields>] [<L3 Fields>] [<L4 Fields>]

<L2 Fields> ::= (eth_src_addr:<Data> | eth_dst_addr:<Data> | eth_proto:<Data>)+

<L3 Fields> ::= (ip_src_addr:<Data> | ip_dst_addr:<Data> | ip_proto:<Data>)+

<L4 Fields> ::= (tcp_seq:<Data> | icmp_seq:<Data> | icmp_id:<Data>)+

<Data> ::= (Some number of bytes depending on what this is data for....)

```

In English, a Protocol Filter contains four items:
  (dev-id) the device id of the outer (physical) device in the requested device path;
  (proc-id) the process, as seen from the root namespace, of the measurement process;
  (in) a flow specification for inbound events
  (out) a flow specification for outbound events

Flow specifications contain two items:
  (flow) a list of fields which should be used to filter events into the desired flow;
  (info) a list of fields which should be reported with in / out latencies in to general output for this protocol specification.


## Design

Flow filteration and correlation. A given installed flow filter might allow some events to be discarded before correlation checks.

Correlation should be automatic to the system using `skb_addr` or other skb attributes readable at all layers.

The question is how to compile filtering and correlation check so as to attain the highest level of efficiency. . .

User-space kernel-space aspect: the params available at the sendto / recvmsg
tracepoints are all in userspace, while the params available at the
device layer are all in kernel space.


Maybe get a reference to the socket struct using
```
struct socket *sock = sockfd_lookup_light(fd, &err, &fput_needed);
```
Where fd is the file descripter relative to the calling process
(i.e. the first argument to sendto()).

