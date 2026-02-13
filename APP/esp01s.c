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
 * @brief 构建JSON字符串（用于MQTT发布）
 * @param json_buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param properties JSON属性字符串（已格式化好的键值对）
 */
static void Build_JSON(char *json_buf, size_t buf_size, const char *properties) {
    snprintf(json_buf, buf_size,
        "{\"services\":[{\"service_id\":\"SensorData\",\"properties\":{%s}}]}",
        properties);
}

/**
 * @brief 发送MQTT消息
 * @param json JSON字符串（已包含转义的引号）
 */
static void MQTT_Publish(const char *json) {
    // 检查JSON长度
    size_t json_len = strlen(json);

    if (json_len > 200) {
        my_printf(&huart1, "WARN: JSON too long (%d bytes)\r\n", json_len);
        return;
    }

    // 构建完整的AT指令
    char at_cmd[512];
    int len = snprintf(at_cmd, sizeof(at_cmd),
        "AT+MQTTPUB=0,\"%s\",\"%s\",0,0\r\n",
        HUAWEI_PUB_TOPIC, json);

    if (len > 0 && len < sizeof(at_cmd)) {
        // 发送AT指令
        HAL_UART_Transmit(&huart3, (uint8_t *)at_cmd, len, 1000);

        // 调试输出
        my_printf(&huart1, "MQTT Pub: %d bytes\r\n", json_len);
    } else {
        my_printf(&huart1, "ERROR: AT command too long\r\n");
    }
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

/**
 * @brief 上报函数4：姿态与安全 (合并版本，保留备用)
 */
void ESP_Report_Safety(void) {
    // 显式转换bool为int，确保格式正确
    int ff = (int)fall_flag;
    int cf = (int)collision_flag;
    int tf = (int)temp_flag;
    int mf = (int)mq2_flag;

    // 检查浮点数是否有效（避免NaN或Inf）
    float p = (pitch != pitch) ? 0.0f : pitch;  // NaN检查
    float r = (roll != roll) ? 0.0f : roll;
    float y = (yaw != yaw) ? 0.0f : yaw;

    // 注意：属性名必须与华为云物模型完全一致（小写pitch/roll/yaw）
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"pitch\\\":%.1f\\,\\\"roll\\\":%.1f\\,\\\"yaw\\\":%.1f\\,\\\"fall_flag\\\":%d\\,\\\"collision_flag\\\":%d\\,\\\"temp_flag\\\":%d\\,\\\"mq2_flag\\\":%d}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, p, r, y, ff, cf, tf, mf);
}

/**
 * @brief 合并上报所有数据（可选方案，但可能超长度限制）
 * 注意：此函数会将所有数据合并为一条MQTT消息
 * 如果数据过多可能超过ESP8266的256字节限制，请谨慎使用
 */
void ESP_Report_All(void) {
    char properties[400];
    char json[512];

    // 合并所有属性（注意长度控制）
    snprintf(properties, sizeof(properties),
        "\"temp\":%.1f,\"humi\":%.1f,\"ppm\":%.2f,"
        "\"dis_hr\":%d,\"dis_spo2\":%d,"
        "\"latitude\":%.6f,\"longitude\":%.6f,"
        "\"pitch\":%.1f,\"roll\":%.1f,\"yaw\":%.1f,"
        "\"fall_flag\":%d,\"collision_flag\":%d,"
        "\"heartrate_flag\":%d,\"spo2_flag\":%d,"
        "\"temp_flag\":%d,\"mq2_flag\":%d",
        temp, humi, ppm,
        dis_hr, dis_spo2,
        latitude, longitude,
        pitch, roll, yaw,
        fall_flag, collision_flag,
        heartrate_flag, spo2_flag,
        temp_flag, mq2_flag);

    Build_JSON(json, sizeof(json), properties);
    MQTT_Publish(json);
}

/**
 * @brief 测试函数：只上报欧拉角
 */
void ESP_Report_Euler_Only(void) {
    float p = (pitch != pitch) ? 0.0f : pitch;
    float r = (roll != roll) ? 0.0f : roll;
    float y = (yaw != yaw) ? 0.0f : yaw;

    // 注意：属性名必须与华为云物模型完全一致（小写pitch/roll/yaw）
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"pitch\\\":%.1f\\,\\\"roll\\\":%.1f\\,\\\"yaw\\\":%.1f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, p, r, y);
}

/**
 * @brief 测试函数：只上报标志位
 */
void ESP_Report_Flags_Only(void) {
    int ff = (int)fall_flag;
    int cf = (int)collision_flag;
    int tf = (int)temp_flag;
    int mf = (int)mq2_flag;

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"fall_flag\\\":%d\\,\\\"collision_flag\\\":%d\\,\\\"temp_flag\\\":%d\\,\\\"mq2_flag\\\":%d}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, ff, cf, tf, mf);
}


