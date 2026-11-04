#ifndef __SYS_LOG_H__
#define __SYS_LOG_H__

#include "main.h"
#include <stdio.h>

/* * 这是一个全局日志打印宏。
 * 格式：[运行毫秒数][模块名] 内容
 * %08lu 会自动把 HAL_GetTick() 补齐8位数字，对齐起来特别好看。
 * ##__VA_ARGS__ 允许你像用 printf 一样往里面传变量。
 */
#define APP_LOG(TAG, format, ...) printf("[%08lu][%s] " format "\r\n", HAL_GetTick(), TAG, ##__VA_ARGS__)

#endif
