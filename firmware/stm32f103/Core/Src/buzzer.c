#include "buzzer.h"

/**
 * @brief  蜂鸣器GPIO初始化
 * @param  无
 * @retval 无
 */
void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 开启GPIO时钟
    BUZZER_GPIO_CLK;

    // 配置GPIO为推挽输出模式
    GPIO_InitStruct.Pin = BUZZER_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;    // 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;            // 无上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;   // 低速模式
    HAL_GPIO_Init(BUZZER_GPIO_PORT, &GPIO_InitStruct);

    // 初始化完成后关闭蜂鸣器
    Buzzer_Off();
}

/**
 * @brief  打开蜂鸣器
 * @param  无
 * @retval 无
 */
void Buzzer_On(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, BUZZER_ON);
}

/**
 * @brief  关闭蜂鸣器
 * @param  无
 * @retval 无
 */
void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, BUZZER_OFF);
}

/**
 * @brief  反转蜂鸣器状态
 * @param  无
 * @retval 无
 */
void Buzzer_Toggle(void)
{
    HAL_GPIO_TogglePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}

/**
 * @brief  蜂鸣器响指定时间
 * @param  ms: 鸣叫时间 单位毫秒
 * @retval 无
 */
void Buzzer_Beep(uint16_t ms)
{
    Buzzer_On();
    HAL_Delay(ms);
    Buzzer_Off();
}
