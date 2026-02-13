#ifndef _CONTROL_H
#define _CONTROL_H

#include "bsp_system.h"

// --- Ӳ�����Ŷ��� (��Ӧ CubeMX ����) ---
#define LED_PORT    GPIOB
#define LED_PIN     GPIO_PIN_3

#define FAN_PORT    GPIOB
#define FAN_PIN     GPIO_PIN_4

// --- ����״̬ö�� ---
typedef enum {
    OFF = 0,
    ON  = 1
} Switch_State;



#define uart_dma_buffer_len3 512

extern uint8_t uart_rx_dma_buffer3[uart_dma_buffer_len3];	//DMA���ܵĻ�����
extern uint8_t uart_dma_buffer3[uart_dma_buffer_len3];		//���Ƶ��������Ļ�����
extern uint8_t uart_flag;					//�������ݱ�־λ
extern uint8_t cmd_flag;					// 收到命令标志位



// --- �������� ---
void Device_Init(void);          // �豸��ʼ������©��ȱ��
void Control_LED(Switch_State state); // ���� LED
void Control_Fan(Switch_State state); // ���� ����
void Control_All(Switch_State state); // һ��ȫ��/ȫ��

void control_init(void);
void control_task(void);


#endif
