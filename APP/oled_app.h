#ifndef _OELD_APP_H
#define _OELD_APP_H

#include "bsp_system.h"

// 声明欧拉角变量（来自mpu6050.c）
extern float pitch;
extern float roll;
extern float yaw;

// MAX30102_Data 和 max30102_data 已在 max30102.h 中声明
// bsp_system.h 已包含 max30102.h

void oled_task(void);
void oled_init(void);

void OLED_BootAnimation(void);

#endif
