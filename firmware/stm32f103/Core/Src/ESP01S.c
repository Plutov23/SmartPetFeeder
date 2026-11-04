#include "ESP01S.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "SystemLog.h"
#include "stepper_motor.h"
#include "schedule.h" // 【核心】引入计划管家，用于保存小程序下发的计划
#include "buzzer.h"
#include "app_config.h"

extern char wifi_rx_buffer[256];
extern uint16_t wifi_rx_index;

static void ESP_SendString(const char *str)
{
    // 把 HAL_MAX_DELAY 换成 100 毫秒，发不出去就算了，别把自己卡死！
    HAL_UART_Transmit(&huart3, (uint8_t *)str, strlen(str), 100);
}

// 模块初始化与联网
void ESP_Init(void)
{
    char cmd[512];

    APP_LOG("WIFI", "Connecting to configured access point...");

    ESP_SendString("AT+CWMODE=1\r\n");
    HAL_Delay(1000); // 等它切换模式

    // 1. 发送连接指令
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
    ESP_SendString(cmd);
    HAL_Delay(8000); // 给它 8 秒钟时间去握手

    // =========================================================
    // ? 真正的防骗验证逻辑 ?
    // =========================================================
    // 先清空一下缓存里的脏数据
    memset(wifi_rx_buffer, 0, sizeof(wifi_rx_buffer));
    wifi_rx_index = 0;

    // 发送查询指令：你现在连在哪个路由器上？
    ESP_SendString("AT+CWJAP?\r\n");
    HAL_Delay(1000); // 等待模块吐出它的状态

    // 去接收缓冲区里抓取，看看有没有咱们的真实热点名字
    if (strstr(wifi_rx_buffer, WIFI_SSID) != NULL)
    {
        APP_LOG("WIFI", "[REAL] Hotspot connected successfully!");
    }
    else
    {
        APP_LOG("ERROR", "[REAL] WiFi FAILED! Please check password or 2.4G settings!");
    }
    // =========================================================

    // 2. 配置 MQTT 用户信息
    snprintf(cmd, sizeof(cmd), "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"\r\n", ONENET_PRODUCT_ID, ONENET_TOKEN);
    ESP_SendString(cmd);
    HAL_Delay(500);

    snprintf(cmd, sizeof(cmd), "AT+MQTTCLIENTID=0,\"%s\"\r\n", ONENET_DEVICE_NAME);
    ESP_SendString(cmd);
    HAL_Delay(500);

    // 3. 连接 OneNET 服务器
    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%s,1\r\n", ONENET_HOST, ONENET_PORT);
    ESP_SendString(cmd);
    HAL_Delay(2000);

    // 同理，OneNET 也是真连上了才会不报错，但这里我们先不加复杂的校验了
    APP_LOG("WIFI", "MQTT Init steps finished.");

    // 4. 订阅下发指令 (用于接收一键加餐和定时计划配置)
    snprintf(cmd, sizeof(cmd), "AT+MQTTSUB=0,\"$sys/%s/%s/thing/property/set\",0\r\n", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    ESP_SendString(cmd);
    HAL_Delay(500);
}

// 发送温度数据测试用
void ESP_Publish_Temperature(int temp)
{
    char payload[128], cmd[256], topic[128];
    int data_len = snprintf(payload, sizeof(payload), "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"tem\":{\"value\":%d}}}", temp);
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%d,0,0\r\n", topic, data_len);
    ESP_SendString(cmd);
    HAL_Delay(100);
    ESP_SendString(payload);
    HAL_Delay(200);
}

// 上传所有传感器数据 (温湿度、重量)
void ESP_Publish_AllData(float temp, float humi, long weight)
{
    char payload[256], cmd[256], topic[128];
    int t = (int)temp, h = (int)humi, w = (int)weight;

    int data_len = snprintf(payload, sizeof(payload), "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"tem\":{\"value\":%d},\"wet\":{\"value\":%d},\"wei\":{\"value\":%d}}}", t, h, w);
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%d,0,0\r\n", topic, data_len);

    ESP_SendString(cmd);
    HAL_Delay(100);
    ESP_SendString(payload);
    APP_LOG("WIFI", "Cloud Sync -> Temp:%d Humi:%d Weight:%d", t, h, w);
    HAL_Delay(200);
}

// 回复已读回执，消除小程序超时报错
void ESP_Reply_Set_Property(const char *msg_id)
{
    char payload[128], cmd[256], topic[128];
    int data_len = snprintf(payload, sizeof(payload), "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}", msg_id);
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/set_reply", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%d,0,0\r\n", topic, data_len);

    ESP_SendString(cmd);
    HAL_Delay(100);
    ESP_SendString(payload);
}

// 接收并解析云端下发的所有指令
void ESP_Handle_Recv_Data(char *recv_str)
{
    if (strstr(recv_str, "thing/property/set") != NULL)
    {
        // 1. 提取 OneNET 的消息 ID，用于回复
        char msg_id[20] = "123";
        char *id_start = strstr(recv_str, "\"id\":\"");
        if (id_start != NULL)
        {
            id_start += 6;
            char *id_end = strchr(id_start, '\"');
            if (id_end != NULL && (id_end - id_start < 20))
            {
                strncpy(msg_id, id_start, id_end - id_start);
                msg_id[id_end - id_start] = '\0';
            }
        }

        // ===========================================
        // 分支 A：处理小程序的“保存喂食计划”指令
        // ===========================================
        if (strstr(recv_str, "\"sch_id\":") != NULL)
        {
            uint8_t id = 0, h = 0, m = 0, w = 0, en = 0;
            char *p;

            p = strstr(recv_str, "\"sch_id\":");
            if (p)
                id = atoi(p + 9);
            p = strstr(recv_str, "\"sch_h\":");
            if (p)
                h = atoi(p + 8);
            p = strstr(recv_str, "\"sch_m\":");
            if (p)
                m = atoi(p + 8);
            p = strstr(recv_str, "\"sch_w\":");
            if (p)
                w = atoi(p + 8);
            p = strstr(recv_str, "\"sch_en\":");
            if (p)
                en = atoi(p + 9);

            // 更新到本地计划数组中！
            Schedule_Update(id, h, m, w, en);

            // 给云平台回执
            ESP_Reply_Set_Property(msg_id);
            return; // 结束，不往下走了
        }

        // ===========================================
        // 分支 B：处理小程序的“一键加餐”指令
        // ===========================================
        char *ptr = strstr(recv_str, "\"feed_amount\":");
        if (ptr != NULL)
        {
            ptr += 14;
            int weight = atoi(ptr);

            if (weight > 0)
            {
                APP_LOG("FEED", "Manual feed: %d g", weight);

                Buzzer_Beep(100);

                // 计算步数并转动电机
                uint16_t total_steps = weight * FEEDER_STEPS_PER_GRAM;
                StepperMotor_SetDirection(CLOCKWISE);
                StepperMotor_SetSpeed(2000);
                StepperMotor_Step(total_steps);

                // 给云平台回执
                ESP_Reply_Set_Property(msg_id);
            }
        }
    }
}
// 上传宠物进食健康报告
void ESP_Publish_HealthReport(int time_sec, int weight_g, int cnt)
{
    char payload[256], cmd[256], topic[128];

    // 把时长、吃掉的重量、记录触发器打包成 JSON
    int data_len = snprintf(payload, sizeof(payload), "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"eat_time\":{\"value\":%d},\"eat_weight\":{\"value\":%d},\"eat_cnt\":{\"value\":%d}}}", time_sec, weight_g, cnt);
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%d,0,0\r\n", topic, data_len);

    ESP_SendString(cmd);
    HAL_Delay(100);
    ESP_SendString(payload);
    APP_LOG("WIFI", "Health Event Sync -> Duration:%d sec Consumed:%d g Count:%d", time_sec, weight_g, cnt);
    HAL_Delay(200);
}
