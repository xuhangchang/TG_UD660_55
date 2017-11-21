#ifndef TG_SPI_H 
#define TG_SPI_H 
#include "tg_main.h"
#include <stdint.h>
// uint16_t CrcCompute(uint8_t *src, uint16_t len, uint16_t crc);

int tg_crc_compute(uint8_t *src, uint16_t len);

int tg_spi_init(int fd);

int tg_spi_once(int fd,int cmd,uint8_t *tx_buf,uint8_t *rx_buf,int len);

int tg_spi_tx_rx(int fd,uint8_t cmd,uint8_t *tx_buf,int len);

void tg_spi_xor(unsigned char * src,int length);

int tg_spi_key_store(int fd,void *src);

int tg_spi_random_num_req(int fd,void *src);

int tg_spi_cert_encrypt(int fd,void *cert);

int tg_spi_cert_decrypt(int fd,void *cert);


#endif  
