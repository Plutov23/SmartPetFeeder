# 第 12 课：把数据发送到 OneNET

## 这一课要完成什么

让 STM32 通过 ESP-01S 把三个测试数字发送到 OneNET。

## OneNET 是什么

OneNET 是云平台。

它帮我们在设备和小程序之间保存、转发数据：

~~~text
智能喂食器 → OneNET → 微信小程序
微信小程序 → OneNET → 智能喂食器
~~~

## 需要准备的信息

在 OneNET 创建产品和设备后，需要：

- Product ID
- Device Name
- 设备 MQTT Token

设备 MQTT Token 用于设备联网。

后面小程序使用的 API Authorization 是另一种凭据，两者不要混用。

## 创建物模型属性

先创建三个属性：

| 标识符 | 含义 | 示例 |
| --- | --- | ---: |
| <code>tem</code> | 温度 | 25 |
| <code>wet</code> | 湿度 | 60 |
| <code>wei</code> | 重量 | 100 |

标识符必须完全一致，包括大小写。

## 填写设备配置

打开本地 <code>app_config.h</code>：

~~~c
#define ONENET_PRODUCT_ID "YOUR_PRODUCT_ID"
#define ONENET_DEVICE_NAME "YOUR_DEVICE_NAME"
#define ONENET_TOKEN "YOUR_DEVICE_TOKEN"
~~~

把占位文字替换为自己的设备信息。

不要把这个文件上传到公开仓库。

## 添加临时上报测试

在 <code>USER CODE BEGIN 2</code> 后加入：

~~~c
HAL_UART_Receive_IT(&huart3, &wifi_rx_char, 1);
ESP_Init();

while (1)
{
    ESP_Publish_AllData(25.0f, 60.0f, 100);
    HAL_Delay(5000);
}
~~~

这段代码每 5 秒上传一次固定数字。

## 正确现象

USART1 显示：

~~~text
[WIFI] Cloud Sync -> Temp:25 Humi:60 Weight:100
~~~

OneNET 控制台中：

- <code>tem</code> 显示 25
- <code>wet</code> 显示 60
- <code>wei</code> 显示 100

## 日志显示上传，但云端没有数据

“程序发送了命令”不等于“OneNET 已经接受”。

检查：

1. OneNET 设备是否在线
2. Product ID、Device Name、Token 是否属于同一设备
3. 物模型标识符是否完全一致
4. ESP-01S 固件是否支持 MQTT AT 命令
5. ESP 原始响应是否包含错误

## 云端出现数据后

把固定数字替换为真实传感器变量，就是完整工程的做法：

~~~c
ESP_Publish_AllData(temp, humi, weight);
~~~

## 安全提醒

Token 相当于设备密码。

不要把它放在：

- GitHub
- 教程截图
- 视频画面
- 公开 Issue
- 聊天群截图

泄露后应立即重新生成。

## 实验完成后

删除固定数字测试循环，恢复完整工程的 5 秒上报任务。

## 这一课的完成标准

- [ ] OneNET 产品和设备已创建
- [ ] 三个属性标识符正确
- [ ] 设备显示在线
- [ ] 云端每 5 秒收到测试数据
- [ ] 知道 Token 不能公开
- [ ] 已删除临时固定数据测试

上一课：[连接 Wi-Fi](11-连接WiFi.md)

下一课：[使用微信小程序](13-使用微信小程序.md)

返回：[零基础教程目录](README.md)
