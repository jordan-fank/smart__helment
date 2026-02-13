#include "asrpro.h"

uint8_t uart_rx_byte = 0;
uint16_t uart_rx_index = 0;
uint32_t uart_rx_ticks = 0;
uint8_t uart_rx_buffer[128];	//存储区

#define UART_TIMEOUT_MS 100			//超时解析的超时时间

void asrpro_init(void)
{
	HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
}

//串口接受中断回调函数--超时解析
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//第一步 确定串口
	if(huart->Instance == USART1)
	{
		//第二步 记录接受时间
		uart_rx_ticks = uwTick;		//记住最后接受数据的时间
		
		//第三步 增加存储指针
        uart_rx_buffer[uart_rx_index++] = uart_rx_byte;
		
		//第四步 重启接收中断
		HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
	}
}



void asrpro_task(void)
{
	//第一步 如果存储指针为0 说明没有数据需要处理
	if(uart_rx_index == 0)
		return;
	
	
	//第二步  超时解析
	if(uwTick - uart_rx_ticks > UART_TIMEOUT_MS)
	{
		//第三步 确保字符串结尾
		 uart_rx_buffer[uart_rx_index] = '\0'; 
		
		//第四步 处理数据
//		printf("Over time uart data : %s \n\r",uart_rx_buffer);
		
		
		if(strstr((char*)uart_rx_buffer, "good"))
		{
			alarm_flag = 0;
			// printf("Switch to Page 0\r\n"); // 调试用
		}
		else if(strstr((char*)uart_rx_buffer, "open"))
		{
			alarm_flag = 1;

		}
		
		
		if(strstr((char*)uart_rx_buffer, "page0"))
		{
			oled_page = 1;
			// printf("Switch to Page 0\r\n"); // 调试用
		}
		else if(strstr((char*)uart_rx_buffer, "page1"))
		{
			oled_page = 2;
		}
		else if(strstr((char*)uart_rx_buffer, "page2"))
		{
			oled_page = 3;
		}
		else if(strstr((char*)uart_rx_buffer, "page3"))
		{
			oled_page = 4;
		}
		else if(strstr((char*)uart_rx_buffer, "page4"))
		{
			oled_page = 5;
		}
		else if(strstr((char*)uart_rx_buffer, "page5"))
		{
			oled_page = 6;
		}
		else if(strstr((char*)uart_rx_buffer, "LED_ON"))
		{
			LED_ON_flag = 1;
		}
		else if(strstr((char*)uart_rx_buffer, "LED_OFF"))
		{
			LED_ON_flag = 0;
		}
		else if(strstr((char*)uart_rx_buffer, "FAN_ON"))
		{
			FAN_ON_flag = 1;
		}
		else if(strstr((char*)uart_rx_buffer, "FAN_OFF"))
		{
			FAN_ON_flag = 0;
		}
		
		//第五步 清理缓冲区和存储指针
		memset(uart_rx_buffer,0,uart_rx_index);
		uart_rx_index = 0;
		

	}		
}

