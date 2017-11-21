#ifndef TG_IMAGE_ADAPTIVE_H 
#define TG_IMAGE_ADAPTIVE_H

#include <stdio.h>  
#include <string.h>  
#include <unistd.h>

#include <errno.h>  
#include <stdlib.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <time.h>  
#include <sys/mman.h>  
#include <assert.h>  
#include <linux/videodev2.h>  
#include <linux/fb.h>  
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include "tg_main.h"
#include "fun.h"

extern unsigned char camera_data[CAMERA_HEIGHT*CAMERA_WIDTH];
extern int camera_flag;
extern int tg_light;
extern int first_cut_row ;
extern int first_cut_col ;

void tg_image_find_edge_9V034(unsigned char *data);

void tg_image_adaptive_9V034(int fd,unsigned char *data);

int tg_image_gray(unsigned char *array_data);

void cut_image(unsigned char *src_pic,unsigned char*des_pic,int start_row,int start_col);

#endif  
