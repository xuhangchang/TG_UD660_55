#ifndef RTX_H 
#define RTX_H 

#define BUFSIZE 1024*1024*16
#define PACKAGE_SIZE 1024

#define PORT 3333
#define SERVICE_ADDR "192.168.1.111"


typedef struct {
	char	device_name[32];	//设备名
	int 	id;					//ID
	int		cmd1;				//指令1
	int		cmd2;				//指令2
	int 	length;				//数据长度
	int		cert_type;			//证书类型
	int		sup_mgr_num;		//本地超管证书个数
	int		mgr_num;			//本地管理员证书个数
	int		usr_num;			//本地用户证书个数
	char 	reserve_char_32[32];//预留备用
	char	serial_num[32];		//序列号
	char	ip_addr[32];		//IP地址
	char	mac_addr[32];		//MAC地址
	char	md5[32];			//数据计算得到的MD5
	char	random_num[32];		//随机数
	char	cert_name[32];		//证书名字
	char 	reserve_char[736];	//预留备用
}TG_package;//指令包


typedef struct {
	char	user_name[32];				//用户名
	int 	user_id;				//用户ID
	int 	cert_type;				//证书类型
	char	reserve[24];			//预留 考虑字节对齐
	char 	cert_valid_date[32];		//证书有效期
	unsigned char chara[4096];			//三根手指特征点 3*1296=3888 预留4k
	char		key[20*96];				//动态加密密钥+打包加密密钥  (96+96)*10
	char	crc[2];						//crc
}TG_cert;//证书；6116B	//因为有int类型，所以按4字节对齐，此结构体大小就为6116


typedef struct {
	char	dev_key[32];				//设备密钥
	char	crc[2];						//crc
}TG_dev_key;//设备密钥；34B


typedef struct {
	unsigned char enroll_chara[4096];
	char new_reg_path[50];					//crc
}TG_enroll_data;//new user data；


extern int wd_fd;

int writeComm(int fd, char* buf, int len);

int readComm(int fd, char* buf, int len, int timeout );

int Compute_string_md5(unsigned char *dest_str, unsigned int dest_len, char *md5_str);

int file_size(char* filename) ;

int sendPackage(int fd,TG_package *pack);

int sendDataPackage(int fd,char *data,int len);

int recvPackage(int fd,TG_package *pack,int timeout);

int recvDataPackage(int fd,char *data,int len,int timeout);

int init_watchdog(int timeout);

int feed_watchdog();

int release_watchdog();

int TG_NetSendPackage(int fd,TG_package* pack,char* buf);

int TG_NetRecvPackage(int fd,TG_package* pack,char* buf);

int net_client_connect(char *addr,int port);



#endif  
