#ifndef _ESP01S_H
#define _ESP01S_H

#include "bsp_system.h"



// --- Wi-Fi ���Ӳ��� ---
#define WIFI_SSID        "CU_KY2D"     // ��� WiFi ����
#define WIFI_PWD         "cbcbytk9"    // ��� WiFi ����

// --- ��Ϊ�� MQTT ���ò��� (�Ѹ���Ϊ���ʵ����Ϣ) ---
#define HUAWEI_ADDR      "0c27d78187.st1.iotda-device.cn-north-4.myhuaweicloud.com"
#define HUAWEI_PORT      "1883"
#define HUAWEI_CLIENTID  "698c76237f2e6c302f534208_Helmet_01_0_0_2026021113"
#define HUAWEI_USERNAME  "698c76237f2e6c302f534208_Helmet_01"
#define HUAWEI_PASSWORD  "11b4586aa5ff0eb265acb797b3705841f0eb4be2e14280c2f2c27dc8622a1890"

// ��Ϊ���ϱ��� Topic
#define HUAWEI_PUB_TOPIC "$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report"

// 华为云命令下发订阅 Topic
#define HUAWEI_SUB_TOPIC "$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/commands/#"



// ��������
void ESP_Init(void);

void ESP_Report_Env(void);
void ESP_Report_Body(void);
void ESP_Report_GPS(void);
void ESP_Report_Safety(void);

// 拆分上报函数（推荐使用，减少单次数据量）
void ESP_Report_Euler(void);   // 只上报欧拉角
void ESP_Report_Flags(void);   // 只上报标志位

// 合并上报函数（可选，避免超长度限制）
void ESP_Report_All(void);

// 测试函数：分开上报欧拉角和标志位
void ESP_Report_Euler_Only(void);
void ESP_Report_Flags_Only(void);




#endif
