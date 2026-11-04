# OneNET 配置

## 设备信息

在 OneNET 创建产品和设备后，需要准备：

- Product ID
- Device Name
- 设备 MQTT Token
- 小程序 HTTPS API Authorization

设备 MQTT Token 和小程序 API Authorization 用途不同，不要混用。

## 物模型

按照 `protocol.md` 创建以下属性：

```text
tem, wet, wei
eat_time, eat_weight, eat_cnt
feed_amount, feed_cmd
sch_id, sch_h, sch_m, sch_w, sch_en
```

属性的访问权限应与用途对应：传感数据和健康记录允许设备上报，喂食命令和计划参数允许应用下发。

## STM32 配置

复制 `app_config.example.h` 为 `app_config.h`，再将设备侧参数写入：

```text
firmware/stm32f103/Core/Inc/app_config.h
```

ESP-01S 使用 AT MQTT 指令连接 OneNET，并订阅：

```text
$sys/<PRODUCT_ID>/<DEVICE_NAME>/thing/property/set
```

设备数据上报到：

```text
$sys/<PRODUCT_ID>/<DEVICE_NAME>/thing/property/post
```

## 小程序配置

复制 `config.example.js` 为 `config.js`，再将 HTTPS API 参数写入：

```text
miniapp/wechat/config.js
```

正式公开演示时，不建议把长期有效的 OneNET Authorization 直接放在小程序前端。更安全的实现是增加自己的后端服务，由后端保存平台凭据并为小程序提供受限接口。
