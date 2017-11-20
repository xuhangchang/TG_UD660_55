#ifndef FUN_H 
#define FUN_H 
#include "rtx.h"
#include <stdint.h>

typedef struct{
	int num;
	char addr[256];
	int cert_type;
}stu_usr;

typedef struct{
	char cert_name[32];
	TG_cert cert_data;
	int  cert_type;
	int  cert_id;
}stu_upper_data;

void trans_encrypt(unsigned char * data,int length);

unsigned char **Make2DArray_uint8(int row,int col);

void Free2DArray_uint8(unsigned char **a,int row);

void read_data_hex(unsigned char *buf,int length,char *string);

void write_data_hex(unsigned char * my_array,int length,char *string);

int tg_get_file_data(char *file_path,char *data);

int tg_dir_filenum(char *dir_path);

int tg_get_one_file(char *dir_path,char *file_name,unsigned char *data);

void display_tgpackage(TG_package* pack);

void display_cert(TG_cert* pack);

int resolve_config(char *path,char *ser_num,char *ip_addr,char *mac_addr);
/*
uint16_t CrcCompute(uint8_t *src, uint16_t len, uint16_t crc);ddd
*/

int test_crc(uint8_t *src,uint16_t dat_len);

void make_crc(uint8_t *src,uint16_t dat_len);

/********************************  dat  ********************************/
int dat_get_local_num(char* addr);

int dat_get_usr_dat(char* addr,stu_usr *usr_addr,unsigned char *dat);

int dat_get_local_cert(int fd,char* addr,stu_usr *usr_addr,TG_cert * cert);

int compare_data(unsigned char *str1,unsigned char *str2,int len);

int make_localcert_to_dat(int fd,unsigned char **local_data,stu_usr **usr_addr,int *num);

int test_make_localcert_to_dat(int fd,unsigned char **local_data,stu_usr **usr_addr,int *num);

void free_localcert_to_dat(unsigned char **local_data,stu_usr **usr_addr);

int tg_uppercert_to_dat(int fd,stu_upper_data *upper_cert,unsigned char *upper_data,int num);

int tg_path_to_info(char *path,char *name,int *id,int *type);

int tg_delete_file(char *path);


#endif  
