#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "bsp_system.h"

// 任务优先级定义
typedef enum {
    TASK_PRIO_CRITICAL = 0,  // 最高优先级：跌倒/碰撞检测
    TASK_PRIO_HIGH     = 1,  // 高优先级：心率/血氧
    TASK_PRIO_NORMAL   = 2,  // 普通优先级：温湿度/烟雾/GPS
    TASK_PRIO_LOW      = 3   // 低优先级：显示/上报
} TaskPriority_t;

void scheduler_init(void);
void scheduler_run(void);

#endif
