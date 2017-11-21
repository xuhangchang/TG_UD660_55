#include "tg_image_adaptive.h"
#include "tg_main.h"


//test acquire camera_data
//data length 425*180
//just test
void acquire_camera_data(int fd,unsigned char *data,int cut_row,int cut_col)
{
	int i,j,k;
	pwm_send(fd,0,tg_light);
	printf("tg_light = %d\n",tg_light);
	usleep(100000);
	for(k= 0 ;k<1;k++)
	{
		printf("k = %d\n",k);
		camera_flag = 1;
		while(camera_flag)
		{
			usleep(10);
		}
		tg_image_gray(camera_data);
	}
	//src pic 280*576
	cut_image(camera_data,data,cut_row,cut_col);
	pwm_send(0,0,255);
}


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

//gain the best tg_light 6 times
//data 425*180
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
		if(gray > 115)
		{
			if(tg_light <= 6)
				tg_light = 6;
			else
				tg_light = tg_light - 2;
		}
		else if(gray < 95)
		{
			if(tg_light >=250)
				tg_light = 250;
			else
				tg_light = tg_light + 2;
		}
		else 
			break;
	}
	camera_flag = 1;
	while(camera_flag)
		usleep(10);
//	tg_image_find_edge_9V034(camera_data);
	cut_image(camera_data,data,first_cut_row,first_cut_col);
	pwm_send(fd,0,0);
}

void tg_image_find_edge_7725(unsigned char *camera_data)
{
	int i , j;
	int bright_count3 = 0;
	int count = 0;	
	int min_row = 278,min_col = 138;
	int first_bright_row = 0;
	unsigned char a[CAMERA_HEIGHT][CAMERA_WIDTH];
	int row = CAMERA_HEIGHT;
	int col = CAMERA_WIDTH;

	for(i = 0;i<row;i++)
	{
		bright_count3 = 0;
		for(j = 0;j<col;j++)
		{
			*(*(a+i)+j) = camera_data[i*col+j];
		
			if(camera_data[i*col+j] > 0xFE)	
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
	first_cut_row = min_row+10+15;
	first_cut_col = min_col;

	if(first_cut_row >278)
		first_cut_row = 278;
	if(first_cut_col > 138)
		first_cut_col = 138;

}

//gain the best tg_light 6 times
//data 200*500
void tg_image_adaptive_7725(int fd,unsigned char *data)
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
		tg_light = 20;

	for(i = 0;i < 7;i++)
	{
		printf("adjust = %d, light = %d\n",i,tg_light);
		pwm_send(fd,0,tg_light,255);
		usleep(10000);
	
		camera_flag = 1;
		while(camera_flag)
			usleep(10);
		gray = tg_image_gray(camera_data);
		if(gray > 115)
		{
			if(tg_light <= 6)
				tg_light = 6;
			else
				tg_light = tg_light - 5;
		}
		else if(gray < 105)
		{
			if(tg_light >=250)
				tg_light = 250;
			else
				tg_light = tg_light + 5;
		}
		else 
			break;
	}
	camera_flag = 1;
	while(camera_flag)
		usleep(10);
//	tg_image_find_edge_7725(camera_data);
	cut_image(camera_data,data,first_cut_row,first_cut_col);
	pwm_send(fd,0,0,255);
}


//acquire roi gray_avge 7725/9v034
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

	for(i = row_start ; i < row_start+row_size ;i++)
	{
		for(j = col_start ; j < col_start+col_size ;j++)
		{
			sum += *(array_data+(i*src_col + j));
		}
	}
	gray_avge = sum/(row_size*col_size);
	printf("gray_avge = %d\n",gray_avge);
	return gray_avge;
}




//9V034  cut_image 280*576 -> 180*425, get des_pic
//7725  cut_image 480*640 -> 200*500, get des_pic
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



