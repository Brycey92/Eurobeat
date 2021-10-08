#!/bin/sh

tce-load -il iproute2
sudo ip link set can0 up type can bitrate 500000
