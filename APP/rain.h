#ifndef _RAIN_H
#define _RAIN_H

#include "bsp_system.h"


// 这里的阈值需要根据实际情况调试
// ADC是12位的，最大值4095 (对应3.3V)
// 经验值：干燥时约0-100，有小雨滴约1000-2000，大雨/泡水约3000以上
#define RAIN_THRESHOLD  500 

// --- 函数声明 ---
uint16_t Rain_Read_Raw(void); // 读取原始ADC值 (用于调试)
void rain_task(void);     // 返回是否下雨 (1:下雨, 0:没雨)

#endif
