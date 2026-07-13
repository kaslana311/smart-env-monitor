/*
 * process.h
 *
 * 数据处理模块 - 头文件
 * 负责从消息队列接收传感器数据，进行滤波处理与阈值检测。
 *
 * 归属: 成员B (thread-function 分支)
 */

#ifndef PROCESS_H__
#define PROCESS_H__

#include "app_config.h"

/* 数据处理线程入口函数 */
void process_thread_entry(void *parameter);

#endif /* PROCESS_H__ */
