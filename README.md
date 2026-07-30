# UGreen-Drivers-Trixie
UGreen Kernel Modules for Debian Trixie
Model: UGREEN DXP4800 Plus

I added a few extra colors on the LED's
If you do add more colors to leds-mcu-28a48.c at line 62, don't forget to update
line 680 (state > 8)

Use the provided scripts in the root folder to enable the LED's, fan control, beeper.

1. Install kernel headers and build tools
apt update
apt install build-essential linux-headers-$(uname -r)

2. Clone the UGreen kernel repo
git clone https://github.com/ugreen-opensource/kernel-6.12
cd kernel-6.12/drivers/ugreen

3. Build the module using your Debian kernel headers
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

5. Install the modules
sudo make -C /lib/modules/$(uname -r)/build M=$(pwd) modules_install
sudo depmod -a
update-initramfs -u

6. Install the root folder scripts, services (enable them), programs

7. Reboot

You could activate network activity on the network LED, and activate disk activity
on the RAID LED's but the poor chip can only handle so many interrupts/second. I
have had the kernel panic and lockup the system after running it for a few days.
I don't recommend using the LED's that way. Changing the power button LED color to
indicate CPU temperature is pushing it.
