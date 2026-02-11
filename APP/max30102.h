#ifndef _MAX30102_H
#define _MAX30102_H

#include "bsp_system.h"


// ���ʲ���ֵ��ÿ�β����̶�������ֵ
#define HEART_RATE_COMPENSATION 10
// �������ڴ�С
#define WINDOW_SIZE 20
// ��ͨ�˲������˲�ϵ��
#define ALPHA 0.05     

// ���ݻ��泤��
#define BUFFER_LENGTH 500

// MAX30102 ���ݽṹ�����ڴ洢���������ݺ���ز���
typedef struct
{
  // ����LED���ݣ�����Ѫ�����㣩
  uint32_t ir_buffer[BUFFER_LENGTH];
  // ��ɫLED���ݣ��������ʼ��㣩
  uint32_t red_buffer[BUFFER_LENGTH];
  // Ѫ�����Ͷ�
  int32_t spO2;
  // Ѫ����Ч��ָʾ
  int8_t spO2_valid;
  // ����
  int32_t heart_rate;
  // ������Ч��ָʾ
  int8_t heart_rate_valid;
  // �ź���Сֵ
  int32_t min_value;
  // �ź����ֵ
  int32_t max_value;
  // ��һ���ݵ�
  int32_t prev_data;
  // �ź����ȣ��������ʼ��㣩
  int32_t brightness;
  // ���ݻ���������
  uint32_t buffer_length;
} MAX30102_Data;

// ����ƽ���˲�����
int SmoothData(int new_value, int *buffer, int *index);

// ��ͨ�˲�����
int LowPassFilter(int new_value, int previous_filtered_value);

// ��ȡMAX30102���������ݣ����������ʲ���
void MAX30102_Read_Data(void);

// 读取单个MAX30102样本（非阻塞）
uint8_t MAX30102_Read_Single_Sample(void);

// �������ʺ�Ѫ��ֵ
void Calculate_Heart_Rate_and_SpO2(void);

// �����źŵ���Сֵ�����ֵ����Ӧ���˲�
void Update_Signal_Min_Max(void);

// ���ݴ�������ʾ�������������ʺ�Ѫ�����ݲ������ӡ����
void Process_And_Display_Data(uint8_t is_final);

// MAX30102 �������������ȡ�ʹ�������������
void max30102_task(void);

// ��ʼ�����ݽṹ
extern MAX30102_Data max30102_data;


// 心率和血氧标志位
extern bool heartrate_flag;
extern bool spo2_flag;

// 显示的心率和血氧值
extern uint8_t dis_hr;
extern uint8_t dis_spo2;

// 当前手指状态（供外部检查是否允许发送报警）
extern uint8_t max30102_finger_detected;

#endif
