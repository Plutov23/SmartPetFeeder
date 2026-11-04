#ifndef __MAIXCAM_H
#define __MAIXCAM_H

#include "main.h" // 确保能识别 UART_HandleTypeDef 等硬件类型

// 1. 核心控制接口
void Maixcam_Init(void);
void Maixcam_Run(void);

// 2. 硬件中断接口 (给 main.c 的底层的中断回调使用)
void Maixcam_UART_IRQHandler(uint8_t ch);
UART_HandleTypeDef *Maixcam_GetUARTHandle(void);
uint8_t *Maixcam_GetRxCharPtr(void);

// 3. 状态读取接口 (如果你以后想在 OLED 屏幕上显示猫的状态，调用这个就行)
const char* Maixcam_GetPetStatus(void);

#endif /* __MAIXCAM_H */
