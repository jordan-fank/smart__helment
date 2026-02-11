#include "sensor.h"
#include "max30102.h"

/*-------------------- 全局变量定义 --------------------*/
bool alarm_flag = 1;                    // 报警使能标志
AlarmQueue_t g_alarm_queue;             // 报警队列（全局）

/*-------------------- 优先级映射表 --------------------*/
/* 用于根据标志位快速查找对应的优先级 */
static const uint8_t flag_to_priority_map[6] = {
    [0] = ALARM_PRIO_FALL,       // fall_flag      -> 0
    [1] = ALARM_PRIO_COLLISION,  // collision_flag -> 1
    [2] = ALARM_PRIO_TEMP,       // temp_flag      -> 2
    [3] = ALARM_PRIO_MQ2,        // mq2_flag       -> 3
    [4] = ALARM_PRIO_HEART,      // heartrate_flag -> 4
    [5] = ALARM_PRIO_SPO2,       // spo2_flag      -> 5
};

/*-------------------- 报警消息表 --------------------*/
/* 存储各优先级的报警消息 */
static const char* alarm_messages[] = {
    [ALARM_PRIO_FALL]      = "fall",
    [ALARM_PRIO_COLLISION] = "collision",
    [ALARM_PRIO_TEMP]      = "temp",
    [ALARM_PRIO_MQ2]       = "density",
    [ALARM_PRIO_HEART]     = "heartrate",
    [ALARM_PRIO_SPO2]      = "spo2",
};

/*-------------------- 报警标志位指针数组 --------------------*/
/* 用于遍历和检查各报警标志位 */
static bool* alarm_flags[] = {
    &fall_flag,
    &collision_flag,
    &temp_flag,
    &mq2_flag,
    &heartrate_flag,
    &spo2_flag,
};

/**
 * @brief  初始化报警队列
 */
void AlarmQueue_Init(AlarmQueue_t* queue)
{
    queue->head  = 0;
    queue->tail  = 0;
    queue->count = 0;
}

/**
 * @brief  将报警压入队列（按优先级排序插入）
 * @note   队列始终保持按优先级从高到低排序
 * @retval 0: 成功, 1: 队列已满
 */
uint8_t AlarmQueue_Push(AlarmQueue_t* queue, AlarmPriority_t priority, const char* message)
{
    if (queue->count >= ALARM_QUEUE_SIZE) {
        return 1;  // 队列已满
    }

    /* 检查队列中是否已存在相同优先级的报警 */
    for (uint8_t i = 0; i < queue->count; i++) {
        uint8_t idx = (queue->head + i) % ALARM_QUEUE_SIZE;
        if (queue->items[idx].priority == priority) {
            return 1;  // 已存在，不重复添加
        }
    }

    /* 找到插入位置（保持优先级从高到低排序） */
    uint8_t insert_pos = queue->tail;
    for (uint8_t i = 0; i < queue->count; i++) {
        uint8_t idx = (queue->head + i) % ALARM_QUEUE_SIZE;
        if (queue->items[idx].priority > priority) {
            insert_pos = idx;
            break;
        }
    }

    /* 如果插入位置在队列中间，需要移动元素 */
    if (insert_pos != queue->tail) {
        /* 从队尾向前移动元素，为新元素腾出空间 */
        uint8_t move_idx = (queue->tail - 1 + ALARM_QUEUE_SIZE) % ALARM_QUEUE_SIZE;
        while (move_idx != (insert_pos - 1 + ALARM_QUEUE_SIZE) % ALARM_QUEUE_SIZE) {
            queue->items[(move_idx + 1) % ALARM_QUEUE_SIZE] = queue->items[move_idx];
            move_idx = (move_idx - 1 + ALARM_QUEUE_SIZE) % ALARM_QUEUE_SIZE;
        }
    }

    /* 插入新元素 */
    queue->items[insert_pos].priority = priority;
    queue->items[insert_pos].message  = message;

    /* 更新队尾和计数 */
    queue->tail = (queue->tail + 1) % ALARM_QUEUE_SIZE;
    queue->count++;

    return 0;
}

/**
 * @brief  从队列头部取出报警
 * @retval 0: 成功, 1: 队列为空
 */
uint8_t AlarmQueue_Pop(AlarmQueue_t* queue, Alarm_t* alarm)
{
    if (queue->count == 0) {
        return 1;  // 队列为空
    }

    /* 取出队头元素 */
    *alarm = queue->items[queue->head];
    queue->head = (queue->head + 1) % ALARM_QUEUE_SIZE;
    queue->count--;

    return 0;
}

/**
 * @brief  检查队列是否为空
 * @retval 0: 非空, 1: 为空
 */
uint8_t AlarmQueue_IsEmpty(AlarmQueue_t* queue)
{
    return (queue->count == 0);
}

/**
 * @brief  根据优先级清除队列中的报警
 */
void AlarmQueue_ClearByPriority(AlarmQueue_t* queue, AlarmPriority_t priority)
{
    uint8_t write_idx = queue->head;
    uint8_t new_count = 0;

    for (uint8_t i = 0; i < queue->count; i++) {
        uint8_t read_idx = (queue->head + i) % ALARM_QUEUE_SIZE;
        /* 保留优先级不等于目标优先级的元素 */
        if (queue->items[read_idx].priority != priority) {
            queue->items[write_idx] = queue->items[read_idx];
            write_idx = (write_idx + 1) % ALARM_QUEUE_SIZE;
            new_count++;
        }
    }

    queue->tail = (write_idx + new_count) % ALARM_QUEUE_SIZE;
    queue->count = new_count;
}

/**
 * @brief  扫描所有报警标志位，将活跃的报警加入队列
 * @note   对于心率和血氧报警，需要同时检测到手指才加入队列
 */
static void Alarm_ScanAndEnqueue(void)
{
    /* 遍历所有报警标志位 */
    for (uint8_t i = 0; i < 6; i++) {
        if (*(alarm_flags[i]) == 1) {
            /* 心率(4)和血氧(5)报警需要检查手指状态 */
            if (i == 4 || i == 5) {
                if (max30102_finger_detected == 0) {
                    /* 没有手指，不加入队列，直接清除标志 */
                    *(alarm_flags[i]) = 0;
                    continue;
                }
            }
            /* 标志位为1，尝试加入队列 */
            AlarmPriority_t prio = (AlarmPriority_t)flag_to_priority_map[i];
            AlarmQueue_Push(&g_alarm_queue, prio, alarm_messages[prio]);
            /* 清除标志位（因为已经加入队列） */
            *(alarm_flags[i]) = 0;
        }
    }
}

/**
 * @brief  发送报警到语音模块
 * @note   1. 先扫描所有标志位，将活跃报警加入队列
 *         2. 然后发送队头（最高优先级）的报警
 */
void send_flag_to_asr(void)
{
    /* 步骤1: 扫描所有报警标志位，将活跃的加入队列 */
    Alarm_ScanAndEnqueue();

    /* 步骤2: 如果报警使能且队列非空，发送最高优先级的报警 */
    if (alarm_flag && !AlarmQueue_IsEmpty(&g_alarm_queue)) {
        Alarm_t alarm;
        if (AlarmQueue_Pop(&g_alarm_queue, &alarm) == 0) {
            /* 发送报警消息 */
            my_printf(&huart1, "%s", alarm.message);

            /* 可选：打印调试信息 */
            // my_printf(&huart1, "[ALARM] prio=%d, msg=%s\r\n", alarm.priority, alarm.message);
        }
    }
}
