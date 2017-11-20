#ifndef BMP_LOADSAVE_H   
#define BMP_LOADSAVE_H   
#include <stdint.h>
#define bool int
#define true 1
#define false 0
//文件信息头  
typedef struct  
{  
    //unsigned short    bfType; //bfType代表文件格式,就是“BM”，用十六进制的数表示是0x4d42 
    uint32_t    bfSize;  //bfSizebfSize代表的是该位图文件的大小（包含文件头、信息头、调色板（如果有）、像素数据）。占4个字节
    uint16_t    bfReserved1;//bfReserved1和bfReserved2都是保留量，因此它们应该都为0  
    uint16_t    bfReserved2;  
    uint32_t    bfOffBits;//bfOffBits代表的是像素数据距离文件头的位置，也就是偏移地址  
} BitMapFileHeader;  
//位图信息头  
typedef struct  
{  
    uint32_t  biSize;//biSize代表的是该结构体的大小，即40字节   
    uint32_t   biWidth;//biWidth代表的是位图的宽度   
    uint32_t   biHeight;//biHeight代表的是位图的高度   
    uint16_t   biPlanes; //biPlanes代表的是位图的平面数  
    uint16_t   biBitCount; //biBitCount代表的是位图的位数，有1、16、8、24、32等
    uint32_t  biCompression;//biCompression代表的是位图像素数据的大小，由于BMP位图是没有压缩的，所以值为0  
    uint32_t  biSizeImage;//biSizeImage代表的是位图像素数据的大小，即高度*每行像素所占的字节数   
    uint32_t   biXPelsPerMeter;   
    uint32_t   biYPelsPerMeter;   
    uint32_t   biClrUsed;   
    uint32_t   biClrImportant;   
} BitMapInfoHeader;  
//RGB颜色阵列 
typedef struct   
{  
    unsigned char rgbBlue; //该颜色的蓝色分量   
    unsigned char rgbGreen; //该颜色的绿色分量   
    unsigned char rgbRed; //该颜色的红色分量   
    unsigned char rgbReserved; //保留值   
} RgbQuad;  
//图像数据  
typedef struct  
{  
    int width;  
    int height;  
    int channels;  
	//unsigned char * imageData;
    unsigned char ** imageData;  
} Image;  

void TG_LoadImage(Image*bmpImg,char* path);
int TG_SaveImage(char* path, Image* bmpImg);  

void bmp_write_data_hex(unsigned char * my_array,int length,char *string);
void bmp_read_data_hex(unsigned char *buf,int length,char *string);
unsigned char **bmp_Make2DArray_uint8(int row,int col);
void bmp_Free2DArray_uint8(unsigned char **a,int row);

void TG_SaveBmp(unsigned char *src,int height,int width,char *bmp_addr);

void TG_SaveRotateBmp(unsigned char *src, int height, int width, char *bmp_addr);


#endif   
