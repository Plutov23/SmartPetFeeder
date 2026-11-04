#ifndef __ESP01S_H__
#define __ESP01S_H__

#include "main.h"

// 将大块逻辑封装成两个核心接口，让 main.c 保持干净
void ESP_Init(void);
void ESP_Publish_Temperature(int temp);
void ESP_Publish_AllData(float temp, float humi, long weight);
void ESP_Publish_HealthReport(int time_sec, int weight_g, int cnt);
#endif
