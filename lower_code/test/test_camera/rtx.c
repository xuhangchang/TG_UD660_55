#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/watchdog.h>
#include <ctype.h>
#include <errno.h>
#include "md5.h"
#include "rtx.h"

//int num=0;

int wd_fd;

int writeComm(int fd, char* buf, int len)
{
  int rs = write(fd, buf, len);
  if(rs == -1)
  {
    perror("writeComm:");
    return -1;
  }
  //printf("writeComm: %s\n", buf);
  return 0;
}
int readComm(int fd, char* buf, int len, int timeout /* 潞脕脙毛 */)
{
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  struct timeval tv = {0, 1000*timeout};
  int total = len,err_count=len/1000;
  char* startRev = buf;
  while(total > 0)
  {
    int res = select(fd+1, &fds, NULL, NULL, &tv);
    if(res == -1)
    {
      if(errno == EINTR)
        continue;
      else
      {
        printf("socket error %d with select", errno);
        return -1;
      }
    }
    else if(res == 0)
    {err_count=0;
      buf[len - total] = '\0';
      return len - total;
    }

    if(FD_ISSET(fd, &fds))
    {
      int rs = read(fd, startRev, len);
      if(rs < 0)
      {
        buf[len - total] = '\0';
        perror("readComm:");
        return -1;
      }
      // printf("total: %d \n", total);
      total -= rs;
      startRev += rs;
    }


    if(err_count--<0)
    {
        //qDebug()<<res<<len/1000<<err_count--;
        usleep(100000);
        return -1;
    }
  }



  //printf("readComm: %s\n", buf);
  return len - total;
}

/**
 * compute the value of a string
 * @param  dest_str
 * @param  dest_len
 * @param  md5_str
 */
int Compute_string_md5(unsigned char *dest_str, unsigned int dest_len, char *md5_str)
{
	int i;
	unsigned char md5_value[MD5_SIZE];
	MD5_CTX md5;

	// init md5
	MD5Init(&md5);

	MD5Update(&md5, dest_str, dest_len);

	MD5Final(&md5, md5_value);

	// convert md5 value to md5 string
	for(i = 0; i < MD5_SIZE; i++)
	{
		snprintf(md5_str + i*2, 2+1, "%02x", md5_value[i]);
	}

	return 0;
}
int file_size(char* filename)  
{  

    struct stat statbuf;  
    stat(filename,&statbuf);  
    int size=statbuf.st_size; 
    return size;  
}


int sendPackage(int fd,TG_package *pack)
{
	return writeComm(fd,pack,sizeof(TG_package));
}
int sendDataPackage(int fd,char *data,int len)
{
	return writeComm(fd,data,len);
}

int recvPackage(int fd,TG_package *pack,int timeout)
{
	return readComm(fd,pack,PACKAGE_SIZE,timeout);
}
int recvDataPackage(int fd,char *data,int len,int timeout)
{
    int r,length=0,cnt=3;
    while(cnt--){
        r=readComm(fd,data,len,timeout);
        if(r>0) length+=r;
		//printf("-----block:%d--%d---%d-\n",r,length,len);
        if(length>=len||r<0) break;

    }
    return length;
}


int init_watchdog(int timeout)
{   
    wd_fd = open("/dev/watchdog",O_RDWR);  
    if(wd_fd < 0)    
    {    
        printf("wd open failed\n");    
        return;  
    }  

    ioctl(wd_fd, WDIOC_SETOPTIONS, WDIOS_ENABLECARD);  
    ioctl(wd_fd, WDIOC_SETTIMEOUT, &timeout);
}
int feed_watchdog()
{
	ioctl(wd_fd,WDIOC_KEEPALIVE,NULL);
}
int release_watchdog()
{
	close(wd_fd);
}







