#ifndef MAIN_H 
#define MAIN_H 

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define CRC_DFE_POLY    0x8005	


#define SPI_DEV "/dev/spidev0.0"
#define SPI_SPEED 10000000  //max =10MHz,10000008=wrong   z32
//#define SPI_SPEED 5000000  //max =10MHz,10000008=wrong   a

#define PRINT_SPI 


#define DEBUG 1





#endif  
