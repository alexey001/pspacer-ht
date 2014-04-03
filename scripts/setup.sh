#!/bin/sh

DEV=eth0

# add the prio qdisc as the root qdisc
sudo /sbin/tc qdisc add dev ${DEV} root handle 1: prio bands 3

# add the PSPacer/HT qdisc whose target rate is 5Gbps
sudo /sbin/tc qdisc add dev ${DEV} parent 1:1 handle 10: pspht rate 5gbit

# add the u32 filter for the iperf benchmark
sudo /sbin/tc filter add dev ${DEV} protocol ip parent 1: prio 1 \
    u32 match ip dport 5001 0xffff flowid 1:1
