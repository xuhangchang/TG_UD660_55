#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gpio.h"

/*****************************************************************  
* function:		sound_send
* description:  send sound cmd
* param1:     	char sound_cmd	: sound cmd (input)
* return:    	void 			            -   
* others£º
*		0x00	Bi
*		0x01	BiBi	
*		0x02	dengji chenggong
* 		0x03	dengji shibai
* 		0x04	qing zaifang yici
*      	0x05	qing zhengque fangru shouzhi
*		0x06	qing ziran qingfang shouzhi
* 		0x07	yanzheng chenggong
* 		0x08	yanzheng shibai
* 		0x09	qingchongshi
* 		0x0A	shanchu chenggong
* 		0x0B	shanchu shibai
* 		0x0C	zhijingmai yiman
*		0xE0~0xE7  sound level,0xE0 min,0xE7 max.
*
*
* date:       	2017/03/04			
* author:     	Hangchang Xu
******************************************************************/ 
void sound_send(int fd,char sound_cmd)
{
	char sound_str[3] = {0};

	sound_str[0] = 's';
	sound_str[1] = sound_cmd;
	write(fd,sound_str,sizeof(sound_str));
}


/*****************************************************************  
* function:		pwm_send
* description:  send pwm cmd,control led
* param1:     	char model	: 0,open; 1,close; (input)
* param2:     	int duty_pct	: pwm percent % (input)
* return:    	void 			            -   
* others£º
*
* date:       	2017/03/04			
* author:     	Hangchang Xu
******************************************************************/
void pwm_send(int fd,char model,int duty_pct)
{
	char pwm_cmd[10] = {0};
	int period_time,duty_time;
	period_time = 255;
	duty_time = duty_pct*255/100;

	*pwm_cmd = 'p';
	*(pwm_cmd + 1) = model;
	*(int *)(pwm_cmd + 2) = period_time - duty_time;
	*(int *)(pwm_cmd + 6) = period_time;
	write(fd,pwm_cmd,sizeof(pwm_cmd));
}




		

/*****************************************************************  
* function:		touch_signal
* description:  detect touch
* param1:     	int flag	: model  1:register model
*								     2:compare model
*									 3:detect release model
*
*
* return:    int :	register model: 1:detect touch	
*									0:over time	
*					compare model:  1:detect touch	
*									0:error
*					detect release model:  1: detect touch
*										   0: not detect touch	
*										   
*
*
*
* others£º	register model : if touch,return 1;
*							 if no touch,waiting until 8s later return 0; 
*			compare model:	 if touch,return 1;
*							 if no touch,waiting ; 
*			detect release model: if touch,return 1;	
*								  if no touch,return 0 ; 
*						 
* date:       	2017/03/04			
* author:     	Hangchang Xu
******************************************************************/

int touch_signal(int fd,int flag)
{
	clock_t start_time,end_time;
	double inter_time = 0;
	char touch = 0;	
	
//register model
	if(1 == flag)
	{
		start_time = clock();
		while(3 != (touch&3) && inter_time < 8.0)
		{
			end_time = clock();
			inter_time = (double)(end_time - start_time)/CLOCKS_PER_SEC;
			usleep(100000);
			read(fd,&touch,sizeof(char));
		}	
		if(3 == (touch&3))		//detect touch 
			return 1;		
		else			//no touch ,overtime
			return 0;
	}

//compare model
	if(2 == flag)
	{
		while( 3 != (touch&3))
		{
			read(fd,&touch,sizeof(char));
			usleep(100000);
		}
		if(3 == (touch&3))				//detect touch 
			return 1;		
		else					//no touch
			return 0;	
	}

//detect release model
	if(3 == flag)
	{
		read(fd,&touch,sizeof(char));
		if(3 == (touch&3))				//detect touch 
			return 1;		
		else					//no touch
			return 0;	
	}	
}




