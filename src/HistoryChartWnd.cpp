#include "pch.h"
#include "HistoryChartWnd.h"
#include <algorithm>
#include <ctime>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

IMPLEMENT_DYNAMIC(CHistoryChartWnd, CWnd)

BEGIN_MESSAGE_MAP(CHistoryChartWnd, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_KILLFOCUS()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_ERASEBKGND()
    ON_WM_TIMER()
    ON_WM_NCCALCSIZE()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

CHistoryChartWnd::CHistoryChartWnd()
    : m_dark_mode(true), m_dpi(96), m_app(nullptr), m_history_capacity(0), m_history_count(0), m_history_index(0), m_timer_id(0), m_gdiplus_token(0)
{
    GdiplusStartupInput input;
    GdiplusStartup(&m_gdiplus_token, &input, NULL);
}

CHistoryChartWnd::~CHistoryChartWnd()
{
    if (m_timer_id)
        KillTimer(m_timer_id);
    GdiplusShutdown(m_gdiplus_token);
}

void CHistoryChartWnd::PostNcDestroy()
{
    if (m_destroy_callback)
        m_destroy_callback(this);
    delete this;
}

BOOL CHistoryChartWnd::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CHistoryChartWnd::SetData(const std::vector<double>& cpu,
    const std::vector<double>& gpu,
    const std::vector<double>& mem,
    const std::vector<double>& hdd,
    const std::vector<double>& net_down,
    const std::vector<double>& net_up,
    bool dark_mode, int dpi,
    ITrafficMonitor* app)
{
    m_dark_mode = dark_mode;
    m_dpi = dpi;
    m_app = app;

    // 计算容量：以最长的输入数据为基准，至少 60
    int max_input = (std::max)({ (int)cpu.size(), (int)gpu.size(), (int)mem.size(),
                                 (int)hdd.size(), (int)net_down.size(), (int)net_up.size() });
    m_history_capacity = (std::max)(max_input, 30);
    if (m_history_capacity > 600) m_history_capacity = 600;

    // 初始化 6 个系列的环形缓冲区
    m_history.assign(6, std::vector<double>(m_history_capacity, 0.0));
    m_history_count = m_history_capacity;
    m_history_index = 0;

    // 复制已有数据到缓冲区末尾（最新的数据在右侧）
    const std::vector<double>* sources[6] = { &cpu, &gpu, &mem, &hdd, &net_down, &net_up };
    for (int s = 0; s < 6; s++)
    {
        int n = (std::min)(static_cast<int>(sources[s]->size()), m_history_capacity);
        // 将已有数据放到缓冲区末尾
        for (int i = 0; i < n; i++)
        {
            int idx = (m_history_capacity - n + i) % m_history_capacity;
            m_history[s][idx] = (*sources[s])[i];
        }
    }
    m_history_index = 0;   // 下一个写入位置从 0 开始（最旧的位置）

    m_series.clear();
    m_series.resize(6);
    if (dark_mode)
    {
        m_series[0].label = L"CPU";
        m_series[0].color = RGB(0, 122, 255);
        m_series[0].is_percent = true;
        m_series[1].label = L"GPU";
        m_series[1].color = RGB(255, 55, 95);
        m_series[1].is_percent = true;
        m_series[2].label = L"MEM";
        m_series[2].color = RGB(255, 149, 0);
        m_series[2].is_percent = true;
        m_series[3].label = L"HDD";
        m_series[3].color = RGB(52, 199, 89);
        m_series[3].is_percent = true;
        m_series[4].label = L"NET↓";
        m_series[4].color = RGB(175, 82, 222);
        m_series[4].is_percent = false;
        m_series[5].label = L"NET↑";
        m_series[5].color = RGB(0, 199, 190);
        m_series[5].is_percent = false;
    }
    else
    {
        m_series[0].label = L"CPU";
        m_series[0].color = RGB(0, 110, 240);
        m_series[0].is_percent = true;
        m_series[1].label = L"GPU";
        m_series[1].color = RGB(240, 45, 85);
        m_series[1].is_percent = true;
        m_series[2].label = L"MEM";
        m_series[2].color = RGB(255, 130, 0);
        m_series[2].is_percent = true;
        m_series[3].label = L"HDD";
        m_series[3].color = RGB(30, 150, 60);
        m_series[3].is_percent = true;
        m_series[4].label = L"NET↓";
        m_series[4].color = RGB(130, 50, 170);
        m_series[4].is_percent = false;
        m_series[5].label = L"NET↑";
        m_series[5].color = RGB(0, 175, 165);
        m_series[5].is_percent = false;
    }

    // 记录最近采样时间（最新数据点对应"当前"）
    m_last_sample_time = time(nullptr);

    UpdateSeriesValues();
}

void CHistoryChartWnd::UpdateSeriesValues()
{
    for (int s = 0; s < 6; s++)
    {
        m_series[s].values.clear();
        m_series[s].values.reserve(m_history_count);
        for (int i = 0; i < m_history_count; i++)
        {
            int idx = (m_history_index - m_history_count + i + m_history_capacity) % m_history_capacity;
            m_series[s].values.push_back(m_history[s][idx]);
        }
    }
}

bool CHistoryChartWnd::CreatePopup(HWND hTaskbar, int plugin_center_x, CWnd* pParent)
{
    int w = Scale(380);
    int h = Scale(420);

    // 获取任务栏窗口矩形，确定上边沿
    CRect taskbar_rect;
    if (hTaskbar)
        ::GetWindowRect(hTaskbar, &taskbar_rect);
    else if (pParent)
        pParent->GetWindowRect(&taskbar_rect);

    HMONITOR hMonitor = MonitorFromWindow(hTaskbar, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    // 水平居中：弹窗中心对准插件中心
    int x = plugin_center_x - w / 2;
    if (x + w > mi.rcWork.right)
        x = mi.rcWork.right - w;
    if (x < mi.rcWork.left)
        x = mi.rcWork.left;

    // 垂直方向：弹窗下边沿紧贴任务栏上边沿，稍微抬高一点
    int offset = Scale(4);
    int y = taskbar_rect.top - h - offset;
    if (y < mi.rcWork.top)
    {
        // 上方空间不够，显示在任务栏下方
        y = taskbar_rect.bottom + offset;
        if (y + h > mi.rcWork.bottom)
            y = mi.rcWork.bottom - h;
    }

    CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW);
    m_parent_hwnd = pParent ? pParent->GetSafeHwnd() : NULL;
    if (!CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, className, L"",
        WS_POPUP, x, y, w, h, pParent ? pParent->GetSafeHwnd() : NULL, 0))
        return false;

    HWND hWnd = GetSafeHwnd();

    // Windows 11+ 原生圆角（属性 33 = DWMWA_WINDOW_CORNER_PREFERENCE）
    // 如果不支持则回退到 GDI 圆角裁剪
    DWORD cornerPref = 2; // DWMWCP_ROUND
    HRESULT hr = DwmSetWindowAttribute(hWnd, 33, &cornerPref, sizeof(cornerPref));
    if (SUCCEEDED(hr))
    {
        // Windows 11: 设置边框颜色（属性 34 = DWMWA_BORDER_COLOR）
        COLORREF borderColor = m_dark_mode ? RGB(60, 60, 60) : RGB(200, 200, 200);
        DwmSetWindowAttribute(hWnd, 34, &borderColor, sizeof(borderColor));
        // DWM 会自动处理圆角裁剪，无需 GDI 裁剪
    }
    else
    {
        // Windows 10 及更早：回退到 GDI 圆角裁剪
        CRgn rgn;
        rgn.CreateRoundRectRgn(0, 0, w + 1, h + 1, Scale(8), Scale(8));
        SetWindowRgn((HRGN)rgn.Detach(), TRUE);
    }

    ShowWindow(SW_SHOW);
    SetFocus();
    return true;
}

int CHistoryChartWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    m_timer_id = SetTimer(1, 1000, NULL);
    return 0;
}

void CHistoryChartWnd::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
    // 消除非客户区边框，使 WS_THICKFRAME 仅用于触发 DWM 阴影
    if (bCalcValidRects && lpncsp != nullptr)
    {
        lpncsp->rgrc[0] = lpncsp->rgrc[2];
    }
}

void CHistoryChartWnd::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1 && m_app)
    {
        // 采样新数据
        double vals[6] = {
            m_app->GetMonitorValue(ITrafficMonitor::MI_CPU),
            m_app->GetMonitorValue(ITrafficMonitor::MI_GPU_USAGE),
            m_app->GetMonitorValue(ITrafficMonitor::MI_MEMORY),
            m_app->GetMonitorValue(ITrafficMonitor::MI_HDD_USAGE),
            m_app->GetMonitorValue(ITrafficMonitor::MI_DOWN),
            m_app->GetMonitorValue(ITrafficMonitor::MI_UP),
        };
        for (int s = 0; s < 6; s++)
        {
            m_history[s][m_history_index] = vals[s];
        }
        m_history_index = (m_history_index + 1) % m_history_capacity;
        if (m_history_count < m_history_capacity)
            m_history_count++;
        m_last_sample_time = time(nullptr);

        UpdateSeriesValues();
        Invalidate(FALSE);
    }
    CWnd::OnTimer(nIDEvent);
}

void CHistoryChartWnd::OnKillFocus(CWnd* pNewWnd)
{
    CWnd::OnKillFocus(pNewWnd);
    // 焦点回到父窗口（任务栏）时不关闭——由 OnMouseEvent 的 toggle 处理开/关
    if (pNewWnd && pNewWnd->GetSafeHwnd() == m_parent_hwnd)
        return;
    // 失焦到其他窗口/桌面时收起图表
    DestroyWindow();
}

void CHistoryChartWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    CWnd::OnMouseMove(nFlags, point);
    m_mouse_pos = point;

    int pad = Scale(10);
    int gap = Scale(12);
    int chartW = Scale(380) - 2 * pad;
    int chartH = (Scale(420) - 2 * pad - 4 * gap) / 5;

    for (int i = 0; i < 5; i++)
    {
        int chartY = pad + i * (chartH + gap);
        CRect chartRect(pad, chartY, pad + chartW, chartY + chartH);
        if (chartRect.PtInRect(point))
        {
            int labelHeight = Scale(16);
            int padL = Scale(8);
            int padR = Scale(8);
            int padT = Scale(8);
            CRect plotRect(chartRect.left + padL, chartRect.top + labelHeight + padT,
                           chartRect.right - padR, chartRect.bottom - padT);

            if (plotRect.PtInRect(point))
            {
                int count = static_cast<int>(m_series[0].values.size());
                if (count > 0)
                {
                    int idx = static_cast<int>((point.x - plotRect.left) / (double)plotRect.Width() * count);
                    idx = max(0, min(count - 1, idx));
                    m_hover_index = idx;
                    m_hover_chart = i;
                    Invalidate(FALSE);
                    return;
                }
            }
        }
    }

    m_hover_index = -1;
    m_hover_chart = -1;
    Invalidate(FALSE);
}

void CHistoryChartWnd::OnMouseLeave()
{
    CWnd::OnMouseLeave();
    m_hover_index = -1;
    m_hover_chart = -1;
    Invalidate(FALSE);
}

BOOL CHistoryChartWnd::IsTaskbarWindow(HWND hWnd) const
{
    UNREFERENCED_PARAMETER(hWnd);
    return FALSE;
}

void CHistoryChartWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    // 左键点击弹窗内部不关闭，仅在失去焦点时关闭
    SetFocus();
}

void CHistoryChartWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
    // 右键点击显式关闭
    DestroyWindow();
}

// ── Fire 主题配色 ──

COLORREF CHistoryChartWnd::GetColorForValue(double value) const
{
    if (value <= 50)
        return RGB(255, 255, 255);
    if (value <= 80)
    {
        float t = static_cast<float>((value - 50) / 30.0);
        return LerpColor(RGB(255, 255, 255), RGB(255, 255, 0), t);
    }
    float t = static_cast<float>((value - 80) / 20.0);
    if (t > 1.0f) t = 1.0f;
    return LerpColor(RGB(255, 255, 0), RGB(255, 77, 0), t);
}

COLORREF CHistoryChartWnd::LerpColor(COLORREF c1, COLORREF c2, float t)
{
    BYTE r = static_cast<BYTE>(GetRValue(c1) + (GetRValue(c2) - GetRValue(c1)) * t);
    BYTE g = static_cast<BYTE>(GetGValue(c1) + (GetGValue(c2) - GetGValue(c1)) * t);
    BYTE b = static_cast<BYTE>(GetBValue(c1) + (GetBValue(c2) - GetBValue(c1)) * t);
    return RGB(r, g, b);
}

COLORREF CHistoryChartWnd::GetBgColor() const
{
    return m_dark_mode ? RGB(28, 28, 30) : RGB(246, 246, 246);
}

COLORREF CHistoryChartWnd::GetTextColor() const
{
    return m_dark_mode ? RGB(220, 220, 220) : RGB(60, 60, 60);
}

COLORREF CHistoryChartWnd::GetSubtleColor() const
{
    return m_dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
}

// ── 绘制 ──

void CHistoryChartWnd::OnPaint()
{
    CPaintDC dc(this);
    CRect clientRect;
    GetClientRect(&clientRect);
    int w = clientRect.Width();
    int h = clientRect.Height();

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(&dc, w, h);
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    Graphics graphics(memDC.m_hDC);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    COLORREF bgCr = GetBgColor();
    SolidBrush bgBrush(Color(255, GetRValue(bgCr), GetGValue(bgCr), GetBValue(bgCr)));
    graphics.FillRectangle(&bgBrush, 0, 0, w, h);

    int pad = Scale(10);
    int gap = Scale(12);
    int chartW = w - 2 * pad;
    int chartH = (h - 2 * pad - 4 * gap) / 5;
    int chartY = pad;

    CFont font;
    int fontHeight = -MulDiv(9, m_dpi, 72);
    font.CreateFont(fontHeight, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Geist Mono"));
    CFont* pOldFont = memDC.SelectObject(&font);

    // CPU/GPU/MEM/HDD 各一个图表，NET上下行合并
    // 独立模式下仅当前悬停图表绘制竖线与数据点
    for (int i = 0; i < 4; i++)
    {
        CRect chartRect(pad, chartY, pad + chartW, chartY + chartH);
        int hi = (m_linked_tooltip || i == m_hover_chart) ? m_hover_index : -1;
        DrawChart(&graphics, &memDC, m_series[i], chartRect, nullptr, hi);
        chartY += chartH + gap;
    }
    // NET 合并图表
    if (m_series.size() >= 6)
    {
        CRect chartRect(pad, chartY, pad + chartW, chartY + chartH);
        int hi = (m_linked_tooltip || m_hover_chart == 4) ? m_hover_index : -1;
        DrawChart(&graphics, &memDC, m_series[4], chartRect, &m_series[5], hi);
        chartY += chartH + gap;
    }

    if (m_hover_index >= 0)
    {
        DrawTooltip(&graphics, &memDC);
    }

    memDC.SelectObject(pOldFont);
    dc.BitBlt(0, 0, w, h, &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBitmap);
}

void CHistoryChartWnd::DrawChart(Gdiplus::Graphics* graphics, CDC* pDC, const Series& series, const CRect& rect, const Series* secondary, int hover_index)
{
    const auto& values = series.values;
    int dpiv = m_dpi;
    auto ScaleV = [dpiv](int v) { return MulDiv(v, dpiv, 96); };
    COLORREF lineCr = series.color;
    Color lineColor(255, GetRValue(lineCr), GetGValue(lineCr), GetBValue(lineCr));

    // 标签区高度（标签 + 当前值）
    int labelHeight = Scale(16);
    int padL = Scale(8);
    int padR = Scale(8);
    int padT = Scale(8);
    int padB = Scale(8);

    CRect plotRect(rect.left + padL, rect.top + labelHeight + padT,
                   rect.right - padR, rect.bottom - padB);
    if (plotRect.Width() <= 0 || plotRect.Height() <= 0) return;

    COLORREF subtleCr = GetSubtleColor();

    // 圆角背景
    int radius = Scale(6);
    GraphicsPath bgPath;
    REAL rx = static_cast<REAL>(rect.left);
    REAL ry = static_cast<REAL>(rect.top);
    REAL rw = static_cast<REAL>(rect.Width());
    REAL rh = static_cast<REAL>(rect.Height());
    REAL rr = static_cast<REAL>(radius);
    bgPath.AddArc(rx, ry, rr * 2, rr * 2, 180, 90);
    bgPath.AddArc(rx + rw - rr * 2, ry, rr * 2, rr * 2, 270, 90);
    bgPath.AddArc(rx + rw - rr * 2, ry + rh - rr * 2, rr * 2, rr * 2, 0, 90);
    bgPath.AddArc(rx, ry + rh - rr * 2, rr * 2, rr * 2, 90, 90);
    bgPath.CloseFigure();

    SolidBrush bgFill(Color(18, GetRValue(subtleCr), GetGValue(subtleCr), GetBValue(subtleCr)));
    graphics->FillPath(&bgFill, &bgPath);

    // Y 轴范围
    double paddedMin, paddedMax;
    if (series.is_percent)
    {
        paddedMin = 0;
        paddedMax = 100;
    }
    else
    {
        // 速度：动态范围，考虑主 series 和副 series
        double vMax = 1;
        if (!values.empty())
            vMax = *std::max_element(values.begin(), values.end());
        if (secondary && !secondary->values.empty())
        {
            double secMax = *std::max_element(secondary->values.begin(), secondary->values.end());
            if (secMax > vMax)
                vMax = secMax;
        }
        double upper = (std::max)(vMax, 1.0);
        double range = (std::max)(upper * 0.18, 1.0);
        paddedMin = 0;
        paddedMax = upper + range;
    }
    double valRange = paddedMax - paddedMin;
    if (valRange <= 0) valRange = 1;

    auto yFor = [&](double v) -> REAL {
        return static_cast<REAL>(plotRect.bottom) -
            static_cast<REAL>((v - paddedMin) / valRange * plotRect.Height());
    };

    // 参考线 25% / 50% / 75%
    for (int frac : {25, 50, 75})
    {
        double yVal = paddedMin + valRange * frac / 100.0;
        REAL y = yFor(yVal);
        Pen guidePen(Color(15, GetRValue(subtleCr), GetGValue(subtleCr), GetBValue(subtleCr)), 0.5f);
        graphics->DrawLine(&guidePen, static_cast<REAL>(plotRect.left), y, static_cast<REAL>(plotRect.right), y);
    }

    BYTE aDot = m_dark_mode ? 30 : 25;
    Color dotBorder(aDot, GetRValue(subtleCr), GetGValue(subtleCr), GetBValue(subtleCr));

    // 辅助：绘制一条曲线
    auto drawLine = [&](const std::vector<double>& vals, Color color) {
        int cnt = static_cast<int>(vals.size());
        if (cnt < 2) return;
        std::vector<PointF> pts;
        for (int i = 0; i < cnt; i++)
        {
            REAL x = static_cast<REAL>(plotRect.left) +
                (cnt > 1 ? static_cast<REAL>(i) / (cnt - 1) * plotRect.Width() : 0);
            pts.push_back(PointF(x, yFor(vals[i])));
        }
        std::vector<PointF> bezierPts;
        bezierPts.push_back(pts[0]);
        for (int i = 0; i < cnt - 1; i++)
        {
            PointF prev = (i > 0) ? pts[i - 1] : pts[i];
            PointF curr = pts[i];
            PointF next = pts[i + 1];
            PointF foll = (i + 2 < cnt) ? pts[i + 2] : next;
            PointF c1(curr.X + (next.X - prev.X) / 14, curr.Y + (next.Y - prev.Y) / 14);
            PointF c2(next.X - (foll.X - curr.X) / 14, next.Y - (foll.Y - curr.Y) / 14);
            bezierPts.push_back(c1);
            bezierPts.push_back(c2);
            bezierPts.push_back(next);
        }
        GraphicsPath linePath;
        linePath.AddBeziers(bezierPts.data(), static_cast<INT>(bezierPts.size()));
        // 填充
        GraphicsPath fillPath;
        fillPath.AddBeziers(bezierPts.data(), static_cast<INT>(bezierPts.size()));
        fillPath.AddLine(
            PointF(bezierPts.back().X, static_cast<REAL>(plotRect.bottom)),
            PointF(bezierPts.front().X, static_cast<REAL>(plotRect.bottom)));
        fillPath.CloseFigure();
        LinearGradientBrush fillBrush(
            Point(plotRect.left, plotRect.top), Point(plotRect.left, plotRect.bottom),
            Color(50, color.GetR(), color.GetG(), color.GetB()),
            Color(4, color.GetR(), color.GetG(), color.GetB()));
        graphics->FillPath(&fillBrush, &fillPath);
        // 主线
        Pen mainPen(color, 2.5f);
        mainPen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        graphics->DrawPath(&mainPen, &linePath);
        // 末端圆点
        PointF last = pts.back();
        SolidBrush dotBrush(color);
        graphics->FillEllipse(&dotBrush, last.X - 3.0f, last.Y - 3.0f, 6.0f, 6.0f);
        Pen dotPen(dotBorder, 1.0f);
        graphics->DrawEllipse(&dotPen, last.X - 3.0f, last.Y - 3.0f, 6.0f, 6.0f);
    };

    // 先画副 series（如果有）
    if (secondary && secondary->values.size() >= 2)
    {
        Color secColor(255, GetRValue(secondary->color), GetGValue(secondary->color), GetBValue(secondary->color));
        drawLine(secondary->values, secColor);
    }

    // 构建数据点
    int count = static_cast<int>(values.size());
    std::vector<PointF> points;
    for (int i = 0; i < count; i++)
    {
        REAL x = static_cast<REAL>(plotRect.left) +
            (count > 1 ? static_cast<REAL>(i) / (count - 1) * plotRect.Width() : 0);
        points.push_back(PointF(x, yFor(values[i])));
    }

    double currentValue = values.empty() ? 0 : values.back();

    if (count >= 2)
    {
        // 平滑贝塞尔路径（参照 tray-pulsy smoothedPath）
        std::vector<PointF> bezierPts;
        bezierPts.push_back(points[0]);

        for (int i = 0; i < count - 1; i++)
        {
            PointF prev = (i > 0) ? points[i - 1] : points[i];
            PointF curr = points[i];
            PointF next = points[i + 1];
            PointF foll = (i + 2 < count) ? points[i + 2] : next;

            PointF c1(
                curr.X + (next.X - prev.X) / 14,
                curr.Y + (next.Y - prev.Y) / 14
            );
            PointF c2(
                next.X - (foll.X - curr.X) / 14,
                next.Y - (foll.Y - curr.Y) / 14
            );
            bezierPts.push_back(c1);
            bezierPts.push_back(c2);
            bezierPts.push_back(next);
        }

        GraphicsPath linePath;
        linePath.AddBeziers(bezierPts.data(), static_cast<INT>(bezierPts.size()));

        // 填充路径
        GraphicsPath fillPath;
        fillPath.AddBeziers(bezierPts.data(), static_cast<INT>(bezierPts.size()));
        fillPath.AddLine(
            PointF(bezierPts.back().X, static_cast<REAL>(plotRect.bottom)),
            PointF(bezierPts.front().X, static_cast<REAL>(plotRect.bottom))
        );
        fillPath.CloseFigure();

        // 渐变填充
        LinearGradientBrush fillBrush(
            Point(plotRect.left, plotRect.top),
            Point(plotRect.left, plotRect.bottom),
            Color(50, lineColor.GetR(), lineColor.GetG(), lineColor.GetB()),
            Color(4, lineColor.GetR(), lineColor.GetG(), lineColor.GetB())
        );
        graphics->FillPath(&fillBrush, &fillPath);

        // 主线
        Pen mainPen(lineColor, 2.5f);
        mainPen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        graphics->DrawPath(&mainPen, &linePath);

        // 末端圆点
        PointF last = points.back();
        SolidBrush dotBrush(lineColor);
        graphics->FillEllipse(&dotBrush, last.X - 3.0f, last.Y - 3.0f, 6.0f, 6.0f);
        Pen dotPen(Color(30, 0, 0, 0), 1.0f);
        graphics->DrawEllipse(&dotPen, last.X - 3.0f, last.Y - 3.0f, 6.0f, 6.0f);
    }
    else if (count == 1)
    {
        PointF pt = points[0];
        SolidBrush dotBrush(lineColor);
        graphics->FillEllipse(&dotBrush, pt.X - 3.0f, pt.Y - 3.0f, 6.0f, 6.0f);
    }

    // 文字标签（GDI）
    pDC->SetBkMode(TRANSPARENT);

    CFont labelFont;
    int labelFontHeight = -MulDiv(9, m_dpi, 72);
    labelFont.CreateFont(labelFontHeight, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Geist Mono"));
    CFont* pOldFont = pDC->SelectObject(&labelFont);

    // 标签（左上）
    pDC->SetTextColor(lineCr);
    CRect labelRect(rect.left + Scale(10), rect.top + Scale(4),
                    rect.right - Scale(10), rect.top + labelHeight + Scale(2));
    // NET 合并图表：左上角标签去除箭头后缀，避免与右侧 ↓/↑ 数值重复
    CString chartLabel = series.label;
    if (secondary && !chartLabel.IsEmpty())
    {
        wchar_t last = chartLabel.GetAt(chartLabel.GetLength() - 1);
        if (last == L'↓' || last == L'↑')
            chartLabel = chartLabel.Left(chartLabel.GetLength() - 1);
    }
    pDC->DrawText(chartLabel, labelRect, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // 当前值（右上）
    if (secondary && !secondary->values.empty())
    {
        // 有副 series：右上角左↓下行  右↑上行
        CString downStr, upStr;
        // 下行
        double kb = currentValue / 1024.0;
        if (kb < 1000)
            downStr.Format(L"%dK", static_cast<int>(kb));
        else if (kb < 10240)
            downStr.Format(L"%.1fM", kb / 1024.0);
        else if (kb < 1024000)
            downStr.Format(L"%dM", static_cast<int>(kb / 1024.0));
        else
            downStr.Format(L"%.1fG", kb / 1048576.0);
        // 上行
        double upKb = secondary->values.back() / 1024.0;
        if (upKb < 1000)
            upStr.Format(L"%dK", static_cast<int>(upKb));
        else if (upKb < 10240)
            upStr.Format(L"%.1fM", upKb / 1024.0);
        else if (upKb < 1024000)
            upStr.Format(L"%dM", static_cast<int>(upKb / 1024.0));
        else
            upStr.Format(L"%.1fG", upKb / 1048576.0);

        // 先画上行值（最右，青色）
        CString upFull = L"↑ " + upStr;
        pDC->SetTextColor(secondary->color);
        CRect upRect = labelRect;
        pDC->DrawText(upFull, upRect, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_CALCRECT);
        int upW = upRect.Width() + Scale(2);
        upRect = labelRect;
        pDC->DrawText(upFull, upRect, DT_RIGHT | DT_TOP | DT_SINGLELINE);

        // 再画下行值（上行值左侧，紫色）
        CString downFull = L"↓ " + downStr;
        pDC->SetTextColor(lineCr);
        CRect downRect = labelRect;
        downRect.right -= upW + Scale(6);
        pDC->DrawText(downFull, downRect, DT_RIGHT | DT_TOP | DT_SINGLELINE);
    }
    else
    {
        // 单 series：右上显示当前值
        CString valueStr;
        if (series.is_percent)
        {
            if (currentValue >= 100)
                valueStr = L"100%";
            else
                valueStr.Format(L"%d%%", static_cast<int>(currentValue));
        }
        else
        {
            double kb = currentValue / 1024.0;
            if (kb < 1000)
                valueStr.Format(L"%dK", static_cast<int>(kb));
            else if (kb < 10240)
                valueStr.Format(L"%.1fM", kb / 1024.0);
            else if (kb < 1024000)
                valueStr.Format(L"%dM", static_cast<int>(kb / 1024.0));
            else
                valueStr.Format(L"%.1fG", kb / 1048576.0);
        }
        pDC->DrawText(valueStr, labelRect, DT_RIGHT | DT_TOP | DT_SINGLELINE);
    }

    pDC->SelectObject(pOldFont);

    if (hover_index >= 0 && hover_index < static_cast<int>(values.size()))
    {
        REAL x = static_cast<REAL>(plotRect.left) +
            (count > 1 ? static_cast<REAL>(hover_index) / (count - 1) * plotRect.Width() : 0);
        
        Pen vLinePen(Color(40, GetRValue(subtleCr), GetGValue(subtleCr), GetBValue(subtleCr)), 0.5f);
        graphics->DrawLine(&vLinePen, x, static_cast<REAL>(plotRect.top), x, static_cast<REAL>(plotRect.bottom));

        REAL y = yFor(values[hover_index]);
        SolidBrush pointBrush(lineColor);
        graphics->FillEllipse(&pointBrush, x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        Pen pointPen(Color(255, 255, 255, 255), 1.5f);
        graphics->DrawEllipse(&pointPen, x - 4.0f, y - 4.0f, 8.0f, 8.0f);

        if (secondary && hover_index < static_cast<int>(secondary->values.size()))
        {
            REAL sy = yFor(secondary->values[hover_index]);
            Color secColor(255, GetRValue(secondary->color), GetGValue(secondary->color), GetBValue(secondary->color));
            SolidBrush secPointBrush(secColor);
            graphics->FillEllipse(&secPointBrush, x - 4.0f, sy - 4.0f, 8.0f, 8.0f);
            graphics->DrawEllipse(&pointPen, x - 4.0f, sy - 4.0f, 8.0f, 8.0f);
        }
    }
}

void CHistoryChartWnd::DrawTooltip(Gdiplus::Graphics* graphics, CDC* pDC)
{
    if (m_hover_index < 0) return;

    COLORREF bgCr = m_dark_mode ? RGB(35, 35, 37) : RGB(255, 255, 255);
    COLORREF borderCr = m_dark_mode ? RGB(60, 60, 60) : RGB(200, 200, 200);

    // 决定要显示哪些 series：
    //   联动模式 -> 全部 6 个
    //   独立模式 -> 仅当前悬停图表包含的 series（chart 0..3 单系列，chart 4 为 NET↓+NET↑）
    int series_to_show[6];
    int n_show = 0;
    if (m_linked_tooltip || m_hover_chart < 0)
    {
        for (int i = 0; i < 6; i++) series_to_show[n_show++] = i;
    }
    else if (m_hover_chart < 4)
    {
        series_to_show[n_show++] = m_hover_chart;
    }
    else
    {
        series_to_show[n_show++] = 4;
        series_to_show[n_show++] = 5;
    }

    // 时间表头：HH:MM:SS（无日期）
    // 最新数据点对应 m_last_sample_time，悬停索引 i 距最新点 (count-1-i) 秒
    CString timeStr;
    int count = static_cast<int>(m_series[0].values.size());
    if (m_last_sample_time > 0 && count > 0)
    {
        int offset = (count - 1) - m_hover_index;
        if (offset < 0) offset = 0;
        time_t t = m_last_sample_time - offset;
        struct tm tmv;
        localtime_s(&tmv, &t);
        timeStr.Format(L"%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    }

    std::vector<CString> labels;
    std::vector<CString> values;
    std::vector<COLORREF> line_colors;

    for (int k = 0; k < n_show; k++)
    {
        int i = series_to_show[k];
        if (m_hover_index >= static_cast<int>(m_series[i].values.size()))
            continue;

        double val = m_series[i].values[m_hover_index];
        CString valueStr;

        if (m_series[i].is_percent)
        {
            if (val >= 100)
                valueStr = L"100%";
            else
                valueStr.Format(L"%d%%", static_cast<int>(val));
        }
        else
        {
            double kb = val / 1024.0;
            if (kb < 1000)
                valueStr.Format(L"%dK", static_cast<int>(kb));
            else if (kb < 10240)
                valueStr.Format(L"%.1fM", kb / 1024.0);
            else if (kb < 1024000)
                valueStr.Format(L"%dM", static_cast<int>(kb / 1024.0));
            else
                valueStr.Format(L"%.1fG", kb / 1048576.0);
        }

        labels.push_back(m_series[i].label);
        values.push_back(valueStr);
        line_colors.push_back(m_series[i].color);
    }

    if (labels.empty() && timeStr.IsEmpty())
        return;

    int padding = Scale(8);
    int lineHeight = Scale(16);
    int header_gap = Scale(3);   // 时间表头下方的小间距
    bool has_header = !timeStr.IsEmpty();
    int total_lines = (has_header ? 1 : 0) + static_cast<int>(labels.size());

    CFont font;
    int fontHeight = -MulDiv(9, m_dpi, 72);
    font.CreateFont(fontHeight, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Geist Mono"));
    CFont* pOldFont = pDC->SelectObject(&font);

    // 固定宽度：联动模式更宽以容纳所有指标
    int tooltipW = m_linked_tooltip ? Scale(110) : Scale(100);
    int tooltipH = total_lines * lineHeight + padding * 2 + (has_header ? header_gap : 0);

    int x = m_mouse_pos.x + Scale(12);
    int y = m_mouse_pos.y - tooltipH / 2;

    CRect clientRect;
    GetClientRect(&clientRect);

    if (x + tooltipW > clientRect.right)
        x = m_mouse_pos.x - tooltipW - Scale(12);
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (y + tooltipH > clientRect.bottom)
        y = clientRect.bottom - tooltipH;

    int radius = Scale(6);
    GraphicsPath bgPath;
    REAL rx = static_cast<REAL>(x);
    REAL ry = static_cast<REAL>(y);
    REAL rw = static_cast<REAL>(tooltipW);
    REAL rh = static_cast<REAL>(tooltipH);
    REAL rr = static_cast<REAL>(radius);
    bgPath.AddArc(rx, ry, rr * 2, rr * 2, 180, 90);
    bgPath.AddArc(rx + rw - rr * 2, ry, rr * 2, rr * 2, 270, 90);
    bgPath.AddArc(rx + rw - rr * 2, ry + rh - rr * 2, rr * 2, rr * 2, 0, 90);
    bgPath.AddArc(rx, ry + rh - rr * 2, rr * 2, rr * 2, 90, 90);
    bgPath.CloseFigure();

    SolidBrush bgBrush(Color(255, GetRValue(bgCr), GetGValue(bgCr), GetBValue(bgCr)));
    graphics->FillPath(&bgBrush, &bgPath);
    Pen borderPen(Color(255, GetRValue(borderCr), GetGValue(borderCr), GetBValue(borderCr)), 0.5f);
    graphics->DrawPath(&borderPen, &bgPath);

    pDC->SetBkMode(TRANSPARENT);

    int textY = y + padding;

    // 时间表头（左对齐，使用弱化色）
    if (has_header)
    {
        COLORREF headerCr = m_dark_mode ? RGB(170, 170, 170) : RGB(110, 110, 110);
        pDC->SetTextColor(headerCr);
        CRect textRect(x + padding, textY, x + tooltipW - padding, textY + lineHeight);
        pDC->DrawText(timeStr, textRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
        textY += lineHeight + header_gap;
    }

    // 每个指标：标签左对齐，数值右对齐，使用各自颜色
    for (size_t i = 0; i < labels.size(); i++)
    {
        pDC->SetTextColor(line_colors[i]);
        CRect textRect(x + padding, textY, x + tooltipW - padding, textY + lineHeight);
        pDC->DrawText(labels[i], textRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
        pDC->DrawText(values[i], textRect, DT_RIGHT | DT_TOP | DT_SINGLELINE);
        textY += lineHeight;
    }

    pDC->SelectObject(pOldFont);
}
