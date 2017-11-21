55所H3镜像

UART1  串口终端  有输出
摄像头 9V034
通信方式：网络
H3为圣点定制板子
55所调试的设备，660结构。

烧写镜像：（由于原版的h3只有.img，所以需要重新烧写emmc）
烧写的镜像在sd卡emmc目录
步骤：
mount /dev/mmcblk0p3 /mnt/sd
yes | mkfs.ext4 /dev/mmcblk1
./write_emmc.sh /dev/mmcblk1
重新上电，扩展
e2fsck -y -f /dev/mmcblk1p2
resize2fs /dev/mmcblk1p2




2017/11/2
