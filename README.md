# TableQCR

## 简介

成绩单自动识别系统，实现了图像校正功能，接入腾讯和百度的OCR表格识别服务进行成绩单识别，对手写成绩进行本地的数字识别优化。

**开发环境：**

- Windows 11
- Visual Studio 2022

**依赖：**

- [Qt 5.15.2](https://www.qt.io/)：程序界面及一些常用接口
- [OpenCV](https://opencv.org/)：图像处理部分完全基于 OpenCV 实现
- [curl](https://github.com/curl/curl)：用于HTTP请求
- [json](https://github.com/nlohmann/json)：主要用于处理 OCR 服务商返回的数据
- [boost](https://www.boost.org/)：线程池，用于多线程数字识别
- [frugally-deep](https://github.com/Dobiasd/frugally-deep)：用于加载数字识别的 Keras 模型
- [spdlog](https://github.com/gabime/spdlog)：日志框架
- [openssl](https://github.com/openssl/openssl)：腾讯 OCR 服务加密相关

推荐使用 [vcpkg](https://github.com/Microsoft/vcpkg) 安装相关库：

```shell
vcpkg install curl frugally-deep nlohmann-json openssl spdlog --triplet x64-windows
```

## 演示

![optimize](optimize.gif)

**注：测试图像中的姓名班级均为随机生成！**
