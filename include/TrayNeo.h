#pragma once
#include "PluginInterface.h"
#include "MonitorPanel.h"
#include <map>

class CTrayNeo : public ITMPlugin
{
private:
    CTrayNeo();

public:
    static CTrayNeo& Instance();
    HICON GetIcon(UINT id);
    int DPI(int x, ITrafficMonitor::DPIType type = ITrafficMonitor::DPI_MAIN_WND);
    ITrafficMonitor* GetApp() const { return m_app; }

    // 通过 ITMPlugin 继承
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    virtual void OnInitialize(ITrafficMonitor* pApp) override;

private:
    CMonitorPanel m_stacked_display_item;
    ITrafficMonitor* m_app{};

    static CTrayNeo m_instance;
    std::map<UINT, HICON> m_icon_map;

    void LoadEmbeddedFonts();   // 从 DLL 资源加载内嵌字体
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();

#ifdef __cplusplus
}
#endif
