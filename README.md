# Simple STM32 Oscilloscope (TFT)

一个为STM32微控制器设计的TFT显示屏驱动程序库，用于快速集成TFT液晶显示屏功能。

## 📋 项目概述

本项目提供了STM32平台下的TFT显示屏完整解决方案，包括初始化、通信协议、图形绘制等核心功能。

### 主要特性

- ✅ 支持多种STM32型号
- ✅ 高效的SPI/并行接口驱动
- ✅ 基础图形绘制函数（像素、直线、矩形、圆形等）
- ✅ 字符和文字显示功能
- ✅ 低功耗设计
- ✅ 易于集成和扩展

## 🛠️ 技术栈

- **主要语言**: C (95.5%)
- **汇编**: Assembly (4.5%)
- **目标平台**: STM32系列微控制器
- **开发工具**: STM32CubeMX, Keil uVision 或 STM32CubeIDE

## 📁 项目结构

```
-stm32-tft-/
├── src/              # 源代码文件
├── inc/              # 头文件
├── drivers/          # TFT驱动程序
├── examples/         # 使用示例
└── docs/             # 文档
```

## 🚀 快速开始

### 硬件要求

- STM32开发板（如STM32F4、STM32H7等���
- TFT LCD显示屏
- 必要的连接器和跳线

### 软件配置

1. **克隆仓库**
   ```bash
   git clone https://github.com/ANTY-J0/-stm32-tft-.git
   ```

2. **配置开发环境**
   - 安装 STM32CubeIDE 或 Keil uVision
   - 导入项目文件

3. **编译和烧录**
   - 编译项目
   - 将固件烧录到STM32开发板

## 📚 使用示例

```c
// 初始化TFT显示屏
TFT_Init();

// 填充屏幕为红色
TFT_FillScreen(RED);

// 绘制像素
TFT_DrawPixel(100, 100, WHITE);

// 绘制直线
TFT_DrawLine(0, 0, 320, 240, BLUE);

// 显示文字
TFT_ShowString(50, 50, "Hello STM32!", WHITE);
```

## 🔧 API 参考

### 初始化函数
- `void TFT_Init(void)` - 初始化TFT显示屏

### 显示控制
- `void TFT_FillScreen(uint16_t color)` - 填充整个屏幕
- `void TFT_Clear(void)` - 清屏

### 图形绘制
- `void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color)` - 绘制像素
- `void TFT_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)` - 绘制直线
- `void TFT_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)` - 绘制矩形
- `void TFT_DrawCircle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)` - 绘制圆形

### 文字显示
- `void TFT_ShowString(uint16_t x, uint16_t y, char *str, uint16_t color)` - 显示字符串

## 🤝 贡献

欢迎提交Issue和Pull Request来改进这个项目！

### 贡献步骤

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📝 许可证

本项目采用 [MIT License](LICENSE) 许可证。

## 📮 联系方式

- GitHub: [@ANTY-J0](https://github.com/ANTY-J0)
- 如有问题，请通过 Issues 页面提出

## 📖 参考资源

- [STM32 官方文档](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html)
- [TFT LCD 驱动规格书](#)
- [STM32CubeIDE 用户指南](https://www.st.com/en/development-tools/stm32cubeide.html)

---

**开发状态**: 🚧 活跃开发中

**最后更新**: 2026-06-05
