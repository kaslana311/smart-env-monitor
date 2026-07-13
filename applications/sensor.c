/*
 * sensor.c
 *
 * 传感器数据采集模块 - 实现
 *
 * 功能:
 *   1. 模拟温度、湿度、光照传感器数据采集
 *   2. 通过消息队列将数据发送给数据处理线程
 *   3. 控制心跳 LED 闪烁 (LED0)
 *   4. 提供告警 LED 控制接口 (LED1)
 *
 * 线程: sensor_thread, 优先级15, 周期500ms
 *
 * 归属: 成员A (driver-main 分支)
 */

#include "sensor.h"
#include <stdlib.h>

/* 日志标签 */
#define DBG_TAG    "sensor"
#define DBG_LVL    DBG_LOG
#include <rtdbg.h>

/* ---------------------------------------------------------------
 * 传感器校准参数
 * --------------------------------------------------------------- */
#define TEMP_OFFSET    -0.5f      /* 温度校准偏移 (°C) */
#define HUMID_OFFSET    2.0f      /* 湿度校准偏移 (%) */
#define LIGHT_GAIN      1.05f     /* 光照增益系数 */

/* ---------------------------------------------------------------
 * LED 驱动实现
 * --------------------------------------------------------------- */

/* 初始化 LED 引脚 */
int led_init(void)
{
    /* 设置 LED0 引脚为输出模式 */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(LED0_PIN, PIN_LOW);

    /* 设置 LED1 引脚为输出模式 */
    rt_pin_mode(LED1_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(LED1_PIN, PIN_LOW);

    LOG_I("LED driver initialized (LED0: heartbeat, LED1: alarm)");
    return RT_EOK;
}

/* 心跳 LED 翻转 */
void led_heartbeat_toggle(void)
{
    static rt_base_t level = PIN_LOW;
    level = (level == PIN_LOW) ? PIN_HIGH : PIN_LOW;
    rt_pin_write(LED0_PIN, level);
}

/* 告警 LED 亮 */
void led_alarm_on(void)
{
    rt_pin_write(LED1_PIN, PIN_HIGH);
}

/* 告警 LED 灭 */
void led_alarm_off(void)
{
    rt_pin_write(LED1_PIN, PIN_LOW);
}

/* 告警 LED 翻转 */
void led_alarm_toggle(void)
{
    static rt_base_t alarm_level = PIN_LOW;
    alarm_level = (alarm_level == PIN_LOW) ? PIN_HIGH : PIN_LOW;
    rt_pin_write(LED1_PIN, alarm_level);
}

/* ---------------------------------------------------------------
 * 传感器数据采集线程
 * --------------------------------------------------------------- */

/* 引用外部定义的消息队列 (定义在 main.c) */
extern struct rt_messagequeue mq_sensor_data;

/*
 * 模拟传感器数据采集
 * 在实际项目中，这里应替换为真实的传感器驱动代码
 * (如 I2C 读取温湿度传感器, ADC 读取光照传感器等)
 */
static void read_sensors(sensor_data_t *data)
{
    /*
     * 使用随机数模拟传感器数值波动
     * rand() 基于上次值做小幅变化，使数据更真实
     */
    static float base_temp   = 25.0f;    /* 基准温度 */
    static float base_humid  = 55.0f;    /* 基准湿度 */
    static float base_light  = 500.0f;   /* 基准光照 */

    /* 温度: ±0.5°C 波动 */
    base_temp += ((float)(rand() % 100) / 100.0f - 0.5f);
    if (base_temp < -10.0f) base_temp = -10.0f;
    if (base_temp > 45.0f)  base_temp = 45.0f;

    /* 湿度: ±1% 波动 */
    base_humid += ((float)(rand() % 200) / 100.0f - 1.0f);
    if (base_humid < 20.0f) base_humid = 20.0f;
    if (base_humid > 95.0f) base_humid = 95.0f;

    /* 光照: ±20 lux 波动 */
    base_light += ((float)(rand() % 4000) / 100.0f - 20.0f);
    if (base_light < 0.0f)    base_light = 0.0f;
    if (base_light > 1000.0f) base_light = 1000.0f;

    /* 应用校准参数 */
    data->temperature = base_temp + TEMP_OFFSET;
    data->humidity    = base_humid + HUMID_OFFSET;
    data->light       = base_light * LIGHT_GAIN;
}

/*
 * 传感器线程入口
 * 每 500ms 采集一次传感器数据，发送到消息队列
 */
void sensor_thread_entry(void *parameter)
{
    sensor_data_t data;
    rt_err_t ret;

    LOG_I("Sensor thread started, priority=%d, tick=%dms",
          SENSOR_THREAD_PRIORITY, SENSOR_THREAD_TICK);

    while (1)
    {
        /* 1. 读取传感器数据 */
        read_sensors(&data);

        /* 2. 发送数据到消息队列 (非阻塞发送) */
        ret = rt_mq_send(&mq_sensor_data, &data, sizeof(sensor_data_t));
        if (ret == RT_EFULL)
        {
            LOG_W("Message queue full, dropping sample");
        }
        else if (ret != RT_EOK)
        {
            LOG_E("Message queue send failed: %d", ret);
        }

        /* 3. 心跳 LED 闪烁 */
        led_heartbeat_toggle();

        /* 4. 延时，等待下一个采集周期 */
        rt_thread_mdelay(SENSOR_THREAD_TICK);
    }
}

/*
 * 传感器模块初始化
 * 创建传感器线程并启动
 */
int sensor_init(void)
{
    rt_thread_t tid;
    rt_err_t ret;

    /* 初始化 LED */
    led_init();

    /* 初始化随机数种子 (使用系统 tick 值 + 硬件噪声) */
    srand(rt_tick_get() ^ 0xA5A5A5A5);

    /* 创建传感器线程 */
    tid = rt_thread_create(
        "sensor",                       /* 线程名称 */
        sensor_thread_entry,            /* 线程入口函数 */
        RT_NULL,                        /* 入口参数 */
        SENSOR_THREAD_STACK_SIZE,       /* 线程栈大小 */
        SENSOR_THREAD_PRIORITY,         /* 线程优先级 */
        SENSOR_THREAD_TICK              /* 时间片 */
    );

    if (tid == RT_NULL)
    {
        LOG_E("Failed to create sensor thread");
        return -RT_ERROR;
    }

    /* 启动线程 */
    ret = rt_thread_startup(tid);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to start sensor thread");
        return ret;
    }

    LOG_I("Sensor module initialized successfully");
    return RT_EOK;
}
