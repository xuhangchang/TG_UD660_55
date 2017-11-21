#ifndef TG_MAIN_H 
#define TG_MAIN_H 

//#define CAM_7725
#define CAM_NEW

#ifdef CAM_7725
#define IMAGE_SIZE 307200 	//480*640
#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480
#define CAMERA_ROI_WIDTH	500
#define CAMERA_ROI_HEIGHT         220	
#define CUT_ROW_START 150
#define CUT_COL_START 50

#else
#define CAM_9V034
#define IMAGE_SIZE 161280 	//280*576
#define CAMERA_WIDTH 576
#define CAMERA_HEIGHT 280
#define CAMERA_ROI_WIDTH	425
#define CAMERA_ROI_HEIGHT	180	
#define CUT_ROW_START 90
#define CUT_COL_START 74
#endif

#define TZD_LENGTH 1296
#define SPI_CRYPT_LENGTH 5136
#define FEED_WATCHDOG_TIME 5

//#define WATCHDOG
#define DEV_NAME "TG-660-H3"
//#define SAVE_PIC

/*****************spi******************/
//#define TG_SPI
//#define TG_SPI_DEBUG 
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define CRC_DFE_POLY    0x8005	
#define SPI_DEV "/dev/spidev0.0"
//#define SPI_SPEED 10000000  //max =10MHz,10000008=wrong 
#define SPI_SPEED 5000000  //max =10MHz,10000008=wrong 



#endif  
