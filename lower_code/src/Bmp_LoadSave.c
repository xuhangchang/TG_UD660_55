/*ÍêÕûµÄbmpÎ»Í¼ÎÄ¼ş£¬¿ÉÒÔ·ÖÎªÎÄ¼şĞÅÏ¢Í·£¬Î»Í¼ĞÅÏ¢Í·ºÍRGBÑÕÉ«ÕóÁĞÈı¸ö²¿·Ö¡£
ÎÄ¼şĞÅÏ¢Í·Ö÷Òª°üº¬¡°ÊÇ·ñÊÇBMPÎÄ¼ş¡±£¬ÎÄ¼şµÄ´óĞ¡µÈĞÅÏ¢¡£¶øÎ»Í¼ĞÅÏ¢Í·ÔòÖ÷Òª
°üº¬bmpÎÄ¼şµÄÎ»Í¼¿í¶È£¬¸ß¶È£¬Î»Æ½Ãæ£¬Í¨µÀÊıµÈĞÅÏ¢¡£¶øRGBÑÕÉ«ÕóÁĞ£¬ÀïÃæ²Å
ÕæÕı°üº¬ÎÒÃÇËùĞèÒªµÄbmpÎ»Í¼µÄÏñËØÊı¾İ¡£ĞèÒªÌáĞÑµÄÊÇ£¬bmpÎ»Í¼µÄÑÕÉ«ÕóÁĞ²¿
·Ö£¬ÏñËØÊı¾İµÄ´æ´¢ÊÇÒÔ×óÏÂ½ÇÎªÔ­µã¡£Ò²¾ÍÊÇËµ£¬µ±Äã´ò¿ªÒ»¸öbmpÍ¼Æ¬²¢ÏÔÊ¾ÔÚ
µçÄÔÆÁÄ»ÉÏµÄÊ±£¬Êµ¼ÊÔÚ´æ´¢µÄÊ±ºò£¬Õâ¸öÍ¼Æ¬µÄ×î×óÏÂ½ÇµÄÏñËØÊÇÊ×ÏÈ±»´æ´¢ÔÚ
bmpÎÄ¼şÖĞµÄ¡£Ö®ºó£¬°´ÕÕ´Ó×óµ½ÓÒ£¬´ÓÏÂµ½ÉÏµÄË³Ğò,ÒÀ´Î½øĞĞÏñËØÊı¾İµÄ´æ´¢¡£
Èç¹û£¬Äã´æ´¢µÄÊÇ3Í¨µÀµÄÎ»Í¼Êı¾İ£¨Ò²¾ÍÊÇÎÒÃÇÍ¨³£ËµµÄ²ÊÍ¼£©£¬ÄÇÃ´ËüÊÇ°´ÕÕ
B0G0R0B1G1R1B2G2R2...µÄË³Ğò½øĞĞ´æ´¢µÄ£¬Í¬Ê±£¬»¹Òª¿¼ÂÇµ½4×Ö½Ú¶ÔÆëµÄÎÊÌâ¡£
*/

#include <stdio.h>   
#include <stdlib.h>  
#include "Bmp_LoadSave.h" 


/*****************************************************************  
* function:		TG_LoadImage
* description:  è¯»å–å›¾åƒ
* param1:     	Image* bmpImg	: å›¾åƒæ•°æ® (output)
* param2:     	char* path		: è¯»å–è·¯å¾„ (input)
* return:    	void 			               
* other:
*
* date:       	2017/11/09			
* author:     	
******************************************************************/ 
void TG_LoadImage(Image* bmpImg,char* path)  
{  
  //Image* bmpImg;  
    FILE* pFile;  
    uint16_t fileType;  
    BitMapFileHeader bmpFileHeader;  
    BitMapInfoHeader bmpInfoHeader;  
    int channels = 1;  
    int width = 0;  
    int height = 0;  
    int step = 0;  
    int offset = 0;  
    unsigned char pixVal;  
    RgbQuad* quad;  
    int i, j;  
	//int k;
  
    pFile = fopen(path, "rb"); //¶ÁĞ´´ò¿ªÒ»¸ö¶ş½øÖÆÎÄ¼ş£¬Ö»ÔÊĞí¶ÁĞ´Êı¾İ;ÎÄ¼şË³Àû´ò¿ªºó£¬Ö¸Ïò¸ÃÁ÷µÄÎÄ¼şÖ¸Õë¾Í»á±»·µ»Ø¡£
	                           // Èç¹ûÎÄ¼ş´ò¿ªÊ§°ÜÔò·µ»ØNULL£¬²¢°Ñ´íÎó´úÂë´æÔÚerrno ÖĞ 
    if (!pFile)  
    {  
        free(bmpImg);  
//		printf("Í¼Æ¬´ò¿ªÊ§°Ü£¬¼ì²éÂ·¾¶£¡£¡£¡\n");
 		bmpImg = NULL;
		exit(0);
    }  
  
    fread(&fileType, sizeof(uint16_t), 1, pFile);//1.ÓÃÓÚ½ÓÊÕÊı¾İµÄÄÚ´æµØÖ·
	                                                   //2.Òª¶ÁÈ¡µÄÃ¿¸öÊı¾İÏîµÄ×Ö½ÚÊı£¬µ¥Î»ÊÇ×Ö½Ú
	                                                   //3.Òª¶Ácount¸öÊı¾İÏî£¬Ã¿¸öÊı¾İÏîsize¸ö×Ö½Ú
	                                                   //4.ÊäÈëÁ÷
                                                       //·µ»ØÖµ£ºÊµ¼Ê¶ÁÈ¡µÄÔªËØ¸öÊı
	if (fileType == 0x4D42)  
    {  
        //printf("bmp file! \n");   
  
        fread(&bmpFileHeader, sizeof(BitMapFileHeader), 1, pFile);  

        fread(&bmpInfoHeader, sizeof(BitMapInfoHeader), 1, pFile);  
  
        if (bmpInfoHeader.biBitCount == 8)  
        {  
            channels = 1;  
            width = bmpInfoHeader.biWidth;  
            height = bmpInfoHeader.biHeight;  
            offset = (channels*width)%4; //??? 
            if (offset != 0)  
            {  
                offset = 4 - offset;  
            }  
            bmpImg->width = width;  
            bmpImg->height = height;  
            bmpImg->channels = 1;  

            step = channels*width; //¿í¶È 
  
            quad = (RgbQuad*)malloc(sizeof(RgbQuad)*256);  
            fread(quad, sizeof(RgbQuad), 256, pFile);  
            free(quad);  
			quad = NULL;
            //¶ÁÈëÏñËØÊı¾İ±£´æÔÚimageData
            for (i=0; i<height; i++)  
            {  
                for (j=0; j<width; j++)  
                {  
                    fread(&pixVal, sizeof(unsigned char), 1, pFile);  
                    //bmpImg->imageData[(height-1-i)*step+j] = pixVal; 
					*(*(bmpImg->imageData+(height-1-i))+j) = pixVal;
                }  
                if (offset != 0)  
                {  
                    for (j=0; j<offset; j++)  
                    {  
                        fread(&pixVal, sizeof(unsigned char), 1, pFile);  
                    }  
                }  
            }             
        }  
     
    }  
	
	fclose(pFile);
}  


 /*****************************************************************  
 * function:	 TG_SaveImage
 * description:  ä¿å­˜å›¾åƒ
 * param1:		 char* path 	 : ä¿å­˜è·¯å¾„ (input)
 * param2:		 Image* bmpImg	 : å›¾åƒæ•°æ® (input)
 * return:		 void						 -	 
 * other:
 *
 * date:		 2017/11/09 		 
 * author:		 
 ******************************************************************/ 
 int TG_SaveImage(char* path, Image* bmpImg)  
 {  
     FILE *pFile;  
     uint16_t fileType;  
     BitMapFileHeader bmpFileHeader;  
     BitMapInfoHeader bmpInfoHeader;  
     int step;  
     int offset;  
     unsigned char pixVal = '\0';  
     int i, j;  
     RgbQuad* quad;  
   
     pFile = fopen(path, "wb");  
     if (!pFile)  
     {  
         return 0;  
     }  
   
     fileType = 0x4D42; // "BM"£¬ÓÃÊ®Áù½øÖÆµÄÊı±íÊ¾ÊÇ0x4d42
     fwrite(&fileType, sizeof(uint16_t), 1, pFile);  
   
    if (bmpImg->channels == 1)//8Î»£¬µ¥Í¨µÀ£¬»Ò¶ÈÍ¼   
     {  
         step = bmpImg->width;  
         offset = step%4;  
         if (offset != 4)  
         {  
             step += 4-offset;  
         }  
   
         bmpFileHeader.bfSize = 54 + 256*4 + bmpImg->width;  
         bmpFileHeader.bfReserved1 = 0;  
         bmpFileHeader.bfReserved2 = 0;  
         bmpFileHeader.bfOffBits = 54 + 256*4;  
         fwrite(&bmpFileHeader, sizeof(BitMapFileHeader), 1, pFile);  
   
         bmpInfoHeader.biSize = 40;  
         bmpInfoHeader.biWidth = bmpImg->width;  
         bmpInfoHeader.biHeight = bmpImg->height;  
         bmpInfoHeader.biPlanes = 1;  
         bmpInfoHeader.biBitCount = 8;  
         bmpInfoHeader.biCompression = 0;  
         bmpInfoHeader.biSizeImage = bmpImg->height*step;  
         bmpInfoHeader.biXPelsPerMeter = 0;  
         bmpInfoHeader.biYPelsPerMeter = 0;  
         bmpInfoHeader.biClrUsed = 256;  
         bmpInfoHeader.biClrImportant = 256;  
         fwrite(&bmpInfoHeader, sizeof(BitMapInfoHeader), 1, pFile);  
   
         quad = (RgbQuad*)malloc(sizeof(RgbQuad)*256);  
         for (i=0; i<256; i++)  
         {  
             quad[i].rgbBlue = i;  
             quad[i].rgbGreen = i;  
             quad[i].rgbRed = i;  
             quad[i].rgbReserved = 0;  
         }  
         fwrite(quad, sizeof(RgbQuad), 256, pFile);  
         free(quad);  
   
         for (i=bmpImg->height-1; i>-1; i--)  
//		 for (i=0; i < bmpImg->height; i++) 
         {  
             for (j=0; j<bmpImg->width; j++)  
             {  
                 //pixVal = bmpImg->imageData[i*bmpImg->width+j];
 				pixVal = *(*(bmpImg->imageData+i)+j);
                 fwrite(&pixVal, sizeof(unsigned char), 1, pFile);  
             }  
             if (offset!=0)  
             {  
                 for (j=0; j<offset; j++)  
                 {  
                     pixVal = 0;  
                     fwrite(&pixVal, sizeof(unsigned char), 1, pFile);  
                 }  
             }  
         }  
     }  
     fclose(pFile);  
   
     return 1;  
 } 


/*****************************************************************	
* function: 	bmp_write_data_hex
* description:	æ•°æ®ä¿å­˜åˆ°ä¸€ä¸ªæ–‡ä»¶
* param1:		unsigned char * my_array : æ•°æ® (input)
* param2:		int length				 : æ•°æ®é•¿åº¦ (input)
* param3:		char *string			 : ä¿å­˜è·¯å¾„(input)
* return:		void							
* other:
*
* date: 		2017/11/09			
* author:		
******************************************************************/ 
void bmp_write_data_hex(unsigned char * my_array,int length,char *string)
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
 

/*****************************************************************	
* function: 	bmp_read_data_hex
* description:	è¯»å–ä¸€ä¸ªæ–‡ä»¶çš„æ•°æ®
* param1:		unsigned char * buf		 : æ•°æ® (output)
* param2:		int length				 : æ•°æ®é•¿åº¦ (input)
* param3:		char *string			 : è¯»å–è·¯å¾„(input)
* return:		void						
* other:
*
* date: 		2017/11/09			
* author:		
******************************************************************/ 
void bmp_read_data_hex(unsigned char *buf,int length,char *string)
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

/*****************************************************************	
* function: 	bmp_Make2DArray_uint8
* description:	å¼€è¾Ÿå¯¹åº”ç©ºé—´çš„äºŒçº§æŒ‡é’ˆ
* param1:		int row		 : è¡Œæ•° (input)
* param2:		int col		 : åˆ—æ•° (input)
* return:		unsigned char **ï¼šå¼€è¾Ÿçš„äºŒçº§æŒ‡é’ˆ						
* other:
*
* date: 		2017/11/09			
* author:		
******************************************************************/ 
unsigned char **bmp_Make2DArray_uint8(int row,int col){
	unsigned char **a;
	int i;
	a=(unsigned char **)calloc(row,sizeof(unsigned char *));
	for(i=0;i<row;i++)
	{
		a[i]=(unsigned char *)calloc(col,sizeof(unsigned char));
	}
	return a;

}

/*****************************************************************	
* function: 	bmp_Free2DArray_uint8
* description:	é‡Šæ”¾äºŒçº§æŒ‡é’ˆ
* param1:		unsigned char **a		 : éœ€è¦é‡Šæ”¾çš„äºŒçº§æŒ‡é’ˆ (input)
* param2:		int row					 : è€³æœºæŒ‡é’ˆè¡Œæ•° (input)
* return:		void						
* other:
*
* date: 		2017/11/09			
* author:		
******************************************************************/ 
void bmp_Free2DArray_uint8(unsigned char **a,int row){
	int i;
	for(i=0;i<row;i++)
	{
		free(a[i]);
		a[i] = NULL;
	}
	free(a);
	a = NULL;
}
 
 
 
/*****************************************************************	
* function: 	TG_SaveBmp
* description:	ä¿å­˜å›¾åƒï¼ˆç›¸å¯¹äºTG_SaveImageï¼Œå°è£…äº†ä¸€å±‚ï¼‰
* param1:		unsigned char *src		 : åŸå§‹å›¾åƒæ•°æ® (input)
* param2:		int height				 : å›¾åƒé«˜åº¦ (input)
* param3:		int width				 : å›¾åƒå®½åº¦ (input)
* param4:		char *bmp_addr			 : ä¿å­˜å›¾åƒçš„è·¯å¾„ (input)
* return:		void						
* other:
*
* date: 		2017/11/09			
* author:		
******************************************************************/ 
void TG_SaveBmp(unsigned char *src,int height,int width,char *bmp_addr)
{
	int i,j,k;
	Image *myfingerImg;
	myfingerImg = (Image *)malloc(sizeof(Image));
	myfingerImg->channels = 1;
	myfingerImg->height = height;
	myfingerImg->width = width;
	myfingerImg->imageData = bmp_Make2DArray_uint8(height,width);
	
	k = 0;
	for(i = 0;i<height;i++)
	{
		for(j = 0;j<width;j++)
		{
			*(*(myfingerImg->imageData+i)+j) = src[k++];
		}
	}
	TG_SaveImage(bmp_addr,myfingerImg) ;
	bmp_Free2DArray_uint8(myfingerImg->imageData,height);
	free(myfingerImg);
	myfingerImg = NULL;
}

 
/*****************************************************************	
* function: 	TG_SaveRotateBmp
* description:	ä¿å­˜æ—‹è½¬åçš„å›¾åƒï¼ˆç›¸å¯¹äºTG_SaveImageï¼Œå°è£…äº†ä¸€å±‚ï¼‰
* param1:		unsigned char *src		 : åŸå§‹å›¾åƒæ•°æ® (input)
* param2:		int height				 : ä¿å­˜åçš„å›¾åƒé«˜åº¦ (input)
* param3:		int width				 : ä¿å­˜åçš„å›¾åƒå®½åº¦ (input)
* param4:		char *bmp_addr			 : ä¿å­˜å›¾åƒçš„è·¯å¾„ (input)
* return:		void						
* other:
*
* date: 		2017/11/09			
* author:		
******************************************************************/ 
void TG_SaveRotateBmp(unsigned char *src, int height, int width, char *bmp_addr)
{
	int i,j,k;
	Image *myfingerImg;
	myfingerImg = (Image *)malloc(sizeof(Image));
	myfingerImg->channels = 1;
	myfingerImg->height = height;
	myfingerImg->width = width;
	myfingerImg->imageData = bmp_Make2DArray_uint8(height,width);
	
	k = 0;

	for (j = 0;j < width;j++)
	{
		for (i = height - 1; i >= 0; i--)
		{
			*(*(myfingerImg->imageData + i) + j) = src[k++];
		}
	}
	TG_SaveImage(bmp_addr,myfingerImg) ;
	bmp_Free2DArray_uint8(myfingerImg->imageData,height);
	free(myfingerImg);
	myfingerImg = NULL;

}



