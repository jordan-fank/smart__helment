# ESP数据上传调试完全指南

## 当前问题

❌ STM32无法上传数据到华为云
❌ 华为云平台看不到数据更新

## 调试步骤（按顺序执行）

### 步骤1：检查ESP8266连接状态

在main.c的主循环中添加测试代码：

```c
// 在 while(1) 循环开始处添加
static uint32_t test_timer = 0;
static uint8_t test_step = 0;

if (HAL_GetTick() - test_timer > 10000) {  // 每10秒测试一次
    test_timer = HAL_GetTick();

    switch(test_step) {
        case 0:
            // 测试1：查询WiFi连接状态
            ESP_Send("AT+CWJAP?\r\n");
            break;

        case 1:
            // 测试2：查询MQTT连接状态
            ESP_Send("AT+MQTTCONN?\r\n");
            break;

        case 2:
            // 测试3：发送最简单的测试消息
            ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"test\\\":1}\",0,0\r\n", HUAWEI_PUB_TOPIC);
            break;

        case 3:
            // 测试4：发送环境数据
            ESP_Report_Env();
            break;
    }

    test_step = (test_step + 1) % 4;
}
```

### 步骤2：监听ESP8266的响应

**方法A：使用串口2（如果可用）**

在esp01s.c中添加调试输出：

```c
void ESP_Send(char *fmt, ...) {
    char buf[512];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, sizeof(buf), fmt, arg);
    va_end(arg);

    // 发送到ESP8266
    HAL_UART_Transmit(&huart3, (uint8_t *)buf, strlen(buf), 1000);

    // 同时输出到调试串口（如果串口2可用）
    HAL_UART_Transmit(&huart2, (uint8_t *)"[TX] ", 5, 100);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 1000);
}
```

**方法B：使用OLED显示**

```c
void ESP_Report_Env(void) {
    static uint8_t send_count = 0;

    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\",\\\"properties\\\":{\\\"temp\\\":%.1f,\\\"humi\\\":%.1f,\\\"ppm\\\":%.2f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, temp, humi, ppm);

    send_count++;

    // 在OLED上显示发送次数
    char msg[32];
    sprintf(msg, "MQTT Sent: %d", send_count);
    OLED_ShowString(0, 0, msg, 8);
}
```

### 步骤3：检查变量值

在调度器中添加变量检查：

```c
// 在scheduler.c的scheduler_run()开始处添加
static uint32_t debug_timer = 0;

if (HAL_GetTick() - debug_timer > 5000) {
    debug_timer = HAL_GetTick();

    // 使用OLED显示变量值
    char line[32];

    sprintf(line, "T:%.1f H:%.1f", temp, humi);
    OLED_ShowString(0, 0, line, 8);

    sprintf(line, "P:%.1f R:%.1f", pitch, roll);
    OLED_ShowString(0, 1, line, 8);

    sprintf(line, "HR:%d SPO2:%d", dis_hr, dis_spo2);
    OLED_ShowString(0, 2, line, 8);
}
```

### 步骤4：简化测试

**替换esp01s.c的内容为esp01s_test.c**

1. 备份当前的esp01s.c
2. 将esp01s_test.c的内容复制到esp01s.c
3. 重新编译烧录

**在main.c中添加简单测试**：

```c
// 在主循环中
static uint32_t mqtt_test_timer = 0;

if (HAL_GetTick() - mqtt_test_timer > 5000) {  // 每5秒测试一次
    mqtt_test_timer = HAL_GetTick();

    // 只发送最简单的测试消息
    ESP_Test_Simple();

    // 在OLED上显示
    OLED_ShowString(0, 0, "MQTT Test Sent", 8);
}
```

### 步骤5：检查华为云物模型

登录华为云IoT平台，确认：

1. **产品模型** → **服务定义**
   - 服务ID必须是：`SensorData`（区分大小写）
   - 服务类型：属性

2. **属性定义**（至少包含）：
   ```
   test (int)      // 用于测试
   temp (float)    // 温度
   humi (float)    // 湿度
   ppm (float)     // 烟雾浓度
   ```

3. **设备详情** → **设备影子**
   - 查看是否有数据更新
   - 查看最后更新时间

### 步骤6：使用AT指令手动测试

如果有USB转TTL模块，可以直接连接ESP8266测试：

```
1. 连接ESP8266的TX/RX到USB转TTL
2. 打开串口工具（波特率115200）
3. 手动发送AT指令：

AT+RST
AT+CWMODE=1
AT+CWJAP="CU_KY2D","cbcbytk9"
AT+MQTTUSERCFG=0,1,"698c76237f2e6c302f534208_Helmet_01_0_0_2026021113","698c76237f2e6c302f534208_Helmet_01","11b4586aa5ff0eb265acb797b3705841f0eb4be2e14280c2f2c27dc8622a1890",0,0,""
AT+MQTTCONN=0,"0c27d78187.st1.iotda-device.cn-north-4.myhuaweicloud.com",1883,1
AT+MQTTPUB=0,"$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report","{\"services\":[{\"service_id\":\"SensorData\",\"properties\":{\"test\":1}}]}",0,0
```

观察ESP8266的响应：
- `OK` - 命令成功
- `ERROR` - 命令失败
- `+MQTTPUB:OK` - MQTT发布成功
- `+MQTTPUB:FAIL` - MQTT发布失败

## 常见问题排查

### 问题1：WiFi连接失败
**症状**：AT+CWJAP返回ERROR或FAIL
**解决**：
- 检查WiFi名称和密码是否正确
- 检查WiFi信号强度
- 尝试重启ESP8266

### 问题2：MQTT连接失败
**症状**：AT+MQTTCONN返回ERROR
**解决**：
- 检查华为云地址是否正确
- 检查设备凭证（ClientID、Username、Password）
- 检查设备是否在华为云平台注册

### 问题3：MQTT发布失败
**症状**：AT+MQTTPUB返回+MQTTPUB:FAIL
**可能原因**：
1. JSON格式错误
2. Topic错误
3. 物模型不匹配
4. 消息过长（超过256字节）

**解决**：
- 先发送最简单的JSON测试
- 检查物模型定义
- 逐步增加属性，找出问题属性

### 问题4：数据不更新
**症状**：MQTT发布成功，但华为云看不到数据
**可能原因**：
1. 服务ID不匹配（区分大小写）
2. 属性名称不匹配
3. 数据类型不匹配（如int vs float）

**解决**：
- 检查华为云物模型定义
- 确保服务ID完全一致
- 确保属性名称和类型匹配

## 推荐调试流程

```
第1天：基础连接测试
1. 验证WiFi连接
2. 验证MQTT连接
3. 发送最简单的测试消息

第2天：逐步增加复杂度
1. 发送单个属性（test:1）
2. 发送两个属性（temp, humi）
3. 发送完整环境数据

第3天：完整功能测试
1. 测试所有上报函数
2. 验证数据准确性
3. 测试错开时间的调度
```

## 最简单的验证方法

如果以上都太复杂，最简单的方法：

1. **在main.c的主循环中添加**：
```c
static uint32_t simple_test = 0;
if (HAL_GetTick() - simple_test > 10000) {
    simple_test = HAL_GetTick();

    // 直接发送，不通过调度器
    ESP_Send("AT+MQTTPUB=0,\"$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\",\\\"properties\\\":{\\\"test\\\":1}}]}\",0,0\r\n");

    // 在OLED上显示
    OLED_ShowString(0, 0, "Test Sent", 8);
}
```

2. **观察华为云设备影子**
   - 如果10秒后看到test属性更新，说明连接正常
   - 如果没有更新，说明MQTT连接有问题

3. **逐步添加属性**
   - 先加temp
   - 再加humi
   - 最后加ppm
   - 找出哪个属性导致失败

## 需要提供的信息

如果问题仍未解决，请提供：

1. ESP8266的响应（如果能看到）
2. 华为云设备日志截图
3. OLED显示的内容
4. 是否看到"Test Sent"字样
5. 华为云设备影子是否有更新