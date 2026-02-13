#include "rain.h"


bool rain_flag = 0;

uint16_t rain_vlaue = 0;


/**
 * @brief  读取雨滴传感器的原始 ADC 值
 * @retval 0 ~ 4095
 */
uint16_t Rain_Read_Raw(void)
{
    uint16_t adc_value = 0;

    // 1. 启动 ADC
    HAL_ADC_Start(&hadc2);

    // 2. 等待转换完成 (超时时间 10ms)
    if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK)
    {
        // 3. 获取数值
        adc_value = HAL_ADC_GetValue(&hadc2);
    }

    // 4. 停止 ADC (省电)
    HAL_ADC_Stop(&hadc2);

    return adc_value;
}



void rain_task(void)
{
    rain_vlaue = Rain_Read_Raw();
    
    // 如果读到的值大于阈值，说明电阻变小了（有水导通了）
    if (rain_vlaue > RAIN_THRESHOLD)
    {
        rain_flag = 1; // 有雨
    }
    else
    {
        rain_flag = 0; // 无雨
    }
}


