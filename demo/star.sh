#!/bin/bash

# usage: vde_test <switch path> <port> [<peer ip> <peer port>]
# a new vde_switch will be started at specified path

sudo true # cache the password now

./vde_test /tmp/vde-1 9001 &
sleep 0.1
./vde_test /tmp/vde-2 9002 &
sleep 0.1
./vde_test /tmp/vde-3 9003 &
sleep 0.1
./vde_test /tmp/vde-master 9000 127.0.0.1 9001 127.0.0.1 9002 127.0.0.1 9003 &
sleep 0.3

echo "nodes started, creating taps"

sudo vde_plug2tap -d -s /tmp/vde-master tap0
sudo ip ad add 192.168.42.42/24 dev tap0
sudo ip li set dev tap0 up

sudo vde_plug2tap -d -s /tmp/vde-1 tap1
sudo ip ad add 192.168.42.1/24 dev tap1
sudo ip li set dev tap1 up

sudo vde_plug2tap -d -s /tmp/vde-2 tap2
sudo ip ad add 192.168.42.2/24 dev tap2
sudo ip li set dev tap2 up

sudo vde_plug2tap -d -s /tmp/vde-3 tap3
sudo ip ad add 192.168.42.3/24 dev tap3
sudo ip li set dev tap3 up

# running "ping -I tap1 192.168.42.42" will fail because linux doesn't respond
# to arp requests from local machine or something. maybe this can be configured
# to work

read key
killall vde_test
sleep 0.2
killall vde_switch
sleep 0.2
