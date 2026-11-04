# MaixCam 视觉端

视觉端负责：

- 使用 YOLOv5 检测猫和狗。
- 根据目标框比例和位置估计 `EATING`、`RESTING`、`WAITING`。
- 每秒通过 UART 向 STM32 发送一次状态。
- 以低码率 JPEG 和 Base64 JSON 形式提供 WebSocket 视频流。

部署前复制 `config.example.py` 为 `config.py` 并修改本地配置。默认模型路径为 `/root/models/yolov5s.mud`，默认串口为 `/dev/ttyS0`，默认 WebSocket 端口为 `8765`。

当前代码使用 COCO 类别 15 和 16 作为猫、狗。自定义模型必须同步修改类别映射。
