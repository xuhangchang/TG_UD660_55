#include "tg_image_adaptive.h"
#include "tg_main.h"

void tg_image_find_edge_9V034(unsigned char *data)
{
	int i , j;
	int bright_count3 = 0;
	int count = 0;	
	int min_row = 98,min_col = 150;
	int first_bright_row = 0;
	unsigned char a[280][576];
	int row = 280;
	int col = 576;

	for(i = 0;i<row;i++)
	{
		bright_count3 = 0;
		for(j = 0;j<col;j++)
		{
			*(*(a+i)+j) = data[i*col+j];
		
			if(data[i*col+j] > 0xFE)	
			{
				if(0 == count)
					first_bright_row = i;
				count++;
			}
			if((*(*(a+i)+j) > 80) && min_row != i)
			{
				bright_count3++;
				if(3 == bright_count3)
				{
					min_row = (min_row < i) ? min_row : i;
					min_col = (min_col < j) ? min_col : j;
					bright_count3 = 0;
				}
			}
		}
	}
	printf("count 3000  = %d\n",count);
	if(count >= 3000)
	{
		if(first_bright_row < 100)	
			min_row = first_bright_row;	
	}
	printf("min_row = %d\n",min_row);
	printf("min_col = %d\n",min_col);
	first_cut_row = min_row;
	first_cut_col = min_col;

	if(first_cut_row >99)
		first_cut_row = 100;
	if(first_cut_col > 150)
		first_cut_col = 151;
}


/*****************************************************************
* function:		tg_image_adaptive_9V034
* description:  获取图像+自适应
* param1:     	int fd				:	摄像头设备文件描述符	(input)
* param2:		unsigned char *data:	图像数据(input)
* return:    	0   : 成功
*				其他: 失败
* others:		
* date:       	2017/11/09			
* author:     	
******************************************************************/
void tg_image_adaptive_9V034(int fd,unsigned char *data)
{
	int i,j;
	int gray;

	pwm_send(fd,0,tg_light);
	usleep(200000);
	camera_flag = 1;
	while(camera_flag)
		usleep(10);
	gray = tg_image_gray(camera_data);
	if(gray > 200)
		tg_light = 10;

	for(i = 0;i < 7;i++)
	{
		printf("adjust = %d, light = %d\n",i,tg_light);
		pwm_send(fd,0,tg_light);
		usleep(10000);
	
		camera_flag = 1;
		while(camera_flag)
			usleep(10);
		gray = tg_image_gray(camera_data);
		if(gray > GRAY_HIGH)
		{
			tg_light -= LIGHT_STEP;
			tg_light =  (tg_light<0) ? 0 : tg_light ;
		}
		else if(gray < GRAY_LOW)
		{

			tg_light += LIGHT_STEP;
			tg_light = (tg_light>100) ? 100 : tg_light;
		}
		else 
			break;
	}
	camera_flag = 1;
	while(camera_flag)
		usleep(10);
//	tg_image_find_edge_9V034(camera_data);
	first_cut_row = CUT_ROW_START;
	first_cut_col = CUT_COL_START;
	printf("first_cut_row = %d\n",first_cut_row);
	printf("first_cut_col = %d\n",first_cut_col);


	cut_image(camera_data,data,first_cut_row,first_cut_col);
	pwm_send(fd,0,0);
}



//acquire roi gray_avge 7725/9v034

/*****************************************************************
* function:		tg_image_gray
* description:  计算图像灰度平均值
* param1:     	unsigned char *array_data	:	图像数据	(input)
* return:    	灰度平均值
* others:		
* date:       	2017/11/09			
* author:     	
******************************************************************/
int tg_image_gray(unsigned char *array_data)
{
	int gray_avge;
	int sum = 0;
	int i,j;
	int src_row = CAMERA_HEIGHT;
	int src_col = CAMERA_WIDTH;
	
	int row_start = first_cut_row;
	int col_start = first_cut_col;
	
	int row_size = CAMERA_ROI_HEIGHT;
	int col_size = CAMERA_ROI_WIDTH;

	for(i = row_start ; i < row_start+row_size ;i += 2)
	{
		for(j = col_start ; j < col_start+col_size ;j += 2)
		{
			sum += *(array_data+(i*src_col + j));
		}
	}
	gray_avge = sum/(row_size*col_size/4);
	printf("gray_avge = %d\n",gray_avge);
	return gray_avge;
}




//9V034  cut_image 280*576 -> 180*425, get des_pic
//7725  cut_image 480*640 -> 200*500, get des_pic
/*****************************************************************
* function:		cut_image
* description:  裁剪图像
* param1:     	unsigned char *src_pic	:	原始图像数据	(input)
* param2:     	unsigned char *des_pic	:	裁剪完图像数据	(input)
* param3:     	int start_row	:	裁剪起始行	(input)
* param4:     	int start_col	:	裁剪起始列	(input)


* return:    	void
* others:		
* date:       	2017/11/09			
* author:     	
******************************************************************/
void cut_image(unsigned char *src_pic,unsigned char*des_pic,int start_row,int start_col)
{
	int src_row = CAMERA_HEIGHT;
	int src_col = CAMERA_WIDTH;
	int des_row = CAMERA_ROI_HEIGHT;
	int des_col = CAMERA_ROI_WIDTH;
	
/*	printf("src_row = %d\n",src_row);
	printf("src_col = %d\n",src_col);
	printf("des_row = %d\n",des_row);
	printf("des_col = %d\n",des_col);
	printf("start_row = %d\n",start_row);
	printf("start_col = %d\n",start_col);
*/	
	int i,j,k = 0;
	for(i = start_row;i < start_row+des_row;i++)
	{
		for(j = start_col;j < start_col+des_col;j++)
		{
			des_pic[k++] = src_pic[i*src_col+j];
		}	
	}
}	



