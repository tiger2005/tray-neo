#pragma once
#include "pch.h"
#include "PluginInterface.h"
#include <ctime>
#include <vector>
#include <functional>

namespace Gdiplus { class Graphics; }

// 历史折线图弹窗
// 参照 tray-pulsy SparklineChart 设计风格
// 点击插件显示区域时弹出，展示 CPU、GPU、内存、HDD 的历史占用率折线图
class CHistoryChartWnd : public CWnd
{
    DECLARE_DYNAMIC(CHistoryChartWnd)
public:
    CHistoryChartWnd();
    ~CHistoryChartWnd();

    using DestroyCallback = std::function<void(CHistoryChartWnd*)>;
    void SetDestroyCallback(DestroyCallback callback) { m_destroy_callback = std::move(callback); }

    // 设置初始数据（6 个系列：CPU、GPU、MEM、HDD、NET↓、NET↑）
    void SetData(const std::vector<double>& cpu,
                 const std::vector<double>& gpu,
                 const std::vector<double>& mem,
                 const std::vector<double>& hdd,
                 const std::vector<double>& net_down,
                 const std::vector<double>& net_up,
                 bool dark_mode, int dpi,
                 ITrafficMonitor* app);

    // 创建弹窗（hTaskbar 为任务栏窗口句柄，用于确定任务栏上边沿；plugin_center_x 为插件中心的屏幕 x 坐标）
    bool CreatePopup(HWND hTaskbar, int plugin_center_x, CWnd* pParent);

    // 设置 tooltip 联动模式：true=显示所有指标，false=仅显示当前悬停图表的指标
    void SetLinkedTooltip(bool linked) { m_linked_tooltip = linked; }

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPaint();
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    virtual void PostNcDestroy() override;

private:
    struct Series
    {
        std::vector<double> values;
        CString label;
        COLORREF color;
        bool is_percent;   // true=百分比(固定0-100), false=速度(动态范围)
    };
    std::vector<Series> m_series;
    bool m_dark_mode;
    int m_dpi;
    ITrafficMonitor* m_app;
    std::vector<std::vector<double>> m_history;  // 6个系列的历史环形缓冲区
    int m_history_capacity;
    int m_history_count;
    int m_history_index;

    // 增量式降采样：当历史超过 150 秒时启用
    static const int DISPLAY_BUFFER_SIZE = 150;
    std::vector<double> m_display[6];     // 显示用循环队列（每组代表值）
    int m_display_index{ 0 };             // 写入位置
    int m_display_count{ 0 };             //有效数量
    std::vector<double> m_collect[6];     // 当前组的收集缓冲区
    int m_tick_size{ 1 };                 // 每组采样数
    bool m_use_downsample{ false };       // 是否启用增量降采样
    double m_latest_sample[6]{};          // 最近一次原始采样值
    UINT_PTR m_timer_id;
    DestroyCallback m_destroy_callback{ nullptr };
    CPoint m_mouse_pos;
    int m_hover_index{ -1 };
    int m_hover_chart{ -1 };          // 当前悬停的图表索引 (0..4)
    bool m_linked_tooltip{ true };    // true=联动显示所有指标，false=仅当前图表
    time_t m_last_sample_time{ 0 };   // 最近一次采样的时间戳（秒）
    HWND m_parent_hwnd{ nullptr };   // 父窗口句柄（用于 OnKillFocus 判断是否点回任务栏）

    // DPI 缩放
    int Scale(int v) const { return MulDiv(v, m_dpi, 96); }

    // Fire 主题配色
    COLORREF GetColorForValue(double value) const;
    static COLORREF LerpColor(COLORREF c1, COLORREF c2, float t);
    COLORREF BlendWithBg(COLORREF color, float alpha) const;
    COLORREF GetBgColor() const;
    COLORREF GetTextColor() const;
    COLORREF GetSubtleColor() const;
    COLORREF GetTooltipBgColor() const;
    COLORREF GetTooltipBorderColor() const;

    // 绘制单个图表（主series + 可选副series，用于NET上下行合并）
    void DrawChart(Gdiplus::Graphics* graphics, CDC* pDC, const Series& series,
                   const CRect& rect, const Series* secondary = nullptr, int hover_index = -1);

    // 构建平滑路径上的点
    std::vector<CPoint> BuildPoints(const std::vector<double>& values, const CRect& plotRect);

    // 绘制平滑贝塞尔线（带辉光）
    void DrawSmoothLine(CDC* pDC, const std::vector<CPoint>& points, COLORREF color, int width);

    // 绘制渐变填充区域
    void DrawGradientFill(CDC* pDC, const std::vector<CPoint>& points, COLORREF color, const CRect& plotRect);

    // 圆角矩形辅助
    void DrawRoundedRect(CDC* pDC, const CRect& rect, int radius, COLORREF fill, COLORREF stroke, int strokeWidth);

    // 更新序列数据（从历史缓冲区构建）
    void UpdateSeriesValues();

    // 从一组数据中选择代表性值（LTTB：最大化三角形面积，保留视觉特征）
    double SelectRepresentative(const std::vector<double>& group, double prev_selected, double next_avg) const;

    // 从有序原始数据初始化显示缓冲区
    void InitDisplayBuffer(const std::vector<double> ordered_data[6]);

    // 检测窗口是否是任务栏或其子窗口
    BOOL IsTaskbarWindow(HWND hWnd) const;

    // 绘制 tooltip
    void DrawTooltip(Gdiplus::Graphics* graphics, CDC* pDC);

    ULONG_PTR m_gdiplus_token;
};
