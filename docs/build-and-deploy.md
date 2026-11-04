# 构建与部署

## 1. STM32 固件

### 环境

- Keil MDK-ARM 5
- STM32F1 Device Family Pack
- ST-Link 或兼容烧录器
- 可选：STM32CubeMX，用于查看和重新生成 `.ioc`

原始工程验证环境使用 ARM Compiler 5.06、ARM CMSIS 5.0.1 和 Keil STM32F1xx DFP 2.2.0。其他版本可能需要重新选择编译器或更新 Pack。

### 配置

先复制配置模板：

```text
app_config.example.h -> app_config.h
```

然后编辑 `firmware/stm32f103/Core/Inc/app_config.h`。

至少修改：

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `ONENET_PRODUCT_ID`
- `ONENET_DEVICE_NAME`
- `ONENET_TOKEN`
- `FEEDER_STEPS_PER_GRAM`

### 编译

1. 打开 `firmware/stm32f103/MDK-ARM/F103_Feeder.uvprojx`。
2. 确认目标器件为 STM32F103C8Tx。
3. 选择兼容的 ARM 编译器。
4. 执行 Build。
5. 连接 ST-Link，烧录并复位。
6. 使用 9600 baud 打开 USART1 日志。

OLED 使用项目内置的最小 ASCII 字形集。增加新的显示字符时，需要在 `Core/Src/OLED.c` 中补充对应 5x7 字形。

## 2. MaixCam

### 前置条件

- 支持当前 `maix` API 的 MaixPy 系统
- `pyserial`、`websockets`
- 可由 MaixPy `nn.YOLOv5` 加载的模型文件

将 `vision/maixcam/config.example.py` 复制为 `config.py`，然后配置：

- 填写 Wi-Fi 名称和密码。
- 确认 UART 设备路径与波特率。
- 设置模型文件路径。
- 根据需要修改 WebSocket 端口。

将 `main.py`、`config.py` 和 `app.yaml` 部署到 MaixCam。视觉端与 STM32 必须共地，并交叉连接 TX/RX。

当前代码假设模型使用 COCO 类别编号 15 表示猫、16 表示狗。如果更换模型，需要同步修改类别映射。

## 3. 微信小程序

将 `miniapp/wechat/config.example.js` 复制为 `config.js`，然后配置：

- `onenet.productId`
- `onenet.deviceName`
- `onenet.authorization`
- `maixcam.websocketUrl`

使用微信开发者工具导入 `miniapp/wechat`。仓库使用 `touristappid`，发布前需要在本地填写自己的 AppID。

局域网调试 WebSocket 时，需要保证手机、开发工具和 MaixCam 能互相访问。正式发布时还需要按照微信平台要求配置合法域名和 TLS。

## 4. 联调顺序

1. 单独确认 STM32 USART1 日志正常。
2. 校准 HX711，确认重量方向和单位正确。
3. 测试步进电机方向与每克步数。
4. 确认 ESP-01S 能连接 Wi-Fi 和 OneNET。
5. 用串口工具模拟 `CAT,EATING` 和 `NONE,NONE`。
6. 部署 MaixCam，确认 STM32 能接收真实识别状态。
7. 配置小程序，最后测试视频、手动喂食和计划同步。
