#!/bin/sh

ip link add br0 type bridge
ip link set br0 type bridge vlan_filtering 1

ip link set lan1 master br0
ip link set lan2 master br0
ip link set lan3 master br0
ip link set lan4 master br0