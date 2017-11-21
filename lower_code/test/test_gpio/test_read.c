#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd;
	char touch = 0;
	
	fd = open("/dev/gpio_ctl",O_RDWR);
	printf("fd = %d\n",fd);

	while(1)
	{
		read(fd,&touch,sizeof(char));
		printf("touch = %d\n",touch);
		if(0 == touch)
			printf("没有touch！！  \n");
		else if(1 == touch)
			printf("-------------有有有有--------------\n");
		usleep(100000);
//		printf("***********************************************************\n");
//		sleep(5);
	}
	close(fd);
	
	printf("---------end---------\n");
	return 0;
}
