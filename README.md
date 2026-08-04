# UGreen-Drivers-Trixie
UGreen Kernel Modules for Debian Trixie  
Model: UGREEN DXP4800 Plus  
  
This ports UGREEN's kernel drivers to Trixie.  
This will give you fan control and LED control and the watchdog timer, which  
is not needed under Trixie has been removed.  
  
There are some scripts to control the fan CPU and case fan, startup scripts to load  
the drivers at boot, as well as scripts to control of the LED's.  
  
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
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules_install  
depmod -a  
update-initramfs -u  
  
6. Install the root folder scripts, services (enable them), programs  

7. Reboot  

You could activate network activity on the network LED, and activate disk activity
on the RAID LED's but the poor chip can only handle so many interrupts/second. I
have had the kernel panic and lockup the system after running it for a few days.
I don't recommend using the LED's that way. Changing the power button LED color to
indicate CPU temperature is pushing it, but it works.

Just remember, when Debian pushes an updated kernel and you reboot, the kernel drivers
exist in the old kernel folder that will be deleted on the next update. When you update
the kernel with 'apt' just remember to re-compile and install the drivers on the new 
kernel each time.

Trixie fixes the problems UGREEN discovered when they installed Bookworm. UGREEN had to
go with a custom kernel to overcome the problems with Bookworm and their selection of
chips. That is not the case with Trixie.

I don't recommend using bcache, it worked very poorly in actual caching. It was designed
only to increase random read/write requests. Large block data is bypassed. No amount of
tuning gave acceptable level of caching. I switched to dm-cache without LVM. 
I don't need LVM but requires a system service to assemble the array without LVM.

Current dm-cache status after one day:  
  
---=== Cache Status for /volume1 ===---  
Volume size:       72.76 TiB  
  
---=== Cache Metadata Overview ===---  
Block size:        4.00 KiB  
Total allocated:   511.00 MiB  
Used:              11.09 MiB  
Metadata usage:    2.2%  
  
---=== Cache Overview ===---  
Cache mode:        writeback  
Cache policy:      no_discard_passdown  
Block size:        4.00 MiB  
Total allocated:   3.64 TiB  
Used:              1.48 TiB  
Cache usage:       40.7%  
Dirty blocks:      315928 (1234.09 GiB)  
Read hits:         1129792  
Read misses:       385380  
Read hit rate:     74.6%  
Write hits:        8905739  
Write misses:      110505099  
Write hit rate:    7.5%  
Cache demotions:   0 blocks  
Cache promotions:  3972 blocks  
  
=== End ===  
