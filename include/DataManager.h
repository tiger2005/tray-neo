#pragma once
#include <string>
#include <map>

#define g_data CDataManager::Instance()

struct SettingData
{
    int history_seconds{120};  // 历史记录最大时长（秒），默认2分钟

    // 颜色阈值：[0]=CPU [1]=GPU [2]=MEM [3]=HDD [4]=NET↓ [5]=NET↑
    // 百分比类(0-3) 值为百分比；网速类(4-5) 值为 MB/s（允许小数）
    // 超过 warn 显示黄色，超过 crit 显示红色
    double threshold_warn[6]{70, 70, 80, 80, 1, 1};
    double threshold_crit[6]{90, 90, 95, 95, 5, 5};

    // Tray 显示
    bool tray_replace_max{false};     // 将 100% 替换为 MAX

    // 图表 tooltip
    bool chart_linked_tooltip{false}; // true=联动显示所有指标，false=仅当前图表
};

class CDataManager
{
private:
    CDataManager();
    ~CDataManager();

public:
    static CDataManager& Instance();

    void LoadConfig(const std::wstring& config_dir);
    void SaveConfig() const;
    const CString& StringRes(UINT id);      //根据资源id获取一个字符串资源

public:
    SettingData m_setting_data;

private:
    static CDataManager m_instance;
    std::wstring m_config_path;
    std::map<UINT, CString> m_string_table;
};
