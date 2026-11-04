# 问题排查

## STM32 无日志

- 确认 USART1 使用 PA9/PA10 和 9600 baud。
- 检查 USB 转串口模块是否与主控共地。
- 检查系统时钟和固件是否成功烧录。

## ESP-01S 无法联网

- 确认使用 2.4 GHz Wi-Fi。
- 检查 `app_config.h` 的 SSID 和密码。
- 测量 ESP-01S 供电，排除发送瞬间压降。
- 确认 USART3 为 115200 baud，TX/RX 交叉连接。
- 使用串口工具单独测试 `AT` 和 `AT+CWJAP?`。

## OneNET 没有数据

- 确认 Product ID、Device Name 和 Token 属于同一设备。
- 检查设备是否在线、物模型标识符是否与 `protocol.md` 一致。
- 检查 ESP-01S MQTT AT 固件是否支持代码中使用的指令。

## MaixCam 没有识别结果

- 确认模型路径和 MaixPy API 版本。
- 确认 UART 使用 9600 baud。
- 确认模型类别编号与代码中的猫、狗编号一致。
- 先在 MaixCam 控制台观察每秒发送的文本。

## STM32 收不到 MaixCam 数据

- 交叉连接 TX/RX，并连接公共 GND。
- 使用串口工具发送 `CAT,EATING\r\n` 验证 STM32 解析。
- 检查 USART2 是否发生溢出错误。

## 小程序没有视频

- 确认 `config.js` 中的 WebSocket 地址可以从当前设备访问。
- 确认 MaixCam 与手机处于同一局域网。
- 在正式小程序环境中检查合法域名、HTTPS/WSS 和平台权限要求。

## 出粮重量不准

- 重新校准 HX711。
- 调整 `FEEDER_STEPS_PER_GRAM`。
- 检查料道是否卡粮、打滑或每次填充状态不同。
- 对多个目标重量重复测试并记录平均值和标准差。
