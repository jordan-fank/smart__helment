#include "max30102.h"


// 心率和血氧标志位
bool heartrate_flag = 0;
bool spo2_flag = 0;

// 初始化 MAX30102 数据结构
MAX30102_Data max30102_data = {
    .buffer_length = BUFFER_LENGTH,
    .min_value = 0x3FFFF,
    .max_value = 0,
    .brightness = 0
};

// 滑动平均滤波器
int hr_buffer[WINDOW_SIZE] = {0};
int spo2_buffer[WINDOW_SIZE] = {0};
int hr_index = 0, spo2_index = 0;

// 低通滤波器先前值
int prev_hr = 0, prev_spo2 = 0;

// 非阻塞测量状态
static uint16_t g_sample_idx = 0;
static uint8_t g_measuring = 0;

// 手指稳定检测：连续N次检测到手指才算真正有手指
static uint8_t g_finger_stable_count = 0;
#define FINGER_STABLE_COUNT 2       // 连续2次检测到才算（降低要求，加快响应）

// 当前手指状态（供外部发送报警时检查）
uint8_t max30102_finger_detected = 0;

// 手指检测状态（已通过 Check_Finger_Now() 替代）
// static uint8_t finger_detected = 0;
// static uint16_t finger_stable_count = 0;
// #define FINGER_STABLE_THRESHOLD 5

// 简单滑动平均
int SmoothData(int new_value, int *buffer, int *index)
{
  buffer[*index] = new_value;
  *index = (*index + 1) % WINDOW_SIZE;

  int sum = 0;
  for (int i = 0; i < WINDOW_SIZE; i++)
    sum += buffer[i];

  return sum / WINDOW_SIZE;
}

// 低通滤波
int LowPassFilter(int new_value, int previous_filtered_value)
{
  return (int)(ALPHA * new_value + (1 - ALPHA) * previous_filtered_value);
}

// 计算心率和血氧
void Calculate_Heart_Rate_and_SpO2(void)
{
  maxim_heart_rate_and_oxygen_saturation(max30102_data.ir_buffer, max30102_data.buffer_length,
                                         max30102_data.red_buffer, &max30102_data.spO2, &max30102_data.spO2_valid,
                                         &max30102_data.heart_rate, &max30102_data.heart_rate_valid);
}

// 更新信号最值
void Update_Signal_Min_Max(void)
{
  uint32_t un_prev_data = max30102_data.red_buffer[max30102_data.buffer_length - 1];

  for (int i = 100; i < max30102_data.buffer_length; i++)
  {
    max30102_data.red_buffer[i - 100] = max30102_data.red_buffer[i];
    max30102_data.ir_buffer[i - 100] = max30102_data.ir_buffer[i];

    max30102_data.red_buffer[i - 100] = LowPassFilter(max30102_data.red_buffer[i - 100], un_prev_data);
    max30102_data.ir_buffer[i - 100] = LowPassFilter(max30102_data.ir_buffer[i - 100], un_prev_data);

    max30102_data.red_buffer[i - 100] = SmoothData(max30102_data.red_buffer[i - 100], hr_buffer, &hr_index);
    max30102_data.ir_buffer[i - 100] = SmoothData(max30102_data.ir_buffer[i - 100], spo2_buffer, &spo2_index);

    if (max30102_data.min_value > max30102_data.red_buffer[i - 100])
      max30102_data.min_value = max30102_data.red_buffer[i - 100];
    if (max30102_data.max_value < max30102_data.red_buffer[i - 100])
      max30102_data.max_value = max30102_data.red_buffer[i - 100];
  }
}

// 显示的心率和血氧值
uint8_t dis_hr = 0;
uint8_t dis_spo2 = 0;

// 手指检测阈值 - 平衡灵敏度和准确性
#define FINGER_IR_THRESHOLD 15000   // IR平均值阈值（适中，避免误检测）
#define FINGER_IR_RANGE 800         // IR变化幅度阈值（需要有脉搏波动）

/**
 * @brief 实时检查是否有手指
 * @return 1=有手指, 0=无手指
 */
static uint8_t Check_Finger_Now(void)
{
    if (g_sample_idx < 10) return 0;  // 样本太少

    uint32_t ir_sum = 0;
    uint32_t ir_max = 0;
    uint32_t ir_min = 0x3FFFF;
    uint16_t start = (g_sample_idx > 100) ? g_sample_idx - 100 : 0;

    for (uint16_t i = start; i < g_sample_idx; i++)
    {
        ir_sum += max30102_data.ir_buffer[i];
        if (max30102_data.ir_buffer[i] > ir_max) ir_max = max30102_data.ir_buffer[i];
        if (max30102_data.ir_buffer[i] < ir_min) ir_min = max30102_data.ir_buffer[i];
    }

    uint32_t ir_avg = ir_sum / (g_sample_idx - start);
    uint32_t ir_range = ir_max - ir_min;

    // 有手指：IR平均值足够大，且有脉搏波动
    return (ir_avg > FINGER_IR_THRESHOLD && ir_range > FINGER_IR_RANGE) ? 1 : 0;
}

/**
 * @brief MAX30102 任务函数
 */
void max30102_task(void)
{
    uint8_t has_finger;

    switch (g_measuring)
    {
        case 0: // 空闲状态，开始新测量
            g_sample_idx = 0;
            g_finger_stable_count = 0;  // 重置稳定计数
            g_measuring = 1;
            break;

        case 1: // 采集中
            // 每次任务读取20个样本（平衡速度和准确性）
            for (uint16_t s = 0; s < 20; s++)
            {
                uint8_t temp[6];
                uint32_t timeout = 30000;

                // 等待数据准备好
                while (MAX30102_INT == 1) {
                    if (--timeout == 0) break;
                }

                // 读取FIFO
                max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);

                // 解析数据
                max30102_data.red_buffer[g_sample_idx] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2];
                max30102_data.ir_buffer[g_sample_idx] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5];

                g_sample_idx++;

                // 防止缓冲区溢出，滑动窗口
                if (g_sample_idx >= BUFFER_LENGTH)
                {
                    // 将后面的数据移到前面
                    for (uint16_t i = 100; i < BUFFER_LENGTH; i++)
                    {
                        max30102_data.red_buffer[i - 100] = max30102_data.red_buffer[i];
                        max30102_data.ir_buffer[i - 100] = max30102_data.ir_buffer[i];
                    }
                    g_sample_idx = BUFFER_LENGTH - 100;
                }
            }

            // 检查手指
            has_finger = Check_Finger_Now();

            if (has_finger)
            {
                // 检测到手指，递增稳定计数
                if (g_finger_stable_count < FINGER_STABLE_COUNT)
                {
                    g_finger_stable_count++;
                }
            }
            else
            {
                // 没有检测到手指，保持上次测量值（不清零）
                // dis_hr = 0;          // 不清零，保持上次测量值
                // dis_spo2 = 0;        // 不清零，保持上次测量值
                // heartrate_flag = 0;  // 不自动清零，等语音模块清零
                // spo2_flag = 0;       // 不自动清零，等语音模块清零
                g_finger_stable_count = 0;  // 重置稳定计数
            }

            // 更新全局手指状态：只有稳定检测到才算真正有手指
            if (g_finger_stable_count >= FINGER_STABLE_COUNT)
            {
                max30102_finger_detected = 1;
            }
            else
            {
                max30102_finger_detected = 0;
            }

            // 只有稳定检测到手指后才进行计算
            if (g_finger_stable_count >= FINGER_STABLE_COUNT && g_sample_idx >= 100)
            {
                // 计算心率和血氧
                Calculate_Heart_Rate_and_SpO2();
                max30102_data.heart_rate += HEART_RATE_COMPENSATION;

                // 有效数据才更新显示（保持上次有效值，不立即清零）
                if (max30102_data.heart_rate > 20 &&
                    max30102_data.heart_rate < 200 &&
                    max30102_data.spO2 >= 50 &&
                    max30102_data.spO2 <= 100)
                {
                    dis_hr = (uint8_t)max30102_data.heart_rate;
                    dis_spo2 = (uint8_t)max30102_data.spO2;
                }
                // 无效数据时保持上次的值，不清零（避免频繁跳变）

                // 检查异常（只有稳定检测到手指后才检查）
                // 修改逻辑：只在数据变化时设置标志位，不会自动清零
                if (dis_hr > 0)
                {
                    // 只在异常时设置标志位，不在正常时清零（由语音模块清零）
                    if (dis_hr < 10 || dis_hr > 280)
                    {
                        if (heartrate_flag == 0)  // 只报警一次
                            heartrate_flag = 1;
                    }

                    if (dis_spo2 < 10)
                    {
                        if (spo2_flag == 0)  // 只报警一次
                            spo2_flag = 1;
                    }
                }
            }
            // 继续停留在 case 1 采集数据
            break;
    }
}

void Process_And_Display_Data(uint8_t is_final)
{
    if (dis_hr > 0)
    {
        // my_printf(&huart1, "HR:%d, SpO2:%d\r\n", dis_hr, dis_spo2);
    }
    else
    {
        // my_printf(&huart1, "HR:---, SpO2:---\r\n");
    }
}

