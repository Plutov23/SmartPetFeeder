#include "aht20.h"
#include "stdio.h"
#include "SystemLog.h"

#define AHT20_ADDRESS 0x70

/* 定义存储温湿度的变量 */
/* 外部变量声明，告诉编译器去别的文件找这两个变量 */
extern float temp;
extern float humi;

/**
 * @brief  初始化AHT20
 */
void AHT20_Init(void)
{
  uint8_t readBuffer;
  HAL_Delay(40);

  // 这里全改成 hi2c1
  HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS, &readBuffer, 1, HAL_MAX_DELAY);
  if ((readBuffer & 0x08) == 0x00)
  {
    uint8_t sendBuffer[3] = {0xBE, 0x08, 0x00};
    HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, sendBuffer, 3, HAL_MAX_DELAY);
  }
}

/**
 * @brief  获取温度和湿度并打印日志
 * @param  Temperature: 存储获取到的温度
 * @param  Humidity: 存储获取到的湿度
 */
void AHT20_Read(float *Temperature, float *Humidity)
{
  uint8_t sendBuffer[3] = {0xAC, 0x33, 0x00};
  uint8_t readBuffer[6] = {0};

  // 1. 发送触发测量命令
  HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, sendBuffer, 3, HAL_MAX_DELAY);

  // 2. 等待测量完成 (AHT20 至少需要 75ms)
  HAL_Delay(75);

  // 3. 读取 6 字节数据
  HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS, readBuffer, 6, HAL_MAX_DELAY);

  // 4. 检查状态位（第0字节的第7位为0代表测量完成且有效）
  if ((readBuffer[0] & 0x80) == 0x00)
  {
    uint32_t data = 0;

    // 计算湿度
    data = ((uint32_t)readBuffer[3] >> 4) + ((uint32_t)readBuffer[2] << 4) + ((uint32_t)readBuffer[1] << 12);
    *Humidity = (float)data * 100.0f / (1 << 20);

    // 计算温度
    data = (((uint32_t)readBuffer[3] & 0x0F) << 16) + ((uint32_t)readBuffer[4] << 8) + (uint32_t)readBuffer[5];
    *Temperature = (float)data * 200.0f / (1 << 20) - 50;

    // 5. 打印标准化日志
    APP_LOG("AHT20", "Read Success - Temp: %.1f C, Humi: %.1f %%", *Temperature, *Humidity);
  }
  else
  {
    // 如果测量未完成或出错，也可以记录一条警告
    APP_LOG("AHT20", "Read Failed - Sensor busy or error.");
  }
}
