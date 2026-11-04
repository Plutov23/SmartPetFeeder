# 第 7 章：MaixCam 视觉识别与状态协议

## 本章目标

完成本章后，你应该能够：

- 部署 MaixCam 视觉程序和本地配置
- 解释目标检测与进食状态启发式判断
- 通过 UART 向 STM32 发送状态
- 通过 WebSocket 向小程序发送画面
- 独立模拟和排查视觉通信
- 理解当前视觉方案的适用边界

## 1. MaixCam 的职责

MaixCam 同时承担三项任务：

1. 运行 YOLOv5，检测猫和狗
2. 每秒通过 UART 向 STM32 发送目标状态
3. 将低码率 JPEG 画面通过 WebSocket 发给小程序

~~~text
摄像头画面
  ├── YOLOv5 → CAT/DOG + 状态 → USART2 → STM32
  └── JPEG Base64 → WebSocket → 微信小程序
~~~

视频画面不经过 STM32，也不经过 OneNET。

## 2. 文件结构

~~~text
vision/maixcam/
├── main.py
├── config.example.py
├── app.yaml
├── app.png
└── README.md
~~~

模型文件没有打包在仓库中。你需要准备与当前 MaixPy <code>nn.YOLOv5</code> API 兼容的模型，并确认模型许可证允许使用和分发。

## 3. 创建本地配置

复制模板：

~~~bash
cp config.example.py config.py
~~~

填写：

~~~python
WIFI_SSID = "YOUR_WIFI_SSID"
WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"

UART_DEVICE = "/dev/ttyS0"
UART_BAUD_RATE = 9600

MODEL_PATH = "/root/models/yolov5s.mud"
WEBSOCKET_HOST = "0.0.0.0"
WEBSOCKET_PORT = 8765
~~~

<code>config.py</code> 已被 Git 忽略。不同系统镜像的串口设备路径可能不同，应在目标设备上确认。

## 4. 部署前准备

视觉端依赖：

- 支持当前 <code>maix</code> API 的 MaixPy 系统
- <code>pyserial</code>
- <code>websockets</code>
- 可加载的 YOLOv5 模型

部署 <code>main.py</code>、<code>config.py</code>、<code>app.yaml</code> 和应用图标。模型放在配置指定的路径。

首次启动前，先确认：

- 摄像头可读取 320×240 图像
- 屏幕显示接口正常
- 模型能单独加载
- Wi-Fi 能获得局域网 IP
- UART 设备没有被其他程序占用

## 5. UART 接线

| MaixCam | STM32 |
| --- | --- |
| TX | PA3 / USART2_RX |
| RX | PA2 / USART2_TX |
| GND | GND |

双方均使用 9600 baud、8N1。当前主要数据方向是 MaixCam TX 到 STM32 RX，但建议保留双向连接。

## 6. 目标类别

当前代码假设模型类别：

| class_id | 目标 |
| ---: | --- |
| 15 | CAT |
| 16 | DOG |

这与当前使用的 COCO 类别映射有关。更换自定义模型后，必须同步修改类别编号，不能只替换模型路径。

## 7. 状态判断

检测到猫或狗后，程序根据检测框做启发式判断：

~~~text
宽高比 > 1.2
  ├── 框中心 Y > 120 → EATING
  └── 否则          → RESTING

宽高比 ≤ 1.2        → WAITING
~~~

这不是独立的行为识别模型。相机角度、宠物体型、遮挡和食盆位置都会影响结果。

部署时应固定相机，并用真实场景重新验证阈值：

- <code>conf_th=0.5</code>
- <code>iou_th=0.45</code>
- 宽高比阈值 1.2
- 中心高度阈值 120

## 8. UART 状态协议

MaixCam 每秒最多发送一行：

~~~text
<TARGET>,<STATUS>\r\n
~~~

目标：

| TARGET | 含义 |
| --- | --- |
| CAT | 猫 |
| DOG | 狗 |
| NONE | 没有目标 |

状态：

| STATUS | 含义 |
| --- | --- |
| EATING | 判断为进食 |
| RESTING | 判断为休息 |
| WAITING | 判断为等待或站立 |
| NONE | 没有目标 |

示例：

~~~text
CAT,EATING
DOG,WAITING
NONE,NONE
~~~

STM32 使用 128 字节缓冲区，在收到回车或换行后解析一帧。

## 9. 先做串口独立测试

### 测试 MaixCam 输出

将 MaixCam TX 连接 USB 转串口，打开 9600 baud 终端。控制台和串口每秒应出现一次状态。

### 测试 STM32 输入

暂时用 USB 转串口连接 STM32 PA3，依次发送：

~~~text
CAT,EATING\r\n
CAT,EATING\r\n
NONE,NONE\r\n
~~~

确认 STM32 能开始和结束一次进食事件。这样可以在没有模型和摄像头的情况下先验证协议。

### 合并测试

确认两端单独正常后再直连，避免同时排查模型、UART 和 STM32 三个问题。

## 10. 进食事件如何产生

STM32 第一次收到 <code>EATING</code>：

1. 记录开始毫秒数
2. 记录开始重量
3. 蜂鸣器响一次

之后收到 <code>NONE</code>：

1. 计算持续时间
2. 计算起始重量减当前重量
3. 负值限制为 0
4. 持续至少 5 秒时生成记录
5. 向 OneNET 上报 <code>eat_time</code>、<code>eat_weight</code>、<code>eat_cnt</code>

源码注释中仍有“大于 10 秒”的旧描述，但实际判断阈值是 5 秒。验证时应以实际条件表达式为准。

## 11. WebSocket 视频流

视觉程序将偶数帧压缩为质量 30 的 JPEG，再编码成 Base64。服务端发送：

~~~json
{
  "image": "BASE64_ENCODED_JPEG",
  "pet": "CAT (EATING)"
}
~~~

默认监听：

~~~text
ws://<MaixCam局域网IP>:8765
~~~

程序把检测和 WebSocket 服务放在同一个异步事件循环中，并将循环频率限制在约 10 FPS。

## 12. 运行观察

MaixCam 控制台应依次显示：

1. Wi-Fi 连接结果
2. 当前局域网 IP
3. 小程序 WebSocket 地址
4. UART 初始化结果
5. 模型加载
6. WebSocket 服务就绪
7. 每秒发送的 UART 状态

先确认这些日志，再打开小程序。

## 13. 当前实现的边界

- 进食判断只是检测框比例和位置启发式
- 相机位置改变后阈值可能失效
- 当前只处理第一只符合条件的猫或狗
- WebSocket 状态文本在检测分支中使用 CAT 前缀，DOG 画面标签需要后续修正
- 模型文件和许可证需要使用者自行准备
- Base64 JPEG 带宽和内存开销较大
- WebSocket 没有鉴权
- 默认使用未加密的 <code>ws://</code>
- 视觉循环异常时只记录并重试，没有持久化故障状态

## 14. 常见问题

### 模型无法加载

- 模型路径错误
- 模型格式与系统版本不兼容
- 内存不足
- MaixPy API 版本不匹配

### 能显示画面但识别不到宠物

- 类别编号不匹配
- 光线不足
- 宠物距离或角度不合适
- 置信度阈值过高
- 模型本身不适合当前场景

### MaixCam 有日志但 STM32 收不到

- TX/RX 没有交叉
- 没有共地
- 波特率不一致
- 串口设备路径错误
- USART2 溢出或帧错误

### 状态频繁跳变

- 宠物被遮挡
- 检测框不稳定
- 阈值不适合当前机位
- 缺少连续多帧确认和状态防抖

### 视频连接后很快断开

- 手机和 MaixCam 不在同一局域网
- IP 地址变化
- 内存压力过大
- WebSocket 库版本接口不同

## 15. 本章完成标准

- [ ] MaixCam 能连接 Wi-Fi 并显示 IP
- [ ] 模型能稳定加载
- [ ] 猫或狗能产生正确 TARGET
- [ ] UART 每秒发送一帧且格式正确
- [ ] STM32 能解析模拟帧和真实帧
- [ ] EATING 到 NONE 能生成健康记录
- [ ] WebSocket 客户端能收到 JSON 和图像
- [ ] 已用真实机位验证状态阈值
- [ ] 已记录模型来源和许可证

上一章：[第 6 章：ESP-01S 与 OneNET](06-ESP01S与OneNET.md)

下一章：[第 8 章：微信小程序](08-微信小程序.md)

返回：[教程目录](README.md)
