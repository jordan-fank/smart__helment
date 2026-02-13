/**
 * ESP数据上报调试指南
 *
 * 问题：设备能连接华为云，但没有数据上报
 *
 * 调试步骤：
 */

// ============ 步骤1：检查串口输出 ============
// 在STM32的串口1（调试串口）查看以下信息：

// 1. 是否有 "[ENV] temp=..." 这样的调试输出？
//    - 如果没有：说明ESP_Report_Env()函数没有被调用
//      → 检查scheduler.c中是否有ESP_Report_Env任务
//      → 检查任务周期是否合理
//
//    - 如果有：继续下一步

// 2. 是否有 "[ENV] JSON: ..." 这样的JSON输出？
//    - 如果没有：说明JSON构建失败
//      → 检查变量是否初始化
//
//    - 如果有：记录JSON内容，继续下一步

// 3. 是否有 "MQTT Pub: XX bytes" 这样的输出？
//    - 如果没有：说明MQTT_Publish被跳过
//      → 可能是JSON长度超限
//
//    - 如果有：说明AT指令已发送，继续下一步


// ============ 步骤2：检查ESP8266响应 ============
// 如果有条件，可以监听USART3（ESP8266串口）的数据

// 正常情况下，ESP8266会返回：
// +MQTTPUB:OK
// 或
// +MQTTPUB:FAIL

// 如果返回FAIL，可能的原因：
// 1. MQTT未连接
// 2. JSON格式错误
// 3. Topic错误


// ============ 步骤3：快速修复方案 ============

// 方案A：回退到原来的上报方式（最简单）
// 如果新代码有问题，可以暂时回退到原来的方式：

void ESP_Report_Env_OLD(void) {
    // 原来的代码（已验证可以工作）
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"temp\\\":%.1f\\,\\\"humi\\\":%.1f\\,\\\"ppm\\\":%.2f}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC, temp, humi, ppm);
}

// 方案B：简化JSON（测试用）
// 先用最简单的JSON测试，确保MQTT通道正常：

void ESP_Report_Test(void) {
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"services\\\":[{\\\"service_id\\\":\\\"SensorData\\\"\\,\\\"properties\\\":{\\\"test\\\":1}}]}\",0,0\r\n",
             HUAWEI_PUB_TOPIC);
}


// ============ 步骤4：检查变量初始化 ============

// 在main.c的主循环前添加测试代码：
void test_variables(void) {
    my_printf(&huart1, "=== Variable Test ===\r\n");
    my_printf(&huart1, "temp = %.1f\r\n", temp);
    my_printf(&huart1, "humi = %.1f\r\n", humi);
    my_printf(&huart1, "ppm = %.2f\r\n", ppm);
    my_printf(&huart1, "dis_hr = %d\r\n", dis_hr);
    my_printf(&huart1, "dis_spo2 = %d\r\n", dis_spo2);
    my_printf(&huart1, "pitch = %.1f\r\n", pitch);
    my_printf(&huart1, "roll = %.1f\r\n", roll);
    my_printf(&huart1, "yaw = %.1f\r\n", yaw);
    my_printf(&huart1, "latitude = %.6f\r\n", latitude);
    my_printf(&huart1, "longitude = %.6f\r\n", longitude);
    my_printf(&huart1, "====================\r\n");
}


// ============ 步骤5：检查调度器 ============

// 在scheduler.c的scheduler_run()中添加调试输出：
void scheduler_run_debug(void) {
    static uint32_t debug_timer = 0;
    uint32_t now = HAL_GetTick();

    // 每5秒打印一次调度器状态
    if (now - debug_timer > 5000) {
        debug_timer = now;
        my_printf(&huart1, "[SCHED] Running, tasks=%d\r\n", task_num);
    }

    // 原来的调度逻辑...
}


// ============ 常见问题排查 ============

/*
问题1：串口没有任何输出
解决：
- 检查USART1是否正确初始化
- 检查波特率设置
- 检查串口线是否连接正确

问题2：有调试输出，但数据全是0
解决：
- 检查传感器初始化是否成功
- 检查传感器任务是否正常执行
- 在传感器任务中添加调试输出

问题3：JSON格式看起来正确，但华为云没收到
解决：
- 检查华为云物模型定义是否匹配
- 检查Topic是否正确
- 检查MQTT连接状态（在ESP_Init后添加延时和状态查询）

问题4：之前能工作，现在不能了
解决：
- 检查是否修改了其他代码
- 尝试重新烧录固件
- 检查ESP8266是否正常（可能需要重新上电）
*/