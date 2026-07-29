/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * LED Core
 *
 * Copyright 2005 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 */
#ifndef __LEDS_H_INCLUDED
#define __LEDS_H_INCLUDED

#include <linux/rwsem.h>
#include <linux/leds.h>

#define STR(s)     #s
static char * idx_to_str[]= {
    STR(NETDEV_UP),
    STR(NETDEV_DOWN),
    STR(NETDEV_REBOOT),
    STR(NETDEV_CHANGE),
    STR(NETDEV_REGISTER),
    STR(NETDEV_UNREGISTER),
    STR(NETDEV_CHANGEMTU),
    STR(NETDEV_CHANGEADDR),
    STR(NETDEV_PRE_CHANGEADDR),
    STR(NETDEV_GOING_DOWN),
    STR(NETDEV_CHANGENAME),
    STR(NETDEV_FEAT_CHANGE),
    STR(NETDEV_BONDING_FAILOVER),
    STR(NETDEV_PRE_UP),
    STR(NETDEV_PRE_TYPE_CHANGE),
    STR(NETDEV_POST_TYPE_CHANGE),
    STR(NETDEV_POST_INIT),
    STR(NETDEV_RELEASE),
    STR(NETDEV_NOTIFY_PEERS),
    STR(NETDEV_JOIN),
    STR(NETDEV_CHANGEUPPER),
    STR(NETDEV_RESEND_IGMP),
    STR(NETDEV_PRECHANGEMTU),
    STR(NETDEV_CHANGEINFODATA),
    STR(NETDEV_BONDING_INFO),
    STR(NETDEV_PRECHANGEUPPER),
    STR(NETDEV_CHANGELOWERSTATE),
    STR(NETDEV_UDP_TUNNEL_PUSH_INFO),
    STR(NETDEV_UDP_TUNNEL_DROP_INFO),
    STR(NETDEV_CHANGE_TX_QUEUE_LEN),
    STR(NETDEV_CVLAN_FILTER_PUSH_INFO),
    STR(NETDEV_CVLAN_FILTER_DROP_INFO),
    STR(NETDEV_SVLAN_FILTER_PUSH_INFO),
    STR(NETDEV_SVLAN_FILTER_DROP_INFO),
};

static inline int led_get_brightness(struct led_classdev *led_cdev)
{
	return led_cdev->brightness;
}

void led_init_core(struct led_classdev *led_cdev);
void led_stop_software_blink(struct led_classdev *led_cdev);
void led_set_brightness_nopm(struct led_classdev *led_cdev,
				enum led_brightness value);
void led_set_brightness_nosleep(struct led_classdev *led_cdev,
				enum led_brightness value);
ssize_t led_trigger_read(struct file *filp, struct kobject *kobj,
			struct bin_attribute *attr, char *buf,
			loff_t pos, size_t count);
ssize_t led_trigger_write(struct file *filp, struct kobject *kobj,
			struct bin_attribute *bin_attr, char *buf,
			loff_t pos, size_t count);

extern struct rw_semaphore leds_list_lock;
extern struct list_head leds_list;
extern struct list_head trigger_list;
extern const char * const led_colors[LED_COLOR_ID_MAX];

#endif	/* __LEDS_H_INCLUDED */
