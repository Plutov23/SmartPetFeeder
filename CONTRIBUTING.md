# 参与贡献

## 开发流程

1. 从主分支创建功能分支。
2. 一个提交只处理一类问题。
3. 不要提交密码、Token、个人 AppID、局域网地址或设备私钥。
4. 修改通信字段时，同步更新 `docs/protocol.md`。
5. 修改接线或外设配置时，同步更新 `docs/hardware.md` 和 `hardware/bom.csv`。
6. 提交前完成对应端的编译或语法检查。

## 代码范围

- STM32 用户代码位于 `firmware/stm32f103/Core`。
- MaixCam 程序位于 `vision/maixcam`。
- 微信小程序位于 `miniapp/wechat`。
- 第三方驱动应保留原始版权和许可证声明。

## 提交信息

建议使用 `feat:`、`fix:`、`docs:`、`refactor:`、`test:`、`build:` 等前缀。
