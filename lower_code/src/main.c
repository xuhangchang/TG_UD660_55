//	1121 net  aa
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <semaphore.h>  
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "fun.h"
#include "gpio.h"
#include "rtx.h"
#include "tg_camera_new.h"
#include "tg_main.h"
#include "tg_spi.h"
#include "Bmp_LoadSave.h"
#include "head.h"

unsigned char camera_data[CAMERA_WIDTH*CAMERA_HEIGHT] = {0}; 	//capture camera data
int camera_flag = 0; 	//1:capture camera data ;	0:none

int tg_light = LIGHT_INIT; 	// PWM 100% is 255

int first_cut_row = CUT_ROW_START;
int first_cut_col = CUT_COL_START;

int upper_cert_num;
char ran_num[32] = {0};


pthread_mutex_t mutex_package; 	//package mutex
pthread_mutex_t mutex_spi; 	//spi mutex


int fd_com; //ttyGS0 fd
int fd_net = -1;
int fd_spi = -1;; //spidev0.0 fd
int fd_gpio;

void *tgthread_camera_data(void *arg);
void *tgthread_heart_beat(void *arg);
void *tgthread_register(void * arg);
void *tgthread_local_compare(void *arg);
void *tgthread_upper_compare(void *arg);
void *tgthread_test_register(void *arg);
void *tgthread_test_compare(void *arg);


char recv_buf[BUFSIZE],send_buf[BUFSIZE];

pthread_t tgthread_camera_data_tid; 		//HeartBeat pthread tid
pthread_t tgthread_heart_beat_tid;		//HeartBeat pthread tid

pthread_t tgthread_register_tid;		//Register pthread tid
pthread_t tgthread_local_compare_tid; 	//Local Compare pthread tid
pthread_t tgthread_upper_compare_tid; 	//Upper Compare pthread tid
pthread_t tgthread_test_register_tid; 	//test register pthread tid
pthread_t tgthread_test_compare_tid; 	//test compare pthread tid

int pthread_reg_flag = 0;
int pthread_loc_flag = 0;
int pthread_up_flag = 0;
int pthread_test_reg_flag = 0;
int pthread_test_com_flag = 0;

int tg_pthread_destroy();


int main(int argc, const char *argv[])
{
	int i;
	int ret;
	int ret_spi;
	
	int test_continue_flag = 1;	//1: 1:N   ;  0: 1:1

	TG_enroll_data new_user;

	TG_package recv_pack;// recv command package
	TG_package send_pack;// send command package
	TG_cert send_cert;// send command cert
	TG_cert enroll_cert;// enroll_cert
	int sizeFeature3 = TZD_LENGTH * 3;
	
	char compare_success_path[256] = {0};
	char cert_name[256] = {0};
	char cert_path[256] = {0};

	stu_upper_data *p_upper_compare_data = NULL;	
	int upper_cert_count = 0;

/***********************init***********************/	

	fd_spi = open("/dev/spidev0.0", O_RDWR);		
	if (fd_spi < 0)		
		printf("open can't open /dev/spidev0.0\n");

 	ret_spi = tg_spi_init(fd_spi);
	if (ret_spi < 0)
		printf("tg_spi_init  failed\n");
	
//	strcpy(send_pack.device_name,"UD55");
	fd_gpio= open("/dev/gpio_ctl",O_RDWR);	
	if(fd_gpio < 0)
		printf("can't open gpio\n");

	sound_send(fd_gpio,0xe3);//set sound value
	pthread_mutex_init(&mutex_package,NULL);
	pthread_mutex_init(&mutex_spi,NULL);
/*************************创建摄像头采集线程*************************/
	if(pthread_create(&tgthread_camera_data_tid,NULL,tgthread_camera_data,NULL))
	{
		perror("fail to pthread_create");
		exit(-1);
	}
	
/*************************创建心跳包线程*************************/
	if(pthread_create(&tgthread_heart_beat_tid,NULL,tgthread_heart_beat,NULL))
	{
		perror("fail to pthread_create");
		exit(-1);
	}

	while(1)
	{
		sleep(1);	
#if defined(HID_DEV_H3)||defined(HID_DEV_JZ)					
		if(fd_net<0){
			fd_net=init_Device();
			if(fd_net<0) continue;
		}

#endif
#ifdef NET_DEV_H3	
		printf("net_client_connect start\n");
		if(fd_net<0){
			fd_net=net_client_connect(SERVICE_ADDR,PORT);
			if(fd_net<0) continue;
		}
		printf("net_client_connect end\n");
#endif


		while(1){	
			usleep(10*1000);
			pthread_mutex_lock(&mutex_package);
			ret = TG_NetRecvPackage(fd_net,&recv_pack,recv_buf);
			if (ret < 0)
			{
				printf("error ret = %d,close fd_net !!\n",ret);
				close(fd_net);
				fd_net=-1;
			}
			pthread_mutex_unlock(&mutex_package);
			printf("ret = %d\n",ret);
	
			if(0 == ret){//idlesse status
				continue;
			}
			else if(1 == ret){//normal cmd status
				
				display_tgpackage(&recv_pack);
				printf("recv_cmd1 = %x\n",recv_pack.cmd1);
				switch(recv_pack.cmd1){
					case DEV_KEY:
						printf("this is DEV_KEY \n");//设备密钥；		
						memset(&send_pack,0,sizeof(TG_package));
						if(test_crc(recv_buf,DEV_KEY_LENGTH))
						{	
							printf("crc right \n");
							pthread_mutex_lock(&mutex_spi); 
							ret_spi = tg_spi_key_store(fd_spi,recv_buf);
							pthread_mutex_unlock(&mutex_spi);
							if (ret_spi < 0){
								printf("tg_spi_key_store  failed\n");
								send_pack.cmd1 = DEV_KEY_SET_FAILED;							
							}
							printf("tg_spi_key_store  success\n");

//							write_data_hex(recv_buf,DEV_KEY_LENGTH,DEV_KEY_PATH);
					
							send_pack.cmd1 = DEV_KEY_SET_SUCCESS;
							
						}
						else
						{
							send_pack.cmd1 = DEV_KEY_SET_FAILED;
							printf("crc wrong \n");
						}
						send_pack.length= 0;
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
						pthread_mutex_unlock(&mutex_package);						
						break;
						
					case ENROLL_REQ://注册请求；
						printf("this is ENROLL_REQ \n");
						tg_pthread_destroy();
						ret  = pthread_create(&tgthread_register_tid,NULL,tgthread_register,(void *)&new_user);
						pthread_detach(tgthread_register_tid);
						if(0  != ret)
						{
							perror("pthread_create register fail ");
							exit(EXIT_FAILURE);
						}
						break;

					case VALIDATE_LOCAL_REQ://本地证书验证请求；
						printf("this is VALIDATE_LOCAL_REQ \n");
						memset(compare_success_path,0,sizeof(compare_success_path));
						tg_pthread_destroy();
						ret  = pthread_create(&tgthread_local_compare_tid,NULL,tgthread_local_compare,compare_success_path);
						pthread_detach(tgthread_local_compare_tid);
						
						if(0  != ret)
						{
							perror("pthread_create tgthread_local_compare_tid fail ");
							exit(EXIT_FAILURE);
						}
						break;	

					case TEST_REG_ON_DEV://测试本地证书一次注册请求；
						printf("this is TEST_REG_ON_DEV \n");
						tg_pthread_destroy();
						ret  = pthread_create(&tgthread_test_register_tid,NULL,tgthread_test_register,NULL);
						pthread_detach(tgthread_test_register_tid);
						
						if(0  != ret)
						{
							perror("pthread_create tgthread_test_register_tid fail ");
							exit(EXIT_FAILURE);
						}
						break;	


					case ONCE_VALIDATING_ON_DEV://测试本地证书一次验证请求；
						printf("this is ONCE_VALIDATING_ON_DEV \n");
						tg_pthread_destroy();
						test_continue_flag = 0;
						ret  = pthread_create(&tgthread_test_compare_tid,NULL,tgthread_test_compare,&test_continue_flag);
						pthread_detach(tgthread_test_compare_tid);
						if(0  != ret)
						{
							perror("pthread_create tgthread_test_compare_tid fail ");
							exit(EXIT_FAILURE);
						}
						break;	

					case KEEP_VALIDATING_ON_DEV://测试本地证书连续验证请求；
						printf("this is KEEP_VALIDATING_ON_DEV \n");
						tg_pthread_destroy();
						test_continue_flag = 1;
						ret  = pthread_create(&tgthread_test_compare_tid,NULL,tgthread_test_compare,&test_continue_flag);
						pthread_detach(tgthread_test_compare_tid);
						
						if(0  != ret)
						{
							perror("pthread_create tgthread_test_compare_tid fail ");
							exit(EXIT_FAILURE);
						}
						break;	

					case VALIDATE_UPPER_CERT_START://上位机证书验证,上位机开始传输证书		PC->ARM
						printf("this is VALIDATE_UPPER_CERT_START \n");
						pthread_mutex_lock(&mutex_spi);	
						tg_spi_key_req(fd_spi);
						pthread_mutex_unlock(&mutex_spi);
						if(NULL != p_upper_compare_data)
						{
							free(p_upper_compare_data);
							p_upper_compare_data = NULL;
						}						
						upper_cert_count = 0;
						upper_cert_num = recv_pack.id;
						p_upper_compare_data = (stu_upper_data *)calloc(upper_cert_num,sizeof(stu_upper_data));
						break;	
						
					case VALIDATE_UPPER_CERT_DATA://上位机证书验证,上位机传输证书(单个证书传输)	PC->ARM
						printf("this is VALIDATE_UPPER_CERT_DATA \n");
						strcpy((p_upper_compare_data+upper_cert_count)->cert_name,recv_pack.cert_name);

						pthread_mutex_lock(&mutex_spi);						
						tg_spi_cert_decrypt(fd_spi,recv_buf);
						pthread_mutex_unlock(&mutex_spi);
						if (ret_spi < 0)
							printf("tg_spi_cert_decrypt  failed\n");
						
						printf("cert_name = %s\n",recv_pack.cert_name);
						memcpy(&(p_upper_compare_data+upper_cert_count)->cert_data,recv_buf,sizeof(TG_cert));
					
						(p_upper_compare_data+upper_cert_count)->cert_type = ((TG_cert *)&recv_buf)->cert_type;
				
						printf("name = %s,count = %d\n",recv_pack.cert_name,upper_cert_count);
//						write_data_hex(recv_buf,sizeof(TG_cert),recv_pack.cert_name);
						upper_cert_count++;
						break;

						
					case VALIDATE_UPPER_CERT_END://上位机证书验证,上位机结束传输证书 (传输结束自动开始验证)		PC->ARM	PC->ARM
						printf("this is VALIDATE_UPPER_CERT_END \n");						
						tg_pthread_destroy();
						ret  = pthread_create(&tgthread_upper_compare_tid,NULL,tgthread_upper_compare,p_upper_compare_data);
						pthread_detach(tgthread_upper_compare_tid);
						
						if(0  != ret)
						{
							perror("pthread_create tgthread_upper_compare_tid fail ");
							exit(EXIT_FAILURE);
						}
						break;

					case VALIDATE_UPPER_END://上位机证书验证结束；		PC->ARM
						printf("this is VALIDATE_UPPER pthread_cancel.\n");	
						tg_pthread_destroy();			
						break;

					case FLOW_CANCEL://结束正在进行中的注册比对流程；	PC->ARM
						printf("this is FLOW_CANCEL .\n");	
						tg_pthread_destroy();

						break;					

					case INFO_DATA://证书密钥+人员信息+CRC;
						printf("this is INFO_DATA \n");
						memset(&send_pack,0,sizeof(TG_package));
						memset(&enroll_cert,0,sizeof(TG_cert));
						memcpy(&enroll_cert,recv_buf,sizeof(TG_cert));

						display_cert(&enroll_cert);

						if(test_crc(recv_buf,CERT_LENGTH-4))
						{
							printf("crc right \n");
							memcpy(enroll_cert.chara,new_user.enroll_chara,4096);
							make_crc((char *)&enroll_cert,CERT_LENGTH-4);
							sprintf(new_user.new_reg_path,"%s/%s",USER_CERT_PATH,enroll_cert.user_name);

							pthread_mutex_lock(&mutex_spi);
							tg_spi_key_req(fd_spi);
							ret_spi = tg_spi_cert_encrypt(fd_spi,&enroll_cert);	
							pthread_mutex_unlock(&mutex_spi);
							
							if (ret_spi < 0)
							{
								send_pack.cmd1 = CERT_DATA_A_FAIL;
								printf("tg_spi_cert_encrypt  failed\n");
							}
							else
							{
								send_pack.cmd1 = CERT_DATA_A;
								printf("tg_spi_cert_encrypt  success\n");
							}

							/*mgr usr upload cert to pc*/
							send_pack.cert_type = recv_pack.cert_type;								
							send_pack.length = sizeof(TG_cert);	
							memcpy(send_pack.random_num,ran_num,32);

							if(recv_pack.cert_type == USR){
								/*user  save cert in arm*/
								printf("new_user.new_reg_path = %s\n",new_user.new_reg_path);
								write_data_hex(&enroll_cert,sizeof(TG_cert),new_user.new_reg_path);
								system("sync");
							}		
							pthread_mutex_lock(&mutex_package);
//							write_data_hex(&enroll_cert,sizeof(TG_cert),"enrollcert1.dat");
							ret = TG_NetSendPackage(fd_net,&send_pack,&enroll_cert);
							pthread_mutex_unlock(&mutex_package);	
							usleep(100*1000);
						}
						else
						{					
							printf("crc wrong \n");
							send_pack.cmd1 = INFO_DATA_CRC_ERR;
							send_pack.length = 0;								
							pthread_mutex_lock(&mutex_package);
							ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
							pthread_mutex_unlock(&mutex_package);
						}					
						break;
					
					case CERT_ENCRYPT://上位机单个证书加密请求；	PC->ARM
						printf("this is CERT_ENCRYPT \n");
						memset(&send_pack,0,sizeof(TG_package));

						pthread_mutex_lock(&mutex_spi);
						tg_spi_key_req(fd_spi);
						ret_spi = tg_spi_cert_encrypt(fd_spi,recv_buf);	
						pthread_mutex_unlock(&mutex_spi);
						if (ret_spi < 0){
							printf("tg_spi_cert_encrypt  failed\n");
							send_pack.length= 0;
							send_pack.cmd1 = CERT_CRYPT_FAIL;
						}
						else{	
							printf("tg_spi_cert_encrypt  success\n");
							send_pack.length= sizeof(TG_cert);
							send_pack.cmd1 = CERT_ENCRYPT_DONE;	
						}				

						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,recv_buf);
						pthread_mutex_unlock(&mutex_package);				
						break;

					case CERT_DECRYPT://上位机单个证书解密请求；	PC->ARM
						printf("this is CERT_DECRYPT \n");
						memset(&send_pack,0,sizeof(TG_package));

						pthread_mutex_lock(&mutex_spi);
						tg_spi_key_req(fd_spi);
						ret_spi = tg_spi_cert_decrypt(fd_spi,recv_buf); 
						pthread_mutex_unlock(&mutex_spi);
						if (ret_spi < 0){
							printf("tg_spi_cert_decrypt  failed\n");
							send_pack.length= 0;
							send_pack.cmd1 = CERT_CRYPT_FAIL;
						}
						else{
							printf("tg_spi_cert_decrypt  success\n");
							send_pack.length= sizeof(TG_cert);
							send_pack.cmd1 = CERT_DECRYPT_DONE; 
						}
										

						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,recv_buf);
						printf("TG_HidSendPackage ret = %d\n",ret);
						pthread_mutex_unlock(&mutex_package);				
						break;

					case GET_LOGINING_CERT://请求获取刚登录用户的证书；			PC->ARM
						printf("this is GET_LOGINING_CERT_REQ \n");
						memset(&send_pack,0,sizeof(TG_package));

						send_pack.cmd1 = GET_LOGINING_CERT;
						send_pack.length= sizeof(TG_cert);

						strcpy(send_pack.device_name,DEV_NAME);
						tg_get_file_data(SN_PATH,send_pack.serial_num);
					
						read_data_hex((char*)&send_cert,sizeof(TG_cert),LOGIN_CERT_PATH);
						
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,(char*)&send_cert);
						pthread_mutex_unlock(&mutex_package);	
						usleep(100*1000);
						break;



					case CERT_INFO_REQ://请求传输h3上存储的各类证书个数,信息；	PC->ARM
						printf("this is CERT_INFO_REQ \n");
						memset(&send_pack,0,sizeof(TG_package));
						send_pack.length= 0;
						send_pack.cmd1 = CERT_NUM;

						send_pack.mgr_num = tg_dir_filenum(MGR_CERT_PATH);
						send_pack.usr_num= tg_dir_filenum(USER_CERT_PATH);
						
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
						pthread_mutex_unlock(&mutex_package);				
						break;
					
					case CERT_DATA_B://上位机的证书(导入到H3中)；PC->ARM
						printf("this is CERT_DATA_B \n");
						memset(&send_pack,0,sizeof(TG_package));
						memset(cert_name,0,sizeof(cert_name));
						sprintf(cert_name,"/%s",recv_pack.cert_name);
//						write_data_hex(recv_buf,sizeof(TG_cert),"upcert.dat");	
						if(test_crc(recv_buf,CERT_LENGTH-4))
						{
							printf("crc right \n");
							switch(recv_pack.cert_type){
								case MGR:
									strcpy(cert_path,MGR_CERT_PATH);
									strcat(cert_path,cert_name);
									printf("write certificate to %s\n",cert_path);
									write_data_hex(recv_buf,sizeof(TG_cert),cert_path);	
									break;
								case USR:	
									strcpy(cert_path,USER_CERT_PATH);	
									strcat(cert_path,cert_name);
									printf("write certificate to %s\n",cert_path);
									write_data_hex(recv_buf,sizeof(TG_cert),cert_path);	
									break;
								case TEST_USER:	
									strcpy(cert_path,TSET_USER_CERT_PATH);	
									strcat(cert_path,cert_name);
									printf("write certificate to %s\n",cert_path);
									write_data_hex(recv_buf,sizeof(TG_cert),cert_path);	
									break;
								default:
									printf("error cert_type = %d,please send a right cert_type\n",recv_pack.cert_type);
									break;
							}
							send_pack.cmd1 = CERT_DATA_B_CRC_RIGHT;
							send_pack.length = 0;	
	
						}
						else
						{					
							printf("crc wrong \n");
							send_pack.cmd1 = CERT_DATA_B_CRC_ERR;
							send_pack.length = 0;								
						}	
						if(TEST_USER == recv_pack.cert_type)
							break;
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
						pthread_mutex_unlock(&mutex_package);					
						break;


					case CERT_DATA_REQ://请求传输h3上存储的各类证书；
						printf("this is CERT_DATA_REQ \n");
						memset(&send_pack,0,sizeof(TG_package));
						printf("compare_success_path = %s\n",compare_success_path);
						tg_path_to_info(compare_success_path,send_pack.cert_name,&send_pack.id,&send_pack.cert_type);
						read_data_hex((char*)&send_cert,sizeof(TG_cert),compare_success_path);	
						send_pack.length= sizeof(TG_cert);
						send_pack.cmd1 = CERT_DATA_C;//存在H3上的证书(新证书导入到PC中)；ARM->PC
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,(char*)&send_cert);
						pthread_mutex_unlock(&mutex_package);						
						break;

					case UPPER_CERT_CONFIRM://上位机收到证书且CRC正确，删除原来的证书；	PC->ARM
						memset(&send_pack,0,sizeof(TG_package));
						send_pack.length = 0;
						if(0 == tg_delete_file(recv_pack.cert_name))
							send_pack.cmd1 = DELETE_SUCCESS;
						else
							send_pack.cmd1 = DELETE_FAILED;
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,(char*)&send_cert);
						pthread_mutex_unlock(&mutex_package);	
						break;

						
					case UPPER_CERT_NO_CONFIRM://上位机没收到证书或者CRC错误，重新传输验证成功的本地证书；	PC->ARM					
						memset(&send_pack,0,sizeof(TG_package));
						send_pack.length= sizeof(TG_cert);
						send_pack.cmd1 = CERT_DATA_C;
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,(char*)&send_cert);
						pthread_mutex_unlock(&mutex_package);						
						break;
						
					case REG_CERT_RESEND://下位机注册完传给上位机的证书CRC错误，重新注册的证书；	PC->ARM			
						//CERT_DATA_A
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,&enroll_cert);
						pthread_mutex_unlock(&mutex_package);						
						break;
						
					case RAN_NUM_REQ://随机数请求；
						printf("this is RAN_NUM_REQ \n");
						memset(&send_pack,0,sizeof(TG_package));
						pthread_mutex_lock(&mutex_spi);
						ret_spi = tg_spi_random_num_req(fd_spi,ran_num);
						if (ret_spi < 0)
							printf("tg_spi_random_num_req  failed\n");
						pthread_mutex_unlock(&mutex_spi);
						printf("tg_spi_random_num_req  success\n");
						
						
						memcpy(send_pack.random_num,ran_num,32);
//						write_data_hex(ran_num,RAN_NUM_LENGTH,RAN_NUM_PATH);
						send_pack.length= 0;
						send_pack.cmd1 = RAN_NUM;
						pthread_mutex_lock(&mutex_package);
						ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
						pthread_mutex_unlock(&mutex_package);				
						break;

					case DELETE_REQ://删除请求；		PC->ARM
						printf("this is DELETE_REQ \n");
//						sprintf(delete_file,"%s",recv_pack.cert_name);
						if(0 == remove(recv_pack.cert_name))
						{
							printf("delete success\n");
							sound_send(fd_gpio,0x0A);	//shanchu chenggong
						}
						else
							printf("delete failed\n");
							sound_send(fd_gpio,0x0B);	//shanchu shibai
						break;
	
					default:
						printf("error cmd = %d,please input a right cmd\n",recv_pack.cmd1);
						break;
					}
			}
			else//error status
			{	
				printf("RecvPackage fail: %d !!\n",ret);
	//			display_tgpackage(&recv_pack);;
				break;
			}	
		}
	}
	if(pthread_join(tgthread_heart_beat_tid,NULL) < 0)    //wait pthread end
	{
		perror("fail to pthread_join");
		exit(-1);		
	}

	if(pthread_join(tgthread_camera_data_tid,NULL) < 0)                              //wait pthread end
	{
		perror("fail to pthread_join");
		exit(-1);		
	}
	printf("error  end \n");
	pthread_mutex_destroy(&mutex_spi);
	pthread_mutex_destroy(&mutex_package);
	
	close(fd_net);
	close(fd_spi);
	close(fd_gpio);
	return 1;
}

/*****************************************************************************
 * 							摄像头采集线程函数
*****************************************************************************/
void *tgthread_camera_data(void *arg)
{
	int i;
	int k0 = 0;
	int k1;
    int ret;
    unsigned char temp_buf[CAMERA_WIDTH*CAMERA_HEIGHT*2] ;
	printf("new camera pthread\n");

    ret = cam_open();
    ASSERT(ret==0);
    ret = cam_select(0);
//    ASSERT(ret==0);
    ret = cam_init();
//    ASSERT(ret==0);
    while (1)
    {
        ret = cam_get_image(temp_buf,IMAGE_SIZE);
//        ASSERT(ret==0);
		if(camera_flag)
		{
#ifdef CAM_9V034
			printf("9v034 camera\n");
			memcpy(camera_data,temp_buf,CAMERA_WIDTH*CAMERA_HEIGHT);
#endif		

#ifdef CAM_7725
			printf("7725 camera\n");
			YUV2Y(temp_buf,CAMERA_WIDTH,CAMERA_HEIGHT,camera_data) ;				
#endif
			camera_flag = 0;
		}
    }
    ret = cam_close();
//    ASSERT(ret==0);
    return 0;
}



/*****************************************************************************
 * 								注册线程函数
*****************************************************************************/
void * tgthread_register(void * arg)
{
	int cntMax = 1,re = -1, sizeFeature3 = TZD_LENGTH * 3;
	int sizeFeature = TZD_LENGTH ;
	int loc;
	int usr_num = 0;
	int i = 0;
	int ret;

	TG_package send_pack;// send command package

	unsigned char ** pic_data;
	unsigned char ** feature;
	unsigned char * tempRegisterData;
	unsigned char * registerData= NULL;
	stu_usr *usr_addr = NULL;
	pthread_reg_flag = 1;

	pic_data = Make2DArray_uint8(3,CAMERA_ROI_HEIGHT*CAMERA_ROI_WIDTH); //src camera data
	feature = Make2DArray_uint8(3,sizeFeature * sizeof(unsigned char));
	tempRegisterData = (unsigned char *)malloc(sizeFeature3 * sizeof(unsigned char));	

	memset(&send_pack,0,sizeof(TG_package));
	memset(arg,0,sizeof(TG_enroll_data));
	usr_num = 0;
	printf("usr_num = %d\n",usr_num);

	sound_send(fd_gpio,0x06);	//qing ziran qingfang shouzhi
	for(i = 0; i < 3 ; i++)                                              //expect capture 3 normal fingervein
	{
		while(1)
		{			
			printf("waiting finger ..........................\n");
			if(1 == touch_signal(fd_gpio,2))                                      //detect touch signal
			{
				printf("camera\n");
				tg_image_adaptive_9V034(fd_gpio,*(pic_data+i));	
#ifdef SAVE_PIC		
				TG_SaveRotateBmp(*(pic_data+i), CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, "showBmp.bmp");
#endif
				XorEncryptDecrypt(*(pic_data+i), CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT, 1);
				if( 0 != TGImgExtractFeature(*(pic_data + i), CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, *(feature + i)) )      //create fingervein chara,return the result of pic quality judegement
				{
					if(cntMax<5)                                         //if cntMax==5,meaning register failed ,dosen't send sound
					{
						sound_send(fd_gpio,0x05);		//qing zhengque fangru shouzhi
						while(1)
						{
							if( 0 == touch_signal(fd_gpio,3))                    //detect finger release
							{
								break;
							}
							usleep(1000);
						}					
						i--;
					}				
					cntMax++; //fail num,initial value = 1 
					if(cntMax == 5)
						goto fail;
					break;
				}
				else
				{					
					if( i < 2)
					{
						sound_send(fd_gpio,0x04);	//qing zaifang yici
						while(1)
						{
							if( 0 == touch_signal(fd_gpio,3) )
							{
								break;
							}
							usleep(1000);
						}
					}					
					break;
				}
			}
			sleep(3);
		}

	}
	if(3 == i)	//capture 3 normal fingervein succeed
	{
			re = TGFusionFeature(*(feature), *(feature + 1), *(feature + 2), tempRegisterData);
		//	printf("re = %d\n",re);

	}
	if( 0 == re )
	{	

		sound_send(fd_gpio,0x02);	//dengji chenggong
		printf("this is ENROLL success \n");
		send_pack.length= 0;
		send_pack.cmd1 = ENROLL_SUCCESS;			
		pthread_mutex_lock(&mutex_package);
		ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
		pthread_mutex_unlock(&mutex_package);
		memcpy(((TG_enroll_data*)arg)->enroll_chara,tempRegisterData,sizeFeature3);
//			write_data_hex(tempRegisterData,sizeFeature3,new_reg_addr);
	}
	else
	{
fail:
		sound_send(fd_gpio,0x03);	//dengji shibai
		printf("this is ENROLL fail \n");
		send_pack.length= 0;
		send_pack.cmd1 = ENROLL_FAILED;			
		pthread_mutex_lock(&mutex_package);
		ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
		pthread_mutex_unlock(&mutex_package);
	}
	pthread_reg_flag = 0;

	free(usr_addr);
    usr_addr = NULL;
	free(registerData);
	registerData = NULL;
	free(tempRegisterData);
	tempRegisterData = NULL;
	Free2DArray_uint8(feature,3);
	Free2DArray_uint8(pic_data,3);
	pthread_exit(NULL);
}


/*****************************************************************************
 * 							arm端证书比对线程函数
*****************************************************************************/
void *tgthread_local_compare(void *arg)
{
	long l1,l2;
	int compare_ret;
	int ret,i = 0;;
	int usr_num = 0;
	int   sizeFeature3 = TZD_LENGTH * 3, loc;
	int   sizeFeature = TZD_LENGTH;

	unsigned char *pic_data;
	unsigned char *feature;
	unsigned char * registerData;
	unsigned char * tempUpdateData;

	struct timeval tv;
	int success_id;
	stu_usr *usr_addr = NULL;
	TG_package send_pack;// send command package

	pthread_loc_flag = 1;
	
	pic_data = (unsigned char *)malloc(CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT*sizeof(unsigned char));//src camera data
	feature = (unsigned char *)malloc(sizeFeature*sizeof(unsigned char));
	tempUpdateData = (unsigned char *)malloc(sizeFeature3*sizeof(unsigned char));

	make_localcert_to_dat(fd_spi,&registerData,&usr_addr,&usr_num);
	printf("usr num  = %d\n",usr_num);

	sound_send(fd_gpio,0x06);	//qing ziran qingfang shouzhi
	while(1)
	{
		printf("waiting finger ..........................\n");
		if(touch_signal(fd_gpio,2) )
		{
			tg_image_adaptive_9V034(fd_gpio,pic_data);
#ifdef SAVE_PIC
//			TG_SaveBmp(camera_data,CAMERA_HEIGHT,CAMERA_WIDTH,"280.bmp");
			TG_SaveRotateBmp(pic_data, CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, "showBmp.bmp");
#endif
			XorEncryptDecrypt(pic_data, CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT, 1);
			printf("camera\n");
		}
		else
			break;

		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
		TGImgExtractFeature(pic_data, CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, feature);
		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000+tv.tv_usec;
		printf("ISP time = %ld ms  \n",(l2-l1)/1000);

		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
		compare_ret = TGMatchFeature(feature, registerData, usr_num, &loc, tempUpdateData);
		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000+tv.tv_usec;
		printf("compare time = %ld ms  \n",(l2-l1)/1000);

		if(0 == compare_ret)
		{
			printf("this is VALIDATE_LOCAL_SUCCESS success \n");
			sound_send(fd_gpio,0x07);	//yanzheng chenggong
			printf("loc = %d\n",loc);
			printf("success name  = %s\n",(usr_addr+loc-1)->addr);
			printf("success cert_type = %d \n",(usr_addr+loc-1)->cert_type);

			memset(&send_pack,0,sizeof(TG_package));
			send_pack.length= 0;
			send_pack.cmd1 = VALIDATE_LOCAL_SUCCESS;
			strcpy(send_pack.cert_name,(usr_addr+loc-1)->addr);
			strcpy((char *)arg,(usr_addr+loc-1)->addr);
			
			pthread_mutex_lock(&mutex_package);
			ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
			pthread_mutex_unlock(&mutex_package);
			break;
		}
		else
		{
			printf("this is VALIDATE_LOCAL_FAILED failed \n");
			sound_send(fd_gpio,0x08);	//yanzheng shibai
			printf("fail\n");
			memset(&send_pack,0,sizeof(TG_package));
			send_pack.length= 0;
			send_pack.cmd1 = VALIDATE_LOCAL_FAILED;
			
			pthread_mutex_lock(&mutex_package);
			ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
			pthread_mutex_unlock(&mutex_package);	
		}
		printf("-----------end--------------\n");
		sleep(1);
	}

	pthread_loc_flag = 0;

	free(registerData);
	registerData = NULL;
	free(usr_addr);
	usr_addr = NULL;
	free(tempUpdateData);
	tempUpdateData = NULL;
	free(feature);
	feature = NULL;
	free(pic_data);
	pic_data = NULL;
	pthread_exit(NULL);
}



/*****************************************************************************
 * 							PC端证书比对线程函数
*****************************************************************************/
void *tgthread_upper_compare(void *arg)
{	
	long l1,l2;
	int compare_ret;
	int ret,i = 0;;
	int usr_num = 0;
	int   sizeFeature3 = TZD_LENGTH * 3, loc;
	int   sizeFeature = TZD_LENGTH;

	unsigned char *pic_data;
	unsigned char *feature;
	unsigned char * registerData;
	unsigned char * tempUpdateData;
	
	struct timeval tv;
	int success_id;
	stu_upper_data *p_upper_data = (stu_upper_data *)arg;
	TG_package send_pack;// send command package
	
	pthread_up_flag = 1;
	
	pic_data = (unsigned char *)malloc(CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT*sizeof(unsigned char));//src camera data
	feature = (unsigned char *)malloc(sizeFeature*sizeof(unsigned char));
	tempUpdateData = (unsigned char *)malloc(sizeFeature3*sizeof(unsigned char));
	registerData  = (unsigned char *)malloc(upper_cert_num* sizeFeature3*sizeof(unsigned char));

	printf("usr num  = %d\n",upper_cert_num);
	tg_uppercert_to_dat(fd_spi,p_upper_data,registerData,upper_cert_num);
//	write_data_hex(registerData, upper_cert_num* sizeFeature3, "aaa.dat");
	sound_send(fd_gpio,0x06);	//qing ziran qingfang shouzhi
	
	while(1)
	{
		printf("waiting finger ..........................\n");
		if(touch_signal(fd_gpio,2) )
		{

			tg_image_adaptive_9V034(fd_gpio,pic_data);
#ifdef SAVE_PIC
//			TG_SaveBmp(camera_data,CAMERA_HEIGHT,CAMERA_WIDTH,"280.bmp");
			TG_SaveRotateBmp(pic_data, CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, "showBmp.bmp");
#endif
			XorEncryptDecrypt(pic_data, CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT, 1);
			printf("camera\n");
		}
		else
			break;

		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
		TGImgExtractFeature(pic_data, CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, feature);
		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000+tv.tv_usec;
		printf("ISP time = %ld ms  \n",(l2-l1)/1000);



		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
		compare_ret = TGMatchFeature(feature, registerData, upper_cert_num, &loc, tempUpdateData);
		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000+tv.tv_usec;
		printf("compare time = %ld ms  \n",(l2-l1)/1000);

		if(0 == compare_ret)
		{
			printf("this is VALIDATE_UPPER_SUCCESS success \n");
			sound_send(fd_gpio,0x07);	//yanzheng chenggong
			printf("loc = %d\n",loc);
			printf("success cert_name = %s \n",(p_upper_data+loc-1)->cert_name);
			printf("success cert_type = %d \n",(p_upper_data+loc-1)->cert_type);
			memset(&send_pack,0,sizeof(TG_package));
			send_pack.length= 0;
			send_pack.cmd1 = VALIDATE_UPPER_SUCCESS;
			strcpy(send_pack.cert_name,(p_upper_data+loc-1)->cert_name);
			send_pack.cert_type = (p_upper_data+loc-1)->cert_type;
			
			pthread_mutex_lock(&mutex_package);
			write_data_hex(&((p_upper_data+loc-1)->cert_data),sizeof(TG_cert),LOGIN_CERT_PATH);
			ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
			pthread_mutex_unlock(&mutex_package);
			break;

		}
		else
		{
			printf("this is VALIDATE_UPPER_FAILED failed \n");
			sound_send(fd_gpio,0x08);	//yanzheng shibai
			memset(&send_pack,0,sizeof(TG_package));
			send_pack.length= 0;
			send_pack.cmd1 = VALIDATE_UPPER_FAILED;
			
			pthread_mutex_lock(&mutex_package);
			ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
			pthread_mutex_unlock(&mutex_package);			
		}
		printf("-----------end--------------\n");
		sleep(1);
	}
	pthread_up_flag = 0;

	free(registerData);
	registerData = NULL;
	free(tempUpdateData);
	tempUpdateData = NULL;
	free(feature);
	feature = NULL;
	free(pic_data);
	pic_data = NULL;
	pthread_exit(NULL);
}



/*****************************************************************************
 * 							测试程序注册线程函数
*****************************************************************************/
void * tgthread_test_register(void * arg)
{

	int cntMax = 1,re = -1, re_verify = -1, sizeFeature3 = TZD_LENGTH * 3, sizeFeature = TZD_LENGTH;
	int loc;
	int usr_num = 0;
	int i = 0;
	int ret;

	TG_cert test_enroll_cert;
	TG_package send_pack;// send command package

	unsigned char ** pic_data;
	unsigned char ** feature;
	unsigned char * tempRegisterData;
	unsigned char * registerData= NULL;
	
	stu_usr *usr_addr = NULL;
	char cert_path[256] = {0};
	pthread_test_reg_flag = 1;

	
	pic_data = Make2DArray_uint8(3,CAMERA_ROI_HEIGHT*CAMERA_ROI_WIDTH); //src camera data
	feature = Make2DArray_uint8(3,sizeFeature * sizeof(unsigned char));
	tempRegisterData = (unsigned char *)malloc(sizeFeature3 * sizeof(unsigned char));	

	memset(&send_pack,0,sizeof(TG_package));
	usr_num = 0;
	printf("usr_num = %d\n",usr_num);
	printf("-----------------------------------------------------------\n");
	
	registerData = NULL;
	sound_send(fd_gpio,0x06);	//qing ziran qingfang shouzhi
	for(i = 0; i < 3 ; i++)                                              //expect capture 3 normal fingervein
	{
		while(1)
		{			
			printf("waiting finger ..........................\n");
			if(1 == touch_signal(fd_gpio,2))                                      //detect touch signal
			{
				printf("camera\n");
				tg_image_adaptive_9V034(fd_gpio,*(pic_data+i));		
#ifdef SAVE_PIC		
				TG_SaveRotateBmp(*(pic_data+i), CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, "showBmp.bmp");
#endif
				XorEncryptDecrypt(pic_data, CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT, 1);

				//sound_send(0x00);		          //Bi   
				if( 0 != TGImgExtractFeature(*(pic_data + i), CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, *(feature + i)) )      //create fingervein chara,return the result of pic quality judegement
				{
					if(cntMax<5)                                         //if cntMax==5,meaning register failed ,dosen't send sound
					{
						sound_send(fd_gpio,0x05);		//qing zhengque fangru shouzhi
						while(1)
						{
							if( 0 == touch_signal(fd_gpio,3))                    //detect finger release
							{
								break;
							}
							usleep(1000);
						}					
						i--;
					}				
					cntMax++; //fail num,initial value = 1 
					if(cntMax == 5)
						goto fail1;
					break;
				}
				else
				{					

					if( i < 2)
					{
						sound_send(fd_gpio,0x04);	//qing zaifang yici
						while(1)
						{
							if( 0 == touch_signal(fd_gpio,3) )
							{
								break;
							}
							usleep(1000);
						}
					}					
					break;
				}
			}
			sleep(3);
		}

	}
	if(3 == i)	//capture 3 normal fingervein succeed
	{
		re = TGFusionFeature(*(feature), *(feature + 1), *(feature + 2), tempRegisterData);
	//	printf("re = %d\n",re);
	}
	if( 0 == re )
	{	
		/*test user save cert in arm*/
		memset(&test_enroll_cert,0,sizeof(TG_cert));
		memcpy(test_enroll_cert.chara,tempRegisterData,sizeFeature3);
		make_crc((char *)&test_enroll_cert,CERT_LENGTH-4);

		int ret_spi;
		pthread_mutex_lock(&mutex_spi);
		tg_spi_key_req(fd_spi);
		ret_spi = tg_spi_cert_encrypt(fd_spi,&test_enroll_cert);
		pthread_mutex_unlock(&mutex_spi);
		if (ret_spi < 0)
		{
			goto fail1;
			printf("tg_spi_cert_encrypt  failed\n");
		}

		sound_send(fd_gpio,0x02);	//dengji chenggong
		printf("this is test ENROLL success \n");
		send_pack.length= 0;
		send_pack.cmd1 = ENROLL_SUCCESS;			
		pthread_mutex_lock(&mutex_package);
		ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
		pthread_mutex_unlock(&mutex_package);

		sprintf(cert_path,"%s/%d",TSET_USER_CERT_PATH,rand());	
		write_data_hex(&test_enroll_cert,sizeof(TG_cert),cert_path);
		system("sync");

	}
	else
	{
fail1:		
		sound_send(fd_gpio,0x03);	//dengji shibai
		printf("this is ENROLL fail \n");
		send_pack.length= 0;
		send_pack.cmd1 = ENROLL_FAILED;			
		pthread_mutex_lock(&mutex_package);
		ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
		pthread_mutex_unlock(&mutex_package);
	}
	pthread_test_reg_flag = 0;
	
	free(usr_addr);
    usr_addr = NULL;
	free(registerData);
	registerData = NULL;
	free(tempRegisterData);
	tempRegisterData = NULL;
	Free2DArray_uint8(feature,3);	
	Free2DArray_uint8(pic_data,3);	
	pthread_exit(NULL);
}



/*****************************************************************************
 * 							测试程序比对线程函数
*****************************************************************************/
void *tgthread_test_compare(void *arg)
{
	long l1,l2;
	int compare_ret;
	int ret,i = 0;;
	int usr_num = 0;
	int   sizeFeature3 = TZD_LENGTH * 3, loc;
	int   sizeFeature = TZD_LENGTH;
		
	unsigned char *pic_data;
	unsigned char *feature;
	unsigned char * registerData;
	unsigned char * tempUpdateData;

	struct timeval tv;
	int continue_flag = *((int *)arg);
	int success_id;
	stu_usr *usr_addr = NULL;
	TG_package send_pack;// send command package
	int compare_time;
	pthread_test_com_flag = 1;

	pic_data = (unsigned char *)malloc(CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT*sizeof(unsigned char));//src camera data
	feature = (unsigned char *)malloc(sizeFeature*sizeof(unsigned char));
	tempUpdateData = (unsigned char *)malloc(sizeFeature3*sizeof(unsigned char));
	
	test_make_localcert_to_dat(fd_spi,&registerData,&usr_addr,&usr_num);
	printf("usr num  = %d\n",usr_num);


//	write_data_hex(registerData,sizeFeature3*usr_num*sizeof(unsigned char),"registerData.dat");
	sound_send(fd_gpio,0x06);	//qing ziran qingfang shouzhi

	while(1)
	{
		printf("waiting finger ..........................\n");
		if(touch_signal(fd_gpio,2) )
		{
			tg_image_adaptive_9V034(fd_gpio,pic_data);
#ifdef SAVE_PIC
//			TG_SaveBmp(camera_data,CAMERA_HEIGHT,CAMERA_WIDTH,"280.bmp");
			TG_SaveRotateBmp(pic_data, CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, "showBmp.bmp");
#endif
			XorEncryptDecrypt(pic_data, CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT, 1);
			printf("camera\n");
		}
		else
			break;

		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
		TGImgExtractFeature(pic_data, CAMERA_ROI_WIDTH, CAMERA_ROI_HEIGHT, feature);
		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000+tv.tv_usec;
		printf("ISP time = %ld ms  \n",(l2-l1)/1000);


		gettimeofday(&tv,NULL);
		l1 = tv.tv_sec*1000*1000 + tv.tv_usec;
		compare_ret = TGMatchFeature(feature, registerData, usr_num, &loc, tempUpdateData);
		gettimeofday(&tv,NULL);
		l2 = tv.tv_sec*1000*1000+tv.tv_usec;
		printf("compare time = %ld ms  \n",(l2-l1)/1000);
		compare_time = (int)((l2-l1)/1000);

		if(0 == compare_ret)
		{
			printf("this is VALIDATE_TEST_SUCCESS success \n");
			sound_send(fd_gpio,0x07);	//yanzheng chenggong
			printf("loc = %d\n",loc);
			printf("success name  = %s\n",(usr_addr+loc-1)->addr);
			printf("success cert_type = %d \n",(usr_addr+loc-1)->cert_type);

			memset(&send_pack,0,sizeof(TG_package));
			send_pack.length= 0;
			send_pack.cmd1 = VALIDATE_LOCAL_SUCCESS;
			send_pack.cmd2 = compare_time;
			strcpy(send_pack.cert_name,(usr_addr+loc-1)->addr);
			
			pthread_mutex_lock(&mutex_package);
			ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
			pthread_mutex_unlock(&mutex_package);
			if(continue_flag)
				printf("continue.....\n");
			else
				break;
		}
		else
		{
			printf("this is VALIDATE_LOCAL_FAILED failed \n");
			sound_send(fd_gpio,0x08);	//yanzheng shibai
			printf("fail\n");
			memset(&send_pack,0,sizeof(TG_package));
			send_pack.length= 0;
			send_pack.cmd1 = VALIDATE_LOCAL_FAILED;
			send_pack.cmd2 = compare_time;
			
			pthread_mutex_lock(&mutex_package);
			ret = TG_NetSendPackage(fd_net,&send_pack,NULL);
			pthread_mutex_unlock(&mutex_package);
			if(continue_flag)
				printf("continue.....\n");
			else
				break;
		}
		printf("-----------end--------------\n");
		sleep(1);
	}

	pthread_test_com_flag = 0;
	free(registerData);
	registerData = NULL;
	free(usr_addr);
	usr_addr = NULL;
	free(tempUpdateData);
	tempUpdateData = NULL;
	free(pic_data);
	pic_data = NULL;
	free(feature);
	feature = NULL;
	pthread_exit(NULL);
}



/*****************************************************************************
 * 							心跳包线程函数
*****************************************************************************/
void *tgthread_heart_beat(void *arg)
{
	TG_package heart_beat_pack;
	int ret;
	memset(&heart_beat_pack,0,sizeof(TG_package));
	strcpy(heart_beat_pack.device_name,DEV_NAME);
	heart_beat_pack.id = 0;
	heart_beat_pack.cmd1 = HEART_BEAT;
	heart_beat_pack.cmd2 = 0;
	heart_beat_pack.length = 0;
	ret = tg_get_file_data(SN_PATH,heart_beat_pack.serial_num);
//	resolve_config(CONFIG_PATH,heart_beat_pack.serial_num,heart_beat_pack.ip_addr,heart_beat_pack.mac_addr);
	while(1)
	{
		heart_beat_pack.mgr_num = tg_dir_filenum(MGR_CERT_PATH);
		heart_beat_pack.usr_num= tg_dir_filenum(USER_CERT_PATH);
//		display_tgpackage( &heart_beat_pack);
		pthread_mutex_lock(&mutex_package);
		if(fd_net >= 0){	
			ret = TG_NetSendPackage(fd_net,&heart_beat_pack,NULL);
			printf("heart beat~~  sended,fd_net = %d,ret = %d\n",fd_net,ret);
		}
		else{
			printf("heart beat no connect!!!!\n");
		}
		pthread_mutex_unlock(&mutex_package);
		sleep(5);		
	}
}

/*****************************************************************  
* function:		tg_pthread_destroy
* description:  线程销毁函数    	
* return:    	0: 成功
*				1: 失败   
* other:
*
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
int tg_pthread_destroy()
{
	int ret;
	if(pthread_reg_flag ||pthread_loc_flag||pthread_up_flag||pthread_test_reg_flag||pthread_test_com_flag)
		printf(" reg = %d,\n loc = %d,\n up = %d,\n test_reg = %d,\n test_com = %d,\n",
				pthread_reg_flag,pthread_loc_flag,pthread_up_flag,pthread_test_reg_flag,pthread_test_com_flag);
	pthread_mutex_lock(&mutex_spi);
	if(pthread_reg_flag){
		ret = pthread_cancel(tgthread_register_tid);				
		if(0  != ret)				
		{					
			perror("tgthread_register_tid pthread_cancel  fail ");				
		}
		else
			printf("tgthread_register_tid pthread_cancel successed\n ");	
		pthread_reg_flag = 0;
	}
	
	if(pthread_loc_flag){
		ret = pthread_cancel(tgthread_local_compare_tid);				
		if(0  != ret)				
		{					
			perror("tgthread_local_compare_tid pthread_cancel  fail "); 			
		}
		else
			printf("tgthread_local_compare_tid pthread_cancel successed\n ");
		pthread_loc_flag = 0;
	}
	
	if(pthread_up_flag){
		ret = pthread_cancel(tgthread_upper_compare_tid);				
		if(0  != ret)				
		{					
			perror("tgthread_upper_compare_tid pthread_cancel  fail "); 			
		}
		else
			printf("tgthread_upper_compare_tid pthread_cancel successed\n ");						
		pthread_up_flag =0;
	}
	
	if(pthread_test_reg_flag){
		ret = pthread_cancel(tgthread_test_register_tid);				
		if(0  != ret)				
		{					
			perror("tgthread_test_register_tid pthread_cancel  fail ");				
		}
		else
			printf("tgthread_test_register_tid pthread_cancel successed\n ");						
		pthread_test_reg_flag =0;
	}

	
	if(pthread_test_com_flag){
		ret = pthread_cancel(tgthread_test_compare_tid);				
		if(0  != ret)				
		{					
			perror("tgthread_test_compare_tid pthread_cancel  fail ");				
		}
		else
			printf("tgthread_test_compare_tid pthread_cancel successed\n ");						
		pthread_test_com_flag =0;
	}
	pthread_mutex_unlock(&mutex_spi);
	return ret;
}



