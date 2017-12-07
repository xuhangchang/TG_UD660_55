#ifndef TG_MAIN_H 
#define TG_MAIN_H 

//#define HID_DEV_H3
//#define HID_DEV_JZ
#define NET_DEV_H3

//#define WATCHDOG
#define DEV_NAME "UD660"
#define SAVE_PIC


/*****************图像尺寸参数******************/
#define CAM_9V034
#define IMAGE_SIZE 161280 	//280*576
#define CAMERA_WIDTH 576
#define CAMERA_HEIGHT 280
#define CAMERA_ROI_WIDTH	425
#define CAMERA_ROI_HEIGHT	180	
#define CUT_ROW_START 90
#define CUT_COL_START 74

/*****************dev position********************/
//	DEV					ROW		COL
//  original			40		40
//	Z1					90		90
//	Z2					10		20->40
//	Z3					50		80
//	Z4
//-------------------------------------------------
//	F1					15		70
//	F2					65		54
//	F3					
//	F4
//
/*************************************************/
#define TZD_LENGTH 1296
#define SPI_CRYPT_LENGTH 5136
#define FEED_WATCHDOG_TIME 5



/*****************红外灯板调节参数******************/
#define LIGHT_INIT 40
#define LIGHT_STEP 2
#define GRAY_LOW 95
#define GRAY_HIGH 120

/*********************spi参数**********************/
#define TG_SPI_Z32
//#define TG_SPI_FPGA
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define CRC_DFE_POLY    0x8005	
#define SPI_DEV "/dev/spidev0.0"
//#define SPI_SPEED 10000000  //max =10MHz,10000008=wrong   z32
#define SPI_SPEED 5000000  //max =10MHz,10000008=wrong   z32

#define TG_DEBUG
//#define REG_DEBUG
#define TG_SPI_DEBUG 




#endif  
