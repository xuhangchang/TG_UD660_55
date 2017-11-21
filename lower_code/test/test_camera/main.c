//	13.0_0502
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
int tg_light = 40; 	// PWM 100% is 255

int first_cut_row = CUT_ROW_START;
int first_cut_col = CUT_COL_START;

TG_cert *p_compare_cert;
int upper_cert_num;

pthread_mutex_t mutex_package; 	//package mutex

int fd_com; //ttyGS0 fd
int fd_net = -1;
int fd_spi = -1;; //spidev0.0 fd
int fd_gpio;

void *tgthread_camera_data(void *arg);
void *tgthread_heart_beat(void *arg);
void *tgthread_register(void * arg);
void *tgthread_local_compare(void *arg);
void *tgthread_upper_compare(void *arg);

char recv_buf[BUFSIZE],send_buf[BUFSIZE];

pthread_t tgthread_camera_data_tid; 		//HeartBeat pthread tid
pthread_t tgthread_heart_beat_tid;		//HeartBeat pthread tid
pthread_t tgthread_register_tid;		//Register pthread tid
pthread_t tgthread_local_compare_tid; 	//Local Compare pthread tid
pthread_t tgthread_upper_compare_tid; 	//Upper Compare pthread tid


int main(int argc, const char *argv[])
{
	int i;
	int ret;
	int ret_spi;
	
	TG_enroll_data new_user;

	TG_package recv_pack;// recv command package
	TG_package send_pack;// send command package
	TG_cert send_cert;// send command cert
	TG_cert enroll_cert;// enroll_cert
	int sizeFeature3 = TZD_LENGTH * 3;

	char ran_num[32] = {0};
	char compare_success_path[128] = {0};
	char cert_name[50] = {0};
	char cert_path[50] = {0};

	stu_upper_data *p_upper_compare_data = NULL;	
	int upper_cert_count = 0;

/***********************init***********************/	
#ifdef TG_SPI
	fd_spi = open("/dev/spidev0.0", O_RDWR);		
	if (fd_spi < 0)		
		printf("open can't open /dev/spidev0.0\n");

 	ret_spi = tg_spi_init(fd_spi);
	if (ret_spi < 0)
		printf("tg_spi_init  failed\n");
#endif
	
//	strcpy(send_pack.device_name,"UD55");
	fd_gpio= open("/dev/gpio_ctl",O_RDWR);	
	if(fd_gpio < 0)
		printf("can't open gpio\n");

	sound_send(fd_gpio,0xe2);//set sound value
	pthread_mutex_init(&mutex_package,NULL);
/**************************************************/
	if(pthread_create(&tgthread_camera_data_tid,NULL,tgthread_camera_data,NULL))
	{
		perror("fail to pthread_create");
		exit(-1);
	}


	sleep(1);

//本地证书验证请求；
	printf("this is VALIDATE_LOCAL_REQ \n");
	memset(compare_success_path,0,sizeof(compare_success_path));

	ret  = pthread_create(&tgthread_local_compare_tid,NULL,tgthread_local_compare,compare_success_path);
	pthread_detach(tgthread_local_compare_tid);
	if(0  != ret)
	{
		perror("pthread_create tgthread_local_compare_tid fail ");
		exit(EXIT_FAILURE);
	}



	if(pthread_join(tgthread_camera_data_tid,NULL) < 0)                              //wait pthread end
	{
		perror("fail to pthread_join");
		exit(-1);		
	}

	pthread_mutex_destroy(&mutex_package);
	
	close(fd_net);
	close(fd_spi);
	close(fd_gpio);
	return 1;
}

/*****************************************************************************
 *
 * 					Camera capture NEW
 *
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
			printf("7735 camera\n");
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
 *
 * 								Local Compare Pthread
 *
*****************************************************************************/
void *tgthread_local_compare(void *arg)
{
	
	int compare_ret;
	int ret,i = 0,j;
	long l1,l2;
	int usr_num = 0;
	TG_package send_pack;// send command package
	struct timeval tv;
	
	int success_id;
	int   sizeFeature3 = TZD_LENGTH * 3, loc;
	unsigned char *ROI_pic;
	unsigned char *pic_data;
	unsigned char * registerData;
	unsigned char * tempUpdateData;
	unsigned char * tempData;
	stu_usr *usr_addr = NULL;
	
	ROI_pic = (unsigned char *)malloc(CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT*sizeof(unsigned char));
	pic_data = (unsigned char *)malloc(CAMERA_ROI_WIDTH*CAMERA_ROI_HEIGHT*sizeof(unsigned char));//src camera data

	while(1)
	{
		printf("waiting finger ..........................\n");
		if(touch_signal(fd_gpio,2) )
		{
#ifdef CAM_9V034
			tg_image_adaptive_9V034(fd_gpio,pic_data);
			TG_SaveBmp(camera_data,CAMERA_HEIGHT,CAMERA_WIDTH,"280.bmp");
			printf("TG_SaveBmp   280.bmp\n");
			int k =0;
			for(i = 0;i<CAMERA_ROI_HEIGHT;i++)
			{
				for(j = 0;j<CAMERA_ROI_WIDTH;j++)
				{
						*(ROI_pic+(CAMERA_ROI_WIDTH-j-1)*CAMERA_ROI_HEIGHT+i) = *(pic_data + i*CAMERA_ROI_WIDTH+j);
		//				printf("x = %d;y = %d\n",(CAMERA_ROI_HEIGHT-j-1),i);
				}
			}
			TG_SaveBmp(ROI_pic,CAMERA_ROI_WIDTH,CAMERA_ROI_HEIGHT,"180.bmp");
			printf("TG_SaveBmp   180.bmp\n");
#endif

		}
		else
			break;
//		cut_image(unsigned char *src_pic,unsigned char*des_pic,int start_row,int start_col)

		printf("-----------end--------------\n");
		sleep(2);
	}

	free(registerData);
	registerData = NULL;
	free(usr_addr);
	usr_addr = NULL;
	free(pic_data);
	pic_data = NULL;
	free(ROI_pic);
	ROI_pic = NULL;
	pthread_exit(NULL);
}




