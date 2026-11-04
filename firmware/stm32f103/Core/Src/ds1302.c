#include "main.h"
#include "ds1302.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include "SystemLog.h"

// 1. 新增两个全局变量存时间（写在函数外面）
uint8_t current_sys_hour = 0;
uint8_t current_sys_min = 0;
uint8_t current_sys_sec = 0;

// 全局串口句柄声明（用于printf，main.c中定义）
extern UART_HandleTypeDef huart1;

/* ******** 新增：DS1302 GPIO初始化函数（移自main.c） ******** */
void DS1302_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能GPIOA时钟（你的DS1302引脚都在GPIOA）
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 配置RST(PA2)、CLK(PA0)为推挽输出
    GPIO_InitStruct.Pin = DS1302_RST_PIN | DS1302_CLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 无上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // 低速
    HAL_GPIO_Init(DS1302_RST_PORT, &GPIO_InitStruct);

    // 初始电平（必须！否则DS1302通信异常）
    DS1302_RST_LOW();
    DS1302_CLK_LOW();
}

/* 原有底层函数（设为static，仅内部使用） */
static void DS1302_IO_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_IO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1302_IO_PORT, &GPIO_InitStruct);
}

static void DS1302_IO_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_IO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DS1302_IO_PORT, &GPIO_InitStruct);
}

static void DS1302_WriteByte(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        DS1302_CLK_LOW();
        HAL_Delay(1);
        if (data & 0x01)
            DS1302_IO_HIGH();
        else
            DS1302_IO_LOW();
        data >>= 1;
        DS1302_CLK_HIGH();
        HAL_Delay(1);
    }
}

static uint8_t DS1302_ReadByte(void)
{
    uint8_t data = 0;
    DS1302_IO_Input();
    for (int i = 0; i < 8; i++)
    {
        DS1302_CLK_LOW();
        HAL_Delay(1);
        data >>= 1;
        if (DS1302_IO_READ())
            data |= 0x80;
        DS1302_CLK_HIGH();
        HAL_Delay(1);
    }
    DS1302_IO_Output();
    return data;
}

static void DS1302_WriteRegister(uint8_t reg, uint8_t data)
{
    DS1302_RST_HIGH();
    DS1302_WriteByte(reg);
    DS1302_WriteByte(data);
    DS1302_RST_LOW();
}

static uint8_t DS1302_ReadRegister(uint8_t reg)
{
    uint8_t data;
    DS1302_RST_HIGH();
    DS1302_WriteByte(reg | 0x01);
    data = DS1302_ReadByte();
    DS1302_RST_LOW();
    return data;
}

/* DS1302初始化（内部调用GPIO初始化） */
void DS1302_Init(void)
{
    DS1302_GPIO_Init(); // 内部调用GPIO初始化，main.c无需再调
    DS1302_RST_LOW();
    DS1302_CLK_LOW();
    DS1302_IO_Output();
    DS1302_WriteRegister(DS1302_CONTROL_REG, 0x00); // 关闭写保护
}

/* 设置时间（内部完成十进制转BCD） */
/* 设置完整时间：年(0-99), 月(1-12), 日(1-31), 时, 分, 秒 */
void DS1302_SetTime(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec)
{
    // 十进制转 BCD 码
    uint8_t bcd_year = (year / 10) << 4 | (year % 10);
    uint8_t bcd_month = (month / 10) << 4 | (month % 10);
    uint8_t bcd_date = (date / 10) << 4 | (date % 10);
    uint8_t bcd_hour = (hour / 10) << 4 | (hour % 10);
    uint8_t bcd_min = (min / 10) << 4 | (min % 10);
    uint8_t bcd_sec = (sec / 10) << 4 | (sec % 10);

    DS1302_WriteRegister(0x8E, 0x00); // 关闭写保护

    DS1302_WriteRegister(0x8C, bcd_year);       // 年
    DS1302_WriteRegister(0x88, bcd_month);      // 月
    DS1302_WriteRegister(0x86, bcd_date);       // 日
    DS1302_WriteRegister(0x84, bcd_hour);       // 时
    DS1302_WriteRegister(0x82, bcd_min);        // 分
    DS1302_WriteRegister(0x80, bcd_sec & 0x7F); // 秒，并确保第7位为0（启动振荡器）

    DS1302_WriteRegister(0x8E, 0x80); // 开启写保护，防止误写
}

/* 读取时间并打印（内部完成BCD转十进制和日志输出） */
void DS1302_ReadAndPrintTime(void)
{
    uint8_t year_bcd, month_bcd, date_bcd, hour_bcd, min_bcd, sec_bcd;
    uint8_t year, month, date, hour, min, sec;

    // 1. 从寄存器读取 BCD 码数据 (读地址为 奇数)
    year_bcd = DS1302_ReadRegister(0x8D);       // 年
    month_bcd = DS1302_ReadRegister(0x89);      // 月
    date_bcd = DS1302_ReadRegister(0x87);       // 日
    hour_bcd = DS1302_ReadRegister(0x85);       // 时
    min_bcd = DS1302_ReadRegister(0x83);        // 分
    sec_bcd = DS1302_ReadRegister(0x81) & 0x7F; // 秒 (屏蔽最高位停止位)

    // 2. BCD 码转十进制
    year = (year_bcd >> 4) * 10 + (year_bcd & 0x0F);
    month = (month_bcd >> 4) * 10 + (month_bcd & 0x0F);
    date = (date_bcd >> 4) * 10 + (date_bcd & 0x0F);
    hour = (hour_bcd >> 4) * 10 + (hour_bcd & 0x0F);
    min = (min_bcd >> 4) * 10 + (min_bcd & 0x0F);
    sec = (sec_bcd >> 4) * 10 + (sec_bcd & 0x0F);

    // ============================================
    // 【最关键的一步：把算出来的时和分，存到全局变量里给计划管家用】
    // ============================================
    current_sys_hour = hour;
    current_sys_min = min;
    current_sys_sec = sec;

    // 3. 使用标准化日志输出 (带时间戳和模块名)
    APP_LOG("RTC", "Current Time: 20%02d-%02d-%02d %02d:%02d:%02d",
            year, month, date, hour, min, sec);
}

// 3. 在文件最末尾加上这两个接口函数，供外面调用
uint8_t DS1302_GetHour(void) { return current_sys_hour; }
uint8_t DS1302_GetMinute(void) { return current_sys_min; }
uint8_t DS1302_GetSecond(void) { return current_sys_sec; }
