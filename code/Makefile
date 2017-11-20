# 指令编译器和选项
#CC = arm-linux-gnueabihf-gcc
CC = gcc
CFLAGS=-Wall -std=gnu99

# 目标文件
TARGET=main.out
# 源文件
SRCS = 	main.c \
		rtx.c \
		fun.c \
		gpio.c \
		net.c \
		tg_camera_new.c \
		tg_image_adaptive.c \
		Bmp_LoadSave.c \
		tg_image_adaptive.c \
		tg_spi.c
		
		
		
# 头文件查找路径
INC = -I../inc

# 库文件和库查找路径  
DLIBS = -lpthread \
		-lTGFingerVeinApi 

LDFLAGS = -L/home/fa/so 
# 指定运行时的库文件路径 
RPATH = -Wl,-rpath=/home/fa/so 
# 目标文件
OBJS = $(SRCS:.c=.o)
# 链接为可执行文件
$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(DLIBS) $(RPATH) -lm -O3
#	@echo TARGET:$@
#	@echo OBJECTS:$^
#	$(CC) -o $@ $^

clean:
	rm -rf $(TARGET) $(OBJS)

# 连续动作，请清除再编译链接，最后执行
exec:clean $(TARGET)
	@echo 开始执行
	./$(TARGET)
	@echo 执行结束

# 编译规则 $@代表目标文件 $< 代表第一个依赖文件
%.o:%.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $< -O3
