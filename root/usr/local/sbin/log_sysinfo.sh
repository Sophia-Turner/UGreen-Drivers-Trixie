#!/bin/bash

# Called by /etc/rc.local

echo 'Started Configuration Capture' > /dev/kmsg	

get_sys_info(){
    echo  "===================SYS INFO START:$date_str=================" >> $LOG_FILE
    cat /etc/os-release >> $LOG_FILE
    if [ -f /tmp/factory/sn.txt ]; then
	    SN=`cat /tmp/factory/sn.txt`
	    echo "-------------------------sys sn:$SN"  >> $LOG_FILE
    fi
	echo "-------------------free -m:$SN"  >> $LOG_FILE
	free -m  >> $LOG_FILE
	echo "-------------------dmi memory:"  >> $LOG_FILE
	dmidecode  -t memory  >> $LOG_FILE
	echo "-------------------lspci:"  >> $LOG_FILE
	lspci -nn  >> $LOG_FILE
	echo "-------------------lsusb:"  >> $LOG_FILE
	lsusb  >> $LOG_FILE
	echo "-------------------lsblk:"  >> $LOG_FILE
        lsblk -o NAME,FSTYPE,LABEL,UUID,PARTUUID,MOUNTPOINT >> $LOG_FILE
	echo "-------------------blkid:"  >> $LOG_FILE
	blkid  >> $LOG_FILE
	echo "-------------------mount:"  >> $LOG_FILE
	mount  >> $LOG_FILE
	echo "-------------------df -lh:"  >> $LOG_FILE
	df -lh  >> $LOG_FILE
	echo "-------------------route -n:"  >> $LOG_FILE
	route -n >> $LOG_FILE
	echo "-------------------FW------"  >> $LOG_FILE
	interfaces=$(ls /sys/class/net | grep '^lan')
	if [ ! -z "$interfaces" ]; then
		for interface in $interfaces; do
			ethtool -i "$interface" | grep  firmware-version >> $LOG_FILE
		done
	fi

	ifconfig | grep wlan0 >/dev/null 2>&1
	[ $? -eq 0 ] && echo "-------------------iwconfig wlan0:"  >> $LOG_FILE && iwconfig wlan0 >> $LOG_FILE
	echo "-------------------ifconfig -a:"  >> $LOG_FILE
	ifconfig -a >> $LOG_FILE
	eval echo "lan1 speed: $(cat /sys/class/net/lan1/speed) Mbps " >> $LOG_FILE	
	eval echo "lan2 speed: $(cat /sys/class/net/lan2/speed) Mbps " >> $LOG_FILE	
	if [ -f /proc/net/bonding/bond-wan ]; then
		echo "-------------------bond-wan:"  >> $LOG_FILE
		cat  /proc/net/bonding/bond-wan  >> $LOG_FILE
	fi
	echo "-------------------ps -aux:"  >> $LOG_FILE
	ps -aux >> $LOG_FILE
   echo  "===================SYS INFO END=================" >> $LOG_FILE
}

START_TIME=`cat /proc/uptime | awk -F. '{print $1}'`

while [ $START_TIME -lt  60 ]; do
echo "get_sysinfo sleep 10, before uptime 60 second" > /dev/kmsg	
sleep 10 
START_TIME=`cat /proc/uptime | awk -F. '{print $1}'`
done 

date_str=`date +%Y%m%d%H%M%S`
LOG_FILE="/tmp/sys_info_log_tmp"

echo "Starting get_sys_info" > /dev/kmsg
get_sys_info

timestamp=$(date +"%Y-%m-%dT%H:%M:%S")

echo "-------------------Disk Power-Off_Retract_Count"  >> $LOG_FILE
for disk in /dev/sd[a-z]; do
    if [ -e "$disk" ]; then

        attribute="Count"
        smart_attribute="192"
        value=$(smartctl -n standby  "$disk" -a | grep "$smart_attribute" | awk '{print $10}')

        echo "$timestamp - $disk - $attribute: $value" >> "$LOG_FILE"
    fi
done

nvme list >> $LOG_FILE
echo "-------------------NVMe Unsafe Shutdowns"  >> $LOG_FILE
for disk in /dev/nvme[0-9]; do
    if [ -e "$disk" ]; then
		attribute="Count"
		smart_attribute="Unsafe Shutdowns"
        value=$(smartctl  -a "$disk"  | grep "$smart_attribute" | awk '{print $3}')
        echo "$timestamp - $disk - $attribute: $value" >> "$LOG_FILE"
    fi
done

cat $LOG_FILE  >> /var/log/syslog
rm $LOG_FILE

echo 'Finished Configuration Capture' > /dev/kmsg	

