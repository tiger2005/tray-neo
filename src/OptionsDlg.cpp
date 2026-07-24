// OptionsDlg.cpp: 实现文件
//

#include "pch.h"
#include "TrayNeo.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"


// COptionsDlg 对话框

IMPLEMENT_DYNAMIC(COptionsDlg, CDialog)

COptionsDlg::COptionsDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_OPTIONS_DIALOG, pParent)
{

}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
    ON_CBN_SELCHANGE(IDC_HISTORY_DURATION_COMBO, &COptionsDlg::OnCbnSelchangeHistoryDurationCombo)
END_MESSAGE_MAP()


// COptionsDlg 消息处理程序

struct DurationOption
{
    const wchar_t* label;
    int seconds;
};

static const DurationOption kDurationOptions[] = {
    { L"30 秒", 30 },
    { L"1 分钟", 60 },
    { L"2 分钟", 120 },
    { L"5 分钟", 300 },
    { L"10 分钟", 600 },
    { L"20 分钟", 1200 },
    { L"30 分钟", 1800 },
    { L"1 小时", 3600 },
};

static const int kDurationOptionCount = _countof(kDurationOptions);

static int FindDurationIndex(int seconds)
{
    int best = 2; // 默认 2 分钟
    int best_diff = abs(seconds - kDurationOptions[best].seconds);
    for (int i = 0; i < kDurationOptionCount; i++)
    {
        int diff = abs(seconds - kDurationOptions[i].seconds);
        if (diff < best_diff)
        {
            best_diff = diff;
            best = i;
        }
    }
    return best;
}


static const UINT kWarnIds[] = {
    IDC_THR_CPU_WARN, IDC_THR_GPU_WARN, IDC_THR_MEM_WARN, IDC_THR_HDD_WARN,
    IDC_THR_NETD_WARN, IDC_THR_NETU_WARN
};
static const UINT kCritIds[] = {
    IDC_THR_CPU_CRIT, IDC_THR_GPU_CRIT, IDC_THR_MEM_CRIT, IDC_THR_HDD_CRIT,
    IDC_THR_NETD_CRIT, IDC_THR_NETU_CRIT
};


BOOL COptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // 初始化记录时长下拉框
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_HISTORY_DURATION_COMBO);
    if (pCombo)
    {
        for (int i = 0; i < kDurationOptionCount; i++)
        {
            pCombo->AddString(kDurationOptions[i].label);
        }
        int idx = FindDurationIndex(m_data.history_seconds);
        pCombo->SetCurSel(idx);
    }

    // 初始化阈值编辑框
    for (int i = 0; i < 6; i++)
    {
        CString str;
        str.Format(L"%g", m_data.threshold_warn[i]);
        SetDlgItemText(kWarnIds[i], str);
        str.Format(L"%g", m_data.threshold_crit[i]);
        SetDlgItemText(kCritIds[i], str);
    }

    // 初始化 MAX 替换复选框
    CheckDlgButton(IDC_REPLACE_MAX_CHECK, m_data.tray_replace_max ? BST_CHECKED : BST_UNCHECKED);

    // 初始化 tooltip 联动复选框
    CheckDlgButton(IDC_LINKED_TOOLTIP_CHECK, m_data.chart_linked_tooltip ? BST_CHECKED : BST_UNCHECKED);

    return TRUE;
}


void COptionsDlg::OnOK()
{
    // 读取阈值
    for (int i = 0; i < 6; i++)
    {
        CString str;
        GetDlgItemText(kWarnIds[i], str);
        m_data.threshold_warn[i] = _wtof(str);
        GetDlgItemText(kCritIds[i], str);
        m_data.threshold_crit[i] = _wtof(str);
    }

    // 读取 MAX 替换
    m_data.tray_replace_max = (IsDlgButtonChecked(IDC_REPLACE_MAX_CHECK) == BST_CHECKED);

    // 读取 tooltip 联动
    m_data.chart_linked_tooltip = (IsDlgButtonChecked(IDC_LINKED_TOOLTIP_CHECK) == BST_CHECKED);

    CDialog::OnOK();
}


void COptionsDlg::OnCbnSelchangeHistoryDurationCombo()
{
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_HISTORY_DURATION_COMBO);
    if (pCombo)
    {
        int idx = pCombo->GetCurSel();
        if (idx >= 0 && idx < kDurationOptionCount)
        {
            m_data.history_seconds = kDurationOptions[idx].seconds;
        }
    }
}
