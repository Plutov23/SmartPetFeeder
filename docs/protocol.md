# 通信协议

## MaixCam 与 STM32

接口：USART2，9600 baud，8N1。

每秒最多发送一行 ASCII 文本：

```text
<TARGET>,<STATUS>\r\n
```

目标值：

| TARGET | 含义 |
| --- | --- |
| `CAT` | 检测到猫 |
| `DOG` | 检测到狗 |
| `NONE` | 没有目标 |

状态值：

| STATUS | 含义 |
| --- | --- |
| `EATING` | 判断为进食 |
| `RESTING` | 判断为休息 |
| `WAITING` | 判断为等待或站立 |
| `NONE` | 没有目标 |

示例：

```text
CAT,EATING
DOG,WAITING
NONE,NONE
```

STM32 接收缓冲区为 128 字节，遇到 `\r` 或 `\n` 后处理一帧。

## OneNET 属性

### STM32 上报

| 标识符 | 类型 | 含义 |
| --- | --- | --- |
| `tem` | number | 温度 |
| `wet` | number | 湿度 |
| `wei` | integer | 当前重量 |
| `eat_time` | integer | 最近一次进食时长，秒 |
| `eat_weight` | integer | 最近一次进食消耗重量，克 |
| `eat_cnt` | integer | 进食事件序号 |

### 小程序下发

| 标识符 | 类型 | 含义 |
| --- | --- | --- |
| `feed_amount` | integer | 手动出粮重量，克 |
| `feed_cmd` | integer | 手动出粮触发值 |
| `sch_id` | integer | 计划编号，0 到 4 |
| `sch_h` | integer | 小时，0 到 23 |
| `sch_m` | integer | 分钟，0 到 59 |
| `sch_w` | integer | 计划出粮重量，克 |
| `sch_en` | integer | 计划开关，0 或 1 |

## MaixCam 与小程序

MaixCam 在配置端口启动 WebSocket 服务，服务端向客户端发送 JSON：

```json
{
  "image": "BASE64_ENCODED_JPEG",
  "pet": "CAT (EATING)"
}
```

`image` 是 JPEG 的 Base64 数据，不包含 Data URL 前缀。小程序接收后添加 `data:image/jpeg;base64,` 前缀用于显示。
