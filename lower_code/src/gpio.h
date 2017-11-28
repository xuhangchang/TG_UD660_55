#ifndef GPIO_H 
#define GPIO_H 

void sound_send(int fd,char sound_cmd);

void pwm_send(int fd,char model,int duty_pct);

int touch_signal(int fd,int flag);

int detect_remove(int fd);


#endif  
