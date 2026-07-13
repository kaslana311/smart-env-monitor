/*
 * alarm.h
 *
 * 告警监控模块 - 头文件
 * 负责监控系统状态，根据告警标志控制告警 LED 的闪烁模式。
 *
 * 归属: 成员B (thread-function 分支)
 */

#ifndef ALARM_H__
#define ALARM_H__

#include "app_config.h"

/* 告警线程入口函数 */
void alarm_thread_entry(void *parameter);

#endif /* ALARM_H__ */
