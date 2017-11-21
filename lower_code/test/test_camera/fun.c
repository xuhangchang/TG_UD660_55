#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "fun.h"
#include "head.h"
#include "tg_main.h"


/*****************************************************************  
* function:		tg_dir_filenum
* description:  count filenum in a dir
* param1:     	char *dir_path	: dir path (input)
* return:    	filenum (int)  			            -   
* others：
*
*
* date:       	2017/04/14			
* author:     	Hangchang Xu
******************************************************************/ 
int tg_dir_filenum(char *dir_path)
{
	int num = 0;	
	DIR *dirptr = NULL;	
	struct dirent *entry;	
	if((dirptr = opendir(dir_path)) == NULL)  
	{  		
		printf("open dir failed! dir = %s\n",dir_path);  		
		return -1;  	
	} 					
	while (entry = readdir(dirptr))  	
	{ 		
		if(entry->d_type == 8)		//8:file
			{
				//printf("%s\n", entry->d_name);/* print all name in this dir  */ 			
				num ++;		
			}	
	}	
	closedir(dirptr);	
	return num;
}


/*****************************************************************  
* function:		tg_get_one_file
* description:  get one file in a dir
* param1:     	char *dir_path	: dir path (input)
* param2:     	char *file_name	: file_name (output)
* param3:     	unsigned char *data	: file data (output)
* return:    	success flag: 	1:success
*								-1:failed
* others：
*
*
* date:       	2017/04/22			
* author:     	Hangchang Xu
******************************************************************/ 
int tg_get_one_file(char *dir_path,char *file_name,unsigned char *data)
{
	int ret = -1 ;	
	DIR *dirptr = NULL;	
	struct dirent *entry;	
	if((dirptr = opendir(dir_path)) == NULL)  
	{  		
		printf("open dir failed! dir = %s\n",dir_path);  		
		return -1;  	
	} 					
	while (entry = readdir(dirptr))  	
	{ 		
		if(entry->d_type == 8)		//8:file
			{		
				//printf("%s\n", entry->d_name);/* print all name in this dir  */
				sprintf(file_name,"%s/%s",dir_path,entry->d_name);
				ret  = 1;
				break;
			}	
	}	
	closedir(dirptr);	
	return ret;
}



void display_tgpackage(TG_package* pack)
{
	printf("--------------------------------------------------------\n");
	printf("|------------------------PACKAGE------------------------\n");
	printf("|device_name = %.32s\n",pack->device_name);
	printf("|id = %d\n",pack->id);
	printf("|cmd1 = %x\n",pack->cmd1);
	printf("|cmd2 = %d\n",pack->cmd2);
	printf("|length = %d\n",pack->length);
	printf("|cert_type = %d\n",pack->cert_type);	
	printf("|sup_mgr_num = %d\n",pack->sup_mgr_num);
	printf("|mgr_num = %d\n",pack->mgr_num);
	printf("|usr_num = %d\n",pack->usr_num);	
	printf("|serial_num = %.32s\n",pack->serial_num);
	printf("|ip_addr = %.32s\n",pack->ip_addr);
	printf("|mac_addr = %.32s\n",pack->mac_addr);
	printf("|md5 = %.32s\n",pack->md5);
	printf("|random_num = %.32s\n",pack->random_num);
	printf("|cert_name = %.32s\n",pack->cert_name);
	printf("--------------------------------------------------------\n");

}

void display_cert(TG_cert* pack)
{
	printf("--------------------------------------------------------\n");
	printf("|---------------------CERTIFICATION---------------------\n");
	printf("|cert user_name = %.32s\n",pack->user_name);//用户名
	printf("|cert user_id = %d\n",pack->user_id);
	printf("|cert cert_type = %d\n",pack->cert_type);//用户类型
	printf("|cert cert_valid_date = %.32s\n",pack->cert_valid_date);
	printf("|cert crc1 = %x\n",pack->crc[0]);
	printf("|cert crc2 = %x\n",pack->crc[1]);
	printf("--------------------------------------------------------\n");
}


/*****************************************************************  
* function:		resolve_config
* description:  resolve h3 config file /etc/config
* param1:     	char *path	: config file path (input)
* param2:     	char *ser_num	: serial num (output)
* param3:     	char *ip_addr	: ip addr (output)
* param3:     	char *mac_addr	: mac addr (output)
* return:    	-1:error 			            -   
* others：
*
* date:       	2017/04/18			
* author:     	Hangchang Xu
******************************************************************/ 

int resolve_config(char *path,char *ser_num,char *ip_addr,char *mac_addr)
{
	char *p;
	char n;	
	struct stat statbuf;
	char buf[256] = {0};

	stat(path,&statbuf);
	read_data_hex(buf,statbuf.st_size,path);
	 		  
	p=strstr(buf,"ID ");		  
	if(p!=NULL){			  
		n=0;p+=3; 		  		  
		memcpy(ser_num,p,20);	
		printf("serial_num = %.20s\n",ser_num);
	}
	else 
		return -1;	

	p=strstr(buf,"IP ");		  
	if(p!=NULL){			  
		n=0;p+=3; 
		while(*(p+n)!='\n'&&n<32) 
			n++;	
		printf("n = %d\n",n);
		memcpy(ip_addr,p,n);			
	}
	else 
		return -2;	

	p=strstr(buf,"MAC ");		  
	if(p!=NULL){			  
		n=0;p+=4; 		  		  
		memcpy(mac_addr,p,17);	
	}
	else 
		return -3;	

	return 1;
}

/*
uint16_t CrcCompute(uint8_t *src, uint16_t len, uint16_t crc)
{
//	#define CRC_DFE_POLY    0x8005
	uint8_t uc;
	for (uint16_t j = 0; j < len; j++)
	{
		uc = *(src + j);
		for(uint16_t i = 0; i < 8; i++)
		{
			crc = ((uc ^ (uint8_t)(crc >> 8)) & 0x80) ? ((crc << 1) ^ 0x8005) : (crc << 1);
			uc <<= 1;
		}
	}
	return crc;
}
*/


int test_crc(uint8_t *src,uint16_t dat_len)
{
	uint16_t crc;
	uint8_t crc1;
	uint8_t crc2;
//	crc = CrcCompute(src, dat_len, 0);
	crc1 = (crc >> 8)  & 0xff;
	crc2 = crc  & 0xff;
	
#ifdef TG_DEBUG
	printf("test_crc crc1 = %x\n",crc1);
	printf("test_crc crc2 = %x\n",crc2);
	printf("test_crc *(src+dat_len) = %x\n",*(src+dat_len));
	printf("test_crc *(src+dat_len+1) = %x\n",*(src+dat_len+1));
#endif	
	if((*(src+dat_len) == crc1 )&&(*(src+dat_len+1) == crc2 ))
		return 1;
	else 
		return 0;
}

void make_crc(uint8_t *src,uint16_t dat_len)
{
	uint16_t crc;
	uint8_t crc1;
	uint8_t crc2;
//	crc = CrcCompute(src, dat_len, 0);
	crc1 = (crc >> 8)  & 0xff;
	crc2 = crc  & 0xff;

	*(src+dat_len) = crc1;
	*(src+dat_len+1) = crc2;
	
#ifdef TG_DEBUG
	printf("make_crc *(src+dat_len) = %x\n",*(src+dat_len));
	printf("make_crc *(src+dat_len+1) = %x\n",*(src+dat_len+1));
#endif	
}

/*****************************************************************  
*
*						.dat operation
*
******************************************************************/ 


/*****************************************************************  
* function:		dat_get_local_num
* description:  get the file num in a dir
* param1:     	char* addr	: path of dir (input)
* return:    	-1		: open dir failed 
*				else	: file num            -   
* others:
* date:       	2017/03/03			
* author:     	Hangchang Xu
******************************************************************/ 
int dat_get_local_num(char* addr)
{
	int num = 0;
	DIR *dirptr = NULL;
	struct dirent *entry;
	if((dirptr = opendir(addr)) == NULL)  
	{  
		printf("open dir failed!\n");  
		return -1;  
	} 				
	while (entry = readdir(dirptr))  
	{ 
		if(entry->d_type == 8)
		{
//			printf("%s\n", entry->d_name);/* print all name in this dir  */ 
			num ++;
		}
	}
	closedir(dirptr);
	return num;
}

/*****************************************************************
* function:		dat_get_usr_dat
* description:  get the all file dat in a dir
* param1:     	char* addr	: path of dir	(input)
* param2:     	stu_usr *usr_addr	: all dat path	(output)
* param3:     	unsigned char *dat	: pointer of all dat	(output)
*
* return:    	1	: succeed 
* 				-1	: open dir failed            -   
* others:		one file  = TZD_LENGTH*3 Bytes
* date:       	2017/03/03			
* author:     	Hangchang Xu
******************************************************************/ 
int dat_get_usr_dat(char* addr,stu_usr *usr_addr,unsigned char *dat)
{
	int i = 0;
	int sizeFeature3 = TZD_LENGTH*3;
	char str_addr[50] = {0};
	DIR *dirptr = NULL;
	struct dirent *entry;
	unsigned char * tempData;
	tempData = (unsigned char *)malloc(sizeFeature3*sizeof(unsigned char));
	
	if((dirptr = opendir(addr)) == NULL)  
	{  
		printf("open dir failed!\n");  
		return -1;  
	} 				
	
	while (entry = readdir(dirptr))  
	{ 
		if(entry->d_type == 8)
		{
			printf("%s\n", entry->d_name);/* print all name in this dir  */ 
			memset(str_addr,0,50*sizeof(char));
			sprintf(str_addr,"%s/%s",addr,entry->d_name);

			printf("addr = %s,i = %d\n", str_addr,i);/* print all address*/ 			
//			memset(tempData,0,sizeFeature3*sizeof(char));
			read_data_hex(tempData,sizeFeature3*sizeof(char),str_addr);
			memcpy(dat + i*sizeFeature3,tempData,sizeFeature3);

			memset((usr_addr+i)->addr,0,50*sizeof(char));
			strcpy((usr_addr+i)->addr,str_addr+7);
			(usr_addr+i)->num = i;
			i++;
		}
	}
	closedir(dirptr);	
	free(tempData);
	tempData = NULL;
	return 1;  
}


/*****************************************************************
* function:		dat_get_local_cert
* description:  get the all certs in a dir
* param1:     	int fd	: spi fd	(input)
* param2:     	char* addr	: path of dir	(input)
* param3:     	stu_usr *usr_addr	: all cert path	(output)
* param4:     	TG_cert *cert: pointer of all cert	(output)
*
* return:    	1	: succeed 
* 				-1	: open dir failed            -   
* others:		one cert size = 6116 Bytes
* date:       	2017/07/11		
* author:     	Hangchang Xu
******************************************************************/ 
int dat_get_local_cert(int fd,char* addr,stu_usr *usr_addr,TG_cert * cert)
{
	int i = 0;
	int sizeCert = sizeof(TG_cert);
	int ret_spi;
	char str_addr[50] = {0};
	DIR *dirptr = NULL;
	struct dirent *entry;
	TG_cert * tempCert;
	tempCert = (TG_cert *)malloc(sizeof(TG_cert));
	
	if((dirptr = opendir(addr)) == NULL)  
	{  
		printf("open dir failed!\n");  
		return -1;  
	} 				
	
	while (entry = readdir(dirptr))  
	{ 
		if(entry->d_type == 8)
		{
//			printf("%s\n", entry->d_name);/* print all name in this dir  */ 
			memset(str_addr,0,50*sizeof(char));
			sprintf(str_addr,"%s/%s",addr,entry->d_name);
			printf("cert addr = %s,i = %d\n", str_addr,i);/* print all address*/ 			
//			memset(tempData,0,sizeFeature3*sizeof(char));
			read_data_hex(tempCert,sizeof(TG_cert),str_addr);
#ifdef TG_SPI
			ret_spi = tg_spi_cert_decrypt(fd,tempCert);
			if (ret_spi < 0)
				printf("tg_spi_cert_decrypt  failed\n");
#endif
			memcpy(cert + i,tempCert,sizeof(TG_cert));
			memset((usr_addr+i)->addr,0,50*sizeof(char));
			strcpy((usr_addr+i)->addr,str_addr);
			(usr_addr+i)->num = i;
			i++;
		}
	}
	closedir(dirptr);	
	free(tempCert);
	tempCert = NULL;
	return 1;  
}


/*****************************************************************
* function:		encryptDecryptAll
* description:  encrypt/decrypt n data
* param1:     	unsigned char *data	: pointer of data	(input&output)
* param2:     	int flag	: 1: encrypt ; 2: decrypt ;	(input)
* param3:     	int user_num: user num	(input)
*
* return:    	void       -   
* others:		
* date:       	2017/05/03			
* author:     	Hangchang Xu
******************************************************************/ 
void encryptDecryptAll(unsigned char *data ,int flag,int user_num)
{
	int i = 0;
	int sizeFeature1 = TZD_LENGTH;
	int sizeFeature3 = TZD_LENGTH*3;

	for(i = 0;i<user_num;i++)
	{
//		encryptDecrypt(data + i * sizeFeature3, flag, sizeFeature1 - 16);
	}
}

int compare_data(unsigned char *str1,unsigned char *str2,int len)
{
	int i;
	int ret = 0;
	for(i = 0;i < len;i++)
	{
		if(*(str1 + i) != *(str2 + i))
		{
			ret = -len;
			break;
		}
	}
	return ret;
}


/*****************************************************************
* function:		make_localcert_to_dat
* description:  malloc local_cert/local_data/usr_addr
* param1:     	int fd	: spi fd;	(input)
* param2:     	unsigned char *local_data	: all local chara;	(output)
* param3:     	stu_usr *usr_addr: local chara list	(output)
* param4:     	int *num: user num	(output)
*
* return:    	1          
* others:		
* date:       	2017/07/11			
* author:     	Hangchang Xu
******************************************************************/ 
int make_localcert_to_dat(int fd,unsigned char **local_data,stu_usr **usr_addr,int *num)
{
	int i;
	int sizeFeature3 = TZD_LENGTH * 3;
	int sup_num;
	int mgr_num;
	int user_num;
	int ret_spi;
	TG_cert *local_cert;
	TG_cert *temp_cert;
	*num = 0;
	
	sup_num = tg_dir_filenum(SUP_MGR_CERT_PATH);
	mgr_num = tg_dir_filenum(MGR_CERT_PATH);
	user_num = tg_dir_filenum(USER_CERT_PATH);
	*num = sup_num + mgr_num + user_num;

#ifdef REG_DEBUG
	printf("sup_num = %d\n",sup_num);
	printf("mgr_num = %d\n",mgr_num);
	printf("user_num = %d\n",user_num);
#endif	

	local_cert = (TG_cert *)malloc(*num *sizeof(TG_cert));
	temp_cert = local_cert;
	
	*local_data = (unsigned char *)malloc(*num * sizeFeature3 *sizeof(unsigned char));
	*usr_addr = (stu_usr *)malloc(*num * sizeof(stu_usr));

	dat_get_local_cert(fd,SUP_MGR_CERT_PATH,*usr_addr,local_cert);
	dat_get_local_cert(fd,MGR_CERT_PATH,*usr_addr+sup_num,local_cert+sup_num);
	dat_get_local_cert(fd,USER_CERT_PATH,*usr_addr+sup_num+mgr_num,local_cert+sup_num+mgr_num);

	for(i = 0;i<*num;i++)
	{
		memcpy(*local_data+i*sizeFeature3,temp_cert->chara,sizeFeature3);
		temp_cert = temp_cert + 1;
	}

	free(local_cert);
	local_cert = NULL;
	return 1;

}

void free_localcert_to_dat(unsigned char **local_data,stu_usr **usr_addr)
{
	free(*local_data);
	*local_data = NULL;

	free(*usr_addr);
	*usr_addr = NULL;
}


/*****************************************************************
* function:		uppercert_to_dat
* description:  uppercert to dat
* param1:     	int fd	:	spi fd;	(input)
* param2:     	stu_upper_data *upper_cert	: all upper stu;	(input)
* param3:     	unsigned char *upper_data	: all local chara;	(output)
* param4:     	int num: user num	(input)
*
* return:    	1          
* others:		
* date:       	2017/07/11			
* author:     	Hangchang Xu
******************************************************************/ 
int tg_uppercert_to_dat(int fd,stu_upper_data *upper_cert,unsigned char *upper_data,int num)
{
	int i;
	int sizeFeature3 = TZD_LENGTH * 3;

	for(i = 0;i<num;i++)
	{
		/********************证书FPGA解密************************/
#ifdef TG_SPI
		tg_spi_cert_decrypt(fd,&(upper_cert+i)->cert_data);
		if (ret_spi < 0)
			printf("tg_spi_cert_decrypt  failed\n");
#endif
		upper_cert->cert_type = upper_cert->cert_data.cert_type;
		upper_cert->cert_id = upper_cert->cert_data.user_id;
		memcpy(upper_data+i*sizeFeature3,&(upper_cert+i)->cert_data,sizeFeature3);
	}
	return 1;
}



int tg_path_to_info(char *path,char *name,int *id,int *type)
{
	char *temp;
	char *str_num;
	int i = 0,j;
	
	if(0 == strncmp("/etc/tg/cert/super/",path,19))
	{
		*type = 0;
		j = 19;
	}
	else if(0 == strncmp("/etc/tg/cert/mgr/",path,17))
	{
		*type = 1;
		j = 17;
	}
	else if(0 == strncmp("/etc/tg/cert/user/",path,18))
	{
		*type = 2;
		j = 18;
	}
	else 
		return -1; 

	temp = path +j;
	while('_' != *temp)
	{
		if('\0' == *temp)
			return -2;
		*(name+i) = *temp++;
		i++;
	}
	*(name+i) = '\0';
	printf("name = %s\n",name);
	str_num = strstr(path,"_");
	str_num ++;
	printf("str_num = %s\n",str_num);

	*id = atoi(str_num);
	printf("num = %d\n",*id);

	return 1;
}


void write_data_hex(unsigned char * my_array,int length,char *string)
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
        fwrite(&my_array[i],sizeof(unsigned char ),1,fp);
        i++;
    }
    fclose(fp);
}



void read_data_hex(unsigned char *buf,int length,char *string)
{
  int i = 0;
	int re;
	FILE *fp;
	fp = fopen(string,"rb");
	if(NULL == fp)
	{
	    printf("file open Fail!\n");
	}
	fread(buf,sizeof(unsigned char),length,fp);
  fclose(fp);
}





