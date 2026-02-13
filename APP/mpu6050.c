#include "mpu6050.h"

uint8_t i = 10; // 循环计数器
float pitch, roll, yaw;    // 欧拉角（姿态数据）
short aacx, aacy, aacz;    // 加速度计原始数据
short gyrox, gyroy, gyroz; // 陀螺仪原始数据
unsigned long walk;        // 步数
float steplength = 0.3, Distance; // 步距和路程计算参数
uint8_t svm_set = 1;       // 路程计算标志
uint16_t AVM;              // 加速度向量模值
uint16_t GVM;              // 陀螺仪向量模值


bool fall_flag = 0;        // 跌倒标志位
bool collision_flag = 0;   // 碰撞标志位

/**
 * @brief  I2C 写操作函数，用于向 MPU6050 写入多个字节数据
 */
uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    uint8_t data[1 + len]; // 组合数据缓冲区，第一个字节为寄存器地址
    data[0] = reg;
    for (uint8_t i = 0; i < len; i++)
        data[i + 1] = buf[i];

    // 通过 I2C 发送数据
    if (HAL_I2C_Master_Transmit(&hi2c1, (addr << 1), data, len + 1, HAL_MAX_DELAY) != HAL_OK)
        return 1; // 传输失败
    return 0;     // 传输成功
}

/**
 * @brief  I2C 读操作函数，用于从 MPU6050 读取多个字节数据
 */
uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    // 发送寄存器地址
    if (HAL_I2C_Master_Transmit(&hi2c1, (addr << 1), &reg, 1, HAL_MAX_DELAY) != HAL_OK)
        return 1; // 发送寄存器地址失败

    // 读取数据
    if (HAL_I2C_Master_Receive(&hi2c1, (addr << 1), buf, len, HAL_MAX_DELAY) != HAL_OK)
        return 1; // 读取数据失败
    return 0;     // 读取成功
}

/**
 * @brief  向 MPU6050 写入单个字节数据
 */
uint8_t MPU_Write_Byte(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    if (HAL_I2C_Master_Transmit(&hi2c1, (MPU_ADDR << 1), buf, 2, HAL_MAX_DELAY) != HAL_OK)
    {
        return 1; // 传输失败
    }
    return 0; // 传输成功
}

/**
 * @brief  从 MPU6050 读取单个字节数据
 */
uint8_t MPU_Read_Byte(uint8_t reg)
{
    uint8_t data;
    // 发送寄存器地址
    if (HAL_I2C_Master_Transmit(&hi2c1, (MPU_ADDR << 1), &reg, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        return 0; // 传输失败
    }

    // 读取数据
    if (HAL_I2C_Master_Receive(&hi2c1, (MPU_ADDR << 1), &data, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        return 0; // 读取失败
    }
    return data; // 返回读取的数据
}

/**
 * @brief  初始化 MPU6050 函数
 */
void MPU_Init(void)
{
    // 解除 MPU6050 休眠状态
    MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x00);
    // 设置陀螺仪采样率（典型值 125Hz）
    MPU_Write_Byte(MPU_SAMPLE_RATE_REG, 0x07);
    // 设置低通滤波器频率（典型值 5Hz）
    MPU_Write_Byte(MPU_CFG_REG, 0x06);
    // 配置陀螺仪（不自检，量程 2000deg/s）
    MPU_Write_Byte(MPU_GYRO_CFG_REG, 0x18);
    // 配置加速度计（不自检，量程 16G，低通滤波 5Hz）
    MPU_Write_Byte(MPU_ACCEL_CFG_REG, 0x18);
}

/**
 * @brief  获取温度值函数
 */
short MPU_Get_Temperature(void)
{
    uint8_t buf[2];
    short raw;
    float temp;
    // 读取温度寄存器的高低 8 位
    MPU_Read_Len(MPU_ADDR, MPU_TEMP_OUTH_REG, 2, buf);
    // 组合 16 位原始数据
    raw = ((uint16_t)buf[0] << 8) | buf[1];
    // 转换为实际温度值（扩大 100 倍返回）
    temp = 36.53 + ((double)raw) / 340;
    return temp * 100;
}

/**
 * @brief  获取陀螺仪原始数据函数
 */
uint8_t MPU_Get_Gyroscope(short *gx, short *gy, short *gz)
{
    uint8_t buf[6], res;
    res = MPU_Read_Len(MPU_ADDR, MPU_GYRO_XOUTH_REG, 6, buf);
    if (res == 0)
    {
        // 组合各轴的高低 8 位数据
        *gx = ((uint16_t)buf[0] << 8) | buf[1];
        *gy = ((uint16_t)buf[2] << 8) | buf[3];
        *gz = ((uint16_t)buf[4] << 8) | buf[5];
    }
    return res;
}

/**
 * @brief  获取加速度计原始数据函数
 */
uint8_t MPU_Get_Accelerometer(short *ax, short *ay, short *az)
{
    uint8_t buf[6], res;
    res = MPU_Read_Len(MPU_ADDR, MPU_ACCEL_XOUTH_REG, 6, buf);
    if (res == 0)
    {
        // 组合各轴的高低 8 位数据
        *ax = ((uint16_t)buf[0] << 8) | buf[1];
        *ay = ((uint16_t)buf[2] << 8) | buf[3];
        *az = ((uint16_t)buf[4] << 8) | buf[5];
    }
    return res;
}










// 定义阈值 (基于 ±16g 量程, 1g = 2048)
#define ACCEL_1G        2048
#define COLLISION_THR   (6 * ACCEL_1G)  // 碰撞阈值 6g (约 12288)，太敏感改为6g
#define FALL_IMPACT_THR (3 * ACCEL_1G)  // 跌倒撞击阈值 3g (约 6144)
#define FALL_ANGLE_THR  60.0f           // 跌倒倾斜角度阈值
#define STATIC_GYRO_THR 100             // 静止时陀螺仪阈值

// 状态变量
uint8_t fall_state = 0;
uint32_t fall_timer = 0;
uint16_t dmp_warmup = 200;  // DMP预热计数器(约2秒)

// 碰撞检测去抖计数
static uint8_t collision_count = 0;
#define COLLISION_DEBOUNCE 3  // 连续3次检测到才确认碰撞

void mpu6050_task(void)
{
    uint8_t dmp_result;

    // 读取原始传感器数据
    MPU_Get_Accelerometer(&aacx, &aacy, &aacz);
    MPU_Get_Gyroscope(&gyrox, &gyroy, &gyroz);

    // 尝试获取DMP姿态数据
    dmp_result = mpu_dmp_get_data(&pitch, &roll, &yaw);

    // 如果DMP未准备好，使用加速度计计算备用姿态角
    if (dmp_result != 0 || dmp_warmup > 0)
    {
        if (dmp_warmup > 0) dmp_warmup--;

        // 使用加速度计计算俯仰角和横滚角(静态姿态)
        float ax = (float)aacx / ACCEL_1G;  // 归一化到g
        float ay = (float)aacy / ACCEL_1G;
        float az = (float)aacz / ACCEL_1G;

        // 计算pitch (绕Y轴旋转)
        pitch = atan2(ax, sqrt(ay*ay + az*az)) * 57.3f;

        // 计算roll (绕X轴旋转)
        roll = atan2(ay, sqrt(ax*ax + az*az)) * 57.3f;

        // yaw无法从静态加速度计获得，保持上次的值或设为0
        if (dmp_warmup > 150)  // 初始阶段
            yaw = 0;
    }

    // 计算合加速度
    float acc_sq_sum = (float)aacx*aacx + (float)aacy*aacy + (float)aacz*aacz;
    AVM = (uint16_t)sqrt(acc_sq_sum);

    // 计算陀螺仪模值
    float gyro_sq_sum = (float)gyrox*gyrox + (float)gyroy*gyroy + (float)gyroz*gyroz;
    GVM = (uint16_t)sqrt(gyro_sq_sum);

    // ---------------------------------------------------------
    // 碰撞检测 (Collision) - 增加去抖
    // 修改逻辑：只报警一次，不自动清零
    // ---------------------------------------------------------
    if (AVM > COLLISION_THR && GVM > STATIC_GYRO_THR)
    {
        collision_count++;
        if (collision_count >= COLLISION_DEBOUNCE)
        {
            if (collision_flag == 0)  // 只报警一次
                collision_flag = 1;
            // my_printf(&huart1, "WARNING: Collision! Force: %.2fg\r\n", (float)AVM/ACCEL_1G);
        }
    }
    else
    {
        collision_count = 0;
        // 不自动清零，由语音模块清零
    }

    // ---------------------------------------------------------
    // 跌倒检测 (Fall) - 状态机逻辑
    // ---------------------------------------------------------
    switch(fall_state)
    {
        case 0: // 状态0: 监测撞击
            if (AVM > FALL_IMPACT_THR && GVM > STATIC_GYRO_THR)
            {
                fall_state = 1;
                fall_timer = HAL_GetTick();
            }
            break;

        case 1: // 状态1: 撞击后等待1秒，检查姿态
            if (HAL_GetTick() - fall_timer > 1000)
            {
                if ((fabs(pitch) > FALL_ANGLE_THR) || (fabs(roll) > FALL_ANGLE_THR))
                {
                    fall_state = 2;
                }
                else
                {
                    fall_state = 0;
                }
            }
            break;

        case 2: // 状态2: 检查是否无法动弹
            if (GVM < STATIC_GYRO_THR)
            {
                if (fall_flag == 0)  // 只报警一次
                    fall_flag = 1;
                // my_printf(&huart1, "ALARM: Fall Detected!\r\n");
            }
            else if ((fabs(pitch) < 30) && (fabs(roll) < 30))
            {
                // 用户恢复正常姿态，退出跌倒状态（但不清零标志位，由语音模块清零）
                fall_state = 0;
            }
            break;
    }
}







///**
// * @brief  MPU6050 任务处理函数
// */
//void mpu6050_task(void)
//{
//    // 获取姿态数据（欧拉角）
//    mpu_dmp_get_data(&pitch, &roll, &yaw);
//    // 获取加速度计数据
//    MPU_Get_Accelerometer(&aacx, &aacy, &aacz);
//    // 获取陀螺仪数据
//    MPU_Get_Gyroscope(&gyrox, &gyroy, &gyroz);
//    
//    // 计算加速度向量模值
//    AVM = sqrt(pow(aacx, 2) + pow(aacy, 2) + pow(aacz, 2));
//    // 计算陀螺仪向量模值
//    GVM = sqrt(pow(gyrox, 2) + pow(gyroy, 2) + pow(gyroz, 2));
//	
//	// 碰撞：只要合力大于 4g
//	if (AVM > (4 * 2048)) {
//		collision_flag = 1;
//	} else {
//		collision_flag = 0;
//	}
//    
//    // 跌倒：合力曾大于 3g (冲击) 并且 当前角度大于 60度
//	// 注意：实际项目中通常需要一个标志位记录“发生过冲击”，这里简化处理
//	// 建议：只有在倾斜角很大时，才去检查有没有发生过大冲击，或者反过来
//	if ( ((fabs(pitch) > 60) || (fabs(roll) > 60)) && (AVM > (1.5 * 2048)) ) 
//	{
//		// 这里加一个 AVM > 1.5g 是为了过滤弯腰系鞋带（系鞋带时比较平稳，AVM 接近 1g）
//		// 只有在身体倾斜且当前加速度不稳定（或之前有过大加速度）时才置位
//		fall_flag = 1;
//	} 
//	else 
//	{
//		fall_flag = 0;
//	}
//    
//    // 打印姿态数据
//    my_printf(&huart1, "pitch:%0.1f   roll:%0.1f   yaw:%0.1f\r\n", pitch, roll, yaw);
//}
