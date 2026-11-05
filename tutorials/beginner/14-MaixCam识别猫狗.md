# 第 14 课：使用 MaixCam 识别猫狗

## 这一课要完成什么

让 MaixCam：

- 显示摄像头画面
- 检测猫或狗
- 每秒向 STM32 发送一次状态
- 向小程序提供局域网视频

## MaixCam 是什么

MaixCam 是带摄像头的开发板。

它比 STM32 更适合处理图像和运行目标检测模型。

## 准备文件

视觉程序位于：

~~~text
vision/maixcam
~~~

复制：

~~~text
config.example.py → config.py
~~~

填写：

- Wi-Fi 名称
- Wi-Fi 密码
- 串口设备路径
- 模型路径
- WebSocket 端口

<code>config.py</code> 不应提交到 Git。

## 准备模型

项目代码使用 YOLOv5 模型。

模型文件没有打包在仓库中，需要自行准备与 MaixPy 版本兼容的模型，并确认许可证允许使用。

当前代码假设：

- 类别 15 是猫
- 类别 16 是狗

更换模型后，类别编号也可能改变。

## UART 接线

| MaixCam | STM32 |
| --- | --- |
| TX | PA3 / USART2_RX |
| RX | PA2 / USART2_TX |
| GND | GND |

双方波特率都是 9600。

## 先运行 MaixCam

部署 <code>main.py</code> 和本地配置。

启动后观察控制台。

## 正确现象

控制台依次显示：

- Wi-Fi 连接成功
- MaixCam 的局域网 IP
- 串口打开成功
- 模型加载
- WebSocket 服务启动
- 每秒发送一次状态

状态示例：

~~~text
CAT,EATING
DOG,WAITING
NONE,NONE
~~~

## 先模拟，再连接真实摄像头

如果 STM32 收不到状态，可以先使用 USB 转串口向 PA3 发送：

~~~text
CAT,EATING
NONE,NONE
~~~

模拟成功，说明 STM32 解析基本正常。此时再检查 MaixCam 串口和接线。

## 小程序视频地址

MaixCam 控制台会显示类似：

~~~text
ws://192.168.1.100:8765
~~~

把实际地址填写到小程序 <code>config.js</code>。

手机、电脑和 MaixCam 需要位于能够互相访问的同一局域网。

## 当前状态判断并不是真正理解行为

程序根据检测框的宽高比例和位置，估计：

- EATING
- RESTING
- WAITING

这是一种简单规则，不是专门训练的宠物行为模型。

相机位置、光线和遮挡都会影响结果。

## 识别不到宠物

检查：

1. 模型是否成功加载
2. 类别编号是否正确
3. 光线是否足够
4. 宠物是否太远或被遮挡
5. 置信度阈值是否过高

## STM32 收不到状态

- TX 和 RX 没有交叉
- 没有共地
- 波特率不是 9600
- 串口设备路径错误
- USART2 出现溢出

## 这一课的完成标准

- [ ] MaixCam 能显示画面
- [ ] 模型能检测猫或狗
- [ ] 每秒输出一条状态
- [ ] STM32 能接收状态
- [ ] 小程序能连接 WebSocket
- [ ] 知道当前行为判断只是简单规则
- [ ] Wi-Fi 密码没有进入 Git

上一课：[使用微信小程序](13-使用微信小程序.md)

下一课：[组合完整系统](15-组合完整系统.md)

返回：[零基础教程目录](README.md)
