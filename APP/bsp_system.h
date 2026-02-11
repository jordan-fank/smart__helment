#ifndef _BSP_SYSTEM_H
#define _BSP_SYSTEM_H




/*----------------------------------------标准库引用-----------------------------------------*/
#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"
#include "math.h"



/*---------------------------------------硬件层引用--------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

#include "adc.h"
#include "dma.h"

#include "tim.h"

#include "i2c.h"

/*---------------------------------------应用层引用--------------------------------------------*/
#include "sys.h"

#include "scheduler.h"
#include "mq2.h"
#include "dht11.h"
#include "delay.h"



#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "dmpKey.h"
#include "dmpmap.h"


#include "soft_iic.h"
#include "max30102_control.h"
#include "algorithm.h"
#include "max30102.h"




#include "atgm336h.h"
#include "ringbuffer.h"



#include "OLED.h"
#include "oled_app.h"

#include "asrpro.h"

#include "sensor.h"




//外部引用的变量
extern float temp;		//温度
extern float humi;		//湿度

extern float ppm;		//浓度
extern const unsigned char BMP_Photo[];	//显示图片


extern uint8_t dis_hr;   // 显示的心率值
extern uint8_t dis_spo2; // 显示的血氧值

extern bool heartrate_flag;

extern bool fall_flag;			//摔倒标志
extern bool collision_flag;		//碰撞标志

extern float longitude; // 经度
extern float latitude;  // 纬度

extern uint8_t oled_page;


extern bool fall_flag;			//摔倒标志
extern bool collision_flag;		//碰撞标志

extern bool alarm_flag;

extern bool mq2_flag;

extern bool heartrate_flag;
extern bool spo2_flag;

extern bool temp_flag;



#endif
