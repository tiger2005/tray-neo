#include "pch.h"
#include "DataManager.h"

CDataManager CDataManager::m_instance;

CDataManager::CDataManager()
{
}

CDataManager::~CDataManager()
{
    SaveConfig();
}

CDataManager& CDataManager::Instance()
{
    return m_instance;
}

void CDataManager::LoadConfig(const std::wstring& config_dir)
{
    //获取模块的路径
    HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);
    std::wstring module_path = path;
    m_config_path = module_path;
    if (!config_dir.empty())
    {
        size_t index = module_path.find_last_of(L"\\/");
        //模块的文件名
        std::wstring module_file_name = module_path.substr(index + 1);
        m_config_path = config_dir + module_file_name;
    }
    m_config_path += L".ini";
    m_setting_data.history_seconds = GetPrivateProfileInt(_T("config"), _T("history_seconds"), 120, m_config_path.c_str());
    if (m_setting_data.history_seconds < 30)
        m_setting_data.history_seconds = 30;
    if (m_setting_data.history_seconds > 600)
        m_setting_data.history_seconds = 600;

    static const wchar_t* kNames[] = { L"cpu", L"gpu", L"mem", L"hdd", L"net_down", L"net_up" };
    static const wchar_t* kDefWarn[] = { L"70", L"70", L"80", L"80", L"1", L"1" };
    static const wchar_t* kDefCrit[] = { L"90", L"90", L"95", L"95", L"5", L"5" };
    for (int i = 0; i < 6; i++)
    {
        CString key;
        wchar_t buf[64];
        key.Format(L"thr_%s_warn", kNames[i]);
        GetPrivateProfileString(_T("config"), key, kDefWarn[i], buf, 64, m_config_path.c_str());
        m_setting_data.threshold_warn[i] = _wtof(buf);
        key.Format(L"thr_%s_crit", kNames[i]);
        GetPrivateProfileString(_T("config"), key, kDefCrit[i], buf, 64, m_config_path.c_str());
        m_setting_data.threshold_crit[i] = _wtof(buf);
    }

    m_setting_data.tray_replace_max = (GetPrivateProfileInt(_T("config"), _T("tray_replace_max"), 0, m_config_path.c_str()) != 0);
    m_setting_data.chart_linked_tooltip = (GetPrivateProfileInt(_T("config"), _T("chart_linked_tooltip"), 0, m_config_path.c_str()) != 0);
}

static void WritePrivateProfileInt(const wchar_t* app_name, const wchar_t* key_name, int value, const wchar_t* file_path)
{
    wchar_t buff[16];
    swprintf_s(buff, L"%d", value);
    WritePrivateProfileString(app_name, key_name, buff, file_path);
}

void CDataManager::SaveConfig() const
{
    WritePrivateProfileInt(_T("config"), _T("history_seconds"), m_setting_data.history_seconds, m_config_path.c_str());

    static const wchar_t* kNames[] = { L"cpu", L"gpu", L"mem", L"hdd", L"net_down", L"net_up" };
    for (int i = 0; i < 6; i++)
    {
        CString key, val;
        key.Format(L"thr_%s_warn", kNames[i]);
        val.Format(L"%g", m_setting_data.threshold_warn[i]);
        WritePrivateProfileString(_T("config"), key, val, m_config_path.c_str());
        key.Format(L"thr_%s_crit", kNames[i]);
        val.Format(L"%g", m_setting_data.threshold_crit[i]);
        WritePrivateProfileString(_T("config"), key, val, m_config_path.c_str());
    }

    WritePrivateProfileInt(_T("config"), _T("tray_replace_max"), m_setting_data.tray_replace_max ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileInt(_T("config"), _T("chart_linked_tooltip"), m_setting_data.chart_linked_tooltip ? 1 : 0, m_config_path.c_str());
}

const CString& CDataManager::StringRes(UINT id)
{
    auto iter = m_string_table.find(id);
    if (iter != m_string_table.end())
    {
        return iter->second;
    }
    else
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        m_string_table[id].LoadString(id);
        return m_string_table[id];
    }
}
