/*
 * main.c
 *
 * 智能环境监测系统 - 系统主入口
 *
 * 功能:
 *   1. 定义全局 IPC 对象和系统状态变量
 *   2. 初始化消息队列、互斥量、信号量
 *   3. 按顺序创建并启动所有功能线程
 *   4. 主循环中输出系统运行状态
 *
 * IPC 对象:
 *   - mq_sensor_data:  消息队列 (传感器→数据处理)
 *   - mutex_sys_status: 互斥量 (保护全局系统状态)
 *   - sem_display:      信号量 (数据处理→显示)
 *
 * 线程启动顺序 (按优先级从高到低):
 *   sensor(15) → process(16) → display(17) → alarm(18)
 *
 * 归属: 成员A (driver-main 分支)
 */

#include "app_config.h"
#include "sensor.h"
#include "process.h"
#include "display.h"
#include "alarm.h"

/* 日志标签 */
#define DBG_TAG    "main"
#define DBG_LVL    DBG_LOG
#include <rtdbg.h>

/* ---------------------------------------------------------------
 * 全局 IPC 对象定义
 * --------------------------------------------------------------- */

/* 传感器数据消息队列 */
static uint8_t mq_sensor_pool[MQ_SENSOR_MAX_MSGS *
                              (sizeof(sensor_data_t) + sizeof(rt_uint32_t))];
struct rt_messagequeue mq_sensor_data;

/* 系统状态互斥量 */
struct rt_mutex mutex_sys_status;

/* 显示就绪信号量 */
struct rt_semaphore sem_display;

/* ---------------------------------------------------------------
 * 全局系统状态变量
 * --------------------------------------------------------------- */
system_status_t g_sys_status = {
    .alarm_flags  = SYS_FLAG_NORMAL,
    .latest_data  = {0},
    .sample_count = 0
};

/* ---------------------------------------------------------------
 * IPC 初始化
 * --------------------------------------------------------------- */

/*
 * 初始化所有 IPC 对象
 * 必须在创建线程之前调用
 */
static int ipc_init(void)
{
    rt_err_t ret;

    /* 1. 初始化消息队列 */
    ret = rt_mq_init(&mq_sensor_data,
                     "mq_sensor",             /* 名称 */
                     mq_sensor_pool,          /* 内存池 */
                     sizeof(sensor_data_t),   /* 每条消息大小 */
                     sizeof(mq_sensor_pool),  /* 内存池大小 */
                     RT_IPC_FLAG_FIFO);       /* FIFO 模式 */
    if (ret != RT_EOK)
    {
        LOG_E("Failed to init sensor message queue: %d", ret);
        return ret;
    }
    LOG_I("Message queue 'mq_sensor' initialized (max %d msgs)",
          MQ_SENSOR_MAX_MSGS);

    /* 2. 初始化互斥量 */
    ret = rt_mutex_init(&mutex_sys_status,
                        "mutex_sys",
                        RT_IPC_FLAG_FIFO);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to init system status mutex: %d", ret);
        return ret;
    }
    LOG_I("Mutex 'mutex_sys' initialized");

    /* 3. 初始化信号量 (初始值 0) */
    ret = rt_sem_init(&sem_display,
                      "sem_disp",
                      0,                      /* 初始值为 0 */
                      RT_IPC_FLAG_FIFO);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to init display semaphore: %d", ret);
        return ret;
    }
    LOG_I("Semaphore 'sem_disp' initialized");

    return RT_EOK;
}

/* ---------------------------------------------------------------
 * 系统启动
 * --------------------------------------------------------------- */

/*
 * 主函数
 * RT-Thread 在系统初始化完成后，自动创建 main 线程并调用此函数
 */
int main(void)
{
    rt_err_t ret;

    LOG_I("========================================");
    LOG_I("  Smart Environment Monitor Starting");
    LOG_I("  Platform: STM32F407IG + RT-Thread");
    LOG_I("========================================");

    /* 1. 初始化 IPC 对象 */
    ret = ipc_init();
    if (ret != RT_EOK)
    {
        LOG_E("IPC initialization failed! System halted.");
        while (1)
        {
            rt_thread_mdelay(1000);
        }
    }

    /* 2. 按优先级顺序初始化各模块 (创建线程并启动) */

    /* 2a. 传感器模块 (优先级15, 最高) */
    ret = sensor_init();
    if (ret != RT_EOK)
    {
        LOG_E("Sensor module init failed!");
        return ret;
    }

    /* 2b. 数据处理模块 (优先级16) */
    ret = process_init();
    if (ret != RT_EOK)
    {
        LOG_E("Process module init failed!");
        return ret;
    }

    /* 2c. 显示输出模块 (优先级17) */
    ret = display_init();
    if (ret != RT_EOK)
    {
        LOG_E("Display module init failed!");
        return ret;
    }

    /* 2d. 告警监控模块 (优先级18, 最低) */
    ret = alarm_init();
    if (ret != RT_EOK)
    {
        LOG_E("Alarm module init failed!");
        return ret;
    }

    LOG_I("========================================");
    LOG_I("  All modules started successfully!");
    LOG_I("  4 threads running, 3 IPC objects active");
    LOG_I("========================================");

    /* 3. 主循环 - 定期输出系统摘要信息 */
    while (1)
    {
        rt_thread_mdelay(5000);  /* 每 5 秒输出一次摘要 */

        rt_mutex_take(&mutex_sys_status, RT_WAITING_FOREVER);
        {
            LOG_I("--- System Summary ---");
            LOG_I("  Samples  : %d", g_sys_status.sample_count);
            LOG_I("  Temp     : %.1f C", g_sys_status.latest_data.temperature);
            LOG_I("  Humidity : %.1f %%", g_sys_status.latest_data.humidity);
            LOG_I("  Light    : %.0f lux", g_sys_status.latest_data.light);
            LOG_I("  Flags    : 0x%02X", g_sys_status.alarm_flags);
        }
        rt_mutex_release(&mutex_sys_status);
    }

    return RT_EOK;
}
