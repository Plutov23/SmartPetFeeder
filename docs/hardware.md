# 硬件与接线

## 主控

- MCU：STM32F103C8T6，LQFP48
- 系统时钟：72 MHz
- 调试串口：USART1，9600 baud
- MaixCam：USART2，9600 baud
- ESP-01S：USART3，115200 baud
- AHT20：I2C1，100 kHz
- 步进电机调度：TIM3，1 ms

## 接线表

| 模块 | 接口 | STM32 引脚 | 建议电压 | 功能 |
| --- | --- | --- | --- | --- |
| 调试串口 | USART1 | PA9 TX / PA10 RX | 3.3 V | 日志输出 |
| MaixCam | USART2 | PA2 TX / PA3 RX | 按模块规格供电 | 识别结果 |
| ESP-01S | USART3 | PB10 TX / PB11 RX | 3.3 V | Wi-Fi 与 MQTT |
| OLED | 软件 I2C | PB13 SCL / PB14 SDA | 3.3 V | 本地显示 |
| AHT20 | I2C1 | PB8 SCL / PB9 SDA | 3.3 V | 温湿度 |
| DS1302 | GPIO | PA4 RST / PA5 CLK / PA6 IO | 3.3 V | 实时时钟 |
| HX711 | GPIO | PA11 DOUT / PA12 SCK | 按模块规格供电 | 称重 |
| ULN2003 | GPIO | PB4 / PB5 / PB6 / PB7 | 5 V 电机电源 | 步进电机 |
| 蜂鸣器 | GPIO | PB12 | 3.3 V | 提示音 |
| LED | GPIO | PB15 | 3.3 V | 状态指示 |

## 供电注意事项

- 所有通信模块必须与 STM32 共地。
- ESP-01S 峰值电流较高，应使用稳定的 3.3 V 电源，不建议直接由弱电流 USB 转串口模块供电。
- 电机应使用独立的 5 V 电源轨，由 ULN2003 驱动，不能直接连接 MCU GPIO。
- MaixCam 的供电电压和串口电平以具体板卡说明为准。
- 首次通电前应检查降压模块输出，确认没有将 5 V 或 12 V 接入 3.3 V 引脚。
- 机械结构应增加防卡粮和电机堵转保护。

## 校准

- 在 `HX711.c` 中校准称重零点和比例系数。
- 在本地 `app_config.h` 中调整 `FEEDER_STEPS_PER_GRAM`。
- 使用多个不同重量点验证出粮误差，不要只用单点标定。
