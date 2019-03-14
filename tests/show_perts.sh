#!/bin/bash

echo "sys_enter:"
cat /sys/class/mace/pert_sys_enter
echo "net_dev_start_xmit:"
cat /sys/class/mace/pert_net_dev_start_xmit
echo "netif_receive_skb:"
cat /sys/class/mace/pert_netif_receive_skb
echo "sys_exit:"
cat /sys/class/mace/pert_sys_exit
