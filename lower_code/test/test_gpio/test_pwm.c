#include<stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, const char *argv[])
{
	int fd;
	char pwm_cmd[10] = {0};
	int num;
	int model = 0;
	int duty_time = 0;
	int period_time = 0;
	
	fd = open("/dev/gpio_ctl",O_RDWR);
	printf("fd = %d\n",fd);

	while(1)
	{
//		printf("please input model:\n");
//		scanf("%d",&model);
		printf("please input duty_time:\n");
		scanf("%d",&duty_time);
//		buf = num;
		*pwm_cmd = 'p';
//		*(pwm_cmd + 1) = model;
		*(pwm_cmd + 1) = 0;
		*(int *)(pwm_cmd + 2) = 255-duty_time;
		*(int *)(pwm_cmd + 6) = 255;
			
//		printf("pwm_cmd = %s\n",pwm_cmd);
		write(fd,&pwm_cmd,sizeof(pwm_cmd));
		memset(pwm_cmd,0,10);
//		sleep(5);
	}
	close(fd);
	
	printf("---------end---------\n");
	return 0;
}
