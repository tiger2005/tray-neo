#pragma once
#include "PluginInterface.h"
#include <vector>

class CHistoryChartWnd;

// 上下分栏显示项
// 上半部分使用稍小的字体显示参数标签（CPU、GPU、MEM、HDD）
// 下半部分使用稍大的字体显示对应参数的数值
class CMonitorPanel : public IPluginItem
{
public:
    CMonitorPanel();
    ~CMonitorPanel();

public:
    // 通过 IPluginItem 继承
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;
    virtual bool IsCustomDraw() const override;
    virtual int GetItemWidth() const override;
    virtual int GetItemWidthEx(void* hDC) const override;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
    virtual int IsDoubleLineExclusive() const override { return 1; }     //在任务栏中独占双行
    virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override;

private:
    void CreateFonts(int dpi) const;
    CString FormatSpeed(double bytes_per_sec) const;

    // 历史数据环形缓冲区（动态大小）
    struct HistoryBuffer
    {
        std::vector<double> data;
        int count = 0;
        int index = 0;
        int capacity = 0;

        void set_capacity(int cap);
        void push(double value);
        std::vector<double> get_ordered() const;
    };

    HistoryBuffer m_history[6];   // CPU, GPU, MEM, HDD, NET↓, NET↑
    DWORD m_last_sample_tick{ 0 };
    bool m_dark_mode{ true };
    int m_dpi{ 96 };
    mutable int m_last_width{ 160 };  // 缓存最近一次计算的插件宽度
    mutable CRect m_last_rc;         // 缓存最近一次绘制的插件矩形（相对于任务栏窗口）

    mutable CFont m_font_small;    // 上半部分使用的小字体
    mutable CFont m_font_large;    // 下半部分使用的大字体
    mutable int m_last_dpi{ 0 };   // 已创建字体对应的DPI

    CHistoryChartWnd* m_chart_wnd{ nullptr };  // 图表弹窗窗口
};
