#!/bin/sh
killall server_proxy
sleep 1
iptables -F
iptables -t nat -F
iptables -t mangle -F
echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -I POSTROUTING -j MASQUERADE
iptables -t nat -I PREROUTING -p tcp --dport  80 -j REDIRECT --to-port 8025
./server_proxy --port 8025 --thread 2
