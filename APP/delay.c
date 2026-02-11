#include "delay.h"


void delay_init(void)
{
	HAL_TIM_Base_Start(&htim2);
}


/**
 * @brief  微秒级延时函数
 * @param  us: 延时时长 (单位: 微秒)
 */
void delay_us(uint16_t us)
{
    // 1. 获取当前计数值作为起点
    uint16_t start = __HAL_TIM_GET_COUNTER(&htim2);
    
    // 2. 循环检查差值是否达到目标值
    // 使用 (uint16_t) 强制转换可以自动处理溢出回绕逻辑
    while ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) - start) < us);
}



void delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}


void delay_s(uint32_t s)
{
	HAL_Delay(s*1000);
}



