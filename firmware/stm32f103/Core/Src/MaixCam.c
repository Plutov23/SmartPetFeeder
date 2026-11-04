#include "maixcam.h"
#include <stdio.h>
#include <string.h>

// 【新增】：引入日志和云平台发送函数的头文件
#include "SystemLog.h"
#include "ESP01S.h"
#include "buzzer.h"


// 引入外部的串口2句柄
extern UART_HandleTypeDef huart2;
// 【新增】：引入 main.c 里面的重量变量，用来计算吃掉了多少猫粮
extern long weight;

/************************* 内部私有宏定义 *************************/
#define MAIXCAM_RX_BUF_SIZE 128

/************************* 内部私有变量 (对外绝对隐藏) *************************/
static uint8_t maixcam_rx_char;
static char rx_buffer[MAIXCAM_RX_BUF_SIZE];
static uint16_t rx_index = 0;
static uint8_t data_received = 0;

static char rx_target[16];
static char rx_status[16];
static char current_pet_status[16] = "NONE";

// 【核心新增】：把秒表计时的变量全部封装在模块内部
static uint8_t is_eating = 0;       // 标记现在猫是不是正在吃
static uint32_t eat_start_tick = 0; // 记录开始吃的时间戳
static long eat_start_weight = 0;   // 记录开始吃的时候粮桶有多重
static int eat_record_cnt = 0;      // 记录是第几次干饭

/************************* 内部私有业务逻辑 *************************/
// 这是底层的白名单校验
static void ProcessData(void)
{
    if (strstr(rx_buffer, "CAT") != NULL ||
        strstr(rx_buffer, "DOG") != NULL ||
        strstr(rx_buffer, "NONE") != NULL)
    {
        char *comma = strchr(rx_buffer, ',');
        if (comma != NULL)
        {
            *comma = '\0';
            strncpy(rx_target, rx_buffer, sizeof(rx_target) - 1);
            strncpy(rx_status, comma + 1, sizeof(rx_status) - 1);

            char *newline = strchr(rx_status, '\n');
            if (newline)
                *newline = '\0';
            char *return_char = strchr(rx_status, '\r');
            if (return_char)
                *return_char = '\0';

            strncpy(current_pet_status, rx_status, sizeof(current_pet_status) - 1);
            data_received = 1;
        }
    }
}

static void Maixcam_OnDetected(const char *target, const char *status)
{
    // 1. 开始干饭 (Start eating)
    if (strcmp(status, "EATING") == 0 && is_eating == 0)
    {
        is_eating = 1;                           
        eat_start_tick = HAL_GetTick();          
        eat_start_weight = weight;               
        // 打印全英文：目标开始进食，开始计时
        APP_LOG("HEALTH", "Target [%s] started EATING. Timer started.", target);
        Buzzer_Beep(100);
    }
    
    // 2. 吃完走人 (Finished eating)
    else if (strcmp(status, "NONE") == 0 && is_eating == 1)
    {
        is_eating = 0;                           
        int duration = (HAL_GetTick() - eat_start_tick) / 1000; 
        int consumed = eat_start_weight - weight;
        if (consumed < 0) consumed = 0; 

        // 大于10秒算作正式干饭
        if (duration >= 5) 
        {
            eat_record_cnt++; 
            // 打印全英文：进食结束，用时 XX 秒，消耗 XX 克
            APP_LOG("HEALTH", "EATING finished! Duration: %d sec, Consumed: %d g.", duration, consumed);
            ESP_Publish_HealthReport(duration, consumed, eat_record_cnt);
            Buzzer_Beep(100);
            HAL_Delay(100);
            Buzzer_Beep(100);
        }
        else
        {
            // 打印全英文：时间太短，忽略不计
            APP_LOG("HEALTH", "Duration too short (%d sec). Ignored.", duration);
        }
    }
}

/************************* 对外接口函数 *************************/
void Maixcam_Init(void)
{
    rx_index = 0;
    data_received = 0;
    memset(rx_buffer, 0, MAIXCAM_RX_BUF_SIZE);
    memset(rx_target, 0, sizeof(rx_target));
    memset(rx_status, 0, sizeof(rx_status));
    strcpy(current_pet_status, "NONE");

    // 初始化秒表变量
    is_eating = 0;
    eat_start_tick = 0;
    eat_start_weight = 0;

    HAL_UART_Receive_IT(&huart2, &maixcam_rx_char, 1);
}

void Maixcam_Run(void)
{
    if (data_received)
    {
        // 直接在内部触发健康监控的计算
        Maixcam_OnDetected(rx_target, rx_status);
        data_received = 0;

        memset(rx_target, 0, sizeof(rx_target));
        memset(rx_status, 0, sizeof(rx_status));
    }
}

void Maixcam_UART_IRQHandler(uint8_t ch)
{
    if (ch == '\n' || ch == '\r')
    {
        if (rx_index > 0)
        {
            rx_buffer[rx_index] = '\0';
            // APP_LOG("DEBUG", "Received Raw Data: %s", rx_buffer);
            ProcessData();
            rx_index = 0;
            memset(rx_buffer, 0, MAIXCAM_RX_BUF_SIZE);
        }
    }
    else
    {
        if (rx_index < MAIXCAM_RX_BUF_SIZE - 1)
        {
            rx_buffer[rx_index++] = ch;
        }
        else
        {
            rx_index = 0;
            memset(rx_buffer, 0, MAIXCAM_RX_BUF_SIZE);
        }
    }
}

UART_HandleTypeDef *Maixcam_GetUARTHandle(void) { return &huart2; }
uint8_t *Maixcam_GetRxCharPtr(void) { return &maixcam_rx_char; }
const char *Maixcam_GetPetStatus(void) { return current_pet_status; }
