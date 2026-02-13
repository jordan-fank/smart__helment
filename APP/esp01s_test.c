#include "esp01s.h"

// 外部变量声明
extern float pitch, roll, yaw;

// 简单的打印函数
void ESP_Send(char *fmt, ...) {
    char buf[512];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, sizeof(buf), fmt, arg);
    va_end(arg);
    HAL_UART_Transmit(&huart3, (uint8_t *)buf, strlen(buf), 1000);
}

/**
 * @brief ESP8266 初始化
 */
void ESP_Init(void) {
    // 1. 复位
    ESP_Send("AT+RST\r\n");
    HAL_Delay(2000);

    // 2. 设置为 STA 模式
    ESP_Send("AT+CWMODE=1\r\n");
    HAL_Delay(1000);

    // 3. 连接 Wi-Fi
    ESP_Send("AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PWD);
    HAL_Delay(6000);

    // 4. MQTT 配置
    ESP_Send("AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
             HUAWEI_CLIENTID, HUAWEI_USERNAME, HUAWEI_PASSWORD);
    HAL_Delay(1000);

    // 5. 连接 MQTT 服务器
    ESP_Send("AT+MQTTCONN=0,\"%s\",1883,1\r\n", HUAWEI_ADDR);
    HAL_Delay(2000);
}

/**
 * @brief 测试函数：发送最简单的MQTT消息
 */
void ESP_Test_Simple(void) {
    // 最简单的JSON，只有一个属性
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\",\\\"properties\\\":{\\\"test\\\":1}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC);
}

/**
 * @brief 上报环境数据（温度、湿度、烟雾）
 */
void ESP_Report_Env(void) {
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\",\\\"properties\\\":{\\\"temp\\\":%.1f,\\\"humi\\\":%.1f,\\\"ppm\\\":%.2f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, temp, humi, ppm);
}

/**
 * @brief 上报身体数据（心率、血氧）
 */
void ESP_Report_Body(void) {
    if (dis_hr == 0 && dis_spo2 == 0) {
        return;
    }

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\",\\\"properties\\\":{\\\"dis_hr\\\":%d,\\\"dis_spo2\\\":%d,\\\"heartrate_flag\\\":%d,\\\"spo2_flag\\\":%d}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, dis_hr, dis_spo2, heartrate_flag, spo2_flag);
}

/**
 * @brief 上报GPS数据
 */
void ESP_Report_GPS(void) {
    if (latitude == 0.0f && longitude == 0.0f) {
        return;
    }

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\",\\\"properties\\\":{\\\"latitude\\\":%.6f,\\\"longitude\\\":%.6f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, latitude, longitude);
}

/**
 * @brief 上报姿态和安全数据
 */
void ESP_Report_Safety(void) {
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\",\\\"properties\\\":{\\\"Pitch\\\":%.1f,\\\"Roll\\\":%.1f,\\\"Yaw\\\":%.1f,\\\"fall_flag\\\":%d,\\\"collision_flag\\\":%d,\\\"temp_flag\\\":%d,\\\"mq2_flag\\\":%d}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, pitch, roll, yaw, fall_flag, collision_flag, temp_flag, mq2_flag);
}