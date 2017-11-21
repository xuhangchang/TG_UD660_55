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
#include "tg_main.h"
#include "tg_spi.h"

/*
32					cmd			成功		失败
密钥存储  			0x08		0x09		0x0a
随机数请求			0x10		0x10
密钥请求			0x20		0x20

6112
加密				0x40		0x41		0x42
解密				0x80		0x81		0x82

密钥存储不需要验证CRC，只根据CMD判断成功失败，其他都需要验证CRC。
6116时其中6112长度的数据会异或。

tg_spi_init中发一次密钥请求；每次密钥存储完发一次。
*/
static uint16_t CrcCompute(uint8_t *src, uint16_t len, uint16_t crc)
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
	uint8_t mode = 0;
	uint8_t bits = 8;
	uint32_t speed = SPI_SPEED;//max =10MHz,10000008=wrong 
	uint8_t temp[100];
	int i;
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

	memset(temp,0x11,100);
	temp[0] = 0x55;
	write(fd,temp,100);
	do{ 
		read(fd,temp,1);
	}
	while(temp[0] != 0x55);
	read(fd,temp+1,100-1);

#ifdef TG_SPI_DEBUG
	for(i = 0;i<100;i++)	
		printf("temp[%d] = %x\n",i,temp[i]);
#endif

	tg_spi_tx_rx(fd ,0x20,temp,32);
	return 1;
}


int tg_spi_once(int fd,int cmd,uint8_t *tx_buf,uint8_t *rx_buf,int len)
{
	int ret;
	uint16_t i; 
	uint16_t crccmpval=0;
	uint8_t idel_flag = 0;
	uint8_t *temp_tx_buf;
	temp_tx_buf = (uint8_t *)calloc(len + 4,sizeof(uint8_t));
	memcpy(temp_tx_buf,tx_buf,len + 4);
	if(6112 == len)
		idel_flag = 0;
	else
		idel_flag = 0x33;

#ifdef TG_SPI_DEBUG
	printf("%s tx_buf:\n",__FUNCTION__);
	for(i = 0; i < 20; i++)
	{
		printf("%02x ",temp_tx_buf[i]);
	}
	printf(".......\n");
	for(i = len-16; i < len+4; i++)
	{
		printf("%02x ",temp_tx_buf[i]);
	}
		printf("\n");
#endif

	if(6112 == len)
		tg_spi_xor(temp_tx_buf+2,len);
	
	//WRITE DATA
	write(fd,temp_tx_buf,len+4);
	//READ DATA      
	do
	{  
		read(fd,rx_buf,1);
	}
//	while(rx_buf[0] == 0|| rx_buf[0] == idel_flag);
	while(rx_buf[0] == idel_flag);
	read(fd,rx_buf+1,len+3);

	//PRINT READ DATE
	if(6112 == len)
		tg_spi_xor(rx_buf+2,len);

#ifdef TG_SPI_DEBUG
	printf("%s rx_buf:\n",__FUNCTION__);
	for(i = 0; i < 20; i++)
	{
		printf("%02x ",rx_buf[i]);
	}
	printf(".......\n");
	for(i = len-16; i < len+4; i++)
	{
		printf("%02x ",rx_buf[i]);
	}
	printf("\n");
#endif

	printf("recv cmd = %x\n",*(rx_buf+1));

	if(0 == *(rx_buf+1)){	//cmd = 0
		ret = -2;
		goto fail1;
	}
	if(0x0b == *(rx_buf+1)){	//cmd error
		ret = -*(rx_buf+1);
		goto fail1;
	}
	if(0x09 == *(rx_buf+1)){	//key store success
		ret = 0;
		goto fail1;
	}
	if((0x0a == *(rx_buf+1))){	//key store fail
		ret = -*(rx_buf+1);
		goto fail1;
	}
	
	if((0x42 == *(rx_buf+1))|| (0x82  == *(rx_buf+1))){///encrypt/decrypt error
		ret = -*(rx_buf+1);
		goto fail1;
	}
	//CRC CHECK READ DATA	
#ifdef TG_SPI_DEBUG
	printf("%s recv crc1 = %x\n",__FUNCTION__,*(rx_buf+len+2));
	printf("%s recv crc2 = %x\n",__FUNCTION__,*(rx_buf+len+3));
#endif

	crccmpval=CrcCompute(rx_buf+2,len,0);	
	//cmd = 0x10/0x20/0x41/0x81
//	if(((*(rx_buf+len+2)||*(rx_buf+len+3))!=0)&&((uint8_t)((crccmpval&0xff00)>>8)==*(rx_buf+len+2))&&((uint8_t)(crccmpval&0x00ff)==*(rx_buf+len+3)))	

	if(((uint8_t)((crccmpval&0xff00)>>8)==*(rx_buf+len+2))&&((uint8_t)(crccmpval&0x00ff)==*(rx_buf+len+3)))	
		ret = 0;
	else	
	{
		printf("%s crc error  !!!!\n",__FUNCTION__);
		ret=-1;
	}

fail1:
	free(temp_tx_buf);
	temp_tx_buf = NULL;
	return ret;
}



//len = 32/6112
 int tg_spi_tx_rx(int fd,uint8_t cmd,uint8_t *tx_buf,int len)
{
	int i;
	long l1,l2;	
	struct timeval tv;
	uint8_t *temp_tx_buf = NULL;
	uint8_t *temp_rx_buf = NULL;
	uint8_t tx_cmd[8] = {0};
	int once_ret;
	int ret;
	int errornum = 0;
	int count = 0;
	uint16_t crccmpval=0;

	temp_tx_buf = (uint8_t *)calloc(len+4,sizeof(uint8_t));
	temp_rx_buf = (uint8_t *)calloc(len+4,sizeof(uint8_t));
	gettimeofday(&tv,NULL);
	l1 = tv.tv_sec*1000*1000+tv.tv_usec;
//cmd   8Bytes	
	tx_cmd[0] = 0x55;
	tx_cmd[1] = cmd;
//buf	len Bytes
	temp_tx_buf[0] = 0x55;
	temp_tx_buf[1] = cmd;


	if(6112 == len)
		memcpy(temp_tx_buf+2,tx_buf,len+2);
	else
	{
		memcpy(temp_tx_buf+2,tx_buf,len);
		crccmpval=CrcCompute(tx_buf,len,0);	
	  	*(temp_tx_buf+2+len)=(uint8_t)((crccmpval&0xff00)>>8);
	  	*(temp_tx_buf+2+len+1)=(uint8_t)(crccmpval&0x00ff);
	}

	do{	
		write(fd,tx_cmd,8);
		once_ret=tg_spi_once(fd,cmd,temp_tx_buf,temp_rx_buf,len);
		count++;
		if(once_ret<0)
			errornum+=1;
		if(count>10)
			break;
		usleep(1000);
	}
	while(once_ret);
	if(errornum == count)
		ret = -1;
	else{
		memcpy(tx_buf,temp_rx_buf+2,len);
		ret = 0;
	}
	gettimeofday(&tv,NULL);
	l2 = tv.tv_sec*1000*1000+tv.tv_usec;
#ifdef TG_SPI_DEBUG
//	printf("time2 = %ld  us\n",l2-l1);
	printf("***************error_num = %d count = %d***************\n",errornum,count);
#endif
	free(temp_rx_buf);
	temp_rx_buf = NULL;
	free(temp_tx_buf);
	temp_tx_buf = NULL;
	return ret;

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


int tg_spi_key_store(int fd,void *src)
{
	int ret = 0;
	uint8_t cmd = 0x08 ;
	int length = 32;
	ret = tg_spi_tx_rx(fd,cmd,(uint8_t *)src,length);
	return ret;
}

int tg_spi_random_num_req(int fd,void *src)
{
	int ret = 0;
	uint8_t cmd = 0x10 ;
	int length = 32;
	ret = tg_spi_tx_rx(fd,cmd,(uint8_t *)src,length);
	return ret;
}


int tg_spi_cert_encrypt(int fd,void *cert)
{
	int ret = 0;
	uint8_t cmd = 0x40 ;
	int length = 6112;
	uint8_t temp[32] = {0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f};
	ret = tg_spi_tx_rx(fd ,0x20,temp,32);
	if(ret < 0)
		return ret;
	ret = tg_spi_tx_rx(fd,cmd,cert,length);
	return ret;
}

int tg_spi_cert_decrypt(int fd,void *cert)
{
	int ret = 0;
	uint8_t cmd = 0x80 ;
	int length = 6112;
	uint8_t temp[32] = {0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f} ;
	ret = tg_spi_tx_rx(fd ,0x40,temp,32);
	if(ret < 0)
		return ret;
	ret = tg_spi_tx_rx(fd,cmd,cert,length);
	return ret;
}




