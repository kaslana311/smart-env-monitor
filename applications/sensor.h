/*
 * sensor.h
 *
 * 传感器数据采集模块 - 头文件
 * 负责模拟环境传感器数据采集，并通过消息队列发送给处理线程。
 *
 * 归属: 成员A (driver-main 分支)
 */

#ifndef SENSOR_H__
#define SENSOR_H__

#include "app_config.h"

/* 传感器线程入口函数 */
void sensor_thread_entry(void *parameter);

/* LED 初始化 */
int led_init(void);

/* 心跳 LED 闪烁 */
void led_heartbeat_toggle(void);

/* 告警 LED 控制 */
void led_alarm_on(void);
void led_alarm_off(void);
void led_alarm_toggle(void);

#endif /* SENSOR_H__ */
