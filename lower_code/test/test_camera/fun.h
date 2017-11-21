#ifndef FUN_H 
#define FUN_H 
#include "rtx.h"
#include <stdint.h>

typedef struct{
	int num;
	char addr[50];
	int cert_type;
}stu_usr;

typedef struct{
	char cert_name[32];
	TG_cert cert_data;
	int  cert_type;
	int  cert_id;
}stu_upper_data;

void write_data_hex(unsigned char * my_array,int length,char *string);

void read_data_hex(unsigned char *buf,int length,char *string);

int tg_dir_filenum(char *dir_path);

int tg_get_one_file(char *dir_path,char *file_name,unsigned char *data);

void display_tgpackage(TG_package* pack);

void display_cert(TG_cert* pack);

int resolve_config(char *path,char *ser_num,char *ip_addr,char *mac_addr);

uint16_t CrcCompute(uint8_t *src, uint16_t len, uint16_t crc);


int test_crc(uint8_t *src,uint16_t dat_len);

void make_crc(uint8_t *src,uint16_t dat_len);

/********************************  dat  ********************************/
int dat_get_local_num(char* addr);

int dat_get_usr_dat(char* addr,stu_usr *usr_addr,unsigned char *dat);

int dat_get_local_cert(int fd,char* addr,stu_usr *usr_addr,TG_cert * cert);

void encryptDecryptAll(unsigned char *data ,int flag,int user_num);

int compare_data(unsigned char *str1,unsigned char *str2,int len);

int make_localcert_to_dat(int fd,unsigned char **local_data,stu_usr **usr_addr,int *num);

void free_localcert_to_dat(unsigned char **local_data,stu_usr **usr_addr);

int tg_uppercert_to_dat(int fd,stu_upper_data *upper_cert,unsigned char *upper_data,int num);

int tg_path_to_info(char *path,char *name,int *id,int *type);



#endif  
