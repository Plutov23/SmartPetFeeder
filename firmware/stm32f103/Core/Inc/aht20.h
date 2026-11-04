#ifndef __AHT20_H__
#define __AHT20_H__

#include "i2c.h"
#include "main.h"

extern float temp, humi; // 用于存储温湿度数据

// 初始化AHT20
void AHT20_Init(void);

// 获取温度和湿度
void AHT20_Read(float *Temperature, float *Humidity);

#endif
