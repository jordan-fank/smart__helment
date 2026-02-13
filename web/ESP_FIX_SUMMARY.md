# ESP数据上报问题修复总结

## 问题分析

### 原因1：任务时间冲突
**问题**：4个ESP上报任务都是1000ms周期，且同时启动（last_run都是0），导致：
- 4条MQTT消息在同一时刻发送
- ESP8266处理不过来，导致部分消息丢失
- MQTT队列拥塞

**解决方案**：
```c
// 修改前：4个任务同时触发
{ESP_Report_Env, 1000, 0, TASK_PRIO_LOW},
{ESP_Report_Body, 1000, 0, TASK_PRIO_LOW},
{ESP_Report_GPS, 1000, 0, TASK_PRIO_LOW},
{ESP_Report_Safety, 1000, 0, TASK_PRIO_LOW},

// 修改后：错开500ms，避免冲突
{ESP_Report_Env, 2000, 0, TASK_PRIO_LOW},      // 0ms启动
{ESP_Report_Body, 2000, 500, TASK_PRIO_LOW},   // 500ms启动
{ESP_Report_GPS, 2000, 1000, TASK_PRIO_LOW},   // 1000ms启动
{ESP_Report_Safety, 2000, 1500, TASK_PRIO_LOW},// 1500ms启动
```

**时间线**：
```
0ms    -> ESP_Report_Env
500ms  -> ESP_Report_Body
1000ms -> ESP_Report_GPS
1500ms -> ESP_Report_Safety
2000ms -> ESP_Report_Env (下一轮)
2500ms -> ESP_Report_Body (下一轮)
...
```

### 原因2：JSON构建逻辑错误
**问题**：修改后的代码使用了复杂的JSON转义逻辑，导致：
- 双重转义，JSON格式错误
- 华为云无法解析

**解决方案**：
- 回退到原来验证过的格式
- 直接使用转义字符串，不做额外处理

### 原因3：数据无效
**问题**：
- 心率血氧数据可能为0（传感器未检测到手指）
- GPS数据可能为0（未定位）
- 这些无效数据上传到华为云没有意义

**解决方案**：
```c
// 添加数据有效性检查
void ESP_Report_Body(void) {
    if (dis_hr == 0 && dis_spo2 == 0) {
        return;  // 数据无效，跳过上报
    }
    // 发送数据...
}

void ESP_Report_GPS(void) {
    if (latitude == 0.0f && longitude == 0.0f) {
        return;  // GPS未定位，跳过上报
    }
    // 发送数据...
}
```

## 修改内容

### 1. scheduler.c
- 将ESP上报周期从1000ms改为2000ms
- 错开启动时间：0ms, 500ms, 1000ms, 1500ms
- 减少MQTT消息频率，避免拥塞

### 2. esp01s.c
- 回退到原来验证过的JSON格式
- 添加数据有效性检查
- 移除复杂的JSON构建逻辑

## 预期效果

### 修改前
```
问题：
- 温湿度、烟雾能上传 ✓
- 心率血氧不能上传 ✗
- GPS不能上传 ✗
- 姿态数据不能上传 ✗

原因：
- 4个任务同时发送，MQTT拥塞
- JSON格式错误
- 数据可能无效
```

### 修改后
```
预期：
- 温湿度、烟雾能上传 ✓
- 心率血氧能上传 ✓（有手指时）
- GPS能上传 ✓（定位后）
- 姿态数据能上传 ✓

改进：
- 任务错开执行，无冲突
- JSON格式正确
- 只上传有效数据
```

## 测试步骤

1. **编译烧录**
   - 重新编译项目
   - 烧录到STM32

2. **观察华为云**
   - 登录华为云IoT平台
   - 查看设备数据
   - 应该能看到所有数据类型

3. **观察Web界面**
   - 打开监控网页
   - 应该能看到实时数据更新
   - 数据包数量持续增加

4. **测试各传感器**
   - 温湿度：应该实时显示
   - 烟雾：应该实时显示
   - 心率血氧：手指放上去后应该显示
   - GPS：定位后应该显示经纬度
   - 姿态：移动头盔应该看到角度变化

## 如果还有问题

### 检查华为云物模型
确保在华为云IoT平台的"产品模型"中定义了所有属性：

**环境数据**：
- temp (float)
- humi (float)
- ppm (float)

**生理数据**：
- dis_hr (int)
- dis_spo2 (int)
- heartrate_flag (int)
- spo2_flag (int)

**GPS数据**：
- latitude (float)
- longitude (float)

**姿态数据**：
- Pitch (float)
- Roll (float)
- Yaw (float)
- fall_flag (int)
- collision_flag (int)
- temp_flag (int)
- mq2_flag (int)

### 检查变量初始化
在main.c中添加测试代码：
```c
// 在主循环前
my_printf(&huart2, "=== Variable Test ===\r\n");
my_printf(&huart2, "pitch=%.1f, roll=%.1f, yaw=%.1f\r\n", pitch, roll, yaw);
my_printf(&huart2, "dis_hr=%d, dis_spo2=%d\r\n", dis_hr, dis_spo2);
my_printf(&huart2, "lat=%.6f, lon=%.6f\r\n", latitude, longitude);
```

### 使用串口2调试
如果串口1被语音模块占用，可以使用串口2（GPS串口）临时调试：
```c
// 在esp01s.c中添加
my_printf(&huart2, "[ESP] Sending data...\r\n");
```

## 优化建议

### 1. 进一步减少MQTT消息
如果华为云流量有限制，可以考虑：
- 将周期改为3000ms或5000ms
- 只在数据变化时上报（增量上报）

### 2. 添加重试机制
如果MQTT发送失败，可以添加重试：
```c
static uint8_t retry_count = 0;
if (mqtt_send_failed) {
    if (retry_count < 3) {
        retry_count++;
        // 重新发送
    }
}
```

### 3. 添加心跳检测
定期发送心跳消息，确保MQTT连接正常：
```c
void ESP_Heartbeat(void) {
    ESP_Send("AT+MQTTPUB=0,\"%s\",\"{\\\"heartbeat\\\":1}\",0,0\r\n", HUAWEI_PUB_TOPIC);
}
```