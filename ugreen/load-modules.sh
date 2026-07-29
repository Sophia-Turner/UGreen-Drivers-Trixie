#!/bin/bash


#modprobe leds-mcu
modprobe leds-mcu-28a48
modprobe ledtrig-breath-ht32f52231
modprobe ledtrig-netdev2
modprobe ledtrig-normal-ht32f52231
modprobe ledtrig-timer2-ht32f52231
modprobe ug_gpio_btn
#modprobe ug_idx6011pro-sio
#modprobe ug_idx6011-sio
modprobe ug_it86x-sio
modprobe ug_sataio_beep
modprobe ug_it86x-cpufan

echo "Modules loaded:"
lsmod | grep -E 'ug_|led'
