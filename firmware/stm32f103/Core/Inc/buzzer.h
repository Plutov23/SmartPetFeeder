#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f1xx_hal.h"

/************************* 蜂鸣器硬件配置 PB12 *************************/
#define BUZZER_GPIO_PORT    GPIOB
#define BUZZER_GPIO_PIN     GPIO_PIN_12
#define BUZZER_GPIO_CLK     __HAL_RCC_GPIOB_CLK_ENABLE()

/************************* 蜂鸣器状态定义 *************************/
#define BUZZER_ON           GPIO_PIN_SET    // 蜂鸣器响
#define BUZZER_OFF          GPIO_PIN_RESET  // 蜂鸣器静音

/************************* 函数声明 *************************/
// 蜂鸣器GPIO初始化
void Buzzer_Init(void);

// 蜂鸣器开
void Buzzer_On(void);

// 蜂鸣器关
void Buzzer_Off(void);

// 蜂鸣器状态反转
void Buzzer_Toggle(void);

// 蜂鸣器响指定时间（ms）
void Buzzer_Beep(uint16_t ms);

#endif
