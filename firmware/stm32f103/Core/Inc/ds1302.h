#ifndef _DS1302_H
#define _DS1302_H
#include "stm32f1xx.h"

/* ******** 替换为你的实际硬件接线 ******** */
#define DS1302_RST_PORT GPIOA
#define DS1302_RST_PIN  GPIO_PIN_6  // RST: PA4
#define DS1302_CLK_PORT GPIOA
#define DS1302_CLK_PIN  GPIO_PIN_4  // CLK: PA5
#define DS1302_IO_PORT  GPIOA
#define DS1302_IO_PIN   GPIO_PIN_5  // IO: PA6

/* 寄存器指令定义不变 */
#define DS1302_SEC_REG    0x80
#define DS1302_MIN_REG    0x82
#define DS1302_HOUR_REG   0x84
#define DS1302_DATE_REG   0x86
#define DS1302_MONTH_REG  0x88
#define DS1302_DAY_REG    0x8A
#define DS1302_YEAR_REG   0x8C
#define DS1302_CONTROL_REG 0x8E

/* 引脚操作宏不变 */
#define DS1302_RST_HIGH() HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_SET)
#define DS1302_RST_LOW()  HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_RESET)
#define DS1302_CLK_HIGH() HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_SET)
#define DS1302_CLK_LOW()  HAL_GPIO_WritePin(DS1302_CLK_PORT, DS1302_CLK_PIN, GPIO_PIN_RESET)
#define DS1302_IO_HIGH()  HAL_GPIO_WritePin(DS1302_IO_PORT, DS1302_IO_PIN, GPIO_PIN_SET)
#define DS1302_IO_LOW()   HAL_GPIO_WritePin(DS1302_IO_PORT, DS1302_IO_PIN, GPIO_PIN_RESET)
#define DS1302_IO_READ()  HAL_GPIO_ReadPin(DS1302_IO_PORT, DS1302_IO_PIN)

/* 函数声明（新增DS1302_GPIO_Init，删除main中的GPIO初始化） */
void DS1302_GPIO_Init(void);  // GPIO初始化移到ds1302.c
void DS1302_Init(void);
void DS1302_SetTime(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec);
void DS1302_ReadAndPrintTime(void);
// 3. 在文件最末尾加上这两个接口函数，供外面调用
uint8_t DS1302_GetHour(void);
uint8_t DS1302_GetMinute(void);
uint8_t DS1302_GetSecond(void);
// 以下函数设为静态（仅ds1302.c内部使用，不对外暴露）
// uint8_t DS1302_ReadRegister(uint8_t reg);  // 注释/删除，仅内部用
// void DS1302_WriteRegister(uint8_t reg, uint8_t data);  // 注释/删除，仅内部用


#endif 

