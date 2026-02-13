/**
 * JSON格式验证
 *
 * 问题：之前使用了 \\, 来转义逗号，这是错误的
 *
 * 错误示例：
 * "{\"temp\":25.5\,\"humi\":60.2}"  // ❌ 逗号不需要转义
 *
 * 正确示例：
 * "{\"temp\":25.5,\"humi\":60.2}"   // ✓ 直接使用逗号
 */

// ============ 实际发送的AT指令格式 ============

// 环境数据（温度=25.5, 湿度=60.2, 烟雾=12.34）
AT+MQTTPUB=0,"$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report","{\"services\":[{\"service_id\":\"SensorData\",\"properties\":{\"temp\":25.5,\"humi\":60.2,\"ppm\":12.34}}]}",0,0

// 身体数据（心率=75, 血氧=98）
AT+MQTTPUB=0,"$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report","{\"services\":[{\"service_id\":\"SensorData\",\"properties\":{\"dis_hr\":75,\"dis_spo2\":98,\"heartrate_flag\":0,\"spo2_flag\":0}}]}",0,0

// GPS数据（纬度=39.909230, 经度=116.397428）
AT+MQTTPUB=0,"$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report","{\"services\":[{\"service_id\":\"SensorData\",\"properties\":{\"latitude\":39.909230,\"longitude\":116.397428}}]}",0,0

// 姿态数据（Pitch=4.9, Roll=0.2, Yaw=180）
AT+MQTTPUB=0,"$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report","{\"services\":[{\"service_id\":\"SensorData\",\"properties\":{\"Pitch\":4.9,\"Roll\":0.2,\"Yaw\":180.0,\"fall_flag\":0,\"collision_flag\":0,\"temp_flag\":0,\"mq2_flag\":1}}]}",0,0


// ============ JSON格式说明 ============

/**
 * 华为云物模型JSON格式：
 *
 * {
 *   "services": [
 *     {
 *       "service_id": "SensorData",
 *       "properties": {
 *         "temp": 25.5,      // 注意：这里的逗号不需要转义
 *         "humi": 60.2,
 *         "ppm": 12.34
 *       }
 *     }
 *   ]
 * }
 *
 * 在AT指令中，JSON作为字符串参数，需要：
 * 1. 整个JSON用双引号包围
 * 2. JSON内部的双引号需要转义为 \"
 * 3. 逗号、冒号、花括号等不需要转义
 */


// ============ C代码中的转义规则 ============

/**
 * 在C字符串中：
 *
 * 1. \" 表示双引号字符
 * 2. \\ 表示反斜杠字符
 * 3. \n 表示换行符
 * 4. \r 表示回车符
 *
 * 在AT指令的JSON参数中：
 *
 * C代码：  "AT+MQTTPUB=0,\"topic\",\"{\\\"temp\\\":25.5}\",0,0\r\n"
 * 实际发送： AT+MQTTPUB=0,"topic","{\"temp\":25.5}",0,0\r\n
 *
 * 注意：
 * - \\\" 在C中表示 \"（转义的引号）
 * - 逗号直接写，不需要 \\,
 */


// ============ 常见错误 ============

// 错误1：逗号转义（之前的错误）
// C代码：  "{\\\"temp\\\":25.5\\,\\\"humi\\\":60.2}"
// 实际发送： {"temp":25.5\,"humi":60.2}  // ❌ JSON解析失败

// 错误2：缺少转义
// C代码：  "{"temp":25.5,"humi":60.2}"
// 编译错误：字符串中的引号未转义

// 正确写法：
// C代码：  "{\\\"temp\\\":25.5,\\\"humi\\\":60.2}"
// 实际发送： {"temp":25.5,"humi":60.2}  // ✓ 正确


// ============ 验证方法 ============

/**
 * 方法1：使用在线JSON验证器
 *
 * 将实际发送的JSON（去掉AT指令部分）复制到：
 * https://jsonlint.com/
 *
 * 例如：
 * {"services":[{"service_id":"SensorData","properties":{"temp":25.5,"humi":60.2,"ppm":12.34}}]}
 *
 * 如果显示"Valid JSON"，说明格式正确
 */

/**
 * 方法2：使用串口监听
 *
 * 如果有USB转TTL模块，可以监听USART3（ESP8266串口）：
 * 1. 连接TX到监听设备的RX
 * 2. 波特率设置为115200
 * 3. 查看实际发送的AT指令
 * 4. 检查JSON格式是否正确
 */

/**
 * 方法3：华为云设备日志
 *
 * 登录华为云IoT平台：
 * 1. 进入设备详情页
 * 2. 查看"设备日志"
 * 3. 如果有JSON解析错误，会显示错误信息
 */