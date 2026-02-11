#include"atgm336h.h"



/*
三种串口工作模式

	1.串口中断+超时解析
	2.DMA+空闲中断
	3.DMA+空闲中断+环形缓冲区

*/
#define UART_MODE 3



#if UART_MODE == 1   //1.串口中断+超时解析

uint8_t uart_rx_byte = 0;
uint16_t uart_rx_index = 0;
uint32_t uart_rx_ticks = 0;
uint8_t uart_rx_buffer[128];	//存储区

#define UART_TIMEOUT_MS 100			//超时解析的超时时间

void atgm336h_init(void)
{
	HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
}

//串口接受中断回调函数--超时解析
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//第一步 确定串口
	if(huart->Instance == USART2)
	{
		//第二步 记录接受时间
		uart_rx_ticks = uwTick;		//记住最后接受数据的时间
		
		//第三步 增加存储指针
        uart_rx_buffer[uart_rx_index++] = uart_rx_byte;
		
		//第四步 重启接收中断
		HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
	}
}



void atgm336h_task(void)
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
		printf("Over time uart data : %s \n\r",uart_rx_buffer);
		
		
		//第五步 清理缓冲区和存储指针
		memset(uart_rx_buffer,0,uart_rx_index);
		uart_rx_index = 0;
		

	}		
}

#elif UART_MODE ==2		//2.DMA+空闲中断

#define uart_dma_buffer_len 1000

uint8_t uart_rx_dma_buffer[uart_dma_buffer_len];	//DMA接受的缓冲区
uint8_t uart_dma_buffer[uart_dma_buffer_len];		//复制到待处理的缓冲区
uint8_t uart_flag;					//处理数据标志位

void atgm336h_init(void)
{
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx ,DMA_IT_HT);
}


//DMA空闲中断回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	//第一步 确认是目标串口
	if(huart->Instance == USART2)
	{
		//第二步 停止DMA传输，空闲中断意味着发送停止，所以停止DMA等待
		HAL_UART_DMAStop(huart);
		
		//第三步 讲DMA缓冲区的有效数据Size字节，复制到待处理缓冲区
		memcpy(uart_dma_buffer,uart_rx_dma_buffer,Size);
		
		//第四步 数据处理标志位置一，告诉主循环有数据待处理
		uart_flag = 1;
		
		//第五步  清空DMA接收缓冲区，未下次做准备
		memset(uart_rx_dma_buffer,0,sizeof(uart_rx_dma_buffer));
		
		//第六步 重新启动下一次DMA空闲接受中断
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
		
		//第七步 关闭DMA半满中断 
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

		
	}
}

void atgm336h_task(void)
{
	//第一步，如果数据处理标志位为0 说明没有数据需要处理，直接返回
	if(uart_flag == 0)
		return;
	
	//第二步，清空标志位
	uart_flag = 0;
	
	//第三步 处理数据
	printf("DMA uart data : %s \r\n",uart_dma_buffer);
	
	//第四步  清空缓冲区 将接收所有置零
	memset(uart_dma_buffer,0,sizeof(uart_dma_buffer));
}


#elif UART_MODE == 3		//3.DMA+空闲中断+环形缓冲区

// 环形缓冲区相关变量
uint8_t uart_rx_dma_buffer[USART_REC_LEN]; // DMA 硬件接收第一站
uint8_t uart_dma_buffer[USART_REC_LEN];    // 解析中转站
uint8_t ringbuffer_pool[USART_REC_LEN * 2]; // 数据蓄水池
struct rt_ringbuffer uart_ringbuffer;      // 环形缓冲区句柄

// 数据结构实例
_SaveData Save_Data;
LatitudeAndLongitude_s g_LatAndLongData;
float longitude = 0.0f;
float latitude = 0.0f;




void atgm336h_init(void)
{
	
	clrStruct();
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
	 __HAL_DMA_DISABLE_IT(&hdma_usart2_rx ,DMA_IT_HT);
	
	rt_ringbuffer_init(&uart_ringbuffer,ringbuffer_pool,sizeof(ringbuffer_pool));
}

//DMA空闲中断回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	//第一步 确认是目标串口
	if(huart->Instance == USART2)
	{
		//第二步 停止DMA传输，空闲中断意味着发送停止，所以停止DMA等待
		HAL_UART_DMAStop(huart);
		
		//第三步 将DMA接受到的数据，放到环形环形缓冲区的池子里面，大小为Size
		rt_ringbuffer_put(&uart_ringbuffer,uart_rx_dma_buffer,Size);

		
		//第四步  清空DMA接收缓冲区，未下次做准备
		memset(uart_rx_dma_buffer,0,sizeof(uart_rx_dma_buffer));
		
		//第五步 重新启动下一次DMA空闲接受中断
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
		
		//第六步 关闭DMA半满中断 
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

		
	}
}


/**
 * @brief  清理数据结构
 */
void clrStruct(void)
{
    memset(&Save_Data, 0, sizeof(_SaveData));
    memset(&g_LatAndLongData, 0, sizeof(LatitudeAndLongitude_s));
}



/**
 * @brief  解析 GPS/北斗 RMC 最小定位帧并执行坐标转换
 * @param  buffer: 存放原始 NMEA 字符串的缓冲区指针
 * @param  length: 数据长度
 */
/**
 * @brief  修正后的解析函数：加入指针偏移逻辑
 */

void parse_uart_command(uint8_t *buffer, uint16_t length)
{
    // 1. 寻找帧头
    char *p_start = strstr((char *)buffer, "$GNRMC");
    if (p_start == NULL) p_start = strstr((char *)buffer, "$GPRMC");
    if (p_start == NULL) return; 

    char *p_current = p_start;
    char *p_next_comma;

    // 2. 字段拆分逻辑
    for (int i = 0; i <= 6; i++) 
    {
        p_next_comma = strchr(p_current, ','); 
        if (p_next_comma == NULL) return;

        // 计算长度
        uint16_t field_len = p_next_comma - p_current; // 注意：这里不需要-1了，因为p_current已经在数据头了
        
        // 【核心修复】直接指向数据头，不要+1
        char *field_ptr = p_current; 

        // 对于 i=0 的情况（$GNRMC），我们其实是从 $ 开始算的，这没关系，switch里没有case 0
        // 但为了逻辑严谨，i=0 时 field_ptr 指向的是 $
        
        // 只有 i>0 时，p_current 才真正指向数据内容
        // 让我们修正一下 i=0 的特殊情况
        if (i == 0) {
            // i=0 是帧头，不需要处理，直接跳过
        }
        else if (field_len > 0) 
        {
            switch (i) {
                case 1: // UTC 时间
                    memcpy(Save_Data.UTCTime, field_ptr, field_len < UTCTime_Length ? field_len : UTCTime_Length - 1);
                    Save_Data.UTCTime[field_len] = '\0'; 
                    break;
                case 2: // A/V 标志 【这里修好了！】
                    Save_Data.isUsefull = (*field_ptr == 'A');
                    break;
                case 3: // 纬度
                    memcpy(Save_Data.latitude, field_ptr, field_len < latitude_Length ? field_len : latitude_Length - 1);
                    Save_Data.latitude[field_len] = '\0'; 
                    break;
                case 4: // N/S
                    Save_Data.N_S[0] = *field_ptr;
                    break;
                case 5: // 经度
                    memcpy(Save_Data.longitude, field_ptr, field_len < longitude_Length ? field_len : longitude_Length - 1);
                    Save_Data.longitude[field_len] = '\0'; 
                    break;
                case 6: // E/W
                    Save_Data.E_W[0] = *field_ptr;
                    break;
            }
        }
        
        // 移动到下一个字段的开头
        p_current = p_next_comma + 1; 
    }

    // 3. 最终判定与转换
    if (Save_Data.isUsefull)
    {
        // my_printf(&huart1, "AA - GPS Logic Hit!\r\n");
        
        float deg = 0.0f, min = 0.0f;

        sscanf(Save_Data.latitude, "%2f%f", &deg, &min);
        g_LatAndLongData.latitude = deg + (min / 60.0f);
        if (Save_Data.N_S[0] == 'S') g_LatAndLongData.latitude = -g_LatAndLongData.latitude;

        sscanf(Save_Data.longitude, "%3f%f", &deg, &min);
        g_LatAndLongData.longitude = deg + (min / 60.0f);
        if (Save_Data.E_W[0] == 'W') g_LatAndLongData.longitude = -g_LatAndLongData.longitude;

        latitude = g_LatAndLongData.latitude;
        longitude = g_LatAndLongData.longitude;
        Save_Data.isParseData = true; 
    }
    else {
        Save_Data.isParseData = false;
    }
}

/**
 * @brief  将解析好的 GPS 数据格式化打印到串口
 */
void printGpsBuffer(void)
{
    // 逻辑开关：只有当 parse_uart_command 成功解析出有效位置时，才打印
    if (Save_Data.isParseData) {
        // 打印时间（UTC格式）和定位状态（Fixed代表已锁定）
        // my_printf(&huart1, ">>> [GPS INFO] Time:%s, Stat:Fixed\r\n", Save_Data.UTCTime);

        // 打印最终的十进制度坐标，保留 6 位小数（约 0.1 米精度）
        // %c 打印方向字符（N/S, E/W）
        // my_printf(&huart1, "    Lat:%.6f (%c), Lon:%.6f (%c)\r\n",
        //           g_LatAndLongData.latitude, Save_Data.N_S[0],
        //           g_LatAndLongData.longitude, Save_Data.E_W[0]);
    } else {
        // 如果 isParseData 为假，说明还没攒够数据或者正在室内，搜不到星
        // my_printf(&huart1, ">>> [GPS INFO] Searching Satellites...\r\n");
    }
	
	
}




void atgm336h_task(void)
{
	//第一步 获取环形缓冲区的大小 若为空，不处理
	uint16_t length;
	length =  rt_ringbuffer_data_len(&uart_ringbuffer);
	if(length == 0) return;
	
	// 防止单次解析缓冲区溢出
    if (length > sizeof(uart_dma_buffer) - 1) length = sizeof(uart_dma_buffer) - 1;

	//第二步，从环形缓冲区里面取出数据到另外一个数组里面进行数据解析
	rt_ringbuffer_get(&uart_ringbuffer,uart_dma_buffer,length);
	uart_dma_buffer[length] = '\0'; // 补齐结束符，确保 strstr 正常工作

	// 第三步 调用命令解析函数
	parse_uart_command(uart_dma_buffer, length);
	
	printGpsBuffer();
	//打印所有数据
//printf("\r\n[RAW LEN: %d] Content:\r\n%s\r\n--------------------\r\n", length, uart_dma_buffer);
	
	//第四步  清空缓冲区 将接收所有置零
	memset(uart_dma_buffer,0,sizeof(uart_dma_buffer));
}

#endif


	



