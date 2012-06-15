
// UsbLockerDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "UsbLocker.h"
#include "UsbLockerDlg.h"
#include "winioctl.h"    //����windows�µ��豸��������һ����DDK���õ��Ŀ⡣

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//========�Զ���======

//���������
#define IOCTL_SETKEY CTL_CODE(\
	FILE_DEVICE_UNKNOWN,\
	0x801,\
	METHOD_BUFFERED,\
	FILE_ANY_ACCESS)

//�����豸�����ṹ��
typedef struct _DEVICE_PARAM{
	PWCHAR szKeyName;//��¼��ѡ�е��豸��
	ULONG uFlag;  //��־,1��ʾ��д������0��ʾ��д����
}DEVICE_PARAM,*PDEVICE_PARAM;


//������Ӧ��Ȩ��Ȩ
BOOL EnablePrivilege(LPCWSTR lpName, BOOL fEnable)
{
	HANDLE hObject;
	LUID Luid;

	//TOKEN_PRIVILEGES�ṹ�����LUID����Ȩ�����Ժ�Ԫ�ظ���
	TOKEN_PRIVILEGES NewStatus;

	//��Ҫ�޸ķ������Ƶ���Ȩ��ָ���ڶ�������ΪTOKEN_ADJUST_PRIVILEGES
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hObject))
		return FALSE;

	//�ú����鿴ϵͳȨ�޵���Ȩֵ��������Ϣ��һ��LUID�ṹ����,NULL��ʾ�鿴����ϵͳ��Ҫ�鿴lpName��Ȩ����ֵ������Luid��
	if (LookupPrivilegeValue(NULL, lpName, &Luid))
	{
		NewStatus.Privileges[0].Luid = Luid;
		NewStatus.PrivilegeCount = 1;

		//ͨ������Ȩ���Ը�ֵΪ SE_PRIVILEGE_ENABLED�� ʹ��Ȩ����.
		NewStatus.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;

		//���û��ֹ ָ���������Ƶ���Ȩ��
		AdjustTokenPrivileges(hObject, FALSE, &NewStatus, 0, 0, 0);

		CloseHandle(hObject);
		return TRUE;
	}

	return FALSE;
}



// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CUsbLockerDlg �Ի���




CUsbLockerDlg::CUsbLockerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUsbLockerDlg::IDD, pParent)
, m_ComboVal(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUsbLockerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//=======================
	//DDX_Control���������������б���ֵΪ��Ͽ�����ݡ�
	DDX_Control(pDX, IDC_COMBO1, m_ComboCtrl);

	//DDX_CBString������ʱ��m_ComboVal������Ϊ��Ͽ�ĵ�ǰѡ��
	DDX_CBString(pDX, IDC_COMBO1, m_ComboVal);
}

BEGIN_MESSAGE_MAP(CUsbLockerDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_COMBO1, &CUsbLockerDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDCANCEL, &CUsbLockerDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &CUsbLockerDlg::OnBnClickedOk)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CUsbLockerDlg ��Ϣ�������

BOOL CUsbLockerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��



	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	//Ҫ��һ��������̽���ָ����д��صķ���Ȩ��OpenProcess������ֻҪ��ǰ���̾���SeDeDebugȨ�޾Ϳ�����
	EnablePrivilege(TEXT("SeDebugPrivilege"),TRUE);

	//�����ע���ָ��ľ��
	HKEY hKEY(NULL);

	//HKEY_LOCAL_MACHINE/SYSTEM/CurrentControlSet/Enum/USBSTOR ����¼��U�̲����Ϣ
	LPCTSTR data_Set=L"SYSTEM\\CurrentControlSet\\Enum\\USBSTOR\\";

	//��һ��ָ����ע����
	::RegOpenKeyEx(HKEY_LOCAL_MACHINE,data_Set, 0, KEY_READ, &hKEY);
	if(!hKEY)
	{
		MessageBox(L"��ȡUSB�豸�б����!",L"����",MB_ICONERROR|MB_OK);
		PostQuitMessage(0);
	}

	//����õ���ע���
	TCHAR LpName[50];
	//����������
	DWORD   cbSize=50;
	DWORD   index=0;

	while(RegEnumKey(hKEY,index,LpName,cbSize)!=ERROR_NO_MORE_ITEMS)
	{
		index++;
		//���õ�������ע���������ӵ��������Ͽ���
		m_ComboCtrl.AddString(LpName);
	}


	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CUsbLockerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CUsbLockerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CUsbLockerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CUsbLockerDlg::OnCbnSelchangeCombo1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}


void CUsbLockerDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	//UpdataData�����ڲ�������DoDataExchange������UpdateData(TRUE)�����ݴӶԻ���Ŀؼ��д��͵���Ӧ�����ݳ�Ա�У�
	//����UpdateData(FALSE)�����ݴ����ݳ�Ա�д��͸���Ӧ�Ŀؼ�
	UpdateData(TRUE);
	HANDLE hFile(NULL); //�������,ָ�򴴽�����Դ

	//CreatFile�����Զ�д��ʽ��InstFilters����Դ��������ָ����Щ��Դ�ľ��
	hFile=CreateFile(L"\\\\.\\InstFilters",GENERIC_READ|GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile==INVALID_HANDLE_VALUE){
		MessageBox(L"�� InstFilters �ļ�ʧ��!",L"ERROR",MB_OK|MB_ICONSTOP);
		PostQuitMessage(0);
	}

	DEVICE_PARAM DeviceParam;
	DeviceParam.szKeyName=new WCHAR[40];
	//��д��������־��Ӧ����Ϊ1
	DeviceParam.uFlag=1;

	//ͨ��GetBuffer����һ���ڴ�ŵ�szKeyName�У�����ѡ��USB�豸�����ݸ�ֵ��szKeyName
	wcscpy(DeviceParam.szKeyName,m_ComboVal.GetBuffer(m_ComboVal.GetLength()+2));

	//��¼������ݵ�ʵ�ʳ���
	DWORD dwFuck;

	//���������ָ��
	WCHAR OutputBuffer[20];

	//�����豸ִ��ָ���Ĳ���,�����InstFilters���DDKDeviceIoControl
	BOOL bRet=DeviceIoControl(hFile, IOCTL_SETKEY,&DeviceParam,20,&OutputBuffer,20,&dwFuck,NULL);

	if(bRet)
		MessageBox(L"��д�����ɹ�!",L"^_^",MB_OK);
	else
		MessageBox(L"��д����ʧ��!",L"-_-||",MB_OK);

	//�رվ��
	CloseHandle(hFile);
}

void CUsbLockerDlg::OnBnClickedCancel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	HANDLE hFile(NULL);

	hFile=CreateFile(L"\\\\.\\InstFilters",GENERIC_READ|GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile==INVALID_HANDLE_VALUE){
		MessageBox(L"�� InstFilters �ļ�ʧ��!",L"ERROR",MB_OK|MB_ICONSTOP);
		PostQuitMessage(0);
	}

	DEVICE_PARAM DeviceParam;
	DeviceParam.szKeyName=new WCHAR[40];
	DeviceParam.uFlag=0;
	wcscpy(DeviceParam.szKeyName,m_ComboVal.GetBuffer(m_ComboVal.GetLength()+2));

	DWORD dwFuck;
	WCHAR OutputBuffer[20];

	//�����InstFilters�ﶨ���DDKDeviceIoControl
	BOOL bRet=DeviceIoControl(hFile, IOCTL_SETKEY,&DeviceParam,20,&OutputBuffer,20,&dwFuck,NULL);

	if(bRet)
		MessageBox(L"��д�����ɹ�!",L"^_^",MB_OK);
	else
		MessageBox(L"��д����ʧ��!",L"-_-||",MB_OK);

	CloseHandle(hFile);
}

void CUsbLockerDlg::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	PostQuitMessage(0);

}

