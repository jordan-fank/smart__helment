#ifndef _SENSOR_H
#define _SENSOR_H

#include "bsp_system.h"

/*-------------------- 报警优先级定义 --------------------*/
typedef enum {
    ALARM_PRIO_FALL      = 0,  // 跌倒 - 最高优先级 (生命危险)
    ALARM_PRIO_COLLISION = 1,  // 碰撞 - 高优先级 (意外伤害)
    ALARM_PRIO_TEMP      = 2,  // 温度过高 - 中优先级 (过热危险)
    ALARM_PRIO_MQ2       = 3,  // 烟雾浓度 - 中优先级 (火灾风险)
    ALARM_PRIO_HEART     = 4,  // 心率异常 - 低优先级
    ALARM_PRIO_SPO2      = 5,  // 血氧异常 - 最低优先级
    ALARM_PRIO_NONE      = 255 // 无报警
} AlarmPriority_t;

/*-------------------- 报警信息结构 --------------------*/
typedef struct {
    AlarmPriority_t priority;  // 优先级
    const char*      message;    // 报警消息字符串
} Alarm_t;

/*-------------------- 报警队列配置 --------------------*/
#define ALARM_QUEUE_SIZE  8   // 队列容量（足够容纳所有报警）

typedef struct {
    Alarm_t items[ALARM_QUEUE_SIZE];
    uint8_t head;              // 队头索引
    uint8_t tail;              // 队尾索引
    uint8_t count;             // 当前元素数量
} AlarmQueue_t;

/*-------------------- 报警队列操作函数 --------------------*/
void AlarmQueue_Init(AlarmQueue_t* queue);
uint8_t AlarmQueue_Push(AlarmQueue_t* queue, AlarmPriority_t priority, const char* message);
uint8_t AlarmQueue_Pop(AlarmQueue_t* queue, Alarm_t* alarm);
uint8_t AlarmQueue_IsEmpty(AlarmQueue_t* queue);
void AlarmQueue_ClearByPriority(AlarmQueue_t* queue, AlarmPriority_t priority);

/*-------------------- 外部变量声明 --------------------*/
extern AlarmQueue_t g_alarm_queue;   // 报警队列（全局）
extern bool alarm_flag;              // 报警使能标志

void send_flag_to_asr(void);

#endif
