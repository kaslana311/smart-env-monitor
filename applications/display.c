/*
 * display.c
 *
 * 显示输出模块 - 实现
 *
 * 功能:
 *   1. 等待信号量 (由数据处理线程释放)
 *   2. 获取系统全局状态 (互斥量保护)
 *   3. 通过串口 (UART1) 格式化输出环境数据
 *   4. 显示系统状态信息和告警标志
 *
 * 线程: display_thread, 优先级17
 * 使用IPC: 信号量(等待数据)、互斥量(读取状态)
 *
 * 归属: 成员C (display-module 分支)
 */

#include "display.h"
#include <stdio.h>

/* 日志标签 */
#define DBG_TAG    "display"
#define DBG_LVL    DBG_LOG
#include <rtdbg.h>

/* ---------------------------------------------------------------
 * 显示模式配置
 * --------------------------------------------------------------- */
typedef enum {
    DISP_MODE_BRIEF,     /* 简洁模式: 仅显示核心数据 */
    DISP_MODE_DETAIL     /* 详细模式: 显示完整信息 */
} display_mode_t;

static display_mode_t current_mode = DISP_MODE_DETAIL;  /* 默认详细模式 */

/*
 * 计算变化趋势符号
 * @return '+'上升 '-'下降 '='平稳
 */
static char get_trend(float current, float previous)
{
    float diff = current - previous;
    if (diff > 0.5f) return '+';    /* 上升 */
    if (diff < -0.5f) return '-';   /* 下降 */
    return '=';                      /* 平稳 */
}

/* ---------------------------------------------------------------
 * 外部引用
 * --------------------------------------------------------------- */
extern struct rt_mutex     mutex_sys_status;
extern struct rt_semaphore sem_display;
extern system_status_t     g_sys_status;

/* ---------------------------------------------------------------
 * 状态描述生成
 * --------------------------------------------------------------- */

/*
 * 将告警标志位转换为可读的状态字符串
 */
static const char *get_status_string(uint8_t flags)
{
    if (flags == SYS_FLAG_NORMAL)
    {
        return "[OK]";
    }

    static char status_buf[64];
    int offset = 0;

    if (flags & SYS_FLAG_TEMP_HIGH)
    {
        offset += rt_snprintf(status_buf + offset,
                              sizeof(status_buf) - offset,
                              "[TEMP HIGH] ");
    }
    if (flags & SYS_FLAG_HUMID_HIGH)
    {
        offset += rt_snprintf(status_buf + offset,
                              sizeof(status_buf) - offset,
                              "[HUMID HIGH] ");
    }
    if (flags & SYS_FLAG_LIGHT_LOW)
    {
        offset += rt_snprintf(status_buf + offset,
                              sizeof(status_buf) - offset,
                              "[LIGHT LOW]");
    }

    return status_buf;
}

/*
 * 获取环境舒适度等级
 * 0-优秀 1-良好 2-一般 3-差
 */
static int get_comfort_level(const sensor_data_t *data)
{
    int score = 0;

    /* 温度舒适度: 20-28°C 为最佳 */
    if (data->temperature < 15.0f || data->temperature > 32.0f)
        score += 2;
    else if (data->temperature < 20.0f || data->temperature > 28.0f)
        score += 1;

    /* 湿度舒适度: 40-60% 为最佳 */
    if (data->humidity < 30.0f || data->humidity > 80.0f)
        score += 2;
    else if (data->humidity < 40.0f || data->humidity > 60.0f)
        score += 1;

    if (score <= 1) return 0;       /* 优秀 */
    else if (score == 2) return 1;  /* 良好 */
    else if (score == 3) return 2;  /* 一般 */
    else return 3;                   /* 差 */
}

static const char *comfort_labels[] = {
    "Good",        /* 优秀 */
    "Not Bad",     /* 良好 */
    "Poor",        /* 一般 */
    "Bad"          /* 差 */
};

/* ---------------------------------------------------------------
 * 显示线程
 * --------------------------------------------------------------- */

/*
 * 显示线程入口
 * 阻塞等待信号量，有新数据时刷新显示
 */
void display_thread_entry(void *parameter)
{
    sensor_data_t data;
    sensor_data_t prev_data = {0};  /* 上一次数据，用于计算趋势 */
    uint8_t       flags;
    uint32_t      count;
    rt_bool_t     first_run = RT_TRUE;

    LOG_I("Display thread started, priority=%d", DISPLAY_THREAD_PRIORITY);

    /* 打印显示表头 */
    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  Smart Environment Monitor v1.0\n");
    rt_kprintf("  STM32F407 + RT-Thread\n");
    rt_kprintf("========================================\n");
    rt_kprintf("\n");

    while (1)
    {
        /* 1. 等待数据就绪信号量 (阻塞等待) */
        rt_sem_take(&sem_display, RT_WAITING_FOREVER);

        /* 2. 读取系统状态 (互斥量保护) */
        rt_mutex_take(&mutex_sys_status, RT_WAITING_FOREVER);
        {
            data  = g_sys_status.latest_data;
            flags = g_sys_status.alarm_flags;
            count = g_sys_status.sample_count;
        }
        rt_mutex_release(&mutex_sys_status);

        /* 3. 计算舒适度 */
        int comfort = get_comfort_level(&data);

        /* 4. 格式化输出到串口 */
        rt_tick_t uptime = rt_tick_get() / RT_TICK_PER_SECOND;

        if (current_mode == DISP_MODE_BRIEF)
        {
            /* 简洁模式: 单行输出 */
            rt_kprintf("[#%d|%ds] T:%.1fC H:%.1f%% L:%.0flux %s\n",
                       count, uptime,
                       data.temperature, data.humidity, data.light,
                       get_status_string(flags));
        }
        else
        {
            /* 详细模式: 完整格式化输出 */
            rt_kprintf("[Sample #%d | Uptime: %ds]\n", count, uptime);
            if (!first_run)
            {
                rt_kprintf("  Temperature : %5.1f C [%c]\n",
                          data.temperature, get_trend(data.temperature, prev_data.temperature));
                rt_kprintf("  Humidity    : %5.1f %% [%c]\n",
                          data.humidity, get_trend(data.humidity, prev_data.humidity));
                rt_kprintf("  Light       : %5.0f lux [%c]\n",
                          data.light, get_trend(data.light, prev_data.light));
            }
            else
            {
                rt_kprintf("  Temperature : %5.1f C\n", data.temperature);
                rt_kprintf("  Humidity    : %5.1f %%\n", data.humidity);
                rt_kprintf("  Light       : %5.0f lux\n", data.light);
            }
            rt_kprintf("  Comfort     : %s\n", comfort_labels[comfort]);
            rt_kprintf("  Status      : %s\n", get_status_string(flags));
            rt_kprintf("----------------------------------------\n");
        }

        /* 保存当前数据，用于下次趋势计算 */
        prev_data = data;
        first_run = RT_FALSE;
    }
}

/*
 * 显示模块初始化
 * 创建显示线程并启动
 */
int display_init(void)
{
    rt_thread_t tid;
    rt_err_t ret;

    /* 创建显示线程 */
    tid = rt_thread_create(
        "display",
        display_thread_entry,
        RT_NULL,
        DISPLAY_THREAD_STACK_SIZE,
        DISPLAY_THREAD_PRIORITY,
        10                          /* 时间片 10 tick */
    );

    if (tid == RT_NULL)
    {
        LOG_E("Failed to create display thread");
        return -RT_ERROR;
    }

    ret = rt_thread_startup(tid);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to start display thread");
        return ret;
    }

    LOG_I("Display module initialized successfully");
    return RT_EOK;
}
