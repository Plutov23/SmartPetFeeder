# 第 6 课：让 OLED 显示第一行文字

## 这一课要完成什么

让 OLED 屏幕显示：

~~~text
HELLO
~~~

这一课只测试 OLED，不运行其他传感器。

## 需要准备

- STM32 开发板
- SSD1306 128×64 OLED
- 杜邦线
- ST-Link
- USART1 调试串口

## 接线

| OLED | STM32 |
| --- | --- |
| SCL | PB13 |
| SDA | PB14 |
| GND | GND |
| VCC | 按模块规格供电 |

本项目使用软件控制 PB13 和 PB14。OLED 不与 AHT20 共用引脚。

## 什么是 SCL 和 SDA

- SCL：时钟线
- SDA：数据线

可以暂时理解成：

- SCL 负责告诉双方什么时候读数据
- SDA 负责传输真正的数据

## 添加临时测试代码

打开 <code>Core/Src/main.c</code>。

确认文件顶部已经包含：

~~~c
#include "OLED.h"
~~~

在 <code>USER CODE BEGIN 2</code> 后临时加入：

~~~c
OLED_Init();
OLED_Clear();
OLED_ShowString(36, 24, "HELLO", OLED_8X16);
OLED_Update();

while (1)
{
    HAL_Delay(100);
}
~~~

这个 <code>while (1)</code> 会让程序停在 OLED 测试中，避免继续初始化其他尚未连接的模块。

## 编译、烧录和复位

1. 按 F7。
2. 确认 0 Error。
3. 下载程序。
4. 按复位键。

## 正确现象

屏幕中间附近显示：

~~~text
HELLO
~~~

文字位置稍有不同不影响实验结果。

## OLED 完全不亮

依次检查：

1. VCC 和 GND
2. PB13 是否连接 SCL
3. PB14 是否连接 SDA
4. 模块是否为兼容的 SSD1306
5. 程序是否重新烧录

更改接线前先断电。

## 屏幕亮但没有文字

- 是否调用 <code>OLED_Update()</code>
- 字符是否使用当前字库支持的英文和数字
- 是否先调用 <code>OLED_Init()</code>
- 文字坐标是否超出屏幕

## 屏幕花屏

- 供电不稳定
- 杜邦线接触不良
- SCL 和 SDA 接反
- 接线太长或受到电机干扰

这一课不要连接电机。

## 想进一步了解

<code>OLED_Clear()</code> 只清空内存中的画面。

<code>OLED_Update()</code> 才会把准备好的画面发送到屏幕。

所以修改内容后通常需要调用 <code>OLED_Update()</code>。

## 实验完成后

保存一张 OLED 显示 HELLO 的照片。

删除本课加入的测试代码和临时 <code>while (1)</code>，再继续下一课。

## 这一课的完成标准

- [ ] OLED 接线正确
- [ ] 屏幕显示 HELLO
- [ ] 知道 SCL 和 SDA 的基本作用
- [ ] 知道更新画面需要调用 OLED_Update
- [ ] 已删除临时测试代码

上一课：[让 STM32 向电脑发送文字](05-让STM32发送文字.md)

下一课：[读取温度和湿度](07-读取温湿度.md)

返回：[零基础教程目录](README.md)
