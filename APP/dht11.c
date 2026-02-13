#include "dht11.h"


float temp = 0;
float humi = 0;

bool temp_flag = 0;


/**
 * @brief DHT11 硬件初始化
 * 作用：开启 GPIOA 时钟，并将 PA8 设置为初始高电平状态（空闲态）
 */
void DHT11_Init(void)
{
    // 1. 开启 GPIOA 时钟 (HAL库标准操作)
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 2. 配置 PA8 为推挽输出，确保初始化时总线是释放状态
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 3. 将总线拉高进入空闲状态
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    
    // 给传感器一点准备时间
    HAL_Delay(100); 
}


// 切换为输出模式
void DHT11_Mode_Out(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 推挽输出
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// 切换为输入模式
void DHT11_Mode_In(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;        // 强力建议开启上拉
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}


uint8_t DHT11_Read_Byte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        uint16_t timeout = 0;
		
        // 等待低电平结束
		//设置超时跳出while
        while (READ_DATA == 0 && timeout < 1000) timeout++;
        
        delay_us(40); // 关键延时：区分0和1
        
        data <<= 1;
        if (READ_DATA == 1) {
            data |= 1;
            timeout = 0;
            // 等待高电平结束
            while (READ_DATA == 1 && timeout < 1000) timeout++;
        }
    }
    return data;
}

uint8_t DHT11_Read_Data(float *temp, float *humi) {
    uint8_t buf[5];
    
    // 1. 起始信号
    DHT11_Mode_Out();
    DHT11_LOW;
    HAL_Delay(20); // 至少18ms
    DHT11_HIGH;
    delay_us(30);
    
    // 2. 检查响应
    DHT11_Mode_In();
    if (READ_DATA == 0) {
        uint16_t timeout = 0;
        while (READ_DATA == 0 && timeout < 1000) timeout++; // 等待响应低电平结束
        timeout = 0;
        while (READ_DATA == 1 && timeout < 1000) timeout++; // 等待响应高电平结束
        
        // 3. 读取40位数据
        for (int i = 0; i < 5; i++) {
            buf[i] = DHT11_Read_Byte();
        }
        
        // 4. 校验和计算
        if (buf[0] + buf[1] + buf[2] + buf[3] == buf[4]) {
            // 合并逻辑：处理你提到的小数部分
            // 湿度 = 整数 + 小数 * 0.1
            *humi = (float)buf[0] + (float)buf[1] * 0.1f;
            // 温度 = 整数 + 小数 * 0.1
            *temp = (float)buf[2] + (float)buf[3] * 0.1f;
            return 1; // 成功
        }
    }
    return 0; // 失败
}

void dht11_task(void) {

    if (DHT11_Read_Data(&temp, &humi)) {

        printf("temp:%.1f,humi:%.1f\r\n", temp, humi);

		// 只在温度异常时设置标志位，不自动清零（由语音模块清零）
		if (temp > 40) {
			if (temp_flag == 0)  // 只报警一次
				temp_flag = 1;
		}
    } else {
        // 也� �以打印错误信息方便调试
        printf("DHT11_Error\r\n"); 
    }
}
