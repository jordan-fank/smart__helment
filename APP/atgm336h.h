#ifndef _ATGM336H_H   // 防止头文件重复包含（幂等性处理）
#define _ATGM336H_H

#include "bsp_system.h" // 包含底层系统支持包

// --- 缓冲区长度定义 ---
#define USART_REC_LEN      1024  // DMA串口接收池大小，必须容纳GPS一秒喷出的所有报文（约800字节）
#define GPS_Buffer_Length  80    // 单条$GNRMC语句的最大期望长度
#define UTCTime_Length     11    // 时间字段长度 (格式: hhmmss.sss)
#define latitude_Length    11    // 纬度字段长度 (格式: ddmm.mmmm)
#define N_S_Length         2     // 南北纬标识位长度 (N/S + 结束符)
#define longitude_Length   12    // 经度字段长度 (格式: dddmm.mmmm)
#define E_W_Length         2     // 东西经标识位长度 (E/W + 结束符)

// --- 结构体定义：原始数据解析仓 ---
// 逻辑：将GPS吐出的“字符串”暂时存放在这里，作为解析的中转站
typedef struct SaveData
{
    char GPS_Buffer[GPS_Buffer_Length];  // 存放筛选出的$GNRMC完整原始字符串
    char isGetData;                      // 标志位：1代表串口已捕获到一帧潜在的RMC数据
    char isParseData;                    // 标志位：1代表该帧数据已通过算法解析完成
    char UTCTime[UTCTime_Length];        // 提取出的UTC时间字符串
    char latitude[latitude_Length];      // 提取出的原始纬度字符串
    char N_S[N_S_Length];                // 纬度方向 (N:北纬, S:南纬)
    char longitude[longitude_Length];    // 提取出的原始经度字符串
    char E_W[E_W_Length];                // 经度方向 (E:东经, W:西经)
    char isUsefull;                      // 核心标志位：'A'代表定位有效，'V'代表无效（正在搜星）
} _SaveData;

// --- 结构体定义：最终坐标输出仓 ---
// 逻辑：将度分格式转换为真正可以用于地图计算的“十进制度”浮点数
typedef struct _LatitudeAndLongitude_s
{
    float latitude;  // 转换后的浮点纬度 (例如: 31.2245)
    float longitude; // 转换后的浮点经度 (例如: 121.4321)
    char N_S;        // 方向备份
    char E_W;        // 方向备份
} LatitudeAndLongitude_s;

// --- 全局变量声明 ---
// 作用：告诉编译器这些变量在.c里定义了，其他文件(如main.c)可以直接调用
extern _SaveData Save_Data;
extern LatitudeAndLongitude_s g_LatAndLongData;
extern float longitude; // 供外部模块直接读取的最终经度
extern float latitude;  // 供外部模块直接读取的最终纬度

// --- 函数原型声明 ---
void atgm336h_init(void);    // 初始化GPS：配置DMA、串口空闲中断及环形缓冲区
void atgm336h_task(void);    // GPS任务引擎：从环形缓冲区捞数据并触发解析
void parse_uart_command(uint8_t *buffer, uint16_t length); // 核心解析器：寻找帧头并切片字符串
void clrStruct(void);        // 数据重置：在搜星失败或初始化时清空旧坐标

#endif


