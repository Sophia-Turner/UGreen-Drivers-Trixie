#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/dmi.h>
//#include <misc/ugreen_product.h>

#undef pr_fmt
#define pr_fmt(fmt) "LEDMCU-68A88: " fmt
#define LED_I2C_ADDRESS 	0x3a
#define LED_COUNT		10
#define BLOCK_LEN       	13
#define CRC_IDX_END       	10

#define MCU_CHIP_ID		0x5A
#define MCU_FW_VER		0x5D

#define  COLOR_NONE		0x00
#define  COLOR_WHITE		0x01
#define  COLOR_ORANGE		0x02
#define  COLOR_RED		0x03
#define  COLOR_GREEN		0x04
#define  COLOR_BLUE		0x05
#define  COLOR_ALL		0x03

#define  LED_BRIGHTNESS_MAX 	255

#define  LED_OFF		0x00
#define  LED_ON			0x01

#define  OP_BRIGHTNESS		0x01
#define  OP_COLOR		0x02
#define  OP_ONOFF		0x03
#define  OP_BLINK		0x04
#define  OP_BREATH		0x05

#define  RUN_STATE_OFF		0x0
#define  RUN_STATE_ON		0x1
#define  RUN_STATE_BLINK	0x2
#define  RUN_STATE_BREATH	0x3

#define  STOP_CHECK_TIMES   	5
#define  ONESHOT_CHECK_DELAY	3000

#define  LOG_INFO		0x02
#define  LOG_MSG		0x01
#ifndef DEBUG_PRINTK
#define DEBUG_PRINTK(log_level,fmt, ...)
#endif
static int debug_on;
static int remove_or_shutdown;
static struct mutex i2cmutex;

static u8 led_color_table[5][3] = {
  {255, 255, 255},  /* white  180 */   
  {220, 40,  00},   /* oringe  255*/
  {255, 00,  00},    /*   */
  {00,  255, 00},    /*   */
  {00,  00, 255}    /*   */
  
};
char * str_op_mode[6] = { "off", "on", "blink", "breath"};
char * str_led_name[] = { "sys", "net", "disk1", "disk2", "disk3", "disk4", "disk5", "disk6", "disk7", "disk8"};
  
typedef struct led_state {
	int ignore_read_num;
	int need_read;    /* need read from mcu */
	atomic_t brightness;   /* real brightness */
	atomic_t color;
	atomic_t run_state;   // 0-off, 1-on, 2-blink, 3-breath

	int check_times;	/*  times of  trig_oneshots is zero ,then stop delay check work  */
	atomic_t t_blink[2];
	atomic_t t_breath[2];
}led_state_t;

struct mcu_led {
	u8 led_bit;
	u8 color;
	unsigned long delay_on;
	unsigned long delay_off;
	const char *name;
	struct led_classdev cdev;
	struct i2c_client *i2c_client;
	struct work_struct	work;
	int brightness;  /* indicate on,of */
	int value;		 /* indicate real brightness */
	led_state_t  cur_state;

};

static struct mcu_led ug_leds[ ] =  { 
	{ .name = "power",   .color=COLOR_NONE, .cur_state={.ignore_read_num=5, .need_read=0, .check_times=0, }},
	{ .name = "network_stat", .color=COLOR_NONE, .cur_state={.ignore_read_num=4, .need_read=0, .check_times=0, }},
	{ .name = "disk1", 	 .color=COLOR_NONE, .cur_state={.ignore_read_num=3, .need_read=0, .check_times=0 }},
	{ .name = "disk2",   .color=COLOR_NONE, .cur_state={.ignore_read_num=2, .need_read=0, .check_times=0, }},
	{ .name = "disk3",   .color=COLOR_NONE, .cur_state={.ignore_read_num=5, .need_read=0, .check_times=0, }},
	{ .name = "disk4",   .color=COLOR_NONE, .cur_state={.ignore_read_num=4, .need_read=0, .check_times=0, }},
	{ .name = "disk5",   .color=COLOR_NONE, .cur_state={.ignore_read_num=6, .need_read=0, .check_times=0, }},
	{ .name = "disk6",   .color=COLOR_NONE, .cur_state={.ignore_read_num=7, .need_read=0, .check_times=0, }},
	{ .name = "disk7",   .color=COLOR_NONE, .cur_state={.ignore_read_num=8, .need_read=0, .check_times=0, }},
	{ .name = "disk8",   .color=COLOR_NONE, .cur_state={.ignore_read_num=9, .need_read=0, .check_times=0, }},
};

static void init_leds_stats(void){
	for(int i = 0; i < LED_COUNT; i++){
		atomic_set(&ug_leds[i].cur_state.brightness,255);
		atomic_set(&ug_leds[i].cur_state.color,COLOR_NONE);
		if(0 == i){
            atomic_set(&ug_leds[i].cur_state.run_state, LED_ON);
		}
		else{
			atomic_set(&ug_leds[i].cur_state.run_state, LED_OFF);
	    }
	}
}
static const unsigned short normal_i2c[] = { LED_I2C_ADDRESS, I2C_CLIENT_END };
static struct i2c_client *g_client = NULL;

static int get_led_addr_by_name(const char *name)
{
    if (!name)
        return -EINVAL;
    if (!strcmp(name, "power"))
        return 0;
    else if (!strcmp(name, "network_stat"))
        return 1;
    else if (!strcmp(name, "disk1"))
        return 2;
    else if (!strcmp(name, "disk2"))
        return 3;
    else if (!strcmp(name, "disk3"))
        return 4;
    else if (!strcmp(name, "disk4"))
        return 5;
    else if (!strcmp(name, "disk5"))
        return 6;
    else if (!strcmp(name, "disk6"))
        return 7;
    else if (!strcmp(name, "disk7"))
        return 8;
    else if (!strcmp(name, "disk8"))
        return 9;
    else
        return -EINVAL;
}
struct ugreen_mcu_data {
   // struct led_classdev *cdev[LED_COUNT];
    char name[16];
	u8 i2c_addr;
	struct i2c_client *i2c_client;
	struct mcu_led *leds;
};

static int do_color_set(int addr,int color);
/* in most cases, should be called while holding */
static inline u16 mcu_read16(struct i2c_client *client, u8 reg){
	usleep_range(1500,2500);
	return (i2c_smbus_read_word_data(client, reg));
}

static inline u8 mcu_read8(struct i2c_client *client, u8 reg){
	usleep_range(1500,2500);
	return i2c_smbus_read_byte_data(client, reg);
}

static s32 i2c_smbus_write_i2c_block_data_and_read(const struct i2c_client *client, u8 command, u8 length, const u8 *values){
	s32 ret;
    union i2c_smbus_data data;
	usleep_range(1500,2500);
    if (length > I2C_SMBUS_BLOCK_MAX)
        length = I2C_SMBUS_BLOCK_MAX;
    data.block[0] = length;
    memcpy(data.block + 1, values, length);
    ret =  i2c_smbus_xfer(client->adapter, client->addr, client->flags, I2C_SMBUS_WRITE, command,
                  I2C_SMBUS_BLOCK_PROC_CALL, &data);
	pr_info("------%s, ret:%d,read-led:%d, read-val:%d \n", __func__, ret,data.block[0],data.block[1]);
	return data.block[0];
	
}

static int led_set_on_off(int addr, int op_state) {
    u8 data[BLOCK_LEN]={0x00,0x00,0xa0,0x01,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00};
    u8 i;
    int num = 0;
	int ret = -1;	
    if(g_client == NULL) {
		pr_info("----%s g_client is null \n", __func__);
        return -ENODEV;
    }
	if((0 > addr) || (LED_COUNT-1 < addr) ) {
		pr_info("%s addr:%d is invalid\n",__func__, addr);
		return -EINVAL;
	}
	if ((op_state != LED_OFF) && (op_state != LED_ON) ){
		pr_info("%s op_state:%d is invalid\n",__func__, op_state);
		return -EINVAL;
	}
	if(op_state == atomic_read(&ug_leds[addr].cur_state.run_state )) {
		DEBUG_PRINTK(LOG_INFO, "%s op_state:%d is already ,need't to set\n",__func__, op_state);
		return 0;
	}
    data[0] = addr;
	data[1] = addr;	
	data[7] = op_state;
	for(i = 2;i <= CRC_IDX_END; i++) {
        num += data[i];
    }
    data[11] = (u8)(num >> 8);
    data[12] = (u8)num;
		
		mutex_lock(&i2cmutex);
		if(op_state != atomic_read(&ug_leds[addr].cur_state.run_state)) {
			usleep_range(200,1500);
			ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));	
			i = 3;				
			do{
				if(1 != mcu_read8(g_client,0x80)){
					msleep(30);
					ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));
					if (ret < 0) {
						DEBUG_PRINTK(LOG_MSG,"---%s retry write failed, ret:%d\n", __func__, ret);
					}
				}
				else {
					break;
				}
				i--;
			}while(i>0);

			
			if( i < 1) {
				ug_leds[addr].cur_state.need_read = 1;
				DEBUG_PRINTK(LOG_MSG,"---%s last command write addr:%d, failed, try 3 times \n", __func__,addr);
				mutex_unlock(&i2cmutex);
				return -EBUSY;
			}
			else {
				atomic_set(&ug_leds[addr].cur_state.run_state,op_state);
			}
			
			DEBUG_PRINTK(LOG_MSG,"%s, addr:%d, ret:%d,state:%d \n", __func__, addr ,ret,op_state );	
		}
		else {				
			DEBUG_PRINTK(LOG_INFO, "%s line:%d,op_state:%d is already ,need't to set\n",__func__, __LINE__,op_state);
		}			
		mutex_unlock(&i2cmutex);

		
	return 0;
}

static int do_brightness_set(int addr, int brightness) {

	int op_state = -1;
    u8 data[BLOCK_LEN]={0x00,0x00,0xa0,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
    u8 i;
    int num = 0;
	int ret = 0;
	int index = 0;
  
    if(g_client == NULL) {
        return -ENODEV;
    }
	
	index = addr;
    data[0] = addr;
	data[1] = addr;	
	
	if(1 == brightness){
		op_state = LED_ON;
	}
	else if( 0 == brightness) {
		op_state = LED_OFF;
	}
	if(op_state != -1) {
		if(op_state != atomic_read(&ug_leds[index].cur_state.run_state))
			led_set_on_off(addr,op_state);	
		return 0;
	}
	data[7] = brightness;
    
    for(i = 2;i <= CRC_IDX_END;i ++) {
        num += data[i];
    }
    data[11] = (u8)(num >> 8);
    data[12] = (u8)num;
    		
	if(atomic_read(&ug_leds[index].cur_state.brightness) != brightness) {
	   //pr_info("brightness 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
	    //  data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);		
		mutex_lock(&i2cmutex);
		usleep_range(200,1500);
		ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));		
			i = 3;				
			do{
				if(1 != mcu_read8(g_client,0x80)){
					DEBUG_PRINTK(LOG_MSG,"---%s last set brightness command write failed, try again:%d \n", __func__,i);
					msleep(30);
					ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));
					if (ret < 0) {
						DEBUG_PRINTK(LOG_MSG,"---%s retry write failed, ret:%d\n", __func__, ret);
					}
				}
				else {
					break;
				}
				i--;
			}while(i>0);
		mutex_unlock(&i2cmutex);
		
			if( i < 1) {
				ug_leds[addr].cur_state.need_read = 1;
				return -EBUSY;
			}
			else {
				atomic_set(&ug_leds[index].cur_state.brightness,brightness);
			}
		pr_info("---%s addr:%d ret:%d,brightness:%d, name:%s \n", __func__, addr ,ret,brightness,str_led_name[addr]);
	}
	else {
		 DEBUG_PRINTK(LOG_INFO, "%s brightness:%d is already ,need't to set\n",str_led_name[addr], brightness);
	}

	
	return 0;
}

static void mcu_work(struct work_struct *work){
	if (remove_or_shutdown) {
		DEBUG_PRINTK(LOG_MSG,"---[%s]remove_or_shutdown:%d, exiting\n", __func__, remove_or_shutdown);
		return;
	}
	struct mcu_led *led = container_of(work, struct mcu_led, work);
	//unsigned char val = (led->color << led->led_bit);
	int brightness = led->value;
	int addr;
		
	struct led_classdev *cdev = &led->cdev;    
    if (g_client == NULL) {
        DEBUG_PRINTK(LOG_MSG,"---[%s]g_client is NULL, exiting\n", __func__);
        return;
    }
    if (cdev == NULL || cdev->name == NULL) {
        DEBUG_PRINTK(LOG_MSG,"---[%s]cdev or cdev->name is NULL, exiting\n", __func__);
        return;
    }
    
    addr = get_led_addr_by_name(cdev->name);
    if (addr < 0 || addr >= LED_COUNT) {
        pr_info("%s: invalid led name %s\n", __func__, cdev->name);
        return;
    }
    if (!remove_or_shutdown && g_client != NULL) {
		do_brightness_set(addr, brightness);
    }
}

static void leds_brightness_set(struct led_classdev *cdev, enum led_brightness brightness){
	if( 1 == remove_or_shutdown) { 
	    DEBUG_PRINTK(LOG_MSG,"---%s remove_or_shutdown:%d \n", __func__, remove_or_shutdown);
            return ;
	}
    struct mcu_led *led = container_of(cdev, struct mcu_led, cdev); 
	if( 0 > brightness || LED_BRIGHTNESS_MAX < brightness) {
		pr_err("---%s brightness:%d is invalid \n", __func__, brightness);
        return ;
	}
    led->value = brightness;
	schedule_work(&led->work);

}

static int led_blink_or_breath(int addr, int op_mode, unsigned long hight, unsigned long low) {
    u8 data[BLOCK_LEN]={0x00,0x00,0xa0,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    u8 i;
    int num = 0;
	int ret = -1;	
    if(g_client == NULL) {
		pr_info("----%s g_client is null \n", __func__);
        return -ENODEV;
    }
	if((0 > addr) || (LED_COUNT-1 < addr) ) {
		pr_info("%s addr:%d is invalid\n",__func__, addr);
		return -EINVAL;
	}
	if(op_mode == (atomic_read(&ug_leds[addr].cur_state.run_state) + 2)) {
		if(op_mode == OP_BLINK && hight == atomic_read(&ug_leds[addr].cur_state.t_blink[0]) && low == atomic_read(&ug_leds[addr].cur_state.t_blink[1]) ) {
			DEBUG_PRINTK(LOG_INFO, "%s op_mode:%d is already ,need't to set\n",__func__, op_mode);
			return 0;
		}
		if(op_mode == OP_BREATH && hight == atomic_read(&ug_leds[addr].cur_state.t_breath[0]) && low == atomic_read(&ug_leds[addr].cur_state.t_breath[1])) {
			DEBUG_PRINTK(LOG_INFO, "%s op_mode:%d is already ,need't to set\n",__func__, op_mode);
			return 0;
		}
	}
    data[0] = addr;
	data[1] = addr;	
	data[6] = op_mode;

    data[7] = (u8)(hight >> 8);
    data[8] = (u8)(hight);
    data[9] = (u8)(low >> 8);
    data[10] = (u8)(low);	
	

	for(i = 2;i <= CRC_IDX_END;i ++) {
        num += data[i];
    }
    data[11] = (u8)(num >> 8);
    data[12] = (u8)num;
	if ((op_mode == OP_BLINK) || (op_mode == OP_BREATH) ){	
			mutex_lock(&i2cmutex);
			usleep_range(200,1500);
			ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));	
			i = 3;				
			do{
				if(1 != mcu_read8(g_client,0x80)){
					msleep(30);
					ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));
					if (ret < 0) {
						DEBUG_PRINTK(LOG_MSG,"---%s retry write failed, ret:%d\n", __func__, ret);
					}
				}
				else {
					break;
				}
				i--;
			}while(i>0);
			mutex_unlock(&i2cmutex);
			
			if( i < 1) {
				ug_leds[addr].cur_state.need_read = 1;
				DEBUG_PRINTK(LOG_MSG,"---%s last command write addr:%d, failed, try 3 times \n", __func__,addr);
				return -EBUSY;
			}
			else {
				atomic_set(&ug_leds[addr].cur_state.run_state, op_mode - 2);
				if (op_mode == OP_BLINK){
					atomic_set(&ug_leds[addr].cur_state.t_blink[0], hight);
					atomic_set(&ug_leds[addr].cur_state.t_blink[1], low);
				}
				if (op_mode ==OP_BREATH){
					atomic_set(&ug_leds[addr].cur_state.t_breath[0], hight);
					atomic_set(&ug_leds[addr].cur_state.t_breath[1], low);
				}
				//ug_leds[addr].cur_state.do_set_blink_time = jiffies_to_msecs(jiffies);
			}			
		DEBUG_PRINTK(LOG_INFO, "blink 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
        data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
    	DEBUG_PRINTK(LOG_MSG, " %s addr:%d ret:%d state:%d high:%ld,low:%ld\n", __func__, addr ,ret,op_mode,hight,low );	
	}
	else {
		DEBUG_PRINTK(LOG_INFO, "%s do nothing, addr:%d, mode:%d is invalid\n",__func__,addr, op_mode);
		return -EINVAL;
	}

	return 0;
}

static int mcu_led_blink_set(struct led_classdev *cdev,unsigned long *delay_on,unsigned long *delay_off){
    u8 addr;
	unsigned long tmp = 0;
	unsigned long blink_on = 0,blink_off = 0;
	int op_mode = OP_BLINK;
	if( 1 == remove_or_shutdown) { 
		DEBUG_PRINTK(LOG_MSG,"---[%s]remove_or_shutdown:%d \n", __func__, remove_or_shutdown);
		return 0;
	}
	if (g_client == NULL) {
		DEBUG_PRINTK(LOG_MSG,"[%s]g_client is NULL\n", __func__);
		return 1;
	}
	if (cdev == NULL || cdev->name == NULL) {
		pr_info("[%s],line:%d cdev or cdev->name is NULL\n", __func__, __LINE__);
		return 1;
	}
	if (cdev->trigger == NULL || cdev->trigger->name == NULL) {
		pr_info("[%s],line:%d cdev->trigger or trigger->name is NULL\n", __func__, __LINE__);
		return 1;
	}
	DEBUG_PRINTK(LOG_INFO, " %s line:%d, name:%s led_data->name[%s]on[%lu]off[%lu]\n", __func__, __LINE__,cdev->name,cdev->trigger->name,*delay_on,*delay_off);

	addr = get_led_addr_by_name(cdev->name);
	if (addr < 0 || addr >= LED_COUNT) {
		pr_info("%s: invalid led name %s\n", __func__, cdev->name);
		return -EINVAL;
	}
	blink_on = *delay_on;
	blink_off = *delay_off;

	if(!strncmp(cdev->trigger->name, "normal",6)) {
		if(RUN_STATE_OFF != atomic_read(&ug_leds[addr].cur_state.run_state) && RUN_STATE_ON != atomic_read(&ug_leds[addr].cur_state.run_state)){
		    led_set_on_off(addr, LED_OFF);
		}
		return 0;
	}
	else if(!strcmp(cdev->trigger->name,"timer2")) {
		if((0 == blink_on) || (0 == blink_off)) {
			return 0;
		}
		if(blink_off < 100)
			blink_off = 100;
		if(blink_on < 100)
			blink_on = 100;
	}
	else if(!strcmp(cdev->trigger->name,"breath")) {
		op_mode = OP_BREATH;
		if(blink_off < 500)
			blink_off = 500;
		if(blink_on < 1000)
			blink_on = 1000;
	}
    tmp = blink_on + blink_off;

    led_blink_or_breath(addr, op_mode, tmp, blink_on);

    return 0;
}
static int led_init_read_state( int addr ){

	int i;
	int ret = -1;
	int idx;
	int color = 0;
	if( 1 == remove_or_shutdown) { 
	    DEBUG_PRINTK(LOG_MSG,"---[%s]remove_or_shutdown:%d \n", __func__, remove_or_shutdown);
       return  -EBUSY;
	}
	if(0 > addr || 9 < addr){
		pr_info("%s: addr:%d is invalid\n", __func__,addr);
		return -EBUSY;
	}

	idx = addr;
	
	addr += 0x81;
{	
	usleep_range(5000,8000);
	//msleep(120);
	mutex_lock(&i2cmutex);
	u8 data[16]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	ret = i2c_smbus_read_i2c_block_data(g_client, addr,11, data);
	//ret = i2c_smbus_read_block_data(g_client, addr, data);
	//ret = i2c_smbus_read_i2c_block_data_or_emulated(g_client, addr,11, data);
	
	if((ret < 0) || (11 != ret) ) {
		pr_info("---%s,ret:%d,read:%d block err! \n", __func__, ret,idx);
		ret = -1;
		goto do_return;
	}

	if(11 == ret){
		int num = 0;	
		for(i = 0;i < 9;i ++) {
			num += data[i];
		}
		if((0 != num ) && (data[9] == (u8)(num >> 8)) && (data[10] == (u8)num)){
		//	pr_info("---state read crc success!\n");
			int hight = (data[5] << 8 ) + data[6];
			int low = (data[7] << 8 ) + data[8];
			if(0 > data[0] || 3 < data[0]) {
				pr_info("---read op_sate:%d err!\n",data[0]);
				ret = -1;
				goto do_return;
			}

			if((220 == data[2]) && (40 == data[3])){  //color is orange
				
				color = COLOR_ORANGE;
			}
			else if((255 == data[2]) && (0 == data[3]) && (0 == data[4])){
				color = COLOR_RED;
			}
			else if((0 == data[2]) && (255 == data[3]) && (0 == data[4])){
				color = COLOR_GREEN;
			}
			else if((0 == data[2]) && (0 == data[3]) && (255 == data[4])){
				color = COLOR_BLUE;
			}
			else {
				color = COLOR_WHITE;
			}
		
			DEBUG_PRINTK(LOG_MSG,  "---addr:%d,op_mode:%s,brightness:%d,color:%d:%d:%d,t-hight:%d, t-low:%d\n",idx,str_op_mode[data[0]],data[1],data[2],data[3],data[4],hight, low);			
			if(RUN_STATE_OFF !=  data[0]){
				atomic_set(&ug_leds[idx].cur_state.run_state, data[0]);				
				atomic_set(&ug_leds[idx].cur_state.brightness, data[1]);
				atomic_set(&ug_leds[idx].cur_state.color, color);	
			}
			//return mode;
			ret = 0;
			goto do_return;
	
		}
		else {
			ug_leds[idx].cur_state.need_read = 1;
			pr_info("--read crc error!,return last mode,addr:%d, ret:%d, status 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
			 idx, ret, data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10]);
			
			goto do_return;
		}
	}
do_return:
	ug_leds[idx].cur_state.need_read = 1;
	mutex_unlock(&i2cmutex);
}
	return ret;
}

static int do_color_set(int addr ,int color){
    u8 data[BLOCK_LEN]={0x00,0x00,0xa0,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00};
    u8 i;

    int num = 0;
	int ret = 0;
	int index = 0;
	int color_index = 0;
		
    index = addr;
    data[0] = addr;
	data[1] = addr;
    
		color_index =color -1;
		
        data[7] = led_color_table[color_index][0];
		data[8] = led_color_table[color_index][1];
		data[9] = led_color_table[color_index][2];
    
    for(i = 2;i <= CRC_IDX_END;i ++) {
        num += data[i];
    }
    data[11] = (u8)(num >> 8);
    data[12] = (u8)num;
	
   

    if(atomic_read(&ug_leds[index].cur_state.color) != color) {
		mutex_lock(&i2cmutex);
		usleep_range(200,1500);
		ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));		
		pr_info("---%s color:%d, ret:%d, name:%s \n", __func__, color, ret,str_led_name[addr]);		
			i = 3;				
			do{
				if(1 != mcu_read8(g_client,0x80)){
					msleep(30);
					ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));
				}
				else {
					break;
				}
				i--;
			}while(i>0);
		mutex_unlock(&i2cmutex);
		
			if( i < 1) {
				ug_leds[index].cur_state.need_read = 1;
				DEBUG_PRINTK(LOG_MSG,"---%s last command write addr:%d, failed, try 3 times \n", __func__,addr);
			}
			else {
				atomic_set(&ug_leds[index].cur_state.color,color);
			}
		DEBUG_PRINTK(LOG_INFO, "color 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
        data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
    }		
	else {
		 DEBUG_PRINTK(LOG_INFO, "%s color:%d is already ,need't to set\n", str_led_name[addr],color);
	}
	return 0;
}

static ssize_t color_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t size){
    int ret;    
	unsigned long state;
	int addr;
	if( 1 == remove_or_shutdown) { 
                DEBUG_PRINTK(LOG_MSG,"---[%s]remove_or_shutdown:%d \n", __func__, remove_or_shutdown);
                return size;
	}
        struct led_classdev *cdev = dev_get_drvdata(dev);
	if (led_sysfs_is_disabled(cdev)) {
		ret = -EBUSY;
	 	return ret;
	}

	ret = kstrtoul(buf, 10, &state);
	if (ret)
		return ret;
		 
	if(state > 5){  // origin is 3
	     ret = -EINVAL;
	      	return ret;
	}
	if(state < 1)
		state = 1;
	state &= 0x07;
	    
    if(cdev == NULL || g_client == NULL) {
        pr_info("[%s][%d]size[%lu] not match\n", __func__, __LINE__ ,size);
        return size;
    }

    addr = get_led_addr_by_name(cdev->name);
    if (addr < 0 || addr >= LED_COUNT) {
        pr_info("%s: invalid led name %s\n", __func__, cdev->name);
        return -EINVAL;
    }

    do_color_set(addr, state);

	return size;
}
static DEVICE_ATTR(color, 0644, NULL, color_set);

static struct attribute *ht32f52231_attrs[] = {
	&dev_attr_color.attr,
	NULL
};
ATTRIBUTE_GROUPS(ht32f52231);

static int init_leds(struct i2c_client *client){
    u8 data[BLOCK_LEN]={0x00,0x00,0xa0,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
    u8 i,j;
    int num = 0;
    u8 addr;
	int ret = 0;
//	int index = 0

    for(i = 0;i < LED_COUNT;i++) {
		data[0] = i;
		data[1] = i;
		addr = i;
		if(0 != i){
			if(0 != led_init_read_state(addr)){
				led_init_read_state(addr);
			}
			if(RUN_STATE_OFF !=  atomic_read(&ug_leds[addr].cur_state.run_state)){
				//pr_info(" ---%s addr:%d run_state:%d, is already\n", __func__, i,ug_leds[addr].cur_state.run_state);
				continue;
			}
		}

		
		data[6] = OP_BRIGHTNESS;		
		data[7] = atomic_read(&ug_leds[i].cur_state.brightness);
		num = 0;		
		for(j = 2;j <= CRC_IDX_END;j++) {
			num += data[j];
		}
		data[11] = (u8)(num >> 8);
		data[12] = (u8)num;		
		ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));	
		if( ret < 0 ) {
			pr_info(" ---%s %d set brightness failed, ret:%d\n", __func__, i,ret);
		}
		usleep_range(200,1500);
		DEBUG_PRINTK(LOG_MSG," ---%s %d set brightness  ret:%d\n", __func__, i,ret);
		
		data[6] = OP_ONOFF;		
		data[7] = atomic_read(&ug_leds[i].cur_state.run_state);
		num = 0;
		for(j = 2;j <= CRC_IDX_END;j++) {
			num += data[j];
		}
		data[11] = (u8)(num >> 8);
		data[12] = (u8)num;	
		
		ret = i2c_smbus_write_i2c_block_data(g_client,addr,12,(data+1));
		if( ret < 0 ) {
			pr_info(" ---%s %d set on-off failed, ret:%d\n", __func__, i,ret);
		}
		usleep_range(200,1500);		
		DEBUG_PRINTK(LOG_MSG," ---%s %d set on-off  ret:%d\n", __func__, i,ret);	
		//do_color_set(addr, COLOR_WHITE); 
    }	
	
	
	return 0;
}

static int leds_ht32f52231_probe(struct i2c_client *client){
    int i;
    struct ugreen_mcu_data *ugreen_mcu_cdev;
    debug_on = 0;
    remove_or_shutdown = 0;
    pr_info("---[%s][%d]addr[0x%x]\n", __func__, __LINE__,client->addr);
    
    ugreen_mcu_cdev = devm_kzalloc(&client->dev, sizeof(struct ugreen_mcu_data), GFP_KERNEL);
    if(!ugreen_mcu_cdev) {
		return -ENOMEM;
    }
    g_client = client;
    i2c_set_clientdata(client, ugreen_mcu_cdev);
    ugreen_mcu_cdev->leds = ug_leds;
//    LED_COUNT = DEF_LED_COUNT;
   // LED_COUNT = 10;
	mutex_init(&i2cmutex);	

    for(i = 0;i < LED_COUNT;i++) {
        ug_leds[i].cdev.name =ugreen_mcu_cdev->leds[i].name;
        ug_leds[i].cdev.brightness_set = leds_brightness_set;
        ug_leds[i].cdev.blink_set = mcu_led_blink_set;
        ug_leds[i].cdev.groups = ht32f52231_groups;
		ug_leds[i].cdev.max_brightness = LED_BRIGHTNESS_MAX;
		ug_leds[i].color = COLOR_NONE;
		ug_leds[i].brightness = 0; 

		strcpy(ugreen_mcu_cdev->name, "ug_mcu_leds");
		INIT_WORK(&ug_leds[i].work, mcu_work);
		led_classdev_register(&client->dev, &ug_leds[i].cdev);
    }
    init_leds_stats();
	init_leds(g_client);

    pr_info("---%s addr:0x%x finish \n", __func__,client->addr);
	return 0;
}

static void leds_ht32f52231_remove(struct i2c_client *client){
	struct ugreen_mcu_data *ugreen_mcu_cdev = i2c_get_clientdata(client);
    int i;
    remove_or_shutdown = 1; 
    DEBUG_PRINTK(LOG_MSG,"[%s][%d]\n", __func__, __LINE__);

    msleep(10);
    for(i = 0;i < LED_COUNT;i++) {
        cancel_work_sync(&ug_leds[i].work);
    }
    for(i = 0;i < LED_COUNT;i++) {
        led_classdev_unregister(&ug_leds[i].cdev);
    }
    
    if (ugreen_mcu_cdev) {
	devm_kfree(&client->dev, ugreen_mcu_cdev);
    }
    mutex_destroy(&i2cmutex);
    g_client = NULL;
    
	return ;
}

static void leds_ht32f52231_shutdown(struct i2c_client *client){
	struct ugreen_mcu_data *ugreen_mcu_cdev = i2c_get_clientdata(client);
    int i;
    remove_or_shutdown = 1;
    DEBUG_PRINTK(LOG_MSG,"---[%s][%d], sleep 3s \n", __func__, __LINE__);
    msleep(3300);
    DEBUG_PRINTK(LOG_MSG,"---[%s][%d]\n", __func__, __LINE__);

    /* 等待一小段时间确保所有正在运行的工作都能看到标志 */
    msleep(10);
    for(i = 0;i < LED_COUNT;i++) {
        cancel_work_sync(&ug_leds[i].work);
    }
    for(i = 0;i < LED_COUNT;i++) {
        led_classdev_unregister(&ug_leds[i].cdev);
    }
    if (ugreen_mcu_cdev) {
        devm_kfree(&client->dev, ugreen_mcu_cdev);
    }
    mutex_destroy(&i2cmutex);
    g_client = NULL;
	 pr_info("---[%s][%d] end\n", __func__, __LINE__);
	//msleep(80);
	//devm_kfree(&client->dev, ugreen_mcu_cdev);
    
	return ;
}

static const struct i2c_board_info ugreen_i2c_info = {
	I2C_BOARD_INFO("ugreen", LED_I2C_ADDRESS),
};

static int leds_ht32f52231_detect(struct i2c_client *client, struct i2c_board_info *info) {
	struct i2c_adapter *adapter = client->adapter;
	const char *name;
	u16 chipid;
	u8 vendid;
	int i;
//	if(!ug_check_product_match(DX4600_SERIES, __func__)){
//		return -ENODEV;
//	}
	for(i=0; i<3; i++){
		chipid = mcu_read16(client, MCU_CHIP_ID);
		//vendid = mcu_read8(client, MCU_FW_VER);
		pr_info("---%s,chipid:%#x \n", __func__,chipid);
		if (chipid == 0xC5B2) {
			name = "led-ht32f52231";
			break;
		}
		msleep(100);
    }

	if (chipid == 0xC5B2)
		name = "led-ht32f52231";
	else
		return -ENODEV;
	vendid = mcu_read8(client, MCU_FW_VER);
	dev_info(&adapter->dev, "found %s version:v1.0.%d\n", name, vendid);
	strscpy(info->type, name, I2C_NAME_SIZE);

	return 0;
}

static const struct i2c_device_id leds_ht32f52231_id[] = {
	{ "led-ht32f52231", 0 },
	{ }
};

static struct i2c_driver leds_ht32f52231_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver		= {
		.name	= "led-ht32f52231",
		.owner	= THIS_MODULE,
	},
	.probe		= leds_ht32f52231_probe,
	.remove		= leds_ht32f52231_remove,
	.shutdown	= leds_ht32f52231_shutdown,
 	.id_table	= leds_ht32f52231_id,
	.detect 	= leds_ht32f52231_detect,
	.address_list   = normal_i2c,
};

module_i2c_driver(leds_ht32f52231_driver);

MODULE_AUTHOR("lijiang@ugreen.com");
MODULE_DESCRIPTION("ugreen_ht32f52231 driver");
MODULE_LICENSE("GPL");
