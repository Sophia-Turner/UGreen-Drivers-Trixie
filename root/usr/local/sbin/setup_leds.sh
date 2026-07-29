#!/bin/bash

set +o noclobber

# Setup Power Button LED
echo normal > /sys/class/leds/power/trigger
echo 4      > /sys/class/leds/power/color
echo 1      > /sys/class/leds/power/brightness

# Setup Network Status LED
echo normal   > /sys/class/leds/network_stat/trigger
echo 2        > /sys/class/leds/network_stat/color
echo 1        > /sys/class/leds/network_stat/brightness

echo 7      > /sys/class/leds/disk1/color
echo 1      > /sys/class/leds/disk1/brightness
echo breath > /sys/class/leds/disk1/trigger
echo 900    > /sys/class/leds/disk1/delay_off
echo 100    > /sys/class/leds/disk1/delay_on

echo 6      > /sys/class/leds/disk2/color
echo 1      > /sys/class/leds/disk2/brightness
echo breath > /sys/class/leds/disk2/trigger
echo 900    > /sys/class/leds/disk2/delay_off
echo 100    > /sys/class/leds/disk2/delay_on

echo 4      > /sys/class/leds/disk3/color
echo 1      > /sys/class/leds/disk3/brightness
echo breath > /sys/class/leds/disk3/trigger
echo 900    > /sys/class/leds/disk3/delay_off
echo 100    > /sys/class/leds/disk3/delay_on

echo 3      > /sys/class/leds/disk4/color
echo 1      > /sys/class/leds/disk4/brightness
echo breath > /sys/class/leds/disk4/trigger
echo 900    > /sys/class/leds/disk4/delay_off
echo 100    > /sys/class/leds/disk4/delay_on

exit 0

