# Web监控界面数据显示问题解决方案

## 问题现状

✅ **STM32 → 华为云**：数据上传成功（华为云平台能看到数据）
❌ **华为云 → Web界面**：数据无法显示

## 问题原因

华为云IoT平台的MQTT有两种接入方式：

### 1. 设备侧接入（Device Access）
- **用途**：设备上报数据到云端
- **凭证**：设备ID + 设备密钥
- **权限**：只能发布（Publish），不能订阅（Subscribe）
- **当前状态**：STM32使用此方式，正常工作 ✓

### 2. 应用侧接入（Application Access）
- **用途**：应用程序从云端获取设备数据
- **凭证**：应用ID + 应用密钥（Access Token）
- **权限**：可以订阅设备数据
- **当前状态**：后端服务器需要此方式，但未配置 ✗

## 解决方案

### 方案1：使用模拟数据（最简单，推荐学习使用）

**优点**：
- 无需配置华为云应用侧
- 可以立即看到Web界面效果
- 适合学习和演示

**步骤**：
1. 停止当前的backend_server.py
2. 运行新的简化版服务器：
```bash
python backend_server_simple.py
```
3. 打开Web界面，应该能看到模拟数据实时更新

**说明**：
- 数据每2秒更新一次
- 数据是随机生成的，用于测试界面功能
- 如果需要真实数据，使用方案2或方案3

### 方案2：配置华为云应用侧接入（推荐生产使用）

**步骤**：

#### 2.1 在华为云创建应用
1. 登录华为云IoT平台
2. 进入"应用管理" → "创建应用"
3. 记录应用ID和应用密钥

#### 2.2 获取Access Token
```python
import requests

# 华为云IAM认证
def get_access_token():
    url = "https://iam.cn-north-4.myhuaweicloud.com/v3/auth/tokens"
    headers = {"Content-Type": "application/json"}
    body = {
        "auth": {
            "identity": {
                "methods": ["password"],
                "password": {
                    "user": {
                        "name": "your_username",
                        "password": "your_password",
                        "domain": {"name": "your_domain"}
                    }
                }
            },
            "scope": {
                "project": {"name": "cn-north-4"}
            }
        }
    }
    response = requests.post(url, headers=headers, json=body)
    return response.headers.get('X-Subject-Token')
```

#### 2.3 订阅设备数据
```python
# 使用AMQP订阅（华为云推荐）
from pika import BlockingConnection, ConnectionParameters, PlainCredentials

credentials = PlainCredentials('your_app_id', 'your_app_secret')
connection = BlockingConnection(ConnectionParameters(
    host='your_amqp_endpoint',
    port=5671,
    credentials=credentials,
    ssl=True
))
channel = connection.channel()
channel.queue_declare(queue='device_data')
channel.basic_consume(queue='device_data', on_message_callback=callback)
channel.start_consuming()
```

### 方案3：直接从华为云读取设备影子（最简单的真实数据方案）

**原理**：
- 华为云会保存设备的最新状态（设备影子）
- 通过HTTP API定期查询设备影子
- 无需MQTT订阅

**实现**：

```python
import requests
import time

def get_device_shadow(access_token, device_id):
    """获取设备影子数据"""
    url = f"https://iotda.cn-north-4.myhuaweicloud.com/v5/iot/{project_id}/devices/{device_id}/shadow"
    headers = {
        "X-Auth-Token": access_token,
        "Content-Type": "application/json"
    }
    response = requests.get(url, headers=headers)
    return response.json()

# 定时轮询
while True:
    shadow = get_device_shadow(token, device_id)
    # 更新latest_data
    time.sleep(5)  # 每5秒查询一次
```

## 当前推荐方案

### 对于学习和演示：使用方案1（模拟数据）

**立即可用**，无需额外配置：

```bash
# 1. 停止当前服务器（Ctrl+C）

# 2. 运行简化版服务器
cd web
python backend_server_simple.py

# 3. 打开浏览器
# 访问 index.html
```

### 对于生产环境：使用方案3（设备影子）

**优点**：
- 获取真实数据
- 实现简单（只需HTTP API）
- 无需复杂的MQTT订阅

**缺点**：
- 有轮询延迟（建议5-10秒）
- 需要华为云Access Token

## 快速测试

### 测试模拟数据方案

1. 运行简化版服务器：
```bash
python backend_server_simple.py
```

2. 测试API：
```bash
curl http://localhost:5000/api/latest
```

3. 打开Web界面，应该能看到：
   - 数据实时更新
   - 图表动态变化
   - 地图位置移动

### 验证数据流

```
STM32 → ESP8266 → 华为云 ✓（已验证，数据正常上传）
                    ↓
                Web界面 ✗（需要配置应用侧接入）
```

使用模拟数据后：
```
模拟数据生成器 → 后端服务器 → Web界面 ✓
```

## 下一步计划

1. **短期**：使用模拟数据验证Web界面功能
2. **中期**：实现设备影子轮询，获取真实数据
3. **长期**：配置应用侧MQTT订阅，实现真正的实时推送

## 常见问题

### Q: 为什么不能直接用设备凭证订阅？
A: 华为云的安全设计，设备凭证只能上报数据，不能订阅。这样可以防止设备被恶意订阅。

### Q: 模拟数据和真实数据有什么区别？
A: 模拟数据是随机生成的，用于测试界面。真实数据来自STM32传感器。

### Q: 如何切换到真实数据？
A: 实现方案3（设备影子轮询）或方案2（应用侧MQTT订阅）。

### Q: 数据更新频率是多少？
A:
- STM32上报：每2秒（4个任务错开）
- 模拟数据：每2秒
- 设备影子轮询：建议5-10秒（避免API限流）

## 总结

当前最佳方案：
1. **立即使用**：运行`backend_server_simple.py`，使用模拟数据
2. **验证功能**：确认Web界面所有功能正常
3. **后续优化**：根据需要实现真实数据获取