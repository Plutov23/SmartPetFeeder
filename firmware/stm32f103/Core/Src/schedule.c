#include "schedule.h"
#include "stepper_motor.h"
#include "SystemLog.h"

#define MAX_SCHEDULES 5    // 最多存5个计划
#define STEPS_PER_GRAM 50  // 每克50步

typedef struct {
    uint8_t id;
    uint8_t hour;
    uint8_t minute;
    uint16_t weight;
    uint8_t enabled;
    uint8_t executed_today; // 防重复执行标志位
} ScheduleTask_t;

static ScheduleTask_t schedules[MAX_SCHEDULES];

void Schedule_Init(void) {
    for(int i = 0; i < MAX_SCHEDULES; i++) {
        schedules[i].enabled = 0;
        schedules[i].executed_today = 0;
    }
    APP_LOG("SCH", "Schedule module initialized.");
}

void Schedule_Update(uint8_t id, uint8_t hour, uint8_t minute, uint16_t weight, uint8_t enabled) {
    if(id < MAX_SCHEDULES) {
        schedules[id].id = id;
        schedules[id].hour = hour;
        schedules[id].minute = minute;
        schedules[id].weight = weight;
        schedules[id].enabled = enabled;
        schedules[id].executed_today = 0; // 改了计划就重置标志位
        
        APP_LOG("SCH", "Schedule updated -> ID:%d Time:%02d:%02d Weight:%dg Status:%d", id, hour, minute, weight, enabled);
    }
}

void Schedule_CheckAndFeed(uint8_t current_hour, uint8_t current_minute) {
    for(int i = 0; i < MAX_SCHEDULES; i++) {
        if(schedules[i].enabled == 1) { 
            // 判断时间到了没
            if(schedules[i].hour == current_hour && schedules[i].minute == current_minute) {
                // 判断这一分钟内有没有喂过
                if(schedules[i].executed_today == 0) {
                    APP_LOG("SCH", "时间到！执行计划 %d, 出粮: %dg", i, schedules[i].weight);
                    
                    uint16_t total_steps = schedules[i].weight * STEPS_PER_GRAM;
                    StepperMotor_SetDirection(CLOCKWISE);
                    StepperMotor_SetSpeed(2000);
                    StepperMotor_Step(total_steps);
                    
                    schedules[i].executed_today = 1; // 标记已喂
                }
            } 
            else {
                schedules[i].executed_today = 0; // 时间过了，恢复标志位等明天
            }
        }
    }
}
