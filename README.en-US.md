# Tray Neo

[中文说明](README.md)

A modern two-line taskbar system monitor for [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor).

Tray Neo combines CPU, GPU, memory, disk, download, and upload activity into one compact taskbar item. Check live values at a glance, then click the panel whenever you need an interactive view of recent trends.

![Tray Neo preview](docs/screenshot.png)

## Installation

1. Open [Releases](https://github.com/tiger2005/tray-neo/releases) and download the DLL that matches your TrafficMonitor architecture:
   - `TrayNeo-x64.dll` for 64-bit TrafficMonitor
   - `TrayNeo-Win32.dll` for 32-bit TrafficMonitor
2. Copy the DLL into TrafficMonitor's `plugins` directory.
3. Restart TrafficMonitor.
4. Right-click TrafficMonitor, open **Options → Plugins**, and enable **Tray Neo**.
5. Add the Tray Neo item in the taskbar display settings.

> The `.pdb` files attached to a release are debugging symbols. Regular users only need the matching DLL.

## Plugin Settings

Right-click the Tray Neo taskbar area and open **Tray Neo → Plugin Options**.

### General Options

| Setting | Description | Default |
|---------|-------------|---------|
| History duration | Controls how much recent data is kept for the charts | 2 minutes |
| Replace 100% with MAX in taskbar | Displays `MAX` when a percentage metric reaches 100%, reducing the required panel width | Off |
| Show all metrics linked in chart | Shows all six metrics for the same point when any chart is hovered | Off |

Available history ranges:

`30 seconds / 1 minute / 2 minutes / 5 minutes / 10 minutes / 20 minutes / 30 minutes / 1 hour`

### Color Thresholds

Warning and critical thresholds can be configured independently for every metric:

- At the warning threshold, the taskbar value turns orange
- At the critical threshold, the taskbar value turns red
- Network thresholds are entered in `MB/s`

| Metric | Default warning | Default critical |
|--------|-----------------|------------------|
| CPU | 70% | 90% |
| GPU | 70% | 90% |
| MEM | 80% | 95% |
| HDD | 80% | 95% |
| NET↓ | 1 MB/s | 5 MB/s |
| NET↑ | 1 MB/s | 5 MB/s |

## History and Downsampling

Tray Neo records all six metrics once per second and keeps them in ring buffers sized for the selected history range.

For ranges up to 2 minutes, the charts draw the per-second samples directly. At 5 minutes or longer, the plugin applies LTTB downsampling and limits the display to roughly 150 representative points. This reduces the cost of drawing long curves while preserving prominent peaks, valleys, and changes in shape as much as possible.

> Long-range charts are intended for trend inspection. Downsampled points represent the original samples and should not be used for second-by-second verification or precise traffic accounting. Choose 30 seconds, 1 minute, or 2 minutes when fine-grained detail matters.

## Screenshots

### Dark Mode

![Tray Neo dark mode](docs/dark-mode.png)

### Light Mode

![Tray Neo light mode](docs/light-mode.png)

### Linked History Tooltip

![Tray Neo linked history tooltip](docs/history-chart.png)

## Building from Source

### Requirements

- Windows
- Visual Studio / MSBuild
- The **Desktop development with C++** workload
- MFC static library components
- Windows 10 SDK or later

The current project file targets the `v145` platform toolset. If your installation provides a different toolset, update **Platform Toolset** in the project properties before building.

### One-command Build

```bat
git clone https://github.com/tiger2005/tray-neo.git
cd tray-neo
build.bat
```

`build.bat` builds both Win32 and x64 Release configurations. The output files are placed at:

```text
Win32: Bin\Release\plugins\TrayNeo.dll
x64:   Bin\x64\Release\plugins\TrayNeo.dll
```

When a GitHub Release is created, the workflow renames these artifacts to `TrayNeo-Win32.dll` and `TrayNeo-x64.dll` before uploading them.

## License

Tray Neo is released under the [MIT License](LICENSE).

Geist Mono is distributed under the SIL Open Font License; see [assets/OFL.txt](assets/OFL.txt).

## Acknowledgments

- [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) for the plugin API and monitoring data
- [tray-pulsy](https://github.com/feelgooder/tray-pulsy) for interface and chart inspiration
- [Geist](https://vercel.com/geist/introduction) for the color and visual language
- [Geist Mono](https://vercel.com/font) for the typeface used by the panel and charts

Bug reports and feature suggestions are welcome in [Issues](https://github.com/tiger2005/tray-neo/issues).
