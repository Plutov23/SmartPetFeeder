# 第 6 章：ESP-01S、MQTT 与 OneNET

## 本章目标

完成本章后，你应该能够：

- 单独验证 ESP-01S 的 AT 固件和网络连接
- 在 OneNET 创建物模型属性
- 配置设备端凭据
- 理解属性上报、属性下发和回复主题
- 完成温湿度、重量上报和远程出粮
- 区分设备 MQTT Token 与小程序 API Authorization

## 1. 网络链路

~~~text
STM32
  ↓ USART3 / AT 指令
ESP-01S
  ↓ 2.4 GHz Wi-Fi / MQTT
OneNET
  ↓ HTTPS API
微信小程序
~~~

STM32 不直接实现 TCP/IP，而是通过 USART3 控制 ESP-01S。ESP-01S 的 AT 固件必须支持项目使用的 MQTT 指令。

## 2. 硬件与串口

| ESP-01S | STM32 |
| --- | --- |
| RX | PB10 / USART3_TX |
| TX | PB11 / USART3_RX |
| GND | GND |
| VCC | 稳定 3.3 V |
| EN/CH_PD | 按模块要求拉高 |

USART3 参数：

~~~text
115200 baud
8 data bits
1 stop bit
No parity
No flow control
~~~

ESP-01S 发送时会产生较高瞬时电流。不要假设任意开发板 3.3 V 引脚都能稳定供电，应测量连接和发送瞬间的电压。

## 3. 先单独验证 AT 固件

先将 ESP-01S 连接 USB 转串口，不接 STM32：

1. 设置 115200、8N1。
2. 发送 <code>AT</code>，应返回 <code>OK</code>。
3. 发送 <code>AT+GMR</code>，记录固件版本。
4. 发送 <code>AT+CWMODE=1</code>。
5. 连接 2.4 GHz 热点。
6. 使用 <code>AT+CWJAP?</code> 检查当前热点。
7. 确认固件支持 <code>AT+MQTTUSERCFG</code>、<code>AT+MQTTCONN</code>、<code>AT+MQTTSUB</code> 和 <code>AT+MQTTPUBRAW</code>。

如果基础 AT 命令正常但 MQTT 命令不存在，应先更换或升级兼容的 AT 固件。

## 4. 创建 OneNET 产品和设备

在 OneNET 中创建产品与设备，准备：

- Product ID
- Device Name
- 设备 MQTT Token
- 小程序 HTTPS API Authorization

设备 MQTT Token 用于 ESP-01S 建立 MQTT 连接；API Authorization 用于小程序调用 HTTPS 接口。两者不要混用。

## 5. 建立物模型

属性标识符必须与源码完全一致：

| 标识符 | 方向 | 含义 |
| --- | --- | --- |
| <code>tem</code> | 设备上报 | 温度 |
| <code>wet</code> | 设备上报 | 湿度 |
| <code>wei</code> | 设备上报 | 当前重量 |
| <code>eat_time</code> | 设备上报 | 最近一次进食时长 |
| <code>eat_weight</code> | 设备上报 | 最近一次进食消耗重量 |
| <code>eat_cnt</code> | 设备上报 | 进食事件序号 |
| <code>feed_amount</code> | 应用下发 | 手动出粮克数 |
| <code>feed_cmd</code> | 应用下发 | 手动出粮触发值 |
| <code>sch_id</code> | 应用下发 | 计划编号 0 到 4 |
| <code>sch_h</code> | 应用下发 | 小时 |
| <code>sch_m</code> | 应用下发 | 分钟 |
| <code>sch_w</code> | 应用下发 | 出粮克数 |
| <code>sch_en</code> | 应用下发 | 计划开关 |

数据类型、读写权限和标识符任一不匹配，都会导致上报失败或下发无法解析。

## 6. 配置 STM32 本地凭据

复制：

~~~bash
cp app_config.example.h app_config.h
~~~

填写 <code>firmware/stm32f103/Core/Inc/app_config.h</code>：

~~~c
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define ONENET_PRODUCT_ID "YOUR_PRODUCT_ID"
#define ONENET_DEVICE_NAME "YOUR_DEVICE_NAME"
#define ONENET_TOKEN "YOUR_DEVICE_TOKEN"
~~~

该文件已被 Git 忽略。不要把真实值写入示例文件、教程、截图或 Issue。

## 7. ESP 初始化流程

当前 <code>ESP_Init()</code> 依次执行：

1. 设置 Station 模式
2. 连接配置的 Wi-Fi
3. 查询当前热点并检查 SSID
4. 配置 MQTT 用户信息
5. 配置 MQTT Client ID
6. 连接 OneNET
7. 订阅属性下发主题

连接期间包含多次固定延时，启动可能需要十几秒。串口日志中应出现热点连接结果和 MQTT 初始化完成提示。

## 8. MQTT 主题

设备上报：

~~~text
$sys/<PRODUCT_ID>/<DEVICE_NAME>/thing/property/post
~~~

接收属性设置：

~~~text
$sys/<PRODUCT_ID>/<DEVICE_NAME>/thing/property/set
~~~

回复属性设置：

~~~text
$sys/<PRODUCT_ID>/<DEVICE_NAME>/thing/property/set_reply
~~~

上报温湿度和重量的 JSON 结构：

~~~json
{
  "id": "123",
  "version": "1.0",
  "params": {
    "tem": {"value": 25},
    "wet": {"value": 60},
    "wei": {"value": 320}
  }
}
~~~

主循环每 5 秒上报一次。

## 9. 验证上报

1. 烧录配置后的固件。
2. 打开 USART1 日志。
3. 等待 Wi-Fi 与 MQTT 初始化。
4. 在 OneNET 控制台确认设备在线。
5. 查看 <code>tem</code>、<code>wet</code>、<code>wei</code>。
6. 改变温湿度或重量，确认云端数值更新。

预期日志：

~~~text
[WIFI] Connecting to configured access point...
[WIFI] [REAL] Hotspot connected successfully!
[WIFI] MQTT Init steps finished.
[WIFI] Cloud Sync -> Temp:... Humi:... Weight:...
~~~

日志中的 MQTT 初始化完成只表示命令步骤已经发送，当前实现没有完整校验服务器连接结果。设备在线状态仍应以 OneNET 控制台和 AT 返回为准。

## 10. 验证远程手动出粮

小程序向 OneNET 设置：

~~~json
{
  "feed_amount": 20,
  "feed_cmd": 1
}
~~~

设备收到后：

1. 解析 <code>feed_amount</code>
2. 蜂鸣器提示
3. 计算电机步数
4. 启动步进电机
5. 回复属性设置结果

先断开真实粮仓，用小步数或测试出粮量验证方向，再进行带粮测试。

## 11. 验证定时计划

计划下发字段：

~~~json
{
  "sch_id": 0,
  "sch_h": 8,
  "sch_m": 30,
  "sch_w": 20,
  "sch_en": 1
}
~~~

STM32 最多保存 5 个计划。计划当前只保存在 RAM 中，主控复位后会丢失，需要小程序重新同步。

## 12. 安全建议

- 不要提交 Wi-Fi 密码和 Token
- 泄露过的凭据应立即吊销并重新生成
- 不要在公开截图中展示完整设备信息
- 不建议在小程序前端长期保存高权限 Authorization
- 正式应用应增加自己的后端，保存平台凭据并校验用户权限
- 公网通信应使用平台当前支持的加密方式
- 对出粮克数、计划时间和计划编号做服务端与设备端双重校验

## 13. 当前实现的边界

- Wi-Fi 和 MQTT 初始化使用固定阻塞延时
- 只检查 Wi-Fi SSID，没有严格验证 MQTT 连接结果
- 下发 JSON 使用字符串搜索和 <code>atoi</code>，不是完整 JSON 解析
- USART3 以换行作为一帧结束，复杂响应可能需要更稳健的状态机
- MQTT 端口模板当前为 1883
- 计划保存在 RAM 中
- 命令参数缺少完整范围限制

这些限制适合课程原型，但不适合直接用于无人看管的商业设备。

## 14. 常见问题

### ESP-01S 反复重启

- 3.3 V 电源电流不足
- 发送瞬间电压跌落
- EN/CH_PD 没有稳定拉高
- 接地不可靠

### 连不上 Wi-Fi

- 热点不是 2.4 GHz
- SSID 或密码错误
- 路由器加密方式不兼容
- 模块天线环境差

### OneNET 设备不在线

- Product ID、Device Name、Token 不属于同一设备
- MQTT AT 固件不兼容
- 主机名或端口错误
- 平台 Token 已过期

### 云端在线但没有数据

- 物模型标识符或类型不一致
- 上报主题错误
- JSON 长度与实际负载不一致
- ESP 返回错误但日志未完整保留

### 下发成功但电机不动

- 设备没有订阅属性设置主题
- ESP 返回内容没有形成完整接收帧
- 字段名称不一致
- 出粮量为 0 或解析失败
- 电机驱动硬件仍有问题

## 15. 本章完成标准

- [ ] ESP-01S 单独 AT 测试通过
- [ ] 能连接 2.4 GHz Wi-Fi
- [ ] OneNET 设备显示在线
- [ ] 三项传感数据能持续更新
- [ ] 手动出粮命令能触发电机
- [ ] 计划下发后 USART1 有更新日志
- [ ] 属性设置回复正常
- [ ] 仓库中没有真实凭据
- [ ] 理解设备 Token 与 API Authorization 的区别

上一章：[第 5 章：显示、传感器与实时时钟](05-显示传感器与实时时钟.md)

下一章：[第 7 章：MaixCam 视觉识别](07-MaixCam视觉识别.md)

返回：[教程目录](README.md)
