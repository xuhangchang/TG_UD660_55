#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <mach/sys_config.h>
//#include <linux/pinctrl/pinconf-sunxi.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#include <asm/io.h>
#include "../base/base.h"
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/kthread.h>//kthread_ceate(), kthread_run()

//GPIO_1 为语音命令

#define DEVICE_NAME		"gpio_ctl"

#define TOUCH_1 2				//touch1 引脚号
#define TOUCH_2 3				//touch2 引脚号
#define SOUND_PIN 6		//语音引脚号
#define SOUND_BUSY_PIN	17//语音busy忙信号输入
#define REMOVE_DEV	200//拆壳
//#define PWM 5			//PWM 红外
//#define  PA17 17

struct pwm_device	*pwm;

/*发送语音时序*/
static inline void set_value_per_cycle(int value)
{
	switch(value)
	{
	case 0:
		gpio_set_value(SOUND_PIN,1);
		udelay(700);
		gpio_set_value(SOUND_PIN,0);
		mdelay(2);
		udelay(100);
		break;
	case 1:
		gpio_set_value(SOUND_PIN,1);
		mdelay(2);
		udelay(100);
		gpio_set_value(SOUND_PIN,0);
		udelay(700);				
		break;
	default:
		printk("set_value_per_cycle error\n");
		break;
	}	
}

/*发送语音命令*/
static void sound_cmd(int cmd)
{	
	gpio_set_value(SOUND_PIN,0);
	mdelay(5);
	/*语音命令开始发送(最低位开始)*/
	set_value_per_cycle((cmd >> 0) & 1);
	set_value_per_cycle((cmd >> 1) & 1);
	set_value_per_cycle((cmd >> 2) & 1);
	set_value_per_cycle((cmd >> 3) & 1);
	set_value_per_cycle((cmd >> 4) & 1);
	set_value_per_cycle((cmd >> 5) & 1);
	set_value_per_cycle((cmd >> 6) & 1);	
	set_value_per_cycle((cmd >> 7) & 1);	
	/*语音命令发送结束*/	
	gpio_set_value(SOUND_PIN,1);
	mdelay(50);	
}

/*检测touch信号*/
static int detect_touch()
{
	int touch1 = 0;
	int touch2 = 0;
	touch1 = gpio_get_value(TOUCH_1);
	touch2 = gpio_get_value(TOUCH_2);
//	printk("touch1 = %d   touch2 = %d\n",touch1,touch2);
	return	(touch1 + 2*touch2);
	
/*	
	if(0 == touch1 && touch2)
		return 0;
	else
	{
		mdelay(5);
		touch1 = gpio_get_value(TOUCH_1);
		touch2 = gpio_get_value(TOUCH_2);
		return	(touch1 + 2*touch2);
	}	
	*/
}


static int tg_gpio_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int tg_gpio_close(struct inode *inode, struct file *file)
{
	return 0;
}


static int tg_gpio_read(struct file *filp, char __user *buff,
		size_t count, loff_t *offp)
{
	char touch_signal = 0;
	char sound_busy = 0 ;
	char remove_flag = 0 ;
	char to_user = 0;
	touch_signal = detect_touch();
	sound_busy = gpio_get_value(SOUND_BUSY_PIN);
	remove_flag = gpio_get_value(REMOVE_DEV);
	to_user = remove_flag * 100 + sound_busy * 10 + touch_signal;
	if (copy_to_user(buff, &to_user, sizeof(char))) {
		pr_info("sunxi_pwm write fail!\n");
	}
//	printk("read = %d\n",to_user);
	return 0 ;
}
static ssize_t tg_gpio_write(struct file *file, const char __user * user_buffer,
		size_t count, loff_t *ppos)
{
	int retval = 0, r;
	int sound_num = 0;
	int pwm_duty = 0,pwm_period = 0;
	char buf[20]={0},*p;
	p=buf;

	if (copy_from_user(p, user_buffer, count)) {
		retval = -EFAULT;
		pr_info("my_gpio write fail!\n");
	}
	switch(*p)
	{
	case 's':
		printk("sound command = %x\n",*(p+1)); 
		sound_cmd(*(p+1));
		break;
	case 'p':
		if(0 == *(p+1))
		{
			pwm_duty =  *(int *)(p+2);
			pwm_period =  *(int *)(p+6);
			pwm_config(pwm,pwm_duty*1000,pwm_period*1000);
			pwm_enable(pwm);
		}
		else if(1 == *(p+1))
		{
			pwm_disable(pwm);
		}				
		break;
	case 'l':
		printk("led command = %d\n",*(p+1)); 
		break;	
	default:
		printk("command error\n");
		break;
	}	
	printk("*p = %x\n",*p);
}

static struct file_operations dev_fops = {
	.owner	= THIS_MODULE,
	.open		= tg_gpio_open,
	.release	= tg_gpio_close, 
	.read		= tg_gpio_read,
	.write	= tg_gpio_write,
};

static struct miscdevice misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DEVICE_NAME,
	.fops		= &dev_fops,
};
static int sw_uart_get_devinfo(void)
{
	u32 i, j;
	char gpio_para[16] = {0};

	script_item_u val;
	script_item_value_type_e type;

	sprintf(gpio_para, "gpio_para");
	/* get used information */
	type = script_get_item(gpio_para, "gpio_used", &val);
	if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
		printk("get uart%d's usedcfg failed\n", i);
		goto fail;
	}
	//pdata->used = val.val;
	if (val.val) {
		type = script_get_item(gpio_para, "gpio_1", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_1 failed\n");
			goto fail;
		}
		printk("--------gpio_1--PG11------   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_2", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_2 failed\n");
			goto fail;
		}
		printk("--------gpio_2--PA02-----   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_3", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_3 failed\n");
			goto fail;
		}
		printk("--------gpio_3--PA03-----   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_4", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_4 failed\n");
			goto fail;
		}
		printk("--------gpio_4--PA08-----   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_5", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_5 failed\n");
			goto fail;
		}
		printk("--------gpio_5--PA09-----   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_6", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_6 failed\n");
			goto fail;
		}
		printk("--------gpio_6--PA06-----   %d  ---\n",val.gpio);	
/*		
		type = script_get_item(gpio_para, "gpio_7", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_7 failed\n");
			goto fail;
		}
		printk("--------gpio_7--PA01-----   %d  ---\n",val.gpio);	
	*/
		type = script_get_item(gpio_para, "gpio_8", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_8 failed\n");
			goto fail;
		}
		printk("--------gpio_8--PA17-----   %d  ---\n",val.gpio);	
		
		type = script_get_item(gpio_para, "gpio_9", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio_9 failed\n");
			goto fail;
		}
		printk("--------gpio_9--PA04-----   %d  ---\n",val.gpio);	
		//pdata->uart_io[j] = val.gpio;
	}

	sprintf(gpio_para, "pwm0_para");
	type = script_get_item(gpio_para, "pwm_used", &val);
	if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
		printk("get uart%d's usedcfg failed\n", i);
		goto fail;
	}

	if (val.val) {
		type = script_get_item(gpio_para, "pwm_positive", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get pwm%d's IO(pwm) failed\n", i);
			goto fail;
		}
		printk("--------pwm-------   %d  ---\n",val.gpio);
	}
	return 0;
fail:
	printk("get uart configuration failed\n");
	return -1;
}
static int __init sensor_dev_init(void)
{
	int ret;

	pwm = pwm_request(0, "backlight");
	if (IS_ERR(pwm)) {
		printk( "unable to request PWM for backlight\n");
		ret = PTR_ERR(pwm);
	} else
		printk("got pwm for backlight\n");

	sw_uart_get_devinfo();


	if(gpio_request(TOUCH_1, NULL)){
		printk("TOUCH_1 gpio_pin gpio_request fail \n");
		return -1;
	}
	printk("TOUCH_1 gpio_request successed \n");

	if(gpio_request(TOUCH_2, NULL)){
		printk("TOUCH_2 gpio_pin gpio_request fail \n");
		return -1;
	}
	printk("TOUCH_2 gpio_request successed \n");
	
	if(gpio_request(SOUND_PIN, NULL)){
		printk("SOUND_PIN gpio_pin gpio_request fail \n");
		return -1;
	}
	printk("SOUND_PIN gpio_request successed \n");
	
	if(gpio_request(SOUND_BUSY_PIN, NULL)){
		printk("SOUND_BUSY_PIN gpio_pin gpio_request fail \n");
		return -1;
	}
	printk("SOUND_BUSY_PIN gpio_request successed \n");
	
	if(gpio_request(REMOVE_DEV, NULL)){
		printk("REMOVE_DEV gpio_pin gpio_request fail \n");
		return -1;
	}
	printk("REMOVE_DEV gpio_request successed \n");
	
	pwm_config(pwm,255*1000,255*1000);
	pwm_enable(pwm);
	
	gpio_direction_input(TOUCH_1);
	gpio_direction_input(TOUCH_2);			
	gpio_direction_output(SOUND_PIN,1);
	gpio_direction_input(SOUND_BUSY_PIN);
	gpio_direction_input(REMOVE_DEV);
	
	gpio_set_value(SOUND_PIN,1);
	sound_cmd(0xE3);
	
	return misc_register(&misc);
}


static void __exit sensor_dev_exit(void)
{
	gpio_free(TOUCH_2);
	gpio_free(TOUCH_1);
	gpio_free(SOUND_PIN);
	printk("Matirx GPIO Int sensor exit\n");
	misc_deregister(&misc);
}

module_init(sensor_dev_init);
module_exit(sensor_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc.");

