#/bin/bash
CEIL=5gbit
/sbin/tc qdisc add dev eth1 root handle 1: htb default 10
/sbin/tc class add dev eth1 parent 1: classid 1:1 htb rate ${CEIL} ceil ${CEIL} mtu 9000
/sbin/tc class add dev eth1 parent 1:1 classid 1:2 htb rate 3gbit ceil 3gbit mtu 9000
/sbin/tc class add dev eth1 parent 1:2 classid 1:10 htb rate 2gbit ceil 3gbit mtu 9000
/sbin/tc class add dev eth1 parent 1:2 classid 1:11 htb rate 1gbit ceil 3gbit mtu 9000
/sbin/tc class add dev eth1 parent 1: classid 1:12 htb rate 2gbit ceil ${CEIL} mtu 9000
/sbin/tc qdisc add dev eth1 parent 1:10 handle 10: tpq rate 2gbit
/sbin/tc filter add dev eth1 protocol ip parent 1: prio 1 u32 match ip dport 5001 0xffff flowid 1:11
