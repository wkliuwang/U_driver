
// UsbLockerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "UsbLocker.h"
#include "UsbLockerDlg.h"
#include "winioctl.h"    //开发windows下的设备驱动程序（一般是DDK）用到的库。

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//========自定义======

//定义控制码
#define IOCTL_SETKEY CTL_CODE(\
	FILE_DEVICE_UNKNOWN,\
	0x801,\
	METHOD_BUFFERED,\
	FILE_ANY_ACCESS)

//定义设备参数结构体
typedef struct _DEVICE_PARAM{
	PWCHAR szKeyName;//记录被选中的设备名
	ULONG uFlag;  //标志,1表示开写保护，0表示关写保护
}DEVICE_PARAM,*PDEVICE_PARAM;


//启用相应特权特权
BOOL EnablePrivilege(LPCWSTR lpName, BOOL fEnable)
{
	HANDLE hObject;
	LUID Luid;

	//TOKEN_PRIVILEGES结构体包括LUID和特权的属性和元素个数
	TOKEN_PRIVILEGES NewStatus;

	//如要修改访问令牌的特权，指定第二个参数为TOKEN_ADJUST_PRIVILEGES
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hObject))
		return FALSE;

	//该函数查看系统权限的特权值，返回信息到一个LUID结构体里,NULL表示查看本地系统，要查看lpName特权返回值保存在Luid中
	if (LookupPrivilegeValue(NULL, lpName, &Luid))
	{
		NewStatus.Privileges[0].Luid = Luid;
		NewStatus.PrivilegeCount = 1;

		//通过将特权属性赋值为 SE_PRIVILEGE_ENABLED， 使特权启用.
		NewStatus.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;

		//启用或禁止 指定访问令牌的特权。
		AdjustTokenPrivileges(hObject, FALSE, &NewStatus, 0, 0, 0);

		CloseHandle(hObject);
		return TRUE;
	}

	return FALSE;
}



// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CUsbLockerDlg 对话框




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
	//DDX_Control函数管理着下拉列表被赋值为组合框的数据。
	DDX_Control(pDX, IDC_COMBO1, m_ComboCtrl);

	//DDX_CBString被调用时，m_ComboVal被设置为组合框的当前选择。
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


// CUsbLockerDlg 消息处理程序

BOOL CUsbLockerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标



	// TODO: 在此添加额外的初始化代码

	//要对一个任意进程进行指定了写相关的访问权的OpenProcess操作，只要当前进程具有SeDeDebug权限就可以了
	EnablePrivilege(TEXT("SeDebugPrivilege"),TRUE);

	//定义打开注册表指向的句柄
	HKEY hKEY(NULL);

	//HKEY_LOCAL_MACHINE/SYSTEM/CurrentControlSet/Enum/USBSTOR 处记录这U盘插拔信息
	LPCTSTR data_Set=L"SYSTEM\\CurrentControlSet\\Enum\\USBSTOR\\";

	//打开一个指定的注册表键
	::RegOpenKeyEx(HKEY_LOCAL_MACHINE,data_Set, 0, KEY_READ, &hKEY);
	if(!hKEY)
	{
		MessageBox(L"获取USB设备列表出错!",L"警告",MB_ICONERROR|MB_OK);
		PostQuitMessage(0);
	}

	//保存得到的注册表
	TCHAR LpName[50];
	//缓冲区长度
	DWORD   cbSize=50;
	DWORD   index=0;

	while(RegEnumKey(hKEY,index,LpName,cbSize)!=ERROR_NO_MORE_ITEMS)
	{
		index++;
		//将得到的所有注册表名称添加到定义的组合框中
		m_ComboCtrl.AddString(LpName);
	}


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CUsbLockerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CUsbLockerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CUsbLockerDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CUsbLockerDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	//UpdataData函数内部调用了DoDataExchange。调用UpdateData(TRUE)将数据从对话框的控件中传送到对应的数据成员中，
	//调用UpdateData(FALSE)则将数据从数据成员中传送给对应的控件
	UpdateData(TRUE);
	HANDLE hFile(NULL); //声明句柄,指向创建的资源

	//CreatFile函数以读写方式打开InstFilters的资源，并返回指向这些资源的句柄
	hFile=CreateFile(L"\\\\.\\InstFilters",GENERIC_READ|GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile==INVALID_HANDLE_VALUE){
		MessageBox(L"打开 InstFilters 文件失败!",L"ERROR",MB_OK|MB_ICONSTOP);
		PostQuitMessage(0);
	}

	DEVICE_PARAM DeviceParam;
	DeviceParam.szKeyName=new WCHAR[40];
	//开写保护，标志相应设置为1
	DeviceParam.uFlag=1;

	//通过GetBuffer申请一段内存放到szKeyName中，将被选中USB设备的内容赋值给szKeyName
	wcscpy(DeviceParam.szKeyName,m_ComboVal.GetBuffer(m_ComboVal.GetLength()+2));

	//记录输出数据的实际长度
	DWORD dwFuck;

	//输出缓冲区指针
	WCHAR OutputBuffer[20];

	//　对设备执行指定的操作,会调用InstFilters里的DDKDeviceIoControl
	BOOL bRet=DeviceIoControl(hFile, IOCTL_SETKEY,&DeviceParam,20,&OutputBuffer,20,&dwFuck,NULL);

	if(bRet)
		MessageBox(L"开写保护成功!",L"^_^",MB_OK);
	else
		MessageBox(L"开写保护失败!",L"-_-||",MB_OK);

	//关闭句柄
	CloseHandle(hFile);
}

void CUsbLockerDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	HANDLE hFile(NULL);

	hFile=CreateFile(L"\\\\.\\InstFilters",GENERIC_READ|GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile==INVALID_HANDLE_VALUE){
		MessageBox(L"打开 InstFilters 文件失败!",L"ERROR",MB_OK|MB_ICONSTOP);
		PostQuitMessage(0);
	}

	DEVICE_PARAM DeviceParam;
	DeviceParam.szKeyName=new WCHAR[40];
	DeviceParam.uFlag=0;
	wcscpy(DeviceParam.szKeyName,m_ComboVal.GetBuffer(m_ComboVal.GetLength()+2));

	DWORD dwFuck;
	WCHAR OutputBuffer[20];

	//会调用InstFilters里定义的DDKDeviceIoControl
	BOOL bRet=DeviceIoControl(hFile, IOCTL_SETKEY,&DeviceParam,20,&OutputBuffer,20,&dwFuck,NULL);

	if(bRet)
		MessageBox(L"关写保护成功!",L"^_^",MB_OK);
	else
		MessageBox(L"关写保护失败!",L"-_-||",MB_OK);

	CloseHandle(hFile);
}

void CUsbLockerDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	PostQuitMessage(0);

}

