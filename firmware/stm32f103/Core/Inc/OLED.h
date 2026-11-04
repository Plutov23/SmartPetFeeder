#ifndef OLED_H
#define OLED_H

#include <stdint.h>

#define OLED_6X8 6
#define OLED_8X16 8

#define OLED_UNFILLED 0
#define OLED_FILLED 1

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_DrawPoint(int16_t x, int16_t y);
void OLED_DrawRectangle(
    int16_t x,
    int16_t y,
    uint8_t width,
    uint8_t height,
    uint8_t filled);
void OLED_ShowString(
    int16_t x,
    int16_t y,
    const char *string,
    uint8_t font_size);

#endif /* OLED_H */
