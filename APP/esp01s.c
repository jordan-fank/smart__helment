#include "esp01s.h"

// 外部变量声明（姿态数据）
extern float pitch, roll, yaw;

// 简单的打印函数，方便发送串口数据
void ESP_Send(char *fmt, ...) {
    char buf[384];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, sizeof(buf), fmt, arg);
    va_end(arg);
    HAL_UART_Transmit(&huart3, (uint8_t *)buf, strlen(buf), 1000);
}

/**
 * @brief ESP8266 自动上云初始化流程
 */
void ESP_Init(void) {
    // 1. 复位并等待稳定
    ESP_Send("AT+RST\r\n");
    HAL_Delay(2000);
    
    // 2. 设置为 STA 模式
    ESP_Send("AT+CWMODE=1\r\n");
    HAL_Delay(1000);
    
    // 3. 连接 Wi-Fi
    ESP_Send("AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PWD);
    HAL_Delay(6000); // 连 WiFi 比较慢，多等会儿
    
    // 4. MQTT 配置 (重点：直接把 ClientID 填在第三个参数，不填 NULL)
    ESP_Send("AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", 
             HUAWEI_CLIENTID, HUAWEI_USERNAME, HUAWEI_PASSWORD);
    HAL_Delay(1000);
    
    // 5. 连接 MQTT 服务器 (华为云端口 1883)
    ESP_Send("AT+MQTTCONN=0,\"%s\",1883,1\r\n", HUAWEI_ADDR);
    HAL_Delay(2000);

    // 6. 订阅华为云命令下发 Topic
    ESP_Send("AT+MQTTSUB=0,\"%s\",1\r\n", HUAWEI_SUB_TOPIC);
    HAL_Delay(1000);
}

/**
 * @brief 上报函数1：基础环境 (温度、湿度、烟雾)
 */
void ESP_Report_Env(void) {
    // 注意：逗号需要转义为 \,（在C代码中写成 \\,）
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"temp\\\":%.1f\\,\\\"humi\\\":%.1f\\,\\\"ppm\\\":%.2f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, temp, humi, ppm);
}

/**
 * @brief 上报函数2：身体健康 (心率、血氧 + 异常标志)
 */
void ESP_Report_Body(void) {
    // 显式转换bool为int，确保格式正确
    int hr_flag = (int)heartrate_flag;
    int sp_flag = (int)spo2_flag;

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"dis_hr\\\":%d\\,\\\"dis_spo2\\\":%d\\,\\\"heartrate_flag\\\":%d\\,\\\"spo2_flag\\\":%d}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, dis_hr, dis_spo2, hr_flag, sp_flag);
}

/**
 * @brief 上报函数3：地理位置 (经纬度)
 */
void ESP_Report_GPS(void) {
    // 移除数据有效性检查，即使为0也上传
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"latitude\\\":%.6f\\,\\\"longitude\\\":%.6f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, latitude, longitude);
}

/**
 * @brief 上报函数4A：姿态角度 (欧拉角)
 */
void ESP_Report_Euler(void) {
    // 检查浮点数是否有效（避免NaN或Inf）
    float p = (pitch != pitch) ? 0.0f : pitch;  // NaN检查
    float r = (roll != roll) ? 0.0f : roll;
    float y = (yaw != yaw) ? 0.0f : yaw;

    // 注意：属性名必须与华为云物模型完全一致（小写pitch/roll/yaw）
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"pitch\\\":%.1f\\,\\\"roll\\\":%.1f\\,\\\"yaw\\\":%.1f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, p, r, y);
}

/**
 * @brief 上报函数4B：安全标志位 (跌倒/碰撞/环境报警)
 */
void ESP_Report_Flags(void) {
    // 显式转换bool为int，确保格式正确
    int ff = (int)fall_flag;
    int cf = (int)collision_flag;
    int tf = (int)temp_flag;
    int mf = (int)mq2_flag;

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"fall_flag\\\":%d\\,\\\"collision_flag\\\":%d\\,\\\"temp_flag\\\":%d\\,\\\"mq2_flag\\\":%d}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, ff, cf, tf, mf);
}



