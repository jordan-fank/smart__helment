#include "esp01s.h"

// 外部变量声明（姿态数据）
extern float pitch, roll, yaw;

// 简单的打印函数，方便发送串口数据
void ESP_Send(char *fmt, ...) {
    char buf[512];
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

    my_printf(&huart1, "ESP Init Done\r\n");
}

/**
 * @brief 上报函数1：基础环境 (温度、湿度、烟雾) - 原始版本
 */
void ESP_Report_Env(void) {
    // 调试输出
    my_printf(&huart1, "[ENV] temp=%.1f, humi=%.1f, ppm=%.2f\r\n", temp, humi, ppm);

    // 使用原来验证过的方式
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"temp\\\":%.1f\\,\\\"humi\\\":%.1f\\,\\\"ppm\\\":%.2f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, temp, humi, ppm);
}

/**
 * @brief 上报函数2：身体健康 (心率、血氧 + 异常标志) - 原始版本
 */
void ESP_Report_Body(void) {
    // 调试输出
    my_printf(&huart1, "[BODY] hr=%d, spo2=%d\r\n", dis_hr, dis_spo2);

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"dis_hr\\\":%d\\,\\\"dis_spo2\\\":%d\\,\\\"heartrate_flag\\\":%d\\,\\\"spo2_flag\\\":%d}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, dis_hr, dis_spo2, heartrate_flag, spo2_flag);
}

/**
 * @brief 上报函数3：地理位置 (经纬度) - 原始版本
 */
void ESP_Report_GPS(void) {
    // 调试输出
    my_printf(&huart1, "[GPS] lat=%.6f, lon=%.6f\r\n", latitude, longitude);

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"latitude\\\":%.6f\\,\\\"longitude\\\":%.6f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, latitude, longitude);
}

/**
 * @brief 上报函数4：姿态与安全 (欧拉角 + 跌倒/碰撞/环境报警) - 原始版本
 */
void ESP_Report_Safety(void) {
    // 调试输出
    my_printf(&huart1, "[SAFETY] pitch=%.1f, roll=%.1f, yaw=%.1f\r\n", pitch, roll, yaw);

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"Pitch\\\":%.1f\\,\\\"Roll\\\":%.1f\\,\\\"Yaw\\\":%.1f\\,\\\"fall_flag\\\":%d\\,\\\"collision_flag\\\":%d\\,\\\"temp_flag\\\":%d\\,\\\"mq2_flag\\\":%d}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, pitch, roll, yaw, fall_flag, collision_flag, temp_flag, mq2_flag);
}