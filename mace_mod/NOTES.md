# Implementation Notes

The general implementation philosophy is 'as light as possible.'

## Correlation

MACE uses the first 8 bytes after the ip header to form a hash key for each skb,
allowing the identification of distinct packets at different tracepoints.
This should capture sufficient identifying information for packets in a number of common formats.
For example, the first 8 bytes for the following protocol headers contain:

  ICMP: type, code, checksum, id, sequence (entire ICMP header);
  UDP: source port, dest port, length, checksum (entire UDP header);
  TCP: source port, dest port, sequence number.

## Synchronization

Intra-kernel egress and ingress tables are protected by per-bin spin locks.


