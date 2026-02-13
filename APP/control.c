#include "control.h"

bool LED_ON_flag = 0;
bool FAN_ON_flag = 0;



/**
 * @brief  设备控制初始化
 */
void Device_Init(void)
{
    Control_LED(OFF);
    Control_Fan(OFF);
}

/**
 * @brief  控制 LED 灯 (PB3)
 */
void Control_LED(Switch_State state)
{
    if (state == ON)
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  控制 风扇 (PB4)
 */
void Control_Fan(Switch_State state)
{
    if (state == ON)
        HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  同时控制所有设备
 */
void Control_All(Switch_State state)
{
    Control_LED(state);
    Control_Fan(state);
}


uint8_t uart_rx_dma_buffer3[uart_dma_buffer_len3];
uint8_t uart_dma_buffer3[uart_dma_buffer_len3];
uint8_t uart_flag;
uint8_t cmd_flag;  // 收到MQTT命令标志

void control_init(void)
{
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, uart_rx_dma_buffer3, sizeof(uart_rx_dma_buffer3));
	__HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
}


/**
 * @brief  解析华为云命令并执行控制
 * @note   ESP8266收到MQTT订阅消息后通过串口输出:
 *         +MQTTSUBRECV:0,"topic",len,{...json...}
 *         JSON格式: {"command_name":"Switch_Control","service_id":"Control",
 *                    "paras":{"device":"LED","action":"ON"}}
 */
void control_process(uint8_t *buffer)
{
    char *str = (char *)buffer;

    // 检查是否是MQTT订阅接收的数据
    if (strstr(str, "+MQTTSUBRECV") == NULL)
        return;

    // 检查是否是我们的控制命令
    if (strstr(str, "Switch_Control") == NULL)
        return;

    // 解析 device 字段
    char *p_device = strstr(str, "\"device\":");
    if (p_device == NULL)
        return;

    // 判断设备类型
    uint8_t is_led = 0;
    uint8_t is_fan = 0;
    if (strstr(p_device, "\"LED\"") != NULL)
        is_led = 1;
    else if (strstr(p_device, "\"FAN\"") != NULL)
        is_fan = 1;
    else
        return;

    // 解析 action 字段
    char *p_action = strstr(str, "\"action\":");
    if (p_action == NULL)
        return;

    Switch_State state = OFF;
    if (strstr(p_action, "\"ON\"") != NULL)
        state = ON;

    // 通过标志位控制，和语音控制共用同一组标志位
    if (is_led)
        LED_ON_flag = (state == ON) ? 1 : 0;
    else if (is_fan)
        FAN_ON_flag = (state == ON) ? 1 : 0;
}


void control_task(void)
{
	// 处理MQTT命令（如果有）
	if (cmd_flag)
	{
		cmd_flag = 0;
		control_process(uart_dma_buffer3);
		memset(uart_dma_buffer3, 0, sizeof(uart_dma_buffer3));
	}

	// 统一根据标志位输出GPIO（语音和网页都通过标志位控制）
	Control_LED(LED_ON_flag ? ON : OFF);
	Control_Fan(FAN_ON_flag ? ON : OFF);
}
