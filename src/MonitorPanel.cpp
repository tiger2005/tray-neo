#include "pch.h"
#include "MonitorPanel.h"
#include "DataManager.h"
#include "TrayNeo.h"
#include "HistoryChartWnd.h"

// ── HistoryBuffer 实现 ──

void CMonitorPanel::HistoryBuffer::set_capacity(int cap)
{
    if (cap <= 0) cap = 30;
    if (cap > 3600) cap = 3600;
    capacity = cap;
    data.assign(capacity, 0.0);
    count = 0;
    index = 0;
}

void CMonitorPanel::HistoryBuffer::push(double value)
{
    if (capacity <= 0 || data.empty()) return;
    data[index] = value;
    index = (index + 1) % capacity;
    if (count < capacity) count++;
}

std::vector<double> CMonitorPanel::HistoryBuffer::get_ordered() const
{
    std::vector<double> result(capacity, 0.0);
    int offset = capacity - count;
    for (int i = 0; i < count; i++)
    {
        int idx = (index - count + i + capacity) % capacity;
        result[offset + i] = data[idx];
    }
    return result;
}

CMonitorPanel::CMonitorPanel()
{
}

CMonitorPanel::~CMonitorPanel()
{
    if (m_chart_wnd != nullptr)
    {
        m_chart_wnd->DestroyWindow();
    }
}

const wchar_t* CMonitorPanel::GetItemName() const
{
    return CDataManager::Instance().StringRes(IDS_STACKED_DISPLAY_ITEM);
}

const wchar_t* CMonitorPanel::GetItemId() const
{
    return L"SDp1aY7x";
}

const wchar_t* CMonitorPanel::GetItemLableText() const
{
    return L"";
}

const wchar_t* CMonitorPanel::GetItemValueText() const
{
    return L"";
}

const wchar_t* CMonitorPanel::GetItemValueSampleText() const
{
    return L"";
}

bool CMonitorPanel::IsCustomDraw() const
{
    return true;
}

int CMonitorPanel::GetItemWidth() const
{
    return 160;
}

int CMonitorPanel::GetItemWidthEx(void* hDC) const
{
    CDC* pDC = CDC::FromHandle((HDC)hDC);
    if (pDC == nullptr)
        return 0;

    ITrafficMonitor* app = CTrayNeo::Instance().GetApp();
    if (app == nullptr)
        return 0;

    int dpi = app->GetDPI(ITrafficMonitor::DPI_TASKBAR);
    CreateFonts(dpi);

    CFont* pOldFont = pDC->SelectObject(&m_font_large);
    bool replace_max = g_data.m_setting_data.tray_replace_max;
    const wchar_t* sample_percent = replace_max ? L"MAX" : L"100%";
    CSize size_percent = pDC->GetTextExtent(sample_percent);
    CSize size_speed = pDC->GetTextExtent(L"9999K");
    CSize size_char = pDC->GetTextExtent(L"0");
    CFont* pOldFont2 = pDC->SelectObject(&m_font_small);
    CSize size_label = pDC->GetTextExtent(L"NET↓");
    pDC->SelectObject(pOldFont2);
    pDC->SelectObject(pOldFont);

    int percent_gap = size_char.cx * 18 / 10;
    int speed_gap = percent_gap;
    if (!replace_max)
        percent_gap -= size_char.cx;  // 百分比列之间的距离减少一个字符
    int col_width_percent = (std::max)(size_percent.cx, size_label.cx);
    int size_speed_cx = size_speed.cx;

    int total_width = col_width_percent * 4 + size_speed_cx * 2 + percent_gap * 3 + speed_gap * 2;
    m_last_width = total_width;
    return total_width;
}

void CMonitorPanel::CreateFonts(int dpi) const
{
    if (m_last_dpi == dpi && m_font_small.m_hObject && m_font_large.m_hObject)
        return;

    if (m_font_small.m_hObject)
        m_font_small.DeleteObject();
    if (m_font_large.m_hObject)
        m_font_large.DeleteObject();

    int small_height = -MulDiv(8, dpi, 72);
    m_font_small.CreateFont(
        small_height, 0, 0, 0, FW_LIGHT,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, _T("Geist Mono"));

    int large_height = -MulDiv(11, dpi, 72);
    m_font_large.CreateFont(
        large_height, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, _T("Geist Mono"));

    m_last_dpi = dpi;
}

CString CMonitorPanel::FormatSpeed(double bytes_per_sec) const
{
    CString result;
    int kb_int = static_cast<int>(bytes_per_sec / 1024.0);
    if (kb_int < 1000)
    {
        result.Format(L"%dK", kb_int);
        return result;
    }
    
    int mb_int = kb_int / 1024;
    if (mb_int < 10)
    {
        int val = static_cast<int>(kb_int * 10 / 1024.0);
        int integer = val / 10;
        int decimal = val % 10;
        result.Format(L"%d.%dM", integer, decimal);
        return result;
    }
    
    if (mb_int < 1000)
    {
        result.Format(L"%dM", mb_int);
        return result;
    }
    
    int val = static_cast<int>(mb_int * 10 / 1024.0);
    int integer = val / 10;
    int decimal = val % 10;
    result.Format(L"%d.%dG", integer, decimal);
    return result;
}

void CMonitorPanel::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    CDC* pDC = CDC::FromHandle((HDC)hDC);
    if (pDC == nullptr)
        return;

    // 保存插件项矩形（相对于任务栏窗口）
    m_last_rc = CRect(x, y, x + w, y + h);

    ITrafficMonitor* app = CTrayNeo::Instance().GetApp();
    if (app == nullptr)
        return;

    int dpi = app->GetDPI(ITrafficMonitor::DPI_TASKBAR);
    CreateFonts(dpi);

    m_dark_mode = dark_mode;
    m_dpi = dpi;

    // 确保历史缓冲区容量正确
    int expected_cap = g_data.m_setting_data.history_seconds;
    if (m_history[0].capacity != expected_cap)
    {
        for (int i = 0; i < 6; i++)
            m_history[i].set_capacity(expected_cap);
    }

    // 历史数据采样（每秒一次）
    DWORD now = GetTickCount();
    if (now - m_last_sample_tick >= 1000)
    {
        m_history[0].push(app->GetMonitorValue(ITrafficMonitor::MI_CPU));
        m_history[1].push(app->GetMonitorValue(ITrafficMonitor::MI_GPU_USAGE));
        m_history[2].push(app->GetMonitorValue(ITrafficMonitor::MI_MEMORY));
        m_history[3].push(app->GetMonitorValue(ITrafficMonitor::MI_HDD_USAGE));
        m_history[4].push(app->GetMonitorValue(ITrafficMonitor::MI_DOWN));
        m_history[5].push(app->GetMonitorValue(ITrafficMonitor::MI_UP));
        m_last_sample_tick = now;
    }

    COLORREF label_color = dark_mode ? RGB(190, 190, 190) : RGB(90, 90, 90);

    CRect rect_top(CPoint(x, y), CSize(w, h / 2));
    CRect rect_bottom(CPoint(x, y + h / 2 - CTrayNeo::Instance().DPI(4, ITrafficMonitor::DPI_TASKBAR)), CSize(w, h - h / 2 + CTrayNeo::Instance().DPI(4, ITrafficMonitor::DPI_TASKBAR)));

    struct ParamInfo
    {
        const wchar_t* label;
        ITrafficMonitor::MonitorItem item;
        bool is_speed;
    };
    static const ParamInfo params[] = {
        { L"CPU", ITrafficMonitor::MI_CPU, false },
        { L"GPU", ITrafficMonitor::MI_GPU_USAGE, false },
        { L"MEM", ITrafficMonitor::MI_MEMORY, false },
        { L"HDD", ITrafficMonitor::MI_HDD_USAGE, false },
        { L"NET↓", ITrafficMonitor::MI_DOWN, true },
        { L"NET↑", ITrafficMonitor::MI_UP, true },
    };
    const int param_count = sizeof(params) / sizeof(params[0]);

    CFont* pTempFont = pDC->SelectObject(&m_font_large);
    bool replace_max = g_data.m_setting_data.tray_replace_max;
    CSize size_speed = pDC->GetTextExtent(L"999K");
    CSize size_char = pDC->GetTextExtent(L"0");
    pDC->SelectObject(pTempFont);
    int size_speed_cx = size_speed.cx;
    int percent_gap = size_char.cx * 18 / 10;
    int speed_gap = percent_gap;
    if (!replace_max)
        percent_gap -= size_char.cx;  // 百分比列之间的距离减少一个字符
    int col_width_percent = (w - size_speed_cx * 2 - percent_gap * 3 - speed_gap * 2) / 4;
    int col_width_speed = size_speed_cx;

    pDC->SetBkMode(TRANSPARENT);

    CFont* pOldFont = pDC->SelectObject(&m_font_small);
    pDC->SetTextColor(label_color);
    int current_x = x;
    for (int i = 0; i < param_count; i++)
    {
        int col_width = params[i].is_speed ? col_width_speed : col_width_percent;
        CRect col_rect(current_x, rect_top.top, current_x + col_width, rect_top.bottom);
        pDC->DrawText(params[i].label, col_rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        current_x += col_width + (i < 3 ? percent_gap : speed_gap);
    }

    pDC->SelectObject(&m_font_large);
    current_x = x;
    for (int i = 0; i < param_count; i++)
    {
        double raw_value = app->GetMonitorValue(params[i].item);
        CString value;

        if (params[i].is_speed)
        {
            value = FormatSpeed(raw_value);
        }
        else
        {
            if (raw_value >= 100)
            {
                value = replace_max ? L"MAX" : L"100%";
            }
            else
            {
                value = app->GetMonitorValueString(params[i].item, 0);
                value.Remove(' ');
            }
        }

        // 颜色：默认白/深灰，超 warn 显示橙色，超 crit 显示红色
        COLORREF color_normal = dark_mode ? RGB(255, 255, 255) : RGB(60, 60, 60);
        COLORREF color_warn = dark_mode ? RGB(255, 149, 0) : RGB(255, 120, 0);
        COLORREF color_crit = dark_mode ? RGB(255, 59, 48) : RGB(230, 50, 40);
        COLORREF current_color = color_normal;
        {
            double grad_value = params[i].is_speed ? (raw_value / (1024.0 * 1024.0)) : raw_value;
            double t_warn = g_data.m_setting_data.threshold_warn[i];
            double t_crit = g_data.m_setting_data.threshold_crit[i];
            if (grad_value >= t_crit)
                current_color = color_crit;
            else if (grad_value >= t_warn)
                current_color = color_warn;
        }

        pDC->SetTextColor(current_color);
        CRect col_rect(current_x, rect_bottom.top, current_x + (params[i].is_speed ? col_width_speed : col_width_percent), rect_bottom.bottom);
        
        CRect text_rect = col_rect;
        pDC->DrawText(value, text_rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_CALCRECT);
        
        if (text_rect.Width() > col_rect.Width())
        {
            col_rect.left = col_rect.right - text_rect.Width();
        }
        
        pDC->DrawText(value, col_rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        current_x += (params[i].is_speed ? col_width_speed : col_width_percent) + (i < 3 ? percent_gap : speed_gap);
    }

    pDC->SelectObject(pOldFont);
}

int CMonitorPanel::OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag)
{
    if (type == MT_LCLICKED)
    {
        CWnd* pParent = CWnd::FromHandle((HWND)hWnd);
        ITrafficMonitor* app = CTrayNeo::Instance().GetApp();
        if (pParent && app)
        {
            if (m_chart_wnd != nullptr)
            {
                // 窗口已存在，关闭它
                m_chart_wnd->DestroyWindow();
                m_chart_wnd = nullptr;
            }
            else
            {
                // 创建新窗口
                CHistoryChartWnd* pWnd = new CHistoryChartWnd();
                pWnd->SetData(
                    m_history[0].get_ordered(),
                    m_history[1].get_ordered(),
                    m_history[2].get_ordered(),
                    m_history[3].get_ordered(),
                    m_history[4].get_ordered(),
                    m_history[5].get_ordered(),
                    m_dark_mode, m_dpi, app);
                // 用插件项中心而非点击点，确保弹窗位置稳定
                int center_x = m_last_rc.left + m_last_rc.Width() / 2;
                CPoint pt(center_x, 0);
                pParent->ClientToScreen(&pt);
                pWnd->SetDestroyCallback([this](CHistoryChartWnd* wnd) {
                    if (m_chart_wnd == wnd)
                        m_chart_wnd = nullptr;
                });
                pWnd->SetLinkedTooltip(g_data.m_setting_data.chart_linked_tooltip);
                pWnd->CreatePopup((HWND)hWnd, pt.x, pParent);
                m_chart_wnd = pWnd;
            }
        }
        return 1;
    }
    return 0;
}
