#!/bin/bash

modprobe -r leds-mcu
modprobe -r leds-mcu-28a48
modprobe -r ledtrig-breath-ht32f52231
modprobe -r ledtrig-netdev2
modprobe -r ledtrig-normal-ht32f52231
modprobe -r ledtrig-timer2-ht32f52231
modprobe -r ug_gpio_btn
modprobe -r ug_idx6011pro-sio
modprobe -r ug_idx6011-sio
modprobe -r ug_it86x-sio
modprobe -r ug_sataio_beep
modprobe -r ug_it86x-cpufan

echo "Modules loaded:"
lsmod | grep -E 'ug_|led'

