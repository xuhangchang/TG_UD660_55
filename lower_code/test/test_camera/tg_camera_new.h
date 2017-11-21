#ifndef TG_CAMERA_H   
#define TG_CAMERA_H 

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define u8 unsigned char
#define  LOGD(...)  do {printf(__VA_ARGS__);printf("\n");} while (0)
#define DBG(fmt, args...) LOGD("%s:%d, " fmt, __FUNCTION__, __LINE__, ##args);
#define ASSERT(b) \
do \
{ \
    if (!(b)) \
    { \
        LOGD("error on %s:%d", __FUNCTION__, __LINE__); \
        return 0; \
    } \
} while (0)

#define VIDEO_DEVICE "/dev/video0"
#define BUFFER_COUNT 25

//int cam_fd = -1;
struct v4l2_buffer video_buffer[BUFFER_COUNT];
u8* video_buffer_ptr[BUFFER_COUNT];

int cam_open();

int cam_close();

int cam_select(int index);

int cam_init();

int cam_get_image(u8* out_buffer, int out_buffer_size);

void  YUV2Y(unsigned char * yuyv,int w,int h,unsigned char *y);

#endif   
