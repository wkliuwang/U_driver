// UsbHostDlg.h : header file 
// 
 
#if !defined(AFX_USBHOSTDLG_H__AB341167_800F_11D7_A300_5254AB177364__INCLUDED_) 
#define AFX_USBHOSTDLG_H__AB341167_800F_11D7_A300_5254AB177364__INCLUDED_ 
 
#if _MSC_VER > 1000 
#pragma once 
#endif // _MSC_VER > 1000 
 
///////////////////////////////////////////////////////////////////////////// 
// CUsbHostDlg dialog 
 
class CUsbHostDlg : public CDialog 
{ 
// Construction 
public: 
	CUsbHostDlg(CWnd* pParent = NULL);	// standard constructor 
 
// Dialog Data 
	//{{AFX_DATA(CUsbHostDlg) 
	enum { IDD = IDD_USBHOST_DIALOG }; 
	CEdit	m_RecvData; 
	CEdit	m_SendData; 
	CStatic	m_Status; 
	CComboBox	m_BaudRate; 
	//}}AFX_DATA 
 
	// ClassWizard generated virtual function overrides 
	//{{AFX_VIRTUAL(CUsbHostDlg) 
	protected: 
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support 
	//}}AFX_VIRTUAL 
 
// Implementation 
protected: 
	int m_MaxDataLen; 
	CString m_RecvDataTotal; 
	HANDLE m_HidHandle; 
	HICON m_hIcon; 
 
	// Generated message map functions 
	//{{AFX_MSG(CUsbHostDlg) 
	virtual BOOL OnInitDialog(); 
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam); 
	afx_msg void OnPaint(); 
	afx_msg HCURSOR OnQueryDragIcon(); 
	afx_msg void OnLink(); 
	afx_msg void OnCut(); 
	afx_msg void OnOpen(); 
	afx_msg void OnSend(); 
	afx_msg void OnClearOpen(); 
	afx_msg LONG OnReceivedData(UINT wParam, LONG lParam); 
	afx_msg void OnSave(); 
	afx_msg void OnClearSave(); 
	afx_msg void OnChangeRecvData(); 
	afx_msg void OnSelchangeBaudrate(); 
	afx_msg void OnEnd(); 
	//}}AFX_MSG 
	DECLARE_MESSAGE_MAP() 
}; 
 
//{{AFX_INSERT_LOCATION}} 
// Microsoft Visual C++ will insert additional declarations immediately before the previous line. 
 
#endif // !defined(AFX_USBHOSTDLG_H__AB341167_800F_11D7_A300_5254AB177364__INCLUDED_) 

