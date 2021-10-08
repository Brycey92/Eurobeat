#!/bin/sh

tce-load -il firmware-rpi3-wireless
tce-load -il wifi
sudo wifi.sh -a
