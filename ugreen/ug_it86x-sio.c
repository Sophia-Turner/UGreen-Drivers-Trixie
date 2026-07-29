// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel MIC Platform Software Stack (MPSS)
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Intel MIC Coprocessor State Management (COSM) Driver
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/uaccess.h>
#include <linux/dmi.h>
//#include <misc/ugreen_product.h>

int set_bios(int);

#define NO_DEV_ID	0xffff
#define IT8613_ID	0x8613

/* IO Ports */
#define REG		0x2e
#define VAL		0x2f

/* Configuration Registers and Functions */
#define LDNREG		0x07
#define CHIPID		0x20
#define CHIPREV		0x22
#define IT87_ACT_REG    0x30
#define PME     0x04    /* The device with the fan registers in it */
// fan2 is sys_fan

//#define FAN2_PWM_CTRL_REG	0x16
//#define FAN2_PWM_DATA_REG	0x6B

#define FAN2_PWM_CTRL_REG	0x17
#define FAN2_PWM_DATA_REG	0x73

//#define FAN3_PWM_CTRL_REG	0x17  //fan3
//#define FAN3_PWM_DATA_REG	0x73 // fan3

#define FAN2_SOFTWARE_MODE	0
#define FAN2_AUTO_MODE		1

#define BUTN    0 /* 开机键启动*/ 
#define AUTO    1 /* 上电自动启动 */
#define LAST    2 /* 最后状态动态*/

#define MAIN_DIR	"it86"

struct proc_dir_entry *nas_dir = NULL;
struct mutex update_lock;
int mode = 0;
module_param(mode, int, 0644);

enum
{
    BIOS_TURNON_OFF = 0,
    BIOS_TURNON_ON,
    RAID_TURNON_LAST_STATE,
};

static inline int superio_inw(int reg)
{
	int val;

	outb(reg++, REG);
	val = inb(VAL) << 8;
	outb(reg, REG);
	val |= inb(VAL);
	return val;
}

static inline int superio_inb(int reg)
{
	outb(reg, REG);
	return inb(VAL);
}

static inline int fan_read8(int reg)
{
	outb(reg, 0xa35);
	return inb(0xa36);
}
static inline void fan_write8(int val, int reg )
{
	outb(reg, 0xa35);
	outb(val, 0xa36);
}

static inline void superio_exit(void)
{
	outb(0x02, REG);
	outb(0x02, VAL);
	release_region(REG, 2);
}

static inline int superio_enter(void)
{
	/*
	 * Try to reserve REG and REG + 1 for exclusive access.
	 */
	if (!request_muxed_region(REG, 2, KBUILD_MODNAME))
		return -EBUSY;

	outb(0x87, REG);
	outb(0x01, REG);
	outb(0x55, REG);
	outb(0x55, REG);
	return 0;
}

static inline void superio_select(int ldn)
{
	outb(LDNREG, REG);
	outb(ldn, VAL);
}
static void fan2_mode(int mode)
{ // xxx.pdf p93
// config mode software operation
	int val;
    	val = fan_read8(FAN2_PWM_CTRL_REG);
	if(FAN2_SOFTWARE_MODE == mode){ // software operation
		val &= ~(1<<7);
	}else{ // automatic operation
		val |= 1<<7;
	}
    	fan_write8(val, FAN2_PWM_CTRL_REG);
}

static int  set_fan(unsigned long pwm)
{
    int rc = 0;

    mutex_lock(&update_lock);
    rc = superio_enter();
	if(rc)
		goto err;

    superio_select(0x04);
    fan2_mode(FAN2_SOFTWARE_MODE);
    fan_write8((int)pwm, FAN2_PWM_DATA_REG);
    superio_exit();
err:
    mutex_unlock(&update_lock);

    return rc;
}

int set_bios(int cmd) {
    int rc = 0;

    mutex_lock(&update_lock);
    rc = superio_enter();
	if(rc)
		goto err;

    superio_select(0x04);
    switch(cmd)  /* <<IT8613_L_V0.9xxx.pdf>> page:33 */
    {
		case 0:     //off
				printk("Set BOOT  power off\n");
				outb(0xF2, REG);
				outb(inb(VAL) & ~0x20,VAL); // 0xf2[5] = 0
			// outb(0xF4, REG);
			// outb(inb(VAL) & ~0x20,VAL); // 0xf4[5] = 0
				outb(0xF4, REG);
				outb(inb(VAL)|0x20,VAL);    // 0XF4[5] = 1
				outb(0xF4, REG);
				outb(inb(VAL)|0x40,VAL); // 0xF4[6] = 1					
				break;
		case 1:     //on    
				printk("Set BOOT power on\n");
				outb(0xF2, REG);
				outb(inb(VAL)&~0x20,VAL);   // 0XF2[5] = 0
				outb(0xF4, REG);
				outb(inb(VAL)|0x20,VAL);    // 0XF4[5] = 1
				outb(0xF4, REG);
				outb(inb(VAL) & ~0x40,VAL); // 0xF4[6]=0
				break;
	
		case 2:     //last state
				outb(0xF2, REG);
				outb(inb(VAL)|0x20,VAL);   // 0XF2[5] = 1
			//  outb(0xF4, REG);
			//  outb(inb(VAL)&~0x20,VAL);    // 0XF4[5] = 0
			//  outb(0xF4, REG);
			//  outb(inb(VAL) &~0x40,VAL); // 0xF4[6]=0
				outb(0xF4, REG);
				outb(inb(VAL)|0x20,VAL);    // 0XF4[5] = 1
				outb(0xF4, REG);
				outb(inb(VAL)|0x40,VAL); // 0xF4[6] = 1
				break;
		default:
                     break;
    }
    superio_exit();
err:
    mutex_unlock(&update_lock);
    return rc;
}
static ssize_t fan_read(struct file *file, char __user *usr_buf, size_t size, loff_t *ppos)
{
    char tmpbuf[128];
    int rc = 0;
    unsigned int val,cnt=0;
    unsigned long speed;

    if (mutex_lock_interruptible(&update_lock) < 0){
                return -EAGAIN;
    }
    rc = superio_enter();
	if(rc)
		goto end;

    superio_select(PME);
    if (!(superio_inb(IT87_ACT_REG) & 0x01)) {
          pr_info("Device not activated, skipping\n");
	  goto err;
    }


    val = fan_read8(0x1A) << 8;
    val |= fan_read8(0x0f);

    if((val == 0) || (val == 0xffff) || (val == 0x0fff)){
   	speed = 0; 
    }else{
   	speed = (unsigned long)(1350000 / (val * 2));
    }
    // fan3  cpu_fan 单风扇机型 dx4600/dx4700系列，dxp2800 和 dxp4800
    //cnt = snprintf(tmpbuf, sizeof(tmpbuf), "fan3 speed:%lu\n",speed);
    cnt = snprintf(tmpbuf, sizeof(tmpbuf), "sysfan1 speed:%lu\n",speed);
/*
// fan2 sys_fan
    val = fan_read8(0x19) << 8;
    val |= fan_read8(0x0e);
    if((val == 0) || (val == 0xffff) || (val == 0x0fff)){
   	speed = 0; 
    }else{
   	speed = (unsigned long)(1350000 / (val * 2));
    }
    cnt += snprintf(tmpbuf + cnt, sizeof(tmpbuf) - cnt, "fan2 speed:%lu\n",speed);
*/

end:
    if(cnt == 0){
    	cnt = snprintf(tmpbuf, sizeof(tmpbuf), "%d", 0);
    }
    superio_exit();

	rc = simple_read_from_buffer(usr_buf, size, ppos, tmpbuf, cnt);
err:	
	mutex_unlock(&update_lock);
    return rc;
}
static ssize_t startup_read(struct file *file, char __user *usr_buf, size_t size, loff_t *ppos)
{
    char tmpbuf[128];
    u8	val = 0;
    int rc = 0;
    int cnt;

    mutex_lock(&update_lock);

    rc = superio_enter();
    if (rc)
		goto err;
	
    superio_select(0x04);

    outb(0xF2, REG);
    val |= (inb(VAL) & 0x20) >> 3;
    outb(0xF4, REG);
    val |= (inb(VAL) & 0x60) >> 5;

	
    switch(val)  /* <<IT8613_L_V0.9xxx.pdf>> page:33 */
    {
        //case 0x00:     //on
		case 0x01:
			cnt = snprintf(tmpbuf, sizeof(tmpbuf), "%s\n","power on");
		   	break;
	    case 0x03:	  // off
			cnt = snprintf(tmpbuf, sizeof(tmpbuf), "%s\n","power off");
			break;
	    case 0x07:    // last status
			cnt = snprintf(tmpbuf, sizeof(tmpbuf), "%s\n","last status");
			break;
	    default:
			cnt = snprintf(tmpbuf, sizeof(tmpbuf), "%s\n","unkown status");
			break;
    }
    superio_exit();

    rc =  simple_read_from_buffer(usr_buf, size, ppos, tmpbuf, cnt);
err:
    mutex_unlock(&update_lock);
	return rc;
}
static ssize_t startup_write(struct file *file,const char __user * buffer,size_t count, loff_t * ppos)
{
        int result = 0;
        char buf[12] = { '\0' };
	

        if (/*!beeper ||*/ (count > sizeof(buf) - 1)){
                return -EINVAL;
	}
        if (copy_from_user(buf, buffer, count)) {
                result = -EFAULT;
                goto end;
        }
	buf[count] = '\0';
	if(buf[count -1] == 0x0a) buf[count - 1] = '\0';
	if((!strncmp(buf, "on",3)) || (!strncmp(buf, "ON",3))){
		set_bios(1);
	}else if((!strncmp(buf, "off",4)) || (!strncmp(buf, "OFF",4))){
		set_bios(0);
	}else if((!strncmp(buf, "last",5)) || (!strncmp(buf, "LAST",5))){
		set_bios(2);
	}else{
		result = -EINVAL;
	}
end:
	if(result){
		return result;
	}
        return count;
}

static ssize_t fan_write(struct file *file,const char __user * buffer,size_t count, loff_t * ppos)
{
        int result = 0;
        char buf[12] = { '\0' };
	char *p;
	unsigned long pwm;

        if (/*!beeper ||*/ (count > sizeof(buf) - 1)){
                return -EINVAL;
	}
        if (copy_from_user(buf, buffer, count)) {
                result = -EFAULT;
                goto end;
        }
	buf[count] = '\0';
	if(buf[count -1] == 0x0a) buf[count - 1] = '\0';
	if((!strncmp(buf, "on",3)) || (!strncmp(buf, "ON",3))){
		set_fan(127);
	}else if((!strncmp(buf, "off",4)) || (!strncmp(buf, "OFF",4))){
		set_fan(0);
	}else if((!strncmp(buf, "set ",4)) || (!strncmp(buf, "SET ",4))){
		p = buf+4;
		while((*p < 0x21) && (*p != '\0'))p++;	
		if(*p != '\0'){
		     if(kstrtoul(p, 10, &pwm)){
		        result = -EINVAL;
			goto end;
		      }
		     if((pwm > 0) && (pwm < 256)){
		        set_fan(pwm);
		     }else{
		        result = -EINVAL;
		    }
		}else{
		     result = -EINVAL;
		}
	}else{
		result = -EINVAL;
	}
end:
	if(result){
		return result;
	}
        return count;
}

static int null_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, pde_data(inode));
}

static const struct proc_ops fan_proc_fops = {
        .proc_open           = null_proc_open,
		.proc_write		= fan_write,
		.proc_read		= fan_read,
        .proc_release        = seq_release,
};

static const struct proc_ops startup_proc_fops = {
        .proc_open           = null_proc_open,
		.proc_write		= startup_write,
		.proc_read		= startup_read,
        .proc_release        = seq_release,
};

static int procfs_create(void)
{

	// Check if already exists
	if (nas_dir) {
		pr_warn("/proc/%s already esists, skipping creation\n", MAIN_DIR);
		return 0;
	}

	struct proc_dir_entry *entry = NULL;
	int ret = 0;

	/* create /proc/nas/ */
	nas_dir = proc_mkdir(MAIN_DIR, NULL);
	if (!nas_dir){
		return -ENODEV;
	}

	entry = proc_create_data("fan", S_IWUGO, nas_dir, &fan_proc_fops, NULL);
	if (!entry) {
		ret = -ENODEV;
		goto remove_dev_dir;
	}

	/* create /proc/nas/startup */
	entry = proc_create_data("startup", S_IRWXUGO, nas_dir, &startup_proc_fops, NULL);
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

static int __init it87x_init(void)
{
	u16 chip_type;
	u8 chip_rev;
    int rc = 0;
//	if(!ug_check_product_match(DX4600_SERIES, __func__)){
//		return -ENODEV;
//	}
    mutex_init(&update_lock);

    rc = superio_enter();
	if (rc){
		return rc;
	}

	chip_type = superio_inw(CHIPID);
	chip_rev  = superio_inb(CHIPREV) & 0x0f;
	superio_exit();
    	if(chip_type != IT8613_ID){
		pr_err("Unknown Chip found, Chip %04x Revision %x\n",
		       chip_type, chip_rev);
		return -ENODEV;
    	}
	pr_info("Found chip IT8613 rev %x. %u \n",chip_type, chip_rev);

	procfs_create();
	return 0;
}

static void __exit it87x_exit(void)
{
    remove_proc_entry("fan", nas_dir);
    remove_proc_entry("startup", nas_dir);
    remove_proc_entry(MAIN_DIR, NULL);

    printk("it87x_exit bios_exit\n");
}

module_init(it87x_init);
module_exit(it87x_exit);

MODULE_LICENSE("GPL v2");
