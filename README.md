# zlgcan

zlgcan plugin for Qt

## 安装

1. 从致远电子官网下载驱动和API库

    [致远电子](https://www.zlg.cn/)

    [资料下载页](https://www.zlg.cn/can/down/down/id/22.html)

    [驱动](https://manual.zlg.cn/web/#/146)

    [API DLL](https://www.zlg.cn/data/upload/software/Can/CAN_lib.rar)

2. 安装驱动

3. 解压 API DLL，根据平台选择 zlgcan 版本，注意不要选 ControlCAN，将解压后的 DLL 与 APP 放在同一个文件夹下

4. 从 Release 页下载本 Qt 插件，放在 Qt 插件目录（如 C:\APP\Qt\5.15.2\msvc2019_64\plugins\canbus）

5. 重新启动 APP

## 编译

1. 用 Qt 打开 CMake 工程编译

