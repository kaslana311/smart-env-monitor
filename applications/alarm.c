/*
 * alarm.c
 *
 * 告警监控模块 - 实现
 *
 * 功能:
 *   1. 周期性读取系统状态 (互斥量保护)
 *   2. 根据告警标志控制 LED1 闪烁模式:
 *      - 正常:           LED1 常灭
 *      - 高温告警:       LED1 快闪 (100ms周期)
 *      - 高湿告警:       LED1 慢闪 (500ms周期)
 *      - 低光照提醒:     LED1 双闪
 *      - 多重告警:       LED1 常亮
 *   3. 使用互斥量安全读取系统状态
 *
 * 线程: alarm_thread, 优先级18, 周期200ms
 *
 * 归属: 成员B (thread-function 分支)
 */

#include "alarm.h"
#include "sensor.h"     /* LED 控制接口 */
#include <string.h>

/* 日志标签 */
#define DBG_TAG    "alarm"
#define DBG_LVL    DBG_LOG
#include <rtdbg.h>

/* ---------------------------------------------------------------
 * 外部引用
 * --------------------------------------------------------------- */
extern struct rt_mutex    mutex_sys_status;
extern system_status_t    g_sys_status;

/* ---------------------------------------------------------------
 * 告警历史记录
 * --------------------------------------------------------------- */
#define ALARM_HISTORY_MAX   20  /* 最多保留20条告警记录 */

typedef struct {
    rt_tick_t timestamp;       /* 告警发生时间 */
    uint8_t   flags;           /* 告警标志 */
    char      desc[32];        /* 告警描述 */
} alarm_record_t;

static alarm_record_t alarm_history[ALARM_HISTORY_MAX];
static int            alarm_history_count = 0;

/*
 * 记录告警事件到历史缓冲区
 */
static void alarm_record_add(uint8_t flags)
{
    if (alarm_history_count >= ALARM_HISTORY_MAX)
    {
        /* 缓冲区满, 移除最旧记录 */
        memmove(alarm_history, alarm_history + 1,
                sizeof(alarm_record_t) * (ALARM_HISTORY_MAX - 1));
        alarm_history_count = ALARM_HISTORY_MAX - 1;
    }

    alarm_record_t *rec = &alarm_history[alarm_history_count];
    rec->timestamp = rt_tick_get();
    rec->flags = flags;

    /* 生成告警描述 */
    int offset = 0;
    if (flags & SYS_FLAG_TEMP_HIGH)
        offset += rt_snprintf(rec->desc + offset, sizeof(rec->desc) - offset, "T_HIGH ");
    if (flags & SYS_FLAG_HUMID_HIGH)
        offset += rt_snprintf(rec->desc + offset, sizeof(rec->desc) - offset, "H_HIGH ");
    if (flags & SYS_FLAG_LIGHT_LOW)
        offset += rt_snprintf(rec->desc + offset, sizeof(rec->desc) - offset, "L_LOW");

    alarm_history_count++;
    LOG_I("Alarm recorded [#%d]: %s", alarm_history_count, rec->desc);
}

/* ---------------------------------------------------------------
 * 告警闪烁状态机
 * --------------------------------------------------------------- */

/* 闪烁模式定义 */
typedef enum {
    BLINK_OFF,          /* 常灭 (正常) */
    BLINK_FAST,         /* 快闪 (高温) */
    BLINK_SLOW,         /* 慢闪 (高湿) */
    BLINK_DOUBLE,       /* 双闪 (低光照) */
    BLINK_SOLID         /* 常亮 (多重告警) */
} blink_mode_t;

/*
 * 根据告警标志位选择闪烁模式
 */
static blink_mode_t select_blink_mode(uint8_t flags)
{
    int alarm_count = 0;

    if (flags & SYS_FLAG_TEMP_HIGH)  alarm_count++;
    if (flags & SYS_FLAG_HUMID_HIGH) alarm_count++;
    if (flags & SYS_FLAG_LIGHT_LOW)  alarm_count++;

    /* 无告警 */
    if (alarm_count == 0)
    {
        return BLINK_OFF;
    }

    /* 多重告警 */
    if (alarm_count >= 2)
    {
        return BLINK_SOLID;
    }

    /* 单一告警: 根据类型选择模式 */
    if (flags & SYS_FLAG_TEMP_HIGH)
    {
        return BLINK_FAST;
    }
    if (flags & SYS_FLAG_HUMID_HIGH)
    {
        return BLINK_SLOW;
    }
    if (flags & SYS_FLAG_LIGHT_LOW)
    {
        return BLINK_DOUBLE;
    }

    return BLINK_OFF;
}

/*
 * 执行闪烁模式
 * @param mode  当前闪烁模式
 * @param tick  线程运行周期计数
 */
static void execute_blink(blink_mode_t mode, rt_tick_t tick)
{
    switch (mode)
    {
    case BLINK_OFF:
        /* 正常: LED 常灭 */
        led_alarm_off();
        break;

    case BLINK_FAST:
        /* 快闪: 周期 = 2 tick (100ms亮, 100ms灭) */
        if (tick % 2 == 0)
            led_alarm_on();
        else
            led_alarm_off();
        break;

    case BLINK_SLOW:
        /* 慢闪: 周期 = 10 tick (500ms亮, 500ms灭) */
        {
            rt_tick_t phase = tick % 10;
            if (phase < 5)
                led_alarm_on();
            else
                led_alarm_off();
        }
        break;

    case BLINK_DOUBLE:
        /* 双闪: 两次快闪 + 长灭 */
        {
            rt_tick_t phase = tick % 20;
            if (phase < 2 || (phase >= 4 && phase < 6))
                led_alarm_on();
            else
                led_alarm_off();
        }
        break;

    case BLINK_SOLID:
        /* 常亮: 多重告警 */
        led_alarm_on();
        break;

    default:
        led_alarm_off();
        break;
    }
}

/* ---------------------------------------------------------------
 * 告警监控线程
 * --------------------------------------------------------------- */

/*
 * 告警线程入口
 * 每 200ms 检查系统状态，更新 LED 指示
 */
void alarm_thread_entry(void *parameter)
{
    uint8_t      current_flags;
    blink_mode_t blink_mode;
    rt_tick_t    tick_count = 0;

    LOG_I("Alarm thread started, priority=%d, tick=%dms",
          ALARM_THREAD_PRIORITY, ALARM_THREAD_TICK);

    /* 告警迟滞计数器 (防止信号抖动导致频繁闪烁) */
    rt_tick_t alarm_confirm_ticks = 0;   /* 告警确认计数 */
    rt_tick_t alarm_clear_ticks  = 0;    /* 告警清除计数 */
    #define ALARM_HYSTERESIS    5        /* 连续5个周期确认才切换状态 */

    /* 初始状态: LED 灭 */
    led_alarm_off();

    while (1)
    {
        /* 1. 读取系统状态 (互斥量保护) */
        rt_mutex_take(&mutex_sys_status, RT_WAITING_FOREVER);
        {
            current_flags = g_sys_status.alarm_flags;
        }
        rt_mutex_release(&mutex_sys_status);

        /* 2. 选择闪烁模式 */
        blink_mode = select_blink_mode(current_flags);

        /* 3. 执行 LED 闪烁 */
        execute_blink(blink_mode, tick_count);

        /* 4. 日志输出 (首次告警或状态变化时) */
        static uint8_t last_flags = 0xFF;  /* 初始化为不可能值 */
        /* 迟滞判断: 防止告警信号抖动 */
        if (current_flags != SYS_FLAG_NORMAL)
        {
            alarm_confirm_ticks++;
            alarm_clear_ticks = 0;
        }
        else
        {
            alarm_clear_ticks++;
            alarm_confirm_ticks = 0;
        }

        if (current_flags != last_flags)
        {
            /* 仅当持续确认超过迟滞阈值时才切换状态 */
            if (current_flags != SYS_FLAG_NORMAL &&
                alarm_confirm_ticks >= ALARM_HYSTERESIS)
            {
                LOG_W("Alarm active - flags=0x%02X, mode=%d",
                      current_flags, blink_mode);
                alarm_record_add(current_flags);
                last_flags = current_flags;
            }
            else if (current_flags == SYS_FLAG_NORMAL &&
                     alarm_clear_ticks >= ALARM_HYSTERESIS)
            {
                rt_tick_t alarm_duration = (rt_tick_get() - alarm_history[alarm_history_count-1].timestamp)
                                           / RT_TICK_PER_SECOND;
                LOG_I("Alarm cleared - system normal (last alarm duration: %d seconds)", alarm_duration);
                last_flags = current_flags;
            }
        }

        /* 5. 周期延时 */
        rt_thread_mdelay(ALARM_THREAD_TICK);
        tick_count++;

        /* 6. 每500个周期(100秒)输出告警统计 */
        static uint32_t total_alarm_cycles = 0;
        if (current_flags != SYS_FLAG_NORMAL)
        {
            total_alarm_cycles++;
        }
        if (tick_count % 500 == 0)
        {
            LOG_I("Alarm stats: %d/%d cycles in alarm state (%.1f%%)",
                  total_alarm_cycles, tick_count,
                  (float)total_alarm_cycles / (float)tick_count * 100.0f);
        }
    }
}

/*
 * 告警模块初始化
 * 创建告警线程并启动
 */
int alarm_init(void)
{
    rt_thread_t tid;
    rt_err_t ret;

    /* 创建告警线程 */
    tid = rt_thread_create(
        "alarm",
        alarm_thread_entry,
        RT_NULL,
        ALARM_THREAD_STACK_SIZE,
        ALARM_THREAD_PRIORITY,
        ALARM_THREAD_TICK
    );

    if (tid == RT_NULL)
    {
        LOG_E("Failed to create alarm thread");
        return -RT_ERROR;
    }

    ret = rt_thread_startup(tid);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to start alarm thread");
        return ret;
    }

    LOG_I("Alarm module initialized successfully");
    return RT_EOK;
}
