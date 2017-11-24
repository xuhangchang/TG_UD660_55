#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd;
	char touch = 0;
	fd = open("/dev/gpio_ctl",O_RDWR);
	while(1)
	{
		read(fd,&touch,sizeof(char));
		printf("touch = %d\n",touch);
		
		usleep(100000);
	}
	close(fd);
	
	printf("---------end---------\n");
	return 0;
}
