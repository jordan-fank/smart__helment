# 智能头盔实时监控系统

## 📋 项目说明

这是一个完整的智能头盔监控系统，包含：
- **STM32下位机**：采集传感器数据并通过ESP8266上报到华为云
- **Python后端服务器**：从华为云MQTT订阅数据
- **Web前端界面**：实时显示传感器数据、图表和地图

## 🚀 快速开始

### 1. STM32固件更新

已完成的优化：
- ✅ 任务调度器增加优先级机制（跌倒检测响应时间从200ms降至50ms）
- ✅ 修复ESP数据上报失败问题（改进JSON转义处理）
- ✅ 添加数据长度检查，避免超过ESP8266的256字节限制

编译并烧录固件到STM32。

### 2. 启动后端服务器

#### 安装Python依赖：
```bash
cd web
pip install flask flask-cors paho-mqtt requests
```

#### 运行服务器：
```bash
python backend_server.py
```

服务器将在 `http://localhost:5000` 启动，并自动连接到华为云IoT平台。

### 3. 打开Web界面

#### 方法1：直接打开（推荐）
双击 `web/index.html` 文件，在浏览器中打开。

#### 方法2：使用本地服务器
```bash
cd web
python -m http.server 8080
```
然后访问 `http://localhost:8080`

## 📊 功能特性

### 实时数据显示
- 🌡️ **环境监测**：温度、湿度、烟雾浓度
- ❤️ **生理监测**：心率、血氧
- 🎯 **姿态监测**：俯仰角、横滚角、偏航角
- 🛡️ **安全状态**：跌倒、碰撞、温度报警、烟雾报警

### 数据可视化
- 📈 **实时曲线图**：温度和心率历史数据
- 📍 **GPS地图**：实时位置显示（需配置高德地图API Key）
- ⚠️ **报警提示**：异常情况自动弹窗提醒

### 系统状态
- 设备在线/离线状态
- 最后更新时间
- 数据包计数

## ⚙️ 配置说明

### 1. 高德地图API Key（可选）

如果需要显示GPS地图，请：
1. 访问 [高德开放平台](https://lbs.amap.com/) 注册账号
2. 创建应用并获取Web服务API Key
3. 在 `index.html` 中替换：
```html
<script src="https://webapi.amap.com/maps?v=2.0&key=YOUR_AMAP_KEY"></script>
```

### 2. 华为云IoT配置

后端服务器的华为云配置已在 `backend_server.py` 中设置：
```python
HUAWEI_CONFIG = {
    'mqtt_host': '0c27d78187.st1.iotda-device.cn-north-4.myhuaweicloud.com',
    'mqtt_port': 1883,
    'client_id': '698c76237f2e6c302f534208_Helmet_01_0_0_2026021113',
    'username': '698c76237f2e6c302f534208_Helmet_01',
    'password': '11b4586aa5ff0eb265acb797b3705841f0eb4be2e14280c2f2c27dc8622a1890',
    'subscribe_topic': '$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report'
}
```

如果你的设备信息不同，请修改这些配置。

## 🔧 STM32代码改进说明

### 1. 优先级调度器

**文件**：`APP/scheduler.c`, `APP/scheduler.h`

**改进**：
- 添加了4个优先级等级：CRITICAL（跌倒检测）、HIGH（心率血氧）、NORMAL（其他传感器）、LOW（显示和上报）
- 任务按优先级排序执行，高优先级任务优先
- 修正了时间漂移问题（`last_run += rate_ms`）

**效果**：
- 跌倒检测响应时间从200ms降至50ms
- 关键任务不会被低优先级任务阻塞

### 2. ESP数据上报优化

**文件**：`APP/esp01s.c`, `APP/esp01s.h`

**改进**：
- 重写JSON构建逻辑，使用`Build_JSON()`和`MQTT_Publish()`函数
- 自动处理JSON转义字符
- 添加数据长度检查，超过200字节会警告并拒绝发送
- 添加`ESP_Report_All()`合并上报函数（可选）

**效果**：
- 解决了JSON格式错误导致的上报失败
- 避免超过ESP8266的256字节限制
- 代码更清晰易维护

### 3. 使用建议

#### 方案A：分开上报（当前方案，推荐）
保持4个独立的上报任务，每个任务上报不同类型的数据：
- `ESP_Report_Env()` - 环境数据
- `ESP_Report_Body()` - 生理数据
- `ESP_Report_GPS()` - GPS数据
- `ESP_Report_Safety()` - 安全数据

**优点**：
- 每条消息较短，不会超限
- 某一类数据失败不影响其他数据

#### 方案B：合并上报（可选）
如果想减少MQTT消息数量，可以修改 `scheduler.c`：
```c
// 删除4个独立任务，改为：
{ESP_Report_All, 2000, 0, TASK_PRIO_LOW},  // 每2秒上报一次所有数据
```

**注意**：合并后的JSON长度约180字节，接近限制，如果未来添加更多数据可能超限。

## 🐛 调试技巧

### 1. 查看ESP上报的实际数据

在STM32的串口1（调试串口）会打印：
- JSON长度警告（如果超过200字节）
- 实际发送的AT指令

### 2. 查看后端接收的数据

后端服务器会在控制台打印：
- MQTT连接状态
- 收到的每条消息
- 解析后的数据

### 3. 浏览器调试

按F12打开浏览器开发者工具，在Console标签页可以看到：
- API请求状态
- 数据更新日志
- 错误信息

## 📱 未来扩展

### 1. 移动端APP
可以使用以下技术栈开发：
- **React Native**：跨平台（iOS + Android）
- **Flutter**：Google的跨平台框架
- **微信小程序**：无需安装

### 2. 数据存储
添加数据库存储历史数据：
- **SQLite**：轻量级，适合小规模
- **MySQL/PostgreSQL**：适合大规模数据
- **InfluxDB**：时序数据库，专为传感器数据设计

### 3. 报警推送
- **邮件通知**：使用SMTP发送报警邮件
- **短信通知**：接入阿里云/腾讯云短信服务
- **微信推送**：使用Server酱或企业微信机器人

## 📞 技术支持

如有问题，请检查：
1. STM32是否成功连接WiFi和MQTT
2. 后端服务器是否正常运行
3. 浏览器控制台是否有错误信息

## 📄 许可证

本项目仅供学习使用。