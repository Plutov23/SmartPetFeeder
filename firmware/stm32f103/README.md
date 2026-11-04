# STM32F103 固件

## 目录

- `Core/Inc`、`Core/Src`：项目业务代码和 CubeMX 生成代码。
- `Drivers`：构建所需的最小 CMSIS 与 STM32F1 HAL 驱动集合。
- `MDK-ARM`：Keil MDK 工程和启动文件。
- `F103_Feeder.ioc`：STM32CubeMX 配置。

## 配置

复制 `Core/Inc/app_config.example.h` 为 `Core/Inc/app_config.h`，然后填写本地参数。`app_config.h` 已被 Git 忽略。

开源整理版使用简化的 SSD1306 驱动和项目所需的最小 ASCII 字形集，不再依赖授权不明确的第三方中文字库。

## 外设

| 外设 | 用途 |
| --- | --- |
| USART1 | 调试日志 |
| USART2 | MaixCam 状态 |
| USART3 | ESP-01S AT 指令 |
| I2C1 | AHT20 |
| TIM3 | 1 ms 调度 |
| GPIO | DS1302、HX711、OLED、电机、蜂鸣器和 LED |
