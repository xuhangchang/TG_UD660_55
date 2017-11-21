#include<stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd;
	char buf = 0x05;
	int num;
	
	fd = open("/dev/gpio_ctl",O_RDWR);
	printf("fd = %d\n",fd);

	while(1)
	{
		scanf("%x",&num);
		buf = num;
		printf("buf = %x\n",buf);
		write(fd,&buf,sizeof(char));
//		sleep(5);
	}
	close(fd);
	
	printf("---------end---------\n");
	return 0;
}
