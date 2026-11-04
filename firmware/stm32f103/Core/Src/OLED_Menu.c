#include "OLED_Menu.h"
#include "OLED.h"
#include <stdio.h>

// 【核心参数】定义你的粮桶满载是多少克，用来计算百分比进度条
#define MAX_FOOD_WEIGHT 500  

void OLED_Menu_Init(void)
{
    OLED_Init();
    OLED_Clear();
    OLED_Update();
}

// 刷新屏幕显示内容
void OLED_Show_Home(uint8_t hour, uint8_t min, uint8_t sec, long weight)
{
    char buf[24];
    OLED_Clear();

    // 1. 显示当前时间 (居中在顶部)
    sprintf(buf, "%02d:%02d:%02d", hour, min, sec);
    OLED_ShowString(32, 0, buf, OLED_8X16);

    // 2. 限制重量范围并计算百分比
    if(weight < 0) weight = 0;
    long percent = (weight * 100) / MAX_FOOD_WEIGHT;
    if(percent > 100) percent = 100;

    // 3. 画进度条外框 (保持你原本的设计)
    OLED_DrawRectangle(10, 20, 108, 12, OLED_UNFILLED);
    
    // 4. 画进度条填充 (动态变化)3
    if(percent > 0)
    {
        uint16_t w = (percent * 104) / 100;
        OLED_DrawRectangle(12, 22, w, 8, OLED_FILLED);
    }

    // 5. Display the remaining-food percentage.
    sprintf(buf, "FOOD %d%%", (int)percent);
    OLED_ShowString(24, 38, buf, OLED_8X16);

    // 刷新到屏幕上
    OLED_Update();
}
