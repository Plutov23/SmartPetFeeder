#ifndef __HX711_H
#define __HX711_H

#include "main.h"

// 引脚定义：PA12 -> 时钟 (SCK/CLK)，PA11 -> 数据 (DOUT/DAT/DI)
#define HX711_SCK_PORT  GPIOA
#define HX711_SCK_PIN   GPIO_PIN_12

#define HX711_DAT_PORT  GPIOA
#define HX711_DAT_PIN   GPIO_PIN_11

// 函数声明
void HX711_Init(void);        
long HX711_Get_Weight(void);  
void HX711_Manual_Tare(void);

#endif

