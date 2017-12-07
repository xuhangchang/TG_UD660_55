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
#include "tg_spi.h"
#include "tg_main.h"

/*****************************************************************  
* function:		spi_CrcCompute
* description:  测试数据串末尾的两字节的CRC是否正确
* param1:     	uint8_t *src	: 数据 (input)
* param2:     	uint16_t dat_len: crc之前的长度 (output)
* param3:     	uint16_t crc	: useless

* return:    	0 : 失败
				1 : 成功			            -   
* other:
*
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
uint16_t spi_CrcCompute(uint8_t *src, uint16_t len, uint16_t crc)
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

/*****************************************************************  
* function:		tg_crc_compute
* description:  在数据串末尾添加两字节的CRC
* param1:     	uint8_t *src : 数据 (input)
* param2:     	uint16_t len : crc之前的长度 (input)
* return:    	void	            -   
* other:
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_crc_compute(uint8_t *src, uint16_t len)
{
	uint16_t crccmpval=0;
	crccmpval = spi_CrcCompute(src, len, 0);
	*(src+len)=(uint8_t)((crccmpval&0xff00)>>8);
	*(src+len+1)=(uint8_t)(crccmpval&0x00ff);
	return 0;
}

/*****************************************************************  
* function:		tg_spi_init
* description:  spi初始化
* param1:     	int fd	: spi设备的文件描述符 (input)
* return:    	void 			            -   
* other: 		设置spi模式，频率，以及z32的初始化
*		
*
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_spi_init(int fd)
{
	int ret = 0;
	int x; 
	uint8_t mode = 0;
	uint8_t bits = 8;
	uint32_t speed = SPI_SPEED;//max =10MHz,10000008=wrong 
	uint8_t temp[200];
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
	int num = 100;
	memset(temp,0x11,num);

	for(i = 0 ;i<num;i++)
	{
		temp[i] = i+1;
	}
	temp[0] = 0x55;
	temp[1] = 0x04;


/***************z32 init********************/
	write(fd,temp,100);	

	do{ 
		read(fd,temp,1);
	}
	while(temp[0] == 0x33);//z32

	read(fd,temp+1,99);

	printf("z32 init end\n");	
/******************************************/
	return 1;
}


/*****************************************************************  
* function:		tg_spi_once_32
* description:  往SPI发送32+2字节数据
* param1:     	int fd			: spi设备的文件描述符 (input)
* param2:     	uint8_t cmd		: 发送给spi的指令 (input)
* param3:     	uint8_t *tx_buf	: 发送的数据，32字节，后面两个字节CRC自动生成 (input)
* param4:     	uint8_t *rx_buf	: 接收的数据，32字节，后面两个字节CRC自动生成 (output)
* return:    	void 			            -   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_spi_once_32(int fd,uint8_t cmd,uint8_t *tx_buf,uint8_t *rx_buf)
{
	int i;
	int ret = -1; 
	uint16_t crccmpval=0;
	uint8_t idel_flag = 0x33;
	uint8_t send_buf[36] = {0};
	uint8_t recv_buf[36] = {0};
	idel_flag = 0x33;
	send_buf[0] = 0x55;
	send_buf[1] = cmd;
	memcpy(send_buf+2,tx_buf,32);

	crccmpval=spi_CrcCompute(tx_buf,32,0);	
	send_buf[34] = (uint8_t)((crccmpval&0xff00)>>8);
	send_buf[35] = (uint8_t)(crccmpval&0x00ff);

#ifdef TG_SPI_DEBUG	
	for(i = 0;i<36;i++)
	{
		if(0 == i%6)
			printf("\n");
		printf("send %d = %x\t",i,send_buf[i]);
	}
	printf("\n-----------------------------\n");
#endif
	//WRITE DATA
	write(fd,send_buf,36);
	//READ DATA      
	usleep(10);
	do
	{  
		read(fd,recv_buf,1);
	}
	while(recv_buf[0] == idel_flag);

	read(fd,recv_buf+1,35);
	
	memcpy(rx_buf,recv_buf+2,32);

	//PRINT READ DATE
#ifdef TG_SPI_DEBUG	
	printf("tg_spi_once_32 recv cmd = %x\n",*(recv_buf+1));
	printf("%s recv crc1 = %x\n",__FUNCTION__,recv_buf[34]);
	printf("%s recv crc2 = %x\n",__FUNCTION__,recv_buf[35]);
	for(i = 0;i<36;i++)
	{
		if(0 == i%6)
			printf("\n");
		printf("recv %d = %x\t",i,recv_buf[i]);
	}
	printf("\n-----------------------------\n");
#endif

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
			crccmpval=spi_CrcCompute(recv_buf+2,32,0);	
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


//tx_buf 3¤??148 = 6144+2+2
//rx_buf 3¤??148
/*****************************************************************  
* function:		tg_spi_once_6k
* description:  往SPI发送6148字节数据
* param1:     	int fd			: spi设备的文件描述符 (input)
* param2:     	uint8_t cmd		: 发送给spi的指令 (input)
* param3:     	uint8_t *tx_buf	: 发送的数据，6148字节 (input)
* param4:     	uint8_t *rx_buf	: 接收的数据，6148字节 (input)
* return:    	void 			            -   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_spi_once_6k(int fd,int cmd,uint8_t *tx_buf,uint8_t *rx_buf)
{
	int ret = -1;
	int i;
	uint16_t crccmpval=0;
	uint8_t idel_flag = 0x33;
	uint8_t send_buf[6148] = {0};
	uint8_t recv_buf[6148] = {0};

	send_buf[0] = 0x55;
	send_buf[1] = cmd;
	memcpy(send_buf+2,tx_buf,6146);
	tg_spi_xor(send_buf+2,6144);
	//WRITE DATA
	write(fd,send_buf,6148);
	usleep(10);
	//READ DATA 
	do{  
		read(fd,recv_buf,1);
	}
#ifdef	TG_SPI_Z32	
	while(recv_buf[0] == idel_flag);
#endif	
#ifdef	TG_SPI_FPGA
	while(recv_buf[0] != 0x55);
#endif	

	read(fd,recv_buf+1,6147);
	tg_spi_xor(recv_buf+2,6144);
	memcpy(rx_buf,recv_buf+2,6146);
//	write_data_hex(recv_buf,6148,"encrypt_recv.dat");
#ifdef TG_SPI_DEBUG
	printf("tg_spi_once_6k recv cmd = %x\n",*(recv_buf+1));
#endif

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
			crccmpval=spi_CrcCompute(recv_buf+2,6144,0); 
#ifdef TG_SPI_DEBUG
			printf("crccmpval = %x\n",crccmpval);
#endif
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
#ifdef TG_SPI_DEBUG
	printf("%s recv crc1 = %x\n",__FUNCTION__,recv_buf[6146]);
	printf("%s recv crc2 = %x\n",__FUNCTION__,recv_buf[6147]);
#endif
	return ret;
}


//tx_buf len = 32/6144
/*****************************************************************  
* function:		tg_spi_tx_rx
* description:  往SPI发送数据
* param1:     	int fd			: spi设备的文件描述符 (input)
* param2:     	uint8_t cmd		: 发送给spi的指令 (input)
* param3:     	uint8_t *tx_buf	: 发送的数据 (input)
* param4:     	uint8_t *rx_buf	: 接收的数据 (output)
* param5:     	int len			: 发送的数据长度，32/6144 (input)
* return:    	void 			            -   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
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
#ifdef	TG_SPI_Z32
	if(0x20 == cmd)
		count_max = 0;
#endif

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
		write(fd,tx_cmd,8);
		do{  
			read(fd,rx_cmd,1);
		}
		while(rx_cmd[0] == 0x33);	
		read(fd,rx_cmd+1,7);

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
	
#ifdef TG_SPI_DEBUG
//	printf("time2 = %ld  us\n",l2-l1);
	printf("***************error_num = %d count = %d***************\n",errornum,count);
#endif
	return ret;
}

/*****************************************************************  
* function:		tg_spi_xor
* description:  FPGA加密证书
* param1:     	unsigned char * src	: 证书数据 (input)
* param2:     	int length			: 证书长度 (input)
* return:    	void 			            -   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
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


/*****************************************************************  
* function:		tg_spi_key_store
* description:  密钥存储
* param1:     	int fd			: spi设备的文件描述符 (input)
* param2:     	uint8_t *tx_buf	: 需要存储的密钥 (input)
* return:    	0 :成功
*				其他:失败   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_spi_key_store(int fd,uint8_t *tx_buf)
{
	int ret = 0;
	uint8_t cmd = 0x08 ;
	int length = 32;
	uint8_t rx_buf[32] = {0};
	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	usleep(1000);
	if(!ret)
		memcpy(tx_buf,rx_buf,32);
	return ret;
}

/*****************************************************************  
* function:		tg_spi_random_num_req
* description:  随机数请求
* param1:     	int fd			: spi设备的文件描述符 (input)
* param2:     	uint8_t *tx_buf	: 获得的随机数 (input)
* return:    	0 :成功
*				其他:失败		            -   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_spi_random_num_req(int fd,uint8_t *tx_buf)
{
	int ret = 0;
	uint8_t cmd = 0x10 ;
	int length = 32;
	uint8_t rx_buf[32] = {0};
	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	usleep(1000);
	if(!ret)
		memcpy(tx_buf,rx_buf,32);
	return ret;
}

/*****************************************************************  
* function:		tg_spi_key_req
* description:  随机数请求
* param1:     	int fd			: spi设备的文件描述符 (input)
* return:    	0 :成功
*				其他:失败			            -   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_spi_key_req(int fd)
{
	int ret = 0;
	uint8_t cmd = 0x20 ;//key req
	int length = 32;
	uint8_t tx_buf[36] = {0};
	uint8_t rx_buf[36] = {0};
	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	usleep(1000);
	if(ret)
		return ret;
	return ret;
}


/*****************************************************************  
* function:		tg_spi_cert_encrypt
* description:  证书加密
* param1:     	int fd			: spi设备的文件描述符 (input)
* param2:		uint8_t *tx_buf : 加密的证书(input/output)
* return:    	0 :成功
*				其他:失败			            -   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_spi_cert_encrypt(int fd,uint8_t *tx_buf)
{
	int ret = 0;
	uint8_t cmd = 0x40 ;
	int length = 6148;
	uint8_t rx_buf[6148] = {0};

	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	usleep(1000);
	if(!ret)
		memcpy(tx_buf,rx_buf,6146);
	else
		memset(tx_buf,0x11,6146);
	return ret;
}

/*****************************************************************  
* function:		tg_spi_cert_decrypt
* description:  证书加密
* param1:     	int fd			: spi设备的文件描述符 (input)
* param2:		uint8_t *tx_buf : 解密的证书(input/output)
* return:    	0 :成功
*				其他:失败			            -   
* other: 		
*		
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_spi_cert_decrypt(int fd,uint8_t *tx_buf)
{
	int ret = 0;
	uint8_t cmd = 0x80 ;
	int length = 6148;
	uint8_t rx_buf[6148] = {0};

	ret = tg_spi_tx_rx(fd,cmd,tx_buf,rx_buf,length);
	usleep(1000);
	if(!ret)
		memcpy(tx_buf,rx_buf,6146);
	else
		memset(tx_buf,0x11,6146);
	return ret;
}


