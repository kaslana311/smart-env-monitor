/*
 * process.c
 *
 * 数据处理模块 - 实现
 *
 * 功能:
 *   1. 从消息队列接收传感器原始数据
 *   2. 使用移动平均滤波算法平滑数据
 *   3. 检测阈值越限 (高温、高湿、低光照)
 *   4. 更新系统全局状态 (互斥量保护)
 *   5. 释放信号量通知显示线程
 *
 * 线程: process_thread, 优先级16
 * 使用IPC: 消息队列(接收)、互斥量(保护状态)、信号量(通知显示)
 *
 * 归属: 成员B (thread-function 分支)
 */

#include "process.h"
#include <string.h>

/* 日志标签 */
#define DBG_TAG    "process"
#define DBG_LVL    DBG_LOG
#include <rtdbg.h>

/* ---------------------------------------------------------------
 * 外部 IPC 对象引用 (定义在 main.c)
 * --------------------------------------------------------------- */
extern struct rt_messagequeue mq_sensor_data;
extern struct rt_mutex        mutex_sys_status;
extern struct rt_semaphore    sem_display;

/* ---------------------------------------------------------------
 * 系统全局状态实例 (定义在 main.c)
 * --------------------------------------------------------------- */
extern system_status_t g_sys_status;

/* ---------------------------------------------------------------
 * 移动平均滤波器实现
 * --------------------------------------------------------------- */

/* 原始值历史 (用于中值滤波预处理) */
static float temp_raw_hist[3]  = {0};
static float humid_raw_hist[3] = {0};
static float light_raw_hist[3] = {0};
static int   raw_hist_index = 0;

/* 温度滤波缓冲区 */
static float temp_buffer[FILTER_WINDOW_SIZE]  = {0};
static float humid_buffer[FILTER_WINDOW_SIZE] = {0};
static float light_buffer[FILTER_WINDOW_SIZE] = {0};
static int   filter_index = 0;          /* 当前缓冲区写入位置 */
static int   filter_count = 0;          /* 已填充的样本数 */

/*
 * 三点中值滤波 - 去除偶发脉冲噪声
 * @param a, b, c  最近三个原始采样值
 * @return         中位数值
 */
static float median3(float a, float b, float c)
{
    if (a > b) { float t = a; a = b; b = t; }
    if (a > c) { float t = a; a = c; c = t; }
    if (b > c) { float t = b; b = c; c = t; }
    return b;
}

/*
 * 计算移动平均值
 * @param buffer  数据缓冲区
 * @param count   有效数据个数
 * @return        平均值
 */
static float calc_moving_average(const float *buffer, int count)
{
    float sum = 0.0f;
    if (count <= 0) return 0.0f;

    for (int i = 0; i < count; i++)
    {
        sum += buffer[i];
    }
    return sum / (float)count;
}

/*
 * 将新数据加入滤波缓冲区，返回滤波后的数据
 */
static sensor_data_t apply_filter(const sensor_data_t *raw)
{
    sensor_data_t filtered;
    sensor_data_t prefiltered;

    /* 记录原始值历史 */
    temp_raw_hist[raw_hist_index]  = raw->temperature;
    humid_raw_hist[raw_hist_index] = raw->humidity;
    light_raw_hist[raw_hist_index] = raw->light;

    /* 中值滤波预处理 (去除脉冲噪声) */
    prefiltered.temperature = median3(temp_raw_hist[0],
                                      temp_raw_hist[1],
                                      temp_raw_hist[2]);
    prefiltered.humidity = median3(humid_raw_hist[0],
                                    humid_raw_hist[1],
                                    humid_raw_hist[2]);
    prefiltered.light = median3(light_raw_hist[0],
                                 light_raw_hist[1],
                                 light_raw_hist[2]);

    raw_hist_index++;
    if (raw_hist_index >= 3) raw_hist_index = 0;

    /* 写入移动平均循环缓冲区 */
    temp_buffer[filter_index]  = prefiltered.temperature;
    humid_buffer[filter_index] = prefiltered.humidity;
    light_buffer[filter_index] = prefiltered.light;

    filter_index++;
    if (filter_index >= FILTER_WINDOW_SIZE)
    {
        filter_index = 0;
    }
    if (filter_count < FILTER_WINDOW_SIZE)
    {
        filter_count++;
    }

    /* 计算滤波输出 */
    filtered.temperature = calc_moving_average(temp_buffer, filter_count);
    filtered.humidity    = calc_moving_average(humid_buffer, filter_count);
    filtered.light       = calc_moving_average(light_buffer, filter_count);

    return filtered;
}

/* ---------------------------------------------------------------
 * 阈值检测
 * --------------------------------------------------------------- */

/*
 * 检查传感器数据是否超过阈值
 * @param data  滤波后的传感器数据
 * @return      告警标志位组合
 */
static uint8_t check_thresholds(const sensor_data_t *data)
{
    uint8_t flags = SYS_FLAG_NORMAL;

    if (data->temperature > TEMP_HIGH_THRESHOLD)
    {
        flags |= SYS_FLAG_TEMP_HIGH;
        LOG_W("High temperature detected: %.1f°C > %.1f°C",
              data->temperature, TEMP_HIGH_THRESHOLD);
    }

    if (data->humidity > HUMIDITY_HIGH_THRESHOLD)
    {
        flags |= SYS_FLAG_HUMID_HIGH;
        LOG_W("High humidity detected: %.1f%% > %.1f%%",
              data->humidity, HUMIDITY_HIGH_THRESHOLD);
    }

    if (data->light < LIGHT_LOW_THRESHOLD)
    {
        flags |= SYS_FLAG_LIGHT_LOW;
        LOG_W("Low light detected: %.1f lux < %.1f lux",
              data->light, LIGHT_LOW_THRESHOLD);
    }

    return flags;
}

/* ---------------------------------------------------------------
 * 数据质量评估
 * --------------------------------------------------------------- */

/*
 * 评估传感器数据质量等级
 * @return 0-优秀 1-良好 2-可疑 3-不可靠
 */
static int evaluate_data_quality(const sensor_data_t *raw, const sensor_data_t *filtered)
{
    int quality = 0;

    /* 检查原始值与滤波值偏差 */
    float temp_diff = raw->temperature - filtered->temperature;
    float humid_diff = raw->humidity - filtered->humidity;
    float light_diff = raw->light - filtered->light;

    if (temp_diff > 3.0f || temp_diff < -3.0f) quality++;
    if (humid_diff > 5.0f || humid_diff < -5.0f) quality++;
    if (light_diff > 100.0f || light_diff < -100.0f) quality++;

    if (quality > 2) quality = 2;  /* 最高为可疑 */

    return quality;
}

/* 数据质量标签 */
static const char *quality_labels[] = {"Excellent", "Good", "Suspicious"};

/* ---------------------------------------------------------------
 * 数据统计跟踪
 * --------------------------------------------------------------- */
static sensor_data_t stat_min = {1000, 1000, 10000};  /* 最小值初始化为极大值 */
static sensor_data_t stat_max = {-1000, -1000, -1000}; /* 最大值初始化为极小值 */
static rt_bool_t stat_initialized = RT_FALSE;

/*
 * 更新数据统计 (最小/最大值)
 */
static void update_statistics(const sensor_data_t *data)
{
    if (!stat_initialized)
    {
        stat_min = *data;
        stat_max = *data;
        stat_initialized = RT_TRUE;
        return;
    }
    if (data->temperature < stat_min.temperature) stat_min.temperature = data->temperature;
    if (data->temperature > stat_max.temperature) stat_max.temperature = data->temperature;
    if (data->humidity < stat_min.humidity)       stat_min.humidity = data->humidity;
    if (data->humidity > stat_max.humidity)       stat_max.humidity = data->humidity;
    if (data->light < stat_min.light)             stat_min.light = data->light;
    if (data->light > stat_max.light)             stat_max.light = data->light;
}

/* ---------------------------------------------------------------
 * 数据处理线程
 * --------------------------------------------------------------- */

/*
 * 数据处理线程入口
 * 阻塞等待消息队列中的数据，处理后更新系统状态
 */
void process_thread_entry(void *parameter)
{
    sensor_data_t raw_data;
    sensor_data_t filtered_data;
    rt_err_t ret;

    LOG_I("Process thread started, priority=%d", PROCESS_THREAD_PRIORITY);

    while (1)
    {
        /* 1. 从消息队列接收数据 (阻塞等待) */
        ret = rt_mq_recv(&mq_sensor_data,
                         &raw_data,
                         sizeof(sensor_data_t),
                         RT_WAITING_FOREVER);

        if (ret != RT_EOK)
        {
            LOG_E("Message queue receive failed: %d", ret);
            continue;
        }

        /* 2. 中值滤波 + 移动平均滤波 */
        filtered_data = apply_filter(&raw_data);

        /* 3. 更新数据统计 */
        update_statistics(&filtered_data);

        /* 4. 阈值检测 */
        uint8_t flags = check_thresholds(&filtered_data);

        /* 5. 更新系统状态 (互斥量保护) */
        rt_mutex_take(&mutex_sys_status, RT_WAITING_FOREVER);
        {
            g_sys_status.alarm_flags = flags;
            g_sys_status.latest_data = filtered_data;
            g_sys_status.sample_count++;
        }
        rt_mutex_release(&mutex_sys_status);

        /* 6. 每10个样本输出统计摘要 */
        if (g_sys_status.sample_count % 10 == 0)
        {
            LOG_I("=== Statistics (samples: %d) ===", g_sys_status.sample_count);
            LOG_I("  Temp  : min=%.1f max=%.1f cur=%.1f °C",
                  stat_min.temperature, stat_max.temperature, filtered_data.temperature);
            LOG_I("  Humid : min=%.1f max=%.1f cur=%.1f %%",
                  stat_min.humidity, stat_max.humidity, filtered_data.humidity);
            LOG_I("  Light : min=%.0f max=%.0f cur=%.0f lux",
                  stat_min.light, stat_max.light, filtered_data.light);
        }

        /* 5. 通知显示线程有新数据 */
        rt_sem_release(&sem_display);

        LOG_D("Processed sample #%d: T=%.1f H=%.1f L=%.0f flags=0x%02X quality=%s",
              g_sys_status.sample_count,
              filtered_data.temperature,
              filtered_data.humidity,
              filtered_data.light,
              flags,
              quality_labels[evaluate_data_quality(&raw_data, &filtered_data)]);
    }
}

/*
 * 数据处理模块初始化
 * 创建数据处理线程并启动
 */
int process_init(void)
{
    rt_thread_t tid;
    rt_err_t ret;

    /* 初始化滤波器缓冲区 */
    memset(temp_buffer,  0, sizeof(temp_buffer));
    memset(humid_buffer, 0, sizeof(humid_buffer));
    memset(light_buffer, 0, sizeof(light_buffer));

    /* 创建数据处理线程 */
    tid = rt_thread_create(
        "process",
        process_thread_entry,
        RT_NULL,
        PROCESS_THREAD_STACK_SIZE,
        PROCESS_THREAD_PRIORITY,
        10                          /* 时间片 10 tick */
    );

    if (tid == RT_NULL)
    {
        LOG_E("Failed to create process thread");
        return -RT_ERROR;
    }

    ret = rt_thread_startup(tid);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to start process thread");
        return ret;
    }

    LOG_I("Process module initialized successfully");
    return RT_EOK;
}
