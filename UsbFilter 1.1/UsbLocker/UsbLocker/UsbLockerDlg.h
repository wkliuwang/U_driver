
// UsbLockerDlg.h : ͷ�ļ�
//

#pragma once


// CUsbLockerDlg �Ի���
class CUsbLockerDlg : public CDialog
{
// ����
public:
	CUsbLockerDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_USBLOCKER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//============================
	 
	//����һ��Windows��Ͽ�
	CComboBox m_ComboCtrl;
	//�����ַ���
	CString m_ComboVal;

	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnClose();
};
