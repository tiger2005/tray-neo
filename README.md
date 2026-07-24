# Tray Neo

[English](README.en-US.md)

一个为 [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) 打造的现代化双行任务栏系统监控面板。

Tray Neo 将 CPU、GPU、内存、硬盘以及网络上下行整合到一个紧凑的任务栏显示项中。平时可以直接查看实时状态；点击面板，即可展开带悬停提示的历史趋势图。

![Tray Neo 预览](docs/screenshot.png)

## 安装使用

1. 前往 [Releases](https://github.com/tiger2005/tray-neo/releases) 下载与 TrafficMonitor 位数一致的 DLL：
   - `TrayNeo-x64.dll`：用于 64 位 TrafficMonitor
   - `TrayNeo-Win32.dll`：用于 32 位 TrafficMonitor
2. 将下载的 DLL 复制到 TrafficMonitor 的 `plugins` 目录。
3. 重启 TrafficMonitor。
4. 右键 TrafficMonitor，进入 **选项 → 插件**，启用 **Tray Neo**。
5. 在任务栏显示设置中添加 Tray Neo 的显示项目。

> Release 中的 `.pdb` 文件是调试符号。普通用户安装时只需要对应的 DLL。

## 插件设置

右键点击 Tray Neo 的任务栏区域，在 **Tray Neo → 插件选项** 中进行设置。

### 常规选项

| 设置项 | 说明 | 默认值 |
|--------|------|--------|
| 历史记录时长 | 设置图表保留最近多长时间的数据 | 2 分钟 |
| 任务栏中将 100% 替换为 MAX | 百分比指标达到 100% 时显示 `MAX`，可缩短任务栏占用宽度 | 关闭 |
| 图表中联动显示所有数值 | 悬停任意图表时显示同一时间点的全部六项指标 | 关闭 |

可选历史时长：

`30 秒 / 1 分钟 / 2 分钟 / 5 分钟 / 10 分钟 / 20 分钟 / 30 分钟 / 1 小时`

### 颜色阈值

每项指标都可以分别设置警告和严重阈值：

- 达到警告阈值：文字变为橙色
- 达到严重阈值：文字变为红色
- 网络阈值的单位为 `MB/s`

| 指标 | 默认警告阈值 | 默认严重阈值 |
|------|--------------|--------------|
| CPU | 70% | 90% |
| GPU | 70% | 90% |
| MEM | 80% | 95% |
| HDD | 80% | 95% |
| NET↓ | 1 MB/s | 5 MB/s |
| NET↑ | 1 MB/s | 5 MB/s |

## 历史记录与降采样

Tray Neo 每秒记录一次六项指标，并使用环形缓冲区保存所选时间范围内的数据。

当历史时长不超过 2 分钟时，图表直接绘制逐秒采样点。选择 5 分钟及以上时，插件会使用 LTTB 算法将数据压缩到约 150 个显示点，以降低长曲线的绘制开销，同时尽量保留明显的峰值、谷值和趋势变化。

> 长时间范围下的图表主要用于观察趋势。降采样点是原始数据的代表值，不适合用于逐秒核对或精确流量统计；需要查看短时细节时，建议选择 30 秒、1 分钟或 2 分钟。

## 效果预览

### 深色模式

![Tray Neo 深色模式](docs/dark-mode.png)

### 浅色模式

![Tray Neo 浅色模式](docs/light-mode.png)

### 历史图表联动提示

![Tray Neo 历史图表联动提示](docs/history-chart.png)

## 从源码构建

### 环境要求

- Windows
- Visual Studio / MSBuild
- “使用 C++ 的桌面开发”工作负载
- MFC 静态库组件
- Windows 10 或更高版本的 Windows SDK

当前项目文件使用 `v145` 平台工具集。若本机安装的是其他工具集，请先在项目属性中调整 `Platform Toolset`。

### 一键构建

```bat
git clone https://github.com/tiger2005/tray-neo.git
cd tray-neo
build.bat
```

`build.bat` 会依次构建 Win32 和 x64 Release 版本，输出位置为：

```text
Win32: Bin\Release\plugins\TrayNeo.dll
x64:   Bin\x64\Release\plugins\TrayNeo.dll
```

创建 GitHub Release 时，工作流会将产物重命名为 `TrayNeo-Win32.dll` 和 `TrayNeo-x64.dll` 后上传。

## 许可证

本项目采用 [MIT License](LICENSE)。

Geist Mono 字体使用 SIL Open Font License，详见 [assets/OFL.txt](assets/OFL.txt)。

## 致谢

- [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor)：插件接口与系统监控数据
- [tray-pulsy](https://github.com/feelgooder/tray-pulsy)：界面与图表设计灵感
- [Geist](https://vercel.com/geist/introduction)：配色与视觉语言
- [Geist Mono](https://vercel.com/font)：任务栏面板与图表所使用的字体

如果遇到问题或有功能建议，欢迎在项目仓库提交 [Issue](https://github.com/tiger2005/tray-neo/issues)。
