#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "main.h"
#include "tg_fpga.h"

/*
32					cmd			成功		失败
密钥存储  			0x08		0x09		0x0a
随机数请求			0x10		0x10
密钥请求			0x20		0x20

6144
加密				0x40		0x41		0x42
解密				0x80		0x81		0x82

密钥存储不需要验证CRC，只根据CMD判断成功失败，其他都需要验证CRC。
6148时其中6144长度的数据会异或。

tg_spi_init中发一次密钥请求；每次密钥存储完发一次。
*/
uint16_t CrcCompute(uint8_t *src, uint16_t len, uint16_t crc)
{
	uint8_t uc;
	for (uint16_t j = 0; j < len; j++)
	{
		uc = *(src + j);
		for(uint16_t i = 0; i < 8; i++)
		{
			crc = ((uc ^ (uint8_t)(crc >> 8)) & 0x80) ? ((crc << 1) ^ CRC_DFE_POLY) : (crc << 1);
			uc <<= 1;
		}
	}
	return crc;
}

int tg_crc_compute(uint8_t *src, uint16_t len)
{
	uint16_t crccmpval=0;
	crccmpval = CrcCompute(src, len, 0);
	*(src+len)=(uint8_t)((crccmpval&0xff00)>>8);
	*(src+len+1)=(uint8_t)(crccmpval&0x00ff);
	return 0;
}

int tg_spi_init(int fd)
{
	int ret = 0;
	int x; 
	uint8_t mode = 0;
	uint8_t bits = 8;
	uint32_t speed = SPI_SPEED;//max =10MHz,10000008=wrong 
	uint8_t temp[200];
	int i;
	long l1,l2,l3,l4;	
	struct timeval tv;
	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		perror("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		perror("can't get spi mode");

	/*bits per word*/
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		perror("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		perror("can't get bits per word");

	/*max speed hz*/
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		perror("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		perror("can't get max speed hz");

/*	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);*/
	int num = 100;
	memset(temp,0x11,num);

	for(i = 0 ;i<num;i++)
	{
		temp[i] = i+1;
	}
	temp[0] = 0x55;
	temp[1] = 0x04;

/*	for(i = 0;i<num;i++)	
	{
		write(fd,temp+i,1);
		printf("send %d\n",i);
	}
	printf("write end\n");
*/	
	gettimeofday(&tv,NULL);
	l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
/***************z32 init********************/
	write(fd,temp,100);	
//	memset(temp,0,num);

//	scanf("%d",&x);
	do{ 
		read(fd,temp,1);
	}
	while(temp[0] == 0x33);//z32


	
	read(fd,temp+1,99);
	for(i = 0;i<num;i++)	
		printf("temp[%d] = %x\n",i,temp[i]);

	gettimeofday(&tv,NULL);
	l2 = tv.tv_sec*1000*1000 + tv.tv_usec;
	printf("time2 = %ld  us\n",l2-l1);

	printf("z32 init end\n");
	
/******************************************/
	printf("send recv 100 end\n");

	return 1;
}


//tx_buf 长度32
//rx_buf 长度32
int tg_spi_once_32(int fd,uint8_t cmd,uint8_t *tx_buf,uint8_t *rx_buf)
{
	int i;
	int ret = -1; 
	uint16_t crccmpval=0;
	uint8_t idel_flag = 0x33;
	uint8_t send_buf[36] = {0};
	uint8_t recv_buf[36] = {0};
	send_buf[0] = 0x55;
	send_buf[1] = cmd;
	memcpy(send_buf+2,tx_buf,32);

	crccmpval=CrcCompute(tx_buf,32,0);	
	send_buf[34] = (uint8_t)((crccmpval&0xff00)>>8);
	send_buf[35] = (uint8_t)(crccmpval&0x00ff);
	printf("tg_spi_once_32 crc1 = %x\n",send_buf[34]);
	printf("tg_spi_once_32 crc2 = %x\n",send_buf[35]);




	for(i = 0;i<36;i++)
	{
		if(0 == i%6)
			printf("\n");
		printf("send %d = %x\t",i,send_buf[i]);
	}
	printf("\n-----------------------------\n");

	//WRITE DATA
	write(fd,send_buf,36);
	//READ DATA      
	usleep(1000);

//	scanf("%d",&i);

	do
	{  
		read(fd,recv_buf,1);
	}
	while(recv_buf[0] == idel_flag);

	read(fd,recv_buf+1,35);
	

	memcpy(rx_buf,recv_buf+2,32);

	//PRINT READ DATE
#ifdef PRINT_SPI	
/*	printf("recv cmd = %x\n",*(rx_buf+1));
	printf("%s recv crc1 = %x\n",__FUNCTION__,recv_buf[34]);
	printf("%s recv crc2 = %x\n",__FUNCTION__,recv_buf[35]);*/
#endif

	for(i = 0;i<36;i++)
	{
		if(0 == i%6)
			printf("\n");
		printf("recv %d = %x\t",i,recv_buf[i]);
	}

	printf("\n-----------------------------\n");
	switch(recv_buf[1])
	{
		case 0: //cmd = 0
			ret = -0xc0;
			break;
		case 0x0b:	//cmd error
			ret = -0x0b;
			break;	
		case 0x06:	//send key fail
			ret = -0x06;
			break;
		case 0x0a:	//key store fail
			ret = -0x0a;
			break;
		//CRC CHECK READ DATA	
		case 0x10: //random num req
		case 0x20://key req
			crccmpval=CrcCompute(recv_buf+2,32,0);	
			if(((uint8_t)((crccmpval&0xff00)>>8)==recv_buf[34])&&((uint8_t)(crccmpval&0x00ff)==recv_buf[35]))	
				ret = 0;
			else{
				printf("32 crc error  !!!!\n");
				ret = -0xc1;
			}
			break;
		case 0x05://send key success
		case 0x09://key store success
			ret = 0;
			break;
		default:
			ret = -1;
			break;
	}
	return ret;
}


//tx_buf 长度6148 = 6144+2+2
//rx_buf 长度6148
int tg_spi_once_6k(int fd,int cmd,uint8_t *tx_buf,uint8_t *rx_buf)
{
	int ret = -1;
	int i;
	uint16_t crccmpval=0;
	uint8_t idel_flag = 0x55;
	uint8_t send_buf[6148] = {0};
	uint8_t recv_buf[6148] = {0};
	long l1,l2,l3,l4;
	struct timeval tv;

	send_buf[0] = 0x55;
	send_buf[1] = cmd;
	memcpy(send_buf+2,tx_buf,6146);
	tg_spi_xor(send_buf+2,6144);
	//WRITE DATA
	gettimeofday(&tv,NULL);
	l1 = tv.tv_sec*1000*1000 + tv.tv_usec;

	write(fd,send_buf,6148);
	gettimeofday(&tv,NULL);
	l2 = tv.tv_sec*1000*1000+tv.tv_usec;
//	printf("6k write time = %ld ms  \n",(l2-l1)/1000);

	usleep(1000);
	//READ DATA 
	do{  
		read(fd,recv_buf,1);
	}
	while(recv_buf[0] != idel_flag);


	gettimeofday(&tv,NULL);
	l3 = tv.tv_sec*1000*1000+tv.tv_usec;

	read(fd,recv_buf+1,6147);

	gettimeofday(&tv,NULL);
	l4 = tv.tv_sec*1000*1000+tv.tv_usec;
//	printf("6k read time = %ld ms  \n",(l4-l3)/1000);

	tg_spi_xor(recv_buf+2,6144);
	memcpy(rx_buf,recv_buf+2,6146);
//	write_data_hex(recv_buf,6148,"encrypt_recv.dat");
	printf("------recv cmd%x------\n",*(recv_buf+1));
	switch(recv_buf[1])
	{
		case 0:	//cmd = 0
			ret = -0xc0;
			break;
		case 0x0b:	//cmd error
			ret = -0x0b;
			break;
		case 0x42:	///encrypt fail
			ret = -0x42;
			break;
		case 0x82:	//decrypt fail
			ret = -0x82;
			break;
		case 0x41:	//encrypt success
		case 0x81:	//decrypt success
			crccmpval=CrcCompute(recv_buf+2,6144,0); 
			printf("crccmpval = %x\n",crccmpval);
			if(((uint8_t)((crccmpval&0xff00)>>8) == recv_buf[6146])&&((uint8_t)(crccmpval&0x00ff)==recv_buf[6147])) 
				ret = 0;
			else	
			{
				printf("6144 crc error  !!!!\n");
				ret = -0xc1;
			}	
			break;
		default:
			ret = -1;
			break;
	}
	//CRC CHECK READ DATA	
#ifdef PRINT_SPI
	printf("%s recv crc1 = %x\n",__FUNCTION__,recv_buf[6146]);
	printf("%s recv crc2 = %x\n",__FUNCTION__,recv_buf[6147]);
#endif
	return ret;
}


//tx_buf len = 32/6144
int tg_spi_tx_rx(int fd,uint8_t cmd,uint8_t *tx_buf,uint8_t *rx_buf,int len)
{
	int i;
	long l1,l2;	
	int dun;
	struct timeval tv;
	uint8_t tx_cmd[8] = {0};
	uint8_t rx_cmd[8] = {0};
	int once_ret;
	int ret;
	int errornum = 0;
	int count = 0;
	int count_max = 10;
//	if(0x20 == cmd)
//		count_max = 0;


//cmd   8Bytes	
	tx_cmd[0] = 0x55;
	tx_cmd[1] = cmd;
	tx_cmd[2] = 0x66;
	tx_cmd[3] = 0x55;
	tx_cmd[4] = 0x44;
	tx_cmd[5] = 0x33;
	tx_cmd[6] = 0x22;
	tx_cmd[7] = 0x11;
//buf	len Bytes
	do{	
		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;

		write(fd,tx_cmd,8);
		printf("write 8\n");

	usleep(1000);

#if DEBUG
		do{  
			read(fd,rx_cmd,1);
		}
		while(rx_cmd[0] == 0x33);	
		read(fd,rx_cmd+1,7);
#endif
		
		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000 + tv.tv_usec;
		printf("time2 = %ld  us\n",l2-l1);
#if DEBUG	
		printf("read 8, ");
		for(i = 0;i < 8;i++)
			printf("%x ",rx_cmd[i]);
		printf("\n");
#endif

		usleep(1000);
		if(32 == len)
			once_ret=tg_spi_once_32(fd,cmd,tx_buf,rx_buf);
		else
			once_ret=tg_spi_once_6k(fd,cmd,tx_buf,rx_buf);
		count++;
		if(once_ret<0)
			errornum+=1;
		if(count>count_max)
			break;
		usleep(1000);
	}
	while(once_ret);
	if(errornum == count)
		ret = -1;
	else
		ret = 0;
	
	gettimeofday(&tv,NULL);
	l2 = tv.tv_sec*1000*1000+tv.tv_usec;
#ifdef PRINT_SPI
//	printf("time2 = %ld  us\n",l2-l1);
	printf("***************error_num = %d count = %d***************\n",errornum,count);
#endif
	return ret;
}


void write_data_hex(unsigned char * array,int length,char *string)
{
    int i = 0;
    FILE *fp;
    fp = fopen(string,"wb+");
    if(NULL == fp)
    {
        printf("file open Fail!\n");
    }
	
    while(i < length)
    {        
        fwrite(&array[i],sizeof(unsigned char ),1,fp);
        i++;
    }
	
    fclose(fp);
}



void read_data_hex(unsigned char * array,int length,char* file)
{
	int i = 0;
	int j = 0;
	FILE *fp;
	
	if(fp=fopen(file,"rb+"))
	{
		for(i = 0;i<length;i++)		
			fread(array+i,sizeof(unsigned char ),1,fp);
	}
	//printf("%d\n",fp);
	fclose(fp);
	fp = NULL;
}


void tg_spi_xor(unsigned char * src,int length)
{
	unsigned char key[16] = {0x59, 0xb4, 0xa0, 0xd7, 0x7a, 0x7c, 0x06, 0x31, 0xe8, 0xed, 0xb3, 0x50, 0x72, 0xc4, 0xc9, 0x4e};	
	int i,j,row,col;
	int key_length = 16;
	row = length / key_length;	
	col = length % key_length;	
//	printf("row = %d col = %d \n",row,col);	
	for(i = 0;i < row;i++)	
	{		
		for(j = 0;j < key_length;j++)				
			src[i*key_length+j] = src[i*key_length+j] ^ key[j];				
	}
}


//-----------------------------------------------------------------------------------


int tg_spi_key_store(int fd,uint8_t *tx_buf,uint8_t *rx_buf)
{
	int ret = 0;
	uint8_t cmd = 0x08 ;
	int length = 32;
	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	return ret;
}

int tg_spi_random_num_req(int fd,uint8_t *tx_buf,uint8_t *rx_buf)
{
	int ret = 0;
	uint8_t cmd = 0x10 ;
	int length = 32;
	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	return ret;
}

int tg_spi_key_req(int fd)
{
	int ret = 0;
	uint8_t cmd = 0x20 ;//key req
	int length = 32;
	uint8_t tx_buf[36] = {0};
	tx_buf[10] = 0x11;
	uint8_t rx_buf[36] = {0};
	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	if(ret)
		return ret;
	return ret;
}




int tg_spi_cert_encrypt(int fd,uint8_t *tx_buf,uint8_t *rx_buf)
{
	int ret = 0;
	uint8_t cmd = 0x40 ;
	int length = 6148;
	uint8_t send_buf[36] = {0};
	uint8_t recv_buf[36] = {0};

//	tg_spi_key_req_and_send(fd,send_buf,recv_buf);

	if(ret < 0)
		return ret;
	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	
	return ret;
}

int tg_spi_cert_decrypt(int fd,uint8_t *tx_buf,uint8_t *rx_buf)
{
	int ret = 0;
	uint8_t cmd = 0x80 ;
	int length = 6148;
	uint8_t send_buf[36] = {0};
	uint8_t recv_buf[36] = {0};

//	tg_spi_key_req_and_send(fd,send_buf,recv_buf);

	if(ret < 0)
		return ret;
	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	return ret;
}


