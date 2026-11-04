#ifndef __OLED_MENU_H
#define __OLED_MENU_H

#include <stdint.h>

void OLED_Menu_Init(void);

// 专门用来刷新屏幕数据（传入时、分、秒、当前重量）
void OLED_Show_Home(uint8_t hour, uint8_t min, uint8_t sec, long weight);

#endif

