#include "mq2.h"

#define mq2_buffer_len 30

float Rs;
float R0=35.904f;
float RL=4.7f;
float ppm;


float mq2_voltage = 0.0f;
uint32_t mq2_buffer[mq2_buffer_len] = {0};


bool mq2_flag = 0;

/*
	1 ADC轮训采样
	2 ADC+DMA 平均值采样
*/

#define ADC_MODE 2



#if ADC_MODE == 1

void mq2_init(void)
{
	
	
}

void ma2_task(void)
{
	uint16_t mq2_value = 0;
	
	//第一步开启ADC转换
	HAL_ADC_Start(&hadc1);
	
	//第二步等待转换完成
	if(HAL_ADC_PollForConversion(&hadc1,10) == HAL_OK)
	{
		mq2_value = HAL_ADC_GetValue(&hadc1);
		
		mq2_voltage = (float)mq2_value*3.3f/4095.0f;
		
		printf("mq2_value = %.2f\r\n",mq2_voltage);
		
	}
}



#elif ADC_MODE == 2

void mq2_init(void)
{
	HAL_ADC_Start_DMA(&hadc1,mq2_buffer,mq2_buffer_len);
	
}

void mq2_task(void)
{
	uint16_t mq2_value = 0;
	
	//第一步开启ADC转换
	for(uint8_t i = 0;i<mq2_buffer_len;++i)
	{
		mq2_value += mq2_buffer[i];
	}
	
	mq2_voltage = (float)mq2_value/mq2_buffer_len * 3.3f /4095.f;
	
	Rs=((5.0f-mq2_voltage)/mq2_voltage)*RL;

    ppm = pow((Rs / (R0 * 11.5428)), -1.5278); // Rs/R0 = 11.5428*ppm^(-0.6549);

	// 只在烟雾浓度异常时设置标志位，不自动清零（由语音模块清零）
	if (ppm > 100) {
		if (mq2_flag == 0)  // 只报警一次
			mq2_flag = 1;
	}

	printf("{ppm}ppm = %.2f\r\n",ppm);
}






#endif
