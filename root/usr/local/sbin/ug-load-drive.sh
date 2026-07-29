#!/bin/bash

#
# called by ugdriver.service once at boot
#

logger -t "UGREEN-DRIVER" "Start ug-load-drive.sh ugdriver.service..."
lockFile=/tmp/.driver-lock
model="DXP4800 Plus"

if [ ! -e "$lockFile" ]; then
   touch "$lockFile"

   logger -t "UGREEN-DRIVER" "Load DXP4800Plus ugdriver.service"
    modprobe leds-mcu-28a48
    modprobe ug_it86x-cpufan
    modprobe ug_gpio_btn
    modprobe ug_sataio_beep

    modprobe ledtrig-breath-ht32f52231
    modprobe ledtrig-normal-ht32f52231
    modprobe ledtrig-timer2-ht32f52231
    modprobe ledtrig-netdev2
fi

# Setup the front panel LED's
/usr/local/sbin/setup_leds.sh

