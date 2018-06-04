#!/bin/sh
killall server_proxy
iptables -F
iptables -t nat -F
iptables -t mangle -F
echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -I POSTROUTING -j MASQUERADE
