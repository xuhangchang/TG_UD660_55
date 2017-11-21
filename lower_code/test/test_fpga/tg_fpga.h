#ifndef TG_FPGA_H 
#define TG_FPGA_H 

#include <stdint.h>
uint16_t CrcCompute(uint8_t *src, uint16_t len, uint16_t crc);

int tg_crc_compute(uint8_t *src, uint16_t len);

int tg_spi_init(int fd);

int tg_spi_once_32(int fd,uint8_t cmd,uint8_t *tx_buf,uint8_t *rx_buf);

int tg_spi_once_6k(int fd,int cmd,uint8_t *tx_buf,uint8_t *rx_buf);

int tg_spi_tx_rx(int fd,uint8_t cmd,uint8_t *tx_buf,uint8_t *rx_buf,int len);

void write_data_hex(unsigned char * array,int length,char* file);

void read_data_hex(unsigned char * array,int length,char* file);

void tg_spi_xor(unsigned char * src,int length);

int tg_spi_key_req_and_send(int fd,uint8_t *tx_buf,uint8_t *rx_buf);

int tg_spi_key_store(int fd,uint8_t *tx_buf,uint8_t *rx_buf);

int tg_spi_random_num_req(int fd,uint8_t *tx_buf,uint8_t *rx_buf);

int tg_spi_key_req(int fd);

int tg_spi_cert_encrypt(int fd,uint8_t *tx_buf,uint8_t *rx_buf);

int tg_spi_cert_decrypt(int fd,uint8_t *tx_buf,uint8_t *rx_buf);


#endif  
