#include "scheduler.h"


// 全局变量，用于存储任务数量
uint8_t task_num;




typedef struct
{
    void (*task_func)(void);
    uint32_t rate_ms;
    uint32_t last_run;
    TaskPriority_t priority;  // 任务优先级
} task_t;



// 静态任务数组，每个任务包含任务函数、执行周期（毫秒）、上次运行时间（毫秒）和优先级
static task_t scheduler_task[] =
{
	  {mpu6050_task, 50, 0, TASK_PRIO_CRITICAL},		// MPU6050 姿态传感器任务，最高优先级
      {mq2_task, 100, 0, TASK_PRIO_NORMAL},			// MQ2 气体传感器任务
      {dht11_task, 100, 0, TASK_PRIO_NORMAL},		// DHT11 温湿度传感器任务
      {max30102_task, 200, 0, TASK_PRIO_HIGH},		// MAX30102 心率血氧传感器任务，高优先级
      {oled_task, 10, 0, TASK_PRIO_LOW},			// OLED显示任务，低优先级
      {atgm336h_task, 100, 0, TASK_PRIO_NORMAL},	// ATGM336H GPS 模块任务
	  {asrpro_task, 20, 0, TASK_PRIO_NORMAL},		// 处理 ASRpro 数据任务
	   {send_flag_to_asr, 20, 0, TASK_PRIO_LOW},	// 向 ASRpro 发送标志任务

	  // ESP上报任务：错开时间避免MQTT冲突，每个任务间隔400ms
    {ESP_Report_Env, 2000, 0, TASK_PRIO_LOW},		// 向服务器上报环境数据任务，每2秒
    {ESP_Report_Body, 2000, 400, TASK_PRIO_LOW},	// 向服务器上报身体数据任务，延迟400ms启动
	{ESP_Report_GPS, 2000, 800, TASK_PRIO_LOW},	// 向服务器上报GPS数据任务，延迟800ms启动
    {ESP_Report_Euler, 2000, 1200, TASK_PRIO_LOW},	// 向服务器上报欧拉角任务，延迟1200ms启动
    {ESP_Report_Flags, 2000, 1600, TASK_PRIO_LOW},	// 向服务器上报标志位任务，延迟1600ms启动
    {control_task, 50, 0, TASK_PRIO_HIGH},			// 命令下发控制任务，高优先级快速响应
};
/**
 * @brief 调度器初始化函数
 * 计算任务数组的元素个数，并按优先级排序
 */
void scheduler_init(void)
{
    // 计算任务数组的元素个数，并将结果存储在 task_num 中
    task_num = sizeof(scheduler_task) / sizeof(task_t);

    // 按优先级排序（冒泡排序，任务数量少，性能可接受）
    for (uint8_t i = 0; i < task_num - 1; i++)
    {
        for (uint8_t j = 0; j < task_num - i - 1; j++)
        {
            if (scheduler_task[j].priority > scheduler_task[j+1].priority)
            {
                // 交换任务
                task_t temp = scheduler_task[j];
                scheduler_task[j] = scheduler_task[j+1];
                scheduler_task[j+1] = temp;
            }
        }
    }
}

/**
 * @brief 调度器运行函数
 * 按优先级顺序遍历任务数组，高优先级任务优先执行
 */
void scheduler_run(void)
{
    // 遍历任务数组中的所有任务（已按优先级排序，高优先级在前）
    for (uint8_t i = 0; i < task_num; i++)
    {
        // 获取当前的系统时间（毫秒）
        uint32_t now_time = HAL_GetTick();

        // 检查当前时间是否达到任务的执行时间
        if (now_time - scheduler_task[i].last_run >= scheduler_task[i].rate_ms)
        {
            // 更新任务的上次运行时间（修正时间漂移）
            scheduler_task[i].last_run += scheduler_task[i].rate_ms;

            // 执行任务函数
            scheduler_task[i].task_func();
        }
    }
}
