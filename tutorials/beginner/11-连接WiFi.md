# 第 11 课：使用 ESP-01S 连接 Wi-Fi

## 这一课要完成什么

让 ESP-01S 连接家里或手机提供的 2.4 GHz Wi-Fi。

这一课只确认联网，不测试 OneNET。

## ESP-01S 是做什么的

STM32 负责控制设备，但不能直接连接 Wi-Fi。

ESP-01S 是它的网络助手。

STM32 通过串口向 ESP-01S 发送 AT 命令。

AT 命令可以理解成给 Wi-Fi 模块的文字命令。

## 电源非常重要

ESP-01S 需要稳定的 3.3 V。

它联网时会突然需要较大电流。如果电源太弱，模块可能：

- 反复重启
- 偶尔连接成功
- 一发送数据就掉线

不要直接给裸 ESP-01S 接 5 V。

## 接线

| ESP-01S | STM32 |
| --- | --- |
| RX | PB10 / USART3_TX |
| TX | PB11 / USART3_RX |
| GND | GND |
| VCC | 稳定 3.3 V |
| EN / CH_PD | 按模块要求拉高 |

USART3 使用 115200 波特率。

## 先单独测试 ESP-01S

如果有 USB 转 TTL，建议先让 ESP-01S 单独连接电脑。

串口设置为 115200，然后发送：

~~~text
AT
~~~

正常返回：

~~~text
OK
~~~

再发送：

~~~text
AT+GMR
~~~

保存固件版本。后面使用的 MQTT 命令需要兼容的 AT 固件。

## 填写 Wi-Fi 配置

打开：

~~~text
firmware/stm32f103/Core/Inc/app_config.h
~~~

找到：

~~~c
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
~~~

填写自己的 Wi-Fi 名称和密码。

不要修改 <code>app_config.example.h</code>。

不要把真实密码提交到 GitHub。

## 添加临时联网测试

在 <code>USER CODE BEGIN 2</code> 后加入：

~~~c
HAL_UART_Receive_IT(&huart3, &wifi_rx_char, 1);
ESP_Init();

while (1)
{
    HAL_Delay(100);
}
~~~

## 正确现象

USART1 日志出现：

~~~text
[WIFI] Connecting to configured access point...
[WIFI] [REAL] Hotspot connected successfully!
~~~

当前 <code>ESP_Init()</code> 还会继续发送 MQTT 初始化命令。OneNET 参数尚未正确时，后面的 MQTT 步骤可能失败。本课只看 Wi-Fi 连接结果。

## 连接失败

检查：

1. 热点是否支持 2.4 GHz
2. Wi-Fi 名称和密码是否正确
3. ESP-01S 是否稳定获得 3.3 V
4. PB10、PB11 是否交叉连接
5. GND 是否共地
6. AT 固件是否正常

## 模块反复重启

优先检查供电，不要先修改程序。

测量模块联网瞬间的 3.3 V 是否明显下降。

## 实验完成后

删除临时测试循环。

保留本地 <code>app_config.h</code>，但不要提交它。

## 这一课的完成标准

- [ ] ESP-01S 单独响应 AT
- [ ] 使用稳定 3.3 V 供电
- [ ] 能连接 2.4 GHz Wi-Fi
- [ ] USART1 显示热点连接成功
- [ ] 真实 Wi-Fi 密码没有进入 Git

上一课：[步进电机与出粮](10-步进电机与出粮.md)

下一课：[连接 OneNET](12-连接OneNET.md)

返回：[零基础教程目录](README.md)
