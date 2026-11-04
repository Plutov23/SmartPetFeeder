#ifndef __SCHEDULE_H
#define __SCHEDULE_H

#include <stdint.h>

void Schedule_Init(void);
// 接收小程序发来的计划
void Schedule_Update(uint8_t id, uint8_t hour, uint8_t minute, uint16_t weight, uint8_t enabled);
// 定时检查
void Schedule_CheckAndFeed(uint8_t current_hour, uint8_t current_minute);

#endif


