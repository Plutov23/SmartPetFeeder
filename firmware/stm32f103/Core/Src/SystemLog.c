#include "SystemLog.h"
#include "usart.h" // 确保这里有 huart1 的定义

// 重定向 C 库的 printf 函数到串口 1
int fputc(int ch, FILE *f)
{
    // 把字符发给串口 1
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
