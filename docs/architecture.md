# 系统架构

## 数据链路

| 类型 | 路径 |
| --- | --- |
| 传感数据 | AHT20、HX711、DS1302 → STM32 → ESP-01S → OneNET → 小程序 |
| 控制指令 | 小程序 → OneNET → ESP-01S → STM32 → 电机或计划模块 |
| 视觉状态 | MaixCam → UART2 → STM32 |
| 视频画面 | MaixCam → WebSocket → 小程序 |

## STM32 调度

主循环持续调用 `Maixcam_Run()`，并执行以下周期任务：

- 1 秒任务：读取 DS1302、读取 HX711、检查喂食计划、刷新 OLED。
- 5 秒任务：读取 AHT20、向 OneNET 上报温度、湿度和重量。
- 实时任务：解析 ESP-01S 下行数据、处理 MaixCam UART 数据、恢复异常 UART 状态。

## 进食检测

1. MaixCam 识别到 `EATING` 后通过 UART 上报。
2. STM32 记录开始时间和起始重量。
3. MaixCam 上报 `NONE` 后，STM32 计算持续时间和重量差。
4. 持续时间达到阈值时，STM32 向 OneNET 上报健康记录。
5. 小程序根据 `eat_cnt` 去重并写入本地喂食记录。

## 当前限制

- 视觉姿态由检测框宽高比和位置启发式判断，不是独立姿态模型。
- 喂食计划保存在 RAM 中，STM32 重启后会丢失。
- Wi-Fi 和 MQTT 初始化仍使用阻塞延时。
- 小程序的进食记录保存在本机缓存，不是多设备同步数据库。
- 视频流为 Base64 JPEG WebSocket，适合局域网演示，不适合公网高并发。
