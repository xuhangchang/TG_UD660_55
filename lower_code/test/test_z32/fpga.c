/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */
//0907
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

int main()
{
	int count = 0;
	int temp; 
	uint16_t crccmpval;
	int spi_fd = -1;
	uint8_t tx_buf_6k[6148] = {0},rx_buf_6k[6148] = {0}; 
	uint8_t tx_buf_32[32] = {0},rx_buf_32[32] = {0}; 
	uint16_t i,len;
	long l1,l2;
	struct timeval tv;
	spi_fd = open("/dev/spidev0.0", O_RDWR);
	
	if (spi_fd < 0)
		perror("can't open device");

	tg_spi_init(spi_fd);

	printf("tg_spi_init end\n");

/**************************************/	
	usleep(1000);

	len = 32;
	for(i=0;i<len;i++)
		tx_buf_32[i] = i + 1;

	tx_buf_32[9] = 0x66 ;

	tx_buf_32[10] = 0x88 ;
	tx_buf_32[11] = 0x99 ;

	usleep(1000);


	tg_spi_key_req(spi_fd);


	gettimeofday(&tv,NULL);
	l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
//	tg_spi_key_store(spi_fd,tx_buf_32,rx_buf_32);
	gettimeofday(&tv,NULL);
	l2 = tv.tv_sec*1000*1000 + tv.tv_usec;
	printf("tg_spi_key_store  time = %ld ms  \n",(l2-l1)/1000);
	printf("key store end .................\n");
//	scanf("%d",&temp);
	memset(tx_buf_32,0,32);
	gettimeofday(&tv,NULL);


	
	l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
	tg_spi_key_req(spi_fd);
	printf("key req end .................\n");
	gettimeofday(&tv,NULL);
	l2 = tv.tv_sec*1000*1000 + tv.tv_usec;
	printf("key req  time = %ld ms  \n",(l2-l1)/1000);


	gettimeofday(&tv,NULL);
	l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
	tg_spi_random_num_req(spi_fd,tx_buf_32,rx_buf_32);
	gettimeofday(&tv,NULL);
	l2 = tv.tv_sec*1000*1000 + tv.tv_usec;
	printf("tg_spi_random_num_req  time = %ld ms  \n",(l2-l1)/1000);
	printf("tg_spi_random_num_req end .................\n");


//	scanf("%d",&temp);
//每次上电发一次，每次密钥存储完发一次

	read_data_hex(tx_buf_6k,6148,"cert.dat");
	scanf("%d",&temp);
	i = 100;

	while(i--)
	{

		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;

		tg_spi_cert_encrypt(spi_fd,tx_buf_6k,rx_buf_6k);

		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000+tv.tv_usec;
		printf("encrypt time = %ld ms  \n",(l2-l1)/1000);
		printf("encrypt............\n");
		
	}
	write_data_hex(tx_buf_6k,6146,"encrypt_send.dat");
	write_data_hex(rx_buf_6k,6146,"encrypt_recv.dat");


	memcpy(tx_buf_6k,rx_buf_6k,6146);
	
	i = 10;
	while(i--)
	{

		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;

		tg_spi_cert_decrypt(spi_fd,tx_buf_6k,rx_buf_6k);

		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000+tv.tv_usec;
		printf("decrypt time = %ld ms  \n",(l2-l1)/1000);
		printf("decrypt............\n");
	}
	write_data_hex(tx_buf_6k,6146,"decrypt_send.dat");
	write_data_hex(rx_buf_6k,6146,"decrypt_recv.dat");
	scanf("%d",&temp);

//	tg_spi_cert_decrypt(spi_fd,rx_buf_6k,tx_buf_6k);
	printf("decrypt............\n");


	printf("END............\n");
	return 1;
	
}
