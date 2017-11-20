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
#include "rtx.h"
#include "tg_main.h"

//int num=0;

int wd_fd;


int file_size(char* filename)  
{  

    struct stat statbuf;  
    stat(filename,&statbuf);  
    int size=statbuf.st_size; 
    return size;  
}

int writeComm(int fd, char* buf, int len)
{
#if defined(HID_DEV_H3)||defined(HID_DEV_JZ)
	int ret, packSize=64,length=0,block;

	if(len<=0) len=PACKAGE_SIZE;
	else len+=(PACKAGE_SIZE-(len%PACKAGE_SIZE));
	length=len;

	while(length>0){
		block=packSize;
	
		ret=write(fd,&buf[len-length],block);
		length-=ret;
//		printf("%d %d %d %d %d\n",ret,len,length,len-length,block);
		if(length<0||ret<0)break;
	}
//printf("%d %s\n",len,buf);
#endif

#ifdef NET_DEV_H3
  int rs = write(fd, buf, len);
  if(rs == -1)
  {
    perror("writeComm:");
    return -1;
  }
#endif
  return 0;
}


int readComm(int fd, char* buf, int len, int timeout /* 潞脕脙毛 */)
{
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  struct timeval tv = {0, 1000*timeout};
  int total1=0,total = len,err_count=len/1000,i;
  char* startRev = buf;

	while(total > 0)
	{
		int res = select(fd+1, &fds, NULL, NULL, &tv);
		if(res == -1)
		{
			if(errno == EINTR)
				continue;
			else{
				printf("socket error %d with select", errno);
				return -1;
			}
		}
		else if(res == 0){
			err_count=0;
			buf[len - total] = '\0';
#ifdef HID_DEV_H3
			return total1;
#endif
#if defined(NET_DEV_H3)||defined(HID_DEV_JZ)
			return len - total;
#endif
		}
		if(FD_ISSET(fd, &fds))
		{
			int rs,rs1;
#ifdef HID_DEV_H3
			if(len!=1024) len+=8;
			rs=read(fd,startRev,len)-8;
			rs1=startRev[6]|startRev[7]<<8;
			for(i=0;i<rs1;i++)
				startRev[i]=startRev[i+8];
#endif

#if defined(NET_DEV_H3)||defined(HID_DEV_JZ)
			rs = read(fd, startRev, len); 
#endif
			if(rs < 0)
			{
				buf[len - total] = '\0';
				perror("readComm:");
				return -1;
			}
			total1+=rs;
			// printf("total:[ %d %d %d %d ]\n", total,total1,rs,rs1);
			total -= rs;
			startRev += rs;
		}

#ifdef NET_DEV_H3
		if(err_count--<0){
		usleep(100000);
		return -1;
		}
#endif
	}
	return len - total;
}


int sendPackage(int fd,TG_package *pack)
{
//	trans_encrypt(dat,sizeof(TG_package));
#if defined(HID_DEV_H3)||defined(HID_DEV_JZ)
	return writeComm(fd,(char *)pack,0);
#endif
#ifdef NET_DEV_H3
//	char temp[1024] = {0};
//	memcpy(temp,pack,sizeof(TG_package));
	return writeComm(fd,(char *)pack,sizeof(TG_package));
//	return writeComm(fd,(char *)pack,PACKAGE_SIZE);
#endif
}

int sendDataPackage(int fd,char *data,int len)
{
#if defined(HID_DEV_H3)||defined(HID_DEV_JZ)
	usleep(250*1000);
#endif
	return writeComm(fd,data,len);
}

int recvPackage(int fd,TG_package *pack,int timeout)
{
	int ret;
	ret = readComm(fd,pack,PACKAGE_SIZE,timeout);
//ret = readComm(fd,pack,256,timeout);

//	trans_encrypt(pack,sizeof(TG_package));
	return ret;
}

int recvDataPackage(int fd,char *data,int len,int timeout)
{
#if defined(HID_DEV_H3)||defined(HID_DEV_JZ)
    int r,length=0,cnt=50;
    while(cnt--){
        r=readComm(fd,&data[length],len-length,timeout);
        if(r>0) 
			length+=r;
		printf("-----block: %d-- %d--- %d-\n",r,len-length,len);
        if(length>=len||r<=0) 
			break;
    }
    return length;
#endif

#ifdef NET_DEV_H3
	int r,length=0,cnt=3;
	while(cnt--){
		r=readComm(fd,data,len,timeout);
		if(r>0) length+=r;
		//printf("-----block:%d--%d---%d-\n",r,length,len);
		if(length>=len||r<0) 
			break;
	}
	return length;
#endif
}

void StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int nLen)
{
	unsigned char h1,h2;
	unsigned char s1,s2;
	int i;

	for (i=0; i<nLen; i++)
	{
		h1 = pbSrc[2*i];
		h2 = pbSrc[2*i+1];
		s1 = toupper(h1) - 0x30;
		if (s1 > 9)
		s1 -= 7;

		s2 = toupper(h2) - 0x30;
		if (s2 > 9)
		s2 -= 7;

		pbDest[i] = s1*16 + s2;
	}
}

void HexToStr(unsigned char *pbDest, unsigned char *pbSrc, int nLen)
{
	unsigned char ddl,ddh;
	int i;

	for (i=0; i<nLen; i++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;
		if (ddh > 57) ddh = ddh + 7;
		if (ddl > 57) ddl = ddl + 7;
		pbDest[i*2] = ddh;
		pbDest[i*2+1] = ddl;
	}

	pbDest[nLen*2] = '\0';
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

int init_Device()
{
	int fd,i,res;
	char dev[64];

#if defined(HID_DEV_H3)||defined(HID_DEV_JZ)
		fd = open("/dev/hidg0", O_RDWR	/*|O_NONBLOCK| O_NDELAY*/);	
		if(fd < 0) 
			return -1;  
		else 
			return fd;
#endif
#ifdef COM_DEV
		for(i=0;i<5;i++){
			memset(dev,0,sizeof(dev));
			sprintf(dev,"/dev/ttyACM%d",i);
			fd=initComm(dev,9600);
			if(fd>0) return fd;
	
			memset(dev,0,sizeof(dev));
			sprintf(dev,"/dev/ttyGS%d",i);
			fd=initComm(dev,115200);
			if(fd>0) return fd;
	
		}
	
#endif

	return -1;	
}

int release_Device(int fd)
{
	close(fd);
	return 1;
}




