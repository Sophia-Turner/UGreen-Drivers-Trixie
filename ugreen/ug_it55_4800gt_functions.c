// SPDX-License-Identifier: GPL-2.0-only
/*
 * Author:Jason.Li
 * date:2026/1/12
 * Description: For 4800gt
 *		control fan and power status Driver.
 * Chip: ITE5571
 * 
 */
#include "linux/delay.h"
#include "linux/kstrtox.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/uaccess.h>
#include <linux/dmi.h>

#undef pr_fmt
#define pr_fmt(fmt) "gt it55: " fmt

#include "it55_helper.h"

#define DEVICE_FAN1_MODE 0xb0 // cpu fan1
#define DEVICE_FAN1_PWM 0xb1
#define DEVICE_FAN3_MODE 0xb4 // sys fan1
#define DEVICE_FAN3_PWM 0xb5
#if 0
#define DEVICE_FAN2_MODE 0xb2 // cpu fan2
#define DEVICE_FAN2_PWM 0xb3
#define DEVICE_FAN4_MODE 0xb6 // sys fan2
#define DEVICE_FAN4_PWM 0xb7
#endif

#define DEVICE_FAN1_SPEED_MBS 0x34
#define DEVICE_FAN1_SPEED_LSB 0x35
#define DEVICE_FAN3_SPEED_MBS 0x38
#define DEVICE_FAN3_SPEED_LSB 0x39
#if 0
#define DEVICE_FAN2_SPEED_MBS 0x36
#define DEVICE_FAN2_SPEED_LSB 0x37
#define DEVICE_FAN4_SPEED_MBS 0x3a
#define DEVICE_FAN4_SPEED_LSB 0x3b
#endif

#define EC_VERSION 0x00
#define EC_SYS_TEMP 0x5F
#define EC_CPU_TEMP 0x70
//#define EC_PWR_ENABLE 0xA0
//#define EC_PWR_VALUE 0xA1

#define AC_POWER_LOST_VAL 0x66
#define AC_POWER_LOST_ON 0x88
#define AC_POWER_LOST_OFF 0x89

#define G3_WAKEUP_ADR 0x9A
#define LAN_WAKEUP_ADR 0x99
#define MAIN_DIR "it86"

//#define POWER_STATUS_REG 	0xfe001020
const char *product = NULL;

struct proc_dir_entry *nas_dir = NULL;
static int mode = 0;
module_param(mode, int, 0644);
static int g_power_status = 0;	/*	4800gt 无法读取ec内的状态，只能读缓存的	*/

/*sys == fan3 & fan4;pwm = 0-198 */
static void set_sys_fan(u8 pwm)
{
	if (pwm >= 0xc6)
		pwm = 0xc6;
	ec_request_ports();
	ec_write_memory(DEVICE_FAN3_MODE, 1);
	ec_write_memory(DEVICE_FAN3_PWM, pwm);
	//ec_write_memory(DEVICE_FAN4_MODE, 1);
	//ec_write_memory(DEVICE_FAN4_PWM, pwm);
	ec_release_ports();
	return;
}

/*cpu == fan1 & fan2; pwm = 0-198 */
static void set_cpu_fan(u8 pwm)
{
	if (pwm >= 0xc6)
		pwm = 0xc6;
	ec_request_ports();
	ec_write_memory(DEVICE_FAN1_MODE, 1);
	ec_write_memory(DEVICE_FAN1_PWM, pwm);
	//ec_write_memory(DEVICE_FAN2_MODE, 1);
	//ec_write_memory(DEVICE_FAN2_PWM, pwm);
	ec_release_ports();

	return;
}

static ssize_t get_power_status(struct file *file, char __user *usr_buf, size_t size, loff_t *ppos)
{
	if (*ppos != 0)
		return 0;
	char tmpbuf[16] = {0};
	int cnt;
	int rc;


	switch(g_power_status) 
	{
		//on
		case 0:
			pr_info("get_power_status: power on\n");
			cnt = snprintf(tmpbuf, sizeof(tmpbuf), "%s\n","power on");
			break;
		case 1:	  // off
			pr_info("get_power_status: power off\n");
			cnt = snprintf(tmpbuf, sizeof(tmpbuf), "%s\n","power off");
			break;
		default:
			cnt = snprintf(tmpbuf, sizeof(tmpbuf), "%s\n","unkown status");
			break;
	}
	rc =  simple_read_from_buffer(usr_buf, size, ppos, tmpbuf, cnt);

	return rc;
}

static int save_power_status( int status )
{
	unsigned int ret_val = 0;
	switch(status)
	{
	    case 0:	//power off
			ec_request_ports();
			ec_send_command(AC_POWER_LOST_OFF);
			ret_val =  inb(EC_C_PORT);
			if(!(0x08 & ret_val))
				pr_err("set ac power lost off fail!\n");			
			ec_release_ports();
			g_power_status = 0;
		break;
	    case 1:	//power on		
			ec_request_ports();
			ec_send_command(AC_POWER_LOST_ON);
			ret_val =  inb(EC_C_PORT);
			if(!(0x08 & ret_val))
				pr_err("set ac power lost on fail!\n");	
			ec_release_ports();
			g_power_status = 1;
		break;
	    case 2:	//last state ,not defined
		    //No this Funciton;
		    break;
	    default:
		    break;
	}
	return 0;

}
static ssize_t set_power_status(struct file *file,const char __user * buffer,size_t count, loff_t * ppos)
{
    int result = 0;
    char buf[12] = { '\0' };
	
	if (count > sizeof(buf) - 1){
        return -EINVAL;
	}
	if (copy_from_user(buf, buffer, count)) {
		result = -EFAULT;
		goto end;
	}
	pr_info(" set_power_status:%s\n",buf);
	buf[count] = '\0';
	if(buf[count -1] == 0x0a) buf[count - 1] = '\0';

	if(!strncasecmp(buf, "on",3)){
		save_power_status(1);
	}else if(!strncasecmp(buf, "off",4)){
		save_power_status(0);
	}else if(!strncasecmp(buf, "last",5)){
		save_power_status(2);
	}else{
		result = -EINVAL;
	}
end:
	if(result){
		return result;
	}
        return count;
}


static ssize_t fan_write(struct file *file, const char __user *buffer,
			 size_t size, loff_t *ppos)
{
	int result = 0;
	char buf[12] = { '\0' };
	char *p;
	unsigned long pwm;

	if (size > sizeof(buf) - 1) {
		return -EINVAL;
	}
	if (copy_from_user(buf, buffer, size)) {
		result = -EFAULT;
		goto end;
	}
	buf[size] = '\0';
	if (buf[size - 1] == 0x0a)
		buf[size - 1] = '\0';
	if (!strncasecmp(buf, "on", 3)) {
		set_sys_fan(127);
	} else if (!strncasecmp(buf, "off", 4)) {
		set_sys_fan(0);
	} else if (!strncasecmp(buf, "set ", 4)) {
		p = buf + 4;
		while ((*p < 0x21) && (*p != '\0'))
			p++;
		if (*p != '\0') {
			if (kstrtoul(p, 10, &pwm)) {
				result = -EINVAL;
				goto end;
			}
			if ((pwm > 0) && (pwm < 256)) {
				set_sys_fan(pwm);
			} else {
				result = -EINVAL;
			}
		} else {
			result = -EINVAL;
		}
	} else if (!strncasecmp(buf, "con", 3)) {
		set_cpu_fan(127);
	} else if (!strncasecmp(buf, "coff", 4)) {
		set_cpu_fan(0);
	} else if (!strncasecmp(buf, "cpu ", 4)) {
		p = buf + 4;
		while ((*p < 0x21) && (*p != '\0'))
			p++;
		if (*p != '\0') {
			if (kstrtoul(p, 10, &pwm)) {
				result = -EINVAL;
				goto end;
			}
			if ((pwm > 0) && (pwm < 256)) {
				set_cpu_fan(pwm);
			} else {
				result = -EINVAL;
			}
		} else {
			result = -EINVAL;
		}
	} else {
		result = -EINVAL;
	}
end:
	if (result) {
		return result;
	}
	return size;
}
static ssize_t fan_read(struct file *file, char __user *buffer, size_t size,
			loff_t *ppos)
{
	if (*ppos != 0)
		return 0;
	char tmpbuf[128];
	int rc = 0;
	unsigned long fan1_val, fan3_val;
	int cnt = 0;
	u8 tmp1, tmp2;
	ec_request_ports();
	ec_read_memory(DEVICE_FAN1_SPEED_MBS, &tmp1) ;
	ec_read_memory(DEVICE_FAN1_SPEED_LSB, &tmp2);
	fan1_val = (tmp1 << 8) + tmp2;

	ec_read_memory(DEVICE_FAN3_SPEED_MBS, &tmp1);
	ec_read_memory(DEVICE_FAN3_SPEED_LSB, &tmp2);
	fan3_val = (tmp1 << 8) + tmp2;

	ec_release_ports();

	cnt = snprintf(
		tmpbuf, sizeof(tmpbuf),
		"cpufan speed:%lu\nsysfan1 speed:%lu\n\n",
		fan1_val, fan3_val);

	rc = simple_read_from_buffer(buffer, size, ppos, tmpbuf, cnt);

	return rc;
}

static ssize_t temp_read(struct file *file, char __user *buffer, size_t size,
			loff_t *ppos)
{
	if (*ppos != 0)
		return 0;
	char tmpbuf[128];
	int rc = 0;
	int cnt = 0;
	u8 tmp1;
	ec_request_ports();
	ec_read_memory(EC_SYS_TEMP, &tmp1) ;
	ec_release_ports();

	cnt = snprintf(tmpbuf, sizeof(tmpbuf),"board_temp:%u\n",tmp1);

	rc = simple_read_from_buffer(buffer, size, ppos, tmpbuf, cnt);

	return rc;
}

static ssize_t version_read(struct file *file, char __user *buffer, size_t size,
			loff_t *ppos)
{
	if (*ppos != 0)
		return 0;
	char tmpbuf[128];
	int rc = 0;
	int cnt = 0;
	u8 ver0,ver1,ver2;
	ec_request_ports();
	ec_read_memory(EC_VERSION, &ver0);
	ec_read_memory(EC_VERSION+1, &ver1);
	ec_read_memory(EC_VERSION+3, &ver2);
	ec_release_ports();

	cnt = snprintf(tmpbuf, sizeof(tmpbuf),"%u.%u%u\n",ver0,ver1,ver2);

	rc = simple_read_from_buffer(buffer, size, ppos, tmpbuf, cnt);

	return rc;
}

static ssize_t g3wakeup_write(struct file *file, const char __user *buffer,
			      size_t size, loff_t *ppos)
{
	char user_buffer[8] = { '\0' };
	int ret;
	ret = copy_from_user(user_buffer, buffer, size);
	if (ret)
		goto err;
	pr_info("%s:%s\n",__func__,user_buffer);
	if (strncmp("on", user_buffer, 2) == 0) {
		ec_request_ports();
		ec_write_memory(G3_WAKEUP_ADR, 1);
		ec_release_ports();
	} else if (strncmp("off", user_buffer, 3) == 0) {
		ec_request_ports();
		ec_write_memory(G3_WAKEUP_ADR, 0);
		ec_release_ports();
	} else {
		return -EBUSY;
	}

	return size;
err:
	return -EINVAL;
}

static ssize_t g3wakeup_read(struct file *file, char __user *buffer,
			     size_t size, loff_t *ppos)
{
	if (*ppos != 0)
		return 0;
	unsigned char out_buffer[128];
	u8 val = 0;
	int ret = 0;
	ec_request_ports();
	ret = ec_read_memory(G3_WAKEUP_ADR, &val);
	ec_release_ports();
	if (val == 0 || val == 1) {
		ret = sprintf(out_buffer, "G3-WakeUp Status:%d\n", val);
	} else
		return -EINVAL;

	ret = simple_read_from_buffer(buffer, size, ppos, out_buffer, ret);
	return ret;
}

static ssize_t lanwakeup_write(struct file *file, const char __user *buffer,
			       size_t size, loff_t *ppos)
{
	char user_buffer[8] = { '\0' };
	int ret;
	ret = copy_from_user(user_buffer, buffer, size);
	if (ret)
		goto err;
	pr_info("%s:%s\n",__func__,user_buffer);
	if (strncmp("on", user_buffer, 2) == 0) {
		ec_request_ports();
		ec_write_memory(LAN_WAKEUP_ADR, 1);
		ec_release_ports();
	} else if (strncmp("off", user_buffer, 3) == 0) {
		ec_request_ports();
		ec_write_memory(LAN_WAKEUP_ADR, 0);
		ec_release_ports();
	} else {
		return -EBUSY;
	}

	return size;
err:
	return -EINVAL;
}

static ssize_t lanwakeup_read(struct file *file, char __user *buffer,
			      size_t size, loff_t *ppos)
{
	if (*ppos != 0)
		return 0;
	unsigned char out_buffer[128];
	u8 val = 0;
	int ret = 0;
	ec_request_ports();
	ret = ec_read_memory(LAN_WAKEUP_ADR, &val);
	ec_release_ports();
	if (val == 0 || val == 1) {
		ret = sprintf(out_buffer, "LAN-WakeUp Status:%d\n", val);
	} else
		return -EINVAL;

	ret = simple_read_from_buffer(buffer, size, ppos, out_buffer, ret);
	return ret;
}

static int null_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, pde_data(inode));
}
static const struct proc_ops fan_proc_fops = {
	.proc_open = null_proc_open,
	.proc_write = fan_write,
	.proc_read = fan_read,
	.proc_release = seq_release,
};
static const struct proc_ops startup_proc_fops = {
	.proc_open = null_proc_open,
	.proc_write	= set_power_status,
	.proc_read	= get_power_status,
	.proc_release = seq_release,
};
static const struct proc_ops temp_proc_fops = {
	.proc_open = null_proc_open,
	.proc_read = temp_read,
	.proc_release = seq_release,
};
static const struct proc_ops version_proc_fops = {
	.proc_open = null_proc_open,
	.proc_read = version_read,
	.proc_release = seq_release,
};
static const struct proc_ops lanwakeup_proc_fops = {
	.proc_open = null_proc_open,
	.proc_write = lanwakeup_write,
	.proc_read = lanwakeup_read,
	.proc_release = seq_release,
};
static const struct proc_ops g3wakeup_proc_fops = {
	.proc_open = null_proc_open,
	.proc_write = g3wakeup_write,
	.proc_read = g3wakeup_read,
	.proc_release = seq_release,
};
static int procfs_create(void)
{
	struct proc_dir_entry *entry = NULL;
	int ret = 0;
	/* create /proc/nas/ */
	nas_dir = proc_mkdir(MAIN_DIR, NULL);
	if (!nas_dir) {
		return -ENODEV;
	}

	entry = proc_create_data("fan", S_IWUGO, nas_dir, &fan_proc_fops, NULL);
	if (!entry) {
		ret = -ENODEV;
		goto remove_dev_dir;
	}

	/* create /proc/nas/startup */
	entry = proc_create_data("startup", S_IRWXUGO, nas_dir,
				 &startup_proc_fops, NULL);
	if (!entry) {
		ret = -ENODEV;
		goto remove_dev_dir;
	}
	
	entry = proc_create_data("temp", S_IRUGO, nas_dir,
				 &temp_proc_fops, NULL);
	if (!entry) {
		ret = -ENODEV;
		goto remove_dev_dir;
	}	

	entry = proc_create_data("version", S_IRUGO, nas_dir,
				 &version_proc_fops, NULL);
	if (!entry) {
		ret = -ENODEV;
		goto remove_dev_dir;
	}
	
	entry = proc_create_data("Lan_wakeup", S_IWUGO, nas_dir,
				 &lanwakeup_proc_fops, NULL);
	if (!entry) {
		ret = -ENODEV;
		goto remove_dev_dir;
	}
	entry = proc_create_data("G3_wakeup", S_IWUGO, nas_dir,
				 &g3wakeup_proc_fops, NULL);
	if (!entry) {
		ret = -ENODEV;
		goto remove_dev_dir;
	}
done:
	return ret;

remove_dev_dir:
	remove_proc_entry(MAIN_DIR, NULL);
	nas_dir = NULL;
	goto done;
}

/***************************** watchdog ************************* */
#include <linux/watchdog.h>
#define WATCHDOG_NAME "IT55 WDT"

/* Defaults for Module Parameter */
#define DEFAULT_TIMEOUT 600
#define MIN_TIMEOUT 1
#define MAX_TIMEOUT 10000

#define EC_IT55_WDT_SWITCH	0x98
#define EC_IT55_WDT_TIME_MSB	0xA2
#define EC_IT55_WDT_TIME_LSB	0xA3

static unsigned int timeout = DEFAULT_TIMEOUT;

module_param(timeout, int, 0);
MODULE_PARM_DESC(timeout,
		 "Watchdog timeout in seconds, default=" __MODULE_STRING(
			 DEFAULT_TIMEOUT));


//static int wdt_set_watchdog_switch(char on)
//{
//	return ec_write_memory( EC_IT55_WDT_SWITCH, on);
//}
//static int wdt_get_counter(void)
//{
//	unsigned char tmp1, tmp2;
//	ec_read_memory(EC_IT55_WDT_TIME_MSB,&tmp1);
//	ec_read_memory(EC_IT55_WDT_TIME_LSB,&tmp2);
//	return ((tmp1 << 8) & 0xff00) | (tmp2 & 0xff);
//}
//static int wdt_update(int value)
//{
//	ec_write_memory(EC_IT55_WDT_TIME_MSB, (u8)((value & 0xff00) >> 8));
//	ec_write_memory(EC_IT55_WDT_TIME_LSB, value & 0xff);
//	return 0;
//}


//static int wdt_start(struct watchdog_device *wdd)
//{
//	//pr_info("watch dog start\n");
//	int ret = 0;
//	ec_request_ports();
//	wdt_update(wdd->timeout); //this line may move to wdt_set_timeout function.
//	ret = wdt_set_watchdog_switch(1);
//	ec_release_ports();
//	 return ret;
//}

//static int wdt_stop(struct watchdog_device *wdd)
//{
//	pr_info("watch dog stop\n");
//	int ret = 0;
//	ec_request_ports();
//	ret = wdt_set_watchdog_switch(0);
//	ec_release_ports();
//	return ret;
//}


//static int wdt_set_timeout(struct watchdog_device *wdd, unsigned int t)
//{
//	pr_info("wdt_set_timeout:%d\n",t);
//	wdd->timeout = t;
//	return 0;
//}

//static const struct watchdog_info ident = {
//	.options = WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE | WDIOF_KEEPALIVEPING,
//	.firmware_version = 1,
//	.identity = WATCHDOG_NAME,
//};

//static const struct watchdog_ops wdt_ops = {
//	.owner = THIS_MODULE,
//	.start = wdt_start,
//	.stop = wdt_stop,
//	// .ping = wdt_ping,
//	.set_timeout = wdt_set_timeout,
//};

//static struct watchdog_device wdt_dev = {
//	.info = &ident,
//	.ops = &wdt_ops,
//	.min_timeout = MIN_TIMEOUT,
//	.max_timeout = MAX_TIMEOUT,
//};

//static int __init it55_wdt_init(void)
//{
//	int rc;
//
//	pr_warn("it55_wdt_init %s \n", product);
//
//	/* 添加超时范围检查 */
//	if (timeout < MIN_TIMEOUT || timeout > MAX_TIMEOUT) {
//		timeout = DEFAULT_TIMEOUT;
//		pr_warn("Timeout value out of range, use default %d sec\n",
//			DEFAULT_TIMEOUT);
//	}
//
//	wdt_dev.timeout = timeout;
//	watchdog_stop_on_reboot(&wdt_dev);
//	rc = watchdog_register_device(&wdt_dev);
//	if (rc) {
//		pr_err("Cannot register watchdog device (err=%d)\n",
//		       rc);
//		return rc;
//	}
//
//	pr_info("Chip IT55xx initialized. timeout=%d sec \n", timeout);
//
//	return 0;
//}

//static void __exit it55_wdt_exit(void)
//{
//	watchdog_unregister_device(&wdt_dev);
//}


/**************************************************************** */

static int __init functions_init(void)
{
	pr_info(" %s \n", __func__);
	product = dmi_get_system_info(DMI_PRODUCT_NAME);
	if(strcmp(product,"DXP4800 GT")) {
		pr_info("unmatch product:%s\n", product);
		return -ENODEV;
	}

	//mipi_backlight_driver_init();
	//it55_wdt_init();
	procfs_create();
	g_power_status = 0;
	return 0;
}

static void __exit functions_exit(void)
{
	// 把风扇开成智能模式, lock unessasary when exit
	ec_write_memory(DEVICE_FAN1_MODE, 0);
	ec_write_memory(DEVICE_FAN3_MODE, 0);

	remove_proc_entry("fan", nas_dir);
	remove_proc_entry("startup", nas_dir);
	remove_proc_entry("temp", nas_dir);
	remove_proc_entry("version", nas_dir);
	remove_proc_entry("G3_wakeup", nas_dir);
	remove_proc_entry("Lan_wakeup", nas_dir);
	remove_proc_entry(MAIN_DIR, NULL);

	//it55_wdt_exit();
	pr_info(" %s \n", __func__);
}

module_init(functions_init);
module_exit(functions_exit);

MODULE_LICENSE("GPL v2");
