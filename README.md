# Tray Neo

[中文说明](README.zh-CN.md)

A modern TrafficMonitor plugin that displays real-time system metrics in the taskbar tray area and provides an interactive history chart popup.

![Preview](docs/screenshot.png)

## Features

- **Real-time monitoring**: CPU, GPU, Memory, HDD, Network (upload/download)
- **Interactive history chart**: Hover to see exact values at specific points in time
- **Smart tooltip**: Three adaptive styles (small, medium, linked), defaults to the left of the cursor
- **Geist UI color scheme**: Light/dark themes based on the Vercel Geist design system
- **Gradient borders**: Each chart's rounded border uses the metric's accent color, fading from top to transparent
- **Smart downsampling**: When history exceeds 2.5 minutes, uses LTTB algorithm to downsample to 150 points, preserving peaks and valleys
- **Customizable thresholds**: Configure warning/critical levels for each metric
- **Toggle display**: Click to show/hide the history chart

## Installation

1. Download the latest release from [Releases](https://github.com/tiger2005/tray-neo/releases)
2. Extract `TrayNeo.dll` to your TrafficMonitor plugins directory:
   - Usually `TrafficMonitor\plugins\`
3. Restart TrafficMonitor
4. Enable the plugin in TrafficMonitor settings

## Configuration

Right-click the plugin area and select "Tray Neo Settings":

| Setting | Description | Default |
|---------|-------------|---------|
| History Duration | How long to keep history data (30 sec ~ 1 hour) | 2 minutes |
| Replace MAX with value in taskbar | Show actual value instead of "MAX" when at 100% | Off |
| Linked tooltip in chart | Show all metrics when hovering any chart | Off |

> **Note**: When selecting 5 minutes or longer history duration, the chart is downsampled to 150 data points (LTTB algorithm), so chart accuracy is not guaranteed.

### Thresholds

Configure when tray text changes color:

- **Warning threshold**: Text turns orange
- **Critical threshold**: Text turns red

| Metric | Warning | Critical |
|--------|---------|----------|
| CPU | 70% | 90% |
| GPU | 70% | 90% |
| Memory | 80% | 95% |
| HDD | 80% | 95% |
| Network | 1 MB/s | 5 MB/s |

## Screenshots

### Dark Mode

![Dark Mode](docs/dark-mode.png)

### Light Mode

![Light Mode](docs/light-mode.png)

### History Chart

![History Chart](docs/history-chart.png)

## Building from Source

### Requirements

- Visual Studio 2022 (or later)
- Windows SDK 10.0
- MFC (Static library)

### Build

```bash
# Clone the repository
git clone https://github.com/tiger2005/tray-neo.git
cd tray-neo

# Build using MSBuild
build.bat
```

The resulting `TrayNeo.dll` will be in `Bin/Release/plugins/` (Win32) or `Bin/x64/Release/plugins/` (x64).

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) - The monitoring framework
- [tray-pulsy](https://github.com/feelgooder/tray-pulsy) - Design inspiration
- [Geist UI](https://vercel.com/geist/introduction) - Color scheme
- [Geist Mono](https://vercel.com/font) - Font used in the plugin
