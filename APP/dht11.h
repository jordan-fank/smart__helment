#ifndef __DHT11_H
#define __DHT11_H

#include "bsp_system.h"


// 引脚定义
#define DHT11_PORT  GPIOA
#define DHT11_PIN   GPIO_PIN_8

// 操作宏
#define DHT11_HIGH  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET)
#define DHT11_LOW   HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET)
#define READ_DATA   HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)

// 函数声明
void DHT11_Init(void);
uint8_t DHT11_Read_Data(float *temp, float *humi);

void dht11_task(void);

#endif

