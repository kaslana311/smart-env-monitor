/*
 * display.h
 *
 * 显示输出模块 - 头文件
 * 负责等待信号量，获取系统状态并通过串口格式化输出。
 *
 * 归属: 成员C (display-module 分支)
 */

#ifndef DISPLAY_H__
#define DISPLAY_H__

#include "app_config.h"

/* 显示线程入口函数 */
void display_thread_entry(void *parameter);

#endif /* DISPLAY_H__ */
