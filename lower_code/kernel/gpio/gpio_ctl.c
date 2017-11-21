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
#define MAX_SENDEV_NUM      8

#define LED_1 200				//LED_1
#define LED_2 201				//LED_2
#define TOUCH_1 2				//touch1 引脚号
#define TOUCH_2 3				//touch2 引脚号
#define SOUND_PIN 6		//语音引脚号
#define SOUND_BUSY_PIN	17//语音busy忙信号输入
//#define PWM 5			//PWM 红外
//#define  PA17 17


struct pwm_device	*pwm;
static struct semaphore lock;


static DECLARE_WAIT_QUEUE_HEAD(sensor_dev_waitq);

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
void sound_cmd(int cmd)
{	
//	gpio_direction_output(SOUND_PIN,1);	
//	gpio_set_value(SOUND_PIN,1);
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

/*控制led灯*/
void led_cmd(char cmd)
{	
	int led1_val = 0;
	int led2_val = 0;
	led1_val = cmd/10;
	led2_val = cmd%10;
	gpio_set_value(LED_1,led1_val);
	gpio_set_value(LED_2,led2_val);
}


/*检测touch信号*/
int detect_touch()
{
	int touch1 = 0;
	int touch2 = 0;
//	gpio_direction_input(TOUCH_1);
//	gpio_direction_input(TOUCH_2);		

	touch1 = gpio_get_value(TOUCH_1);
	touch2 = gpio_get_value(TOUCH_2);
	printk("touch1 = %d   touch2 = %d\n",touch1,touch2);
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




/*把PWM相关字符串转换成 int 类型*/
int StrPwmToInt(const char* str,int *duty_ns,int *period_ns)  
{  
	long num=0,i=2;  
	*duty_ns = 0;
	*period_ns = 0;
	if(*str==NULL)  
		return 0 ;  

	while(*(str + i) != '_')  
	{  
		if(*(str + i) >= '0' && *(str + i) <= '9')  
		{      
			*duty_ns=*duty_ns*10+(*(str + i)-'0');  
			i++;
		}  
		else  
		{  
			break;  
		}  
	} 
	i++; 
	while(*(str + i) != '\0')  
	{  
		if(*(str + i) >= '0' && *(str + i) <= '9')  
		{      
			*period_ns=*period_ns*10+(*(str + i)-'0');
			i++;  
		}  
		else  
		{  
			break;  
		}  
	} 
	printk("duty_ns = %d , period_ns = %d \n",*duty_ns,*period_ns); 
	return 0;  
} 

static int gpio_int_sensors_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int gpio_int_sensors_close(struct inode *inode, struct file *file)
{
	return 0;
}

// static int read_counter = 0;
static int gpio_int_sensors_read(struct file *filp, char __user *buff,
		size_t count, loff_t *offp)
{
	char touch_signal = 0;
	char sound_busy = 0 ;
	char to_user = 0;
	touch_signal = detect_touch();
	sound_busy = gpio_get_value(SOUND_BUSY_PIN);
	to_user = sound_busy * 10 + touch_signal;
	if (copy_to_user(buff, &touch_signal, sizeof(char))) {
		pr_info("sunxi_pwm write fail!\n");
	}
	//	printk("touch_signal = %d\n",*buff);

	/*
	   err = copy_to_user((void *)buff, (const void *)(&sensor_dev_value),
	   min(sensor_num, count));
	   for (i = 0; i<sensor_num; i++) {
	   sensor_dev_value[i] = 0;
	   }
	   */
	return 0 ;
}
static ssize_t sunxi_pwm_write(struct file *file, const char __user * user_buffer,
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
			//				StrPwmToInt(p,&pwm_duty,&pwm_period);  
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
		led_cmd(*(p+1));
		break;	
	default:
		printk("command error\n");
		break;
	}	


	//	pwm_config(pwm,128,500000);
	//	pwm_enable(pwm);
	printk("*p = %c\n",*p);

}



static struct file_operations dev_fops = {
	.owner		= THIS_MODULE,
	.open		= gpio_int_sensors_open,
	.release	= gpio_int_sensors_close, 
	.read		= gpio_int_sensors_read,
	.write		= sunxi_pwm_write,
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
		/* get type information */
		/*
		   type = script_get_item(gpio_para, "uart_type", &val);
		   if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
		   printk("get uart%d's type failed\n", i);
		   goto fail;
		   }
		 */
		/*get gpio*/
		type = script_get_item(gpio_para, "gpio_1", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio%d's IO(uart_tx) failed\n", i);
			goto fail;
		}
		printk("--------gpio_1--PG11------   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_2", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio%d's IO(uart_tx) failed\n", i);
			goto fail;
		}
		printk("--------gpio_2--PA02-----   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_3", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio%d's IO(uart_tx) failed\n", i);
			goto fail;
		}
		printk("--------gpio_3--PA03-----   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_4", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio%d's IO(uart_tx) failed\n", i);
			goto fail;
		}
		printk("--------gpio_4--PA08-----   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_5", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio%d's IO(uart_tx) failed\n", i);
			goto fail;
		}
		printk("--------gpio_5--PA09-----   %d  ---\n",val.gpio);

		type = script_get_item(gpio_para, "gpio_6", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio%d's IO(uart_tx) failed\n", i);
			goto fail;
		}
		printk("--------gpio_6--PA06-----   %d  ---\n",val.gpio);	
		
		
		type = script_get_item(gpio_para, "gpio_7", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			printk("get gpio%d's IO(uart_tx) failed\n", i);
			goto fail;
		}
		printk("--------gpio_7--PA01-----   %d  ---\n",val.gpio);	
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

	sema_init(&lock, 1);
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
	
	if(gpio_request(LED_1, NULL)){
		printk("LED_1 gpio_pin gpio_request fail \n");
		return -1;
	}
	printk("LED_1 gpio_request successed \n");

	if(gpio_request(LED_2, NULL)){
		printk("LED_2 gpio_pin gpio_request fail \n");
		return -1;
	}
	printk("LED_2 gpio_request successed \n");
	
	
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
	

	pwm_config(pwm,255*1000,255*1000);
	pwm_enable(pwm);
	
	gpio_direction_input(TOUCH_1);
	gpio_direction_input(TOUCH_2);			
	gpio_direction_output(LED_1,0);
	gpio_direction_output(LED_2,0);
	gpio_direction_output(SOUND_PIN,1);
	gpio_direction_input(SOUND_BUSY_PIN);
	
	gpio_set_value(SOUND_PIN,1);
	sound_cmd(0xE7);
	
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

