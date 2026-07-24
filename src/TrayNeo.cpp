#include "pch.h"
#include "TrayNeo.h"
#include "DataManager.h"
#include "OptionsDlg.h"

CTrayNeo CTrayNeo::m_instance;

CTrayNeo::CTrayNeo()
{
}

CTrayNeo& CTrayNeo::Instance()
{
    return m_instance;
}

HICON CTrayNeo::GetIcon(UINT id)
{
    auto iter = m_icon_map.find(id);
    if (iter != m_icon_map.end())
    {
        return iter->second;
    }
    else
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(id), IMAGE_ICON, DPI(16), DPI(16), 0);;
        m_icon_map[id] = hIcon;
        return hIcon;
    }
}

int CTrayNeo::DPI(int x, ITrafficMonitor::DPIType type)
{
    int dpi = m_app->GetDPI(type);
    return x * dpi / 96;
}

IPluginItem* CTrayNeo::GetItem(int index)
{
    switch (index)
    {
    case 0:
        return &m_stacked_display_item;
    default:
        break;
    }
    return nullptr;
}

void CTrayNeo::DataRequired()
{
}

const wchar_t* CTrayNeo::GetInfo(PluginInfoIndex index)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    static CString str;
    switch (index)
    {
    case TMI_NAME:
        str.LoadString(IDS_PLUGIN_NAME);
        return str.GetString();
    case TMI_DESCRIPTION:
        str.LoadString(IDS_PLUGIN_DESCRIPTION);
        return str.GetString();
    case TMI_AUTHOR:
        return L"tiger2005";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2026 tiger2005";
    case TMI_VERSION:
        return L"0.2.0";
    case ITMPlugin::TMI_URL:
        return L"";
    default:
        break;
    }
    return L"";
}

ITMPlugin::OptionReturn CTrayNeo::ShowOptionsDialog(void* hParent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    COptionsDlg dlg(CWnd::FromHandle((HWND)hParent));
    dlg.m_data = CDataManager::Instance().m_setting_data;
    if (dlg.DoModal() == IDOK)
    {
        CDataManager::Instance().m_setting_data = dlg.m_data;
        return ITMPlugin::OR_OPTION_CHANGED;
    }
    return ITMPlugin::OR_OPTION_UNCHANGED;
}


void CTrayNeo::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    switch (index)
    {
    case ITMPlugin::EI_CONFIG_DIR:
        //从配置文件读取配置
        g_data.LoadConfig(std::wstring(data));

        break;
    default:
        break;
    }
}

void CTrayNeo::OnInitialize(ITrafficMonitor* pApp)
{
    m_app = pApp;
    LoadEmbeddedFonts();
    std::wstring str = m_app->GetStringRes(L"IDS_MEMORY_USAGE", L"text");
    std::wstring str1 = m_app->GetStringRes(L"BCP_47", L"general");
    int a = 0;
}

void CTrayNeo::LoadEmbeddedFonts()
{
    // 从 DLL 资源加载内嵌的 Geist Mono 字体，注册到当前进程
    HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);
    static const UINT fontIds[] = {
        IDR_FONT_GEIST_LIGHT, IDR_FONT_GEIST_REGULAR,
        IDR_FONT_GEIST_MEDIUM, IDR_FONT_GEIST_BOLD
    };
    for (UINT id : fontIds)
    {
        HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(id), RT_FONT);
        if (!hRes) continue;
        HGLOBAL hMem = LoadResource(hModule, hRes);
        if (!hMem) continue;
        void* pData = LockResource(hMem);
        DWORD size = SizeofResource(hModule, hRes);
        if (!pData || size == 0) continue;
        DWORD count = 0;
        AddFontMemResourceEx(pData, size, nullptr, &count);
    }
}

ITMPlugin* TMPluginGetInstance()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return &CTrayNeo::Instance();
}
