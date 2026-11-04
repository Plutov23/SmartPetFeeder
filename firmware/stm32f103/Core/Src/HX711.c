#include "hx711.h"
#include <stdio.h>
#include "SystemLog.h"
// 内部变量
long HX711_Offset = 0;
float HX711_Scale = 420.0f; // 根据你放50g砝码测出来的差值(21000)算出的系数

// 底层读取 24 位原始数据
long HX711_Read_Raw(void)
{
    long count = 0;
    uint8_t i;

    // 拉低时钟线，准备读取
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);

    // 等待数据线变低，说明 HX711 转换结束
    while (HAL_GPIO_ReadPin(HX711_DAT_PORT, HX711_DAT_PIN) == GPIO_PIN_SET)
        ;

    // 循环 24 次，读取数据
    for (i = 0; i < 24; i++)
    {
        HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
        count = count << 1;
        HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
        if (HAL_GPIO_ReadPin(HX711_DAT_PORT, HX711_DAT_PIN) == GPIO_PIN_SET)
        {
            count++;
        }
    }

    // 第 25 个脉冲：通道 A，128 倍增益
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
    count = count ^ 0x800000; // 转换数据格式
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);

    return count;
}

void HX711_Init(void)
{
    // 1. 硬件预热，连续读两次丢掉，让传感器稳定
    HX711_Read_Raw();
    HAL_Delay(50);
    HX711_Read_Raw();

    // 2. 执行一次自动去皮（假设用户开机时碗是空的）
    // 如果不为空，用户后续按按键调用 HX711_Manual_Tare 即可
    HX711_Manual_Tare();

    APP_LOG("WEIGHT", "HX711 Hardware initialized.");
}

long HX711_Get_Weight(void)
{
    long raw_data = HX711_Read_Raw();

    // 计算重量
    long weight = (long)((raw_data - HX711_Offset) / HX711_Scale);

    // 逻辑优化：如果重量小于0（比如去皮后拿走了碗），强制显示0，防止喂食器逻辑出错
    if (weight < 0)
    {
        weight = 0;
    }

    // 只有重量变化时才打印日志，避免串口被刷屏
    static long last_weight = 0;
    if (weight != last_weight)
    {
        APP_LOG("WEIGHT", "Current: %ld g", weight);
        last_weight = weight;
    }

    return weight;
}

/**
 * @brief 手动去皮函数
 * @note 当你按下按键时调用此函数。它会连续读取多次取平均值，更新全局偏移量。
 */
void HX711_Manual_Tare(void)
{
    long sum = 0;
    const int sample_times = 10; // 采样10次取平均，结果更稳

    APP_LOG("WEIGHT", "Manual taring start... Please keep the bowl empty.");

    for (int i = 0; i < sample_times; i++)
    {
        sum += HX711_Read_Raw();
        HAL_Delay(10); // 每次读取间隔10ms，给硬件反应时间
    }

    HX711_Offset = sum / sample_times;

    APP_LOG("WEIGHT", "Tare done! New Offset: %ld", HX711_Offset);
}
