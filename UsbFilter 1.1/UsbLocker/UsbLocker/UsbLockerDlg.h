
// UsbLockerDlg.h : 头文件
//

#pragma once


// CUsbLockerDlg 对话框
class CUsbLockerDlg : public CDialog
{
// 构造
public:
	CUsbLockerDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_USBLOCKER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//============================
	 
	//定义一个Windows组合框
	CComboBox m_ComboCtrl;
	//定义字符串
	CString m_ComboVal;

	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnClose();
};
