// UsbHostDlg.cpp : implementation file   
//   
   
#include "stdafx.h"   
#include "UsbHost.h"   
#include "UsbHostDlg.h"   
#include "process.h"   
   
extern "C"{   
#include "setupapi.h"   
#include "hidsdi.h"   
}     
   
#ifdef _DEBUG   
#define new DEBUG_NEW   
#undef THIS_FILE   
static char THIS_FILE[] = __FILE__;   
#endif   
   
#define USB_VID    (0xXXXX)         // XXXX-十六进制数，需更改。   
#define USB_PID    (0xXXXX)         // XXXX-十六进制数，需更改。   
#define USBRS232_VERSION (0x0001)   
   
#define RID_RECEIVE     (1)   
#define RID_TRANSMIT    (2)   
#define RID_COMMAND     (3)   
#define MAX_DATA_LEN    (6)   
#define RECV_DATA_LEN   (8)   
   
struct Rate {   
    char *String;   
    BYTE parameter;    
} RateTable[] = {   
        "default",  0,    
        "2400",     1,   
        "4800",     2,   
        "9600",     3,   
        "19200",    4,   
        "38400",    5,   
        "57600",    6   
};   
#define NRATES (sizeof RateTable / sizeof RateTable[0])   
   
#define IDM_RECV_DATA    (WM_USER + 1)   
bool g_KeepGoing = false;   
void __cdecl RecvThreadFunction(HANDLE hidHandle);   
   
/////////////////////////////////////////////////////////////////////////////   
// CAboutDlg dialog used for App About   
   
class CAboutDlg : public CDialog   
{   
public:   
    CAboutDlg();   
   
// Dialog Data   
    //{{AFX_DATA(CAboutDlg)   
    enum { IDD = IDD_ABOUTBOX };   
    //}}AFX_DATA   
   
    // ClassWizard generated virtual function overrides   
    //{{AFX_VIRTUAL(CAboutDlg)   
    protected:   
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support   
    //}}AFX_VIRTUAL   
   
// Implementation   
protected:   
    //{{AFX_MSG(CAboutDlg)   
    //}}AFX_MSG   
    DECLARE_MESSAGE_MAP()   
};   
   
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)   
{   
    //{{AFX_DATA_INIT(CAboutDlg)   
    //}}AFX_DATA_INIT   
}   
   
void CAboutDlg::DoDataExchange(CDataExchange* pDX)   
{   
    CDialog::DoDataExchange(pDX);   
    //{{AFX_DATA_MAP(CAboutDlg)   
    //}}AFX_DATA_MAP   
}   
   
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)   
    //{{AFX_MSG_MAP(CAboutDlg)   
        // No message handlers   
    //}}AFX_MSG_MAP   
END_MESSAGE_MAP()   
   
/////////////////////////////////////////////////////////////////////////////   
// CUsbHostDlg dialog   
   
CUsbHostDlg::CUsbHostDlg(CWnd* pParent /*=NULL*/)   
    : CDialog(CUsbHostDlg::IDD, pParent)   
{   
    //{{AFX_DATA_INIT(CUsbHostDlg)   
        // NOTE: the ClassWizard will add member initialization here   
    //}}AFX_DATA_INIT   
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32   
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);   
    m_HidHandle = INVALID_HANDLE_VALUE;   
    m_MaxDataLen = MAX_DATA_LEN;   
}   
   
void CUsbHostDlg::DoDataExchange(CDataExchange* pDX)   
{   
    CDialog::DoDataExchange(pDX);   
    //{{AFX_DATA_MAP(CUsbHostDlg)   
    DDX_Control(pDX, IDC_RECV_DATA, m_RecvData);   
    DDX_Control(pDX, IDC_SEND_DATA, m_SendData);   
    DDX_Control(pDX, IDC_STATUS, m_Status);   
    DDX_Control(pDX, IDC_BAUDRATE, m_BaudRate);   
    //}}AFX_DATA_MAP   
}   
   
BEGIN_MESSAGE_MAP(CUsbHostDlg, CDialog)   
    //{{AFX_MSG_MAP(CUsbHostDlg)   
    ON_WM_SYSCOMMAND()   
    ON_WM_PAINT()   
    ON_WM_QUERYDRAGICON()   
    ON_BN_CLICKED(IDC_LINK, OnLink)   
    ON_BN_CLICKED(IDC_CUT, OnCut)   
    ON_BN_CLICKED(IDC_OPEN, OnOpen)   
    ON_BN_CLICKED(IDC_SEND, OnSend)   
    ON_BN_CLICKED(IDC_CLEAROPEN, OnClearOpen)   
    ON_MESSAGE(IDM_RECV_DATA, OnReceivedData)   
    ON_BN_CLICKED(IDC_SAVE, OnSave)   
    ON_BN_CLICKED(IDC_CLEARSAVE, OnClearSave)   
    ON_EN_CHANGE(IDC_RECV_DATA, OnChangeRecvData)   
    ON_CBN_SELCHANGE(IDC_BAUDRATE, OnSelchangeBaudrate)   
    ON_BN_CLICKED(IDC_END, OnEnd)   
    //}}AFX_MSG_MAP   
END_MESSAGE_MAP()   
   
/////////////////////////////////////////////////////////////////////////////   
// CUsbHostDlg message handlers   
   
BOOL CUsbHostDlg::OnInitDialog()   
{   
    CDialog::OnInitDialog();   
   
    // Add "About..." menu item to system menu.   
   
    // IDM_ABOUTBOX must be in the system command range.   
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);   
    ASSERT(IDM_ABOUTBOX  0xF000);   
   
    CMenu* pSysMenu = GetSystemMenu(FALSE);   
    if (pSysMenu != NULL)   
    {   
        CString strAboutMenu;   
        strAboutMenu.LoadString(IDS_ABOUTBOX);   
        if (!strAboutMenu.IsEmpty())   
        {   
            pSysMenu->AppendMenu(MF_SEPARATOR);   
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);   
        }   
    }   
   
    // Set the icon for this dialog.  The framework does this automatically   
    //  when the application's main window is not a dialog   
    SetIcon(m_hIcon, TRUE);         // Set big icon   
    SetIcon(m_hIcon, FALSE);        // Set small icon   
       
    // TODO: Add extra initialization here   
    char i;   
    for (i= 0; i NRATES; i++)    
    {   
        m_BaudRate.AddString(RateTable[i].String);   
    }         
    m_BaudRate.SelectString(-1,"default");   
       
    return TRUE;  // return TRUE  unless you set the focus to a control   
}   
   
void CUsbHostDlg::OnSysCommand(UINT nID, LPARAM lParam)   
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
   
// If you add a minimize button to your dialog, you will need the code below   
//  to draw the icon.  For MFC applications using the document/view model,   
//  this is automatically done for you by the framework.   
   
void CUsbHostDlg::OnPaint()    
{   
    if (IsIconic())   
    {   
        CPaintDC dc(this); // device context for painting   
   
        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);   
   
        // Center icon in client rectangle   
        int cxIcon = GetSystemMetrics(SM_CXICON);   
        int cyIcon = GetSystemMetrics(SM_CYICON);   
        CRect rect;   
        GetClientRect(&rect);   
        int x = (rect.Width() - cxIcon + 1) / 2;   
        int y = (rect.Height() - cyIcon + 1) / 2;   
   
        // Draw the icon   
        dc.DrawIcon(x, y, m_hIcon);   
    }   
    else   
    {   
        CDialog::OnPaint();   
    }   
}   
   
// The system calls this to obtain the cursor to display while the user drags   
//  the minimized window.   
HCURSOR CUsbHostDlg::OnQueryDragIcon()   
{   
    return (HCURSOR) m_hIcon;   
}   
   
void CUsbHostDlg::OnLink()    
{   
    // TODO: Add your control notification handler code here   
    if(m_HidHandle != INVALID_HANDLE_VALUE)   
    {   
        m_Status.SetWindowText("RS232-Usb设备已连接!");   
        return;   
    }   
   
    GUID hidGuid;   
   
    HidD_GetHidGuid(&hidGuid);   
   
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&hidGuid,   
                                            NULL,   
                                            NULL,   
                                            (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));    
    if(!hDevInfo)   
    {   
        m_Status.SetWindowText("获取HID设备信息失败!");   
        return;   
    }   
   
    SP_DEVICE_INTERFACE_DATA devInfoData;   
    devInfoData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);   
    int deviceNo = 0;   
   
    SetLastError(NO_ERROR);   
   
    while(GetLastError() != ERROR_NO_MORE_ITEMS)   
    {   
        if(SetupDiEnumInterfaceDevice (hDevInfo,   
                                       0,    
                                       &hidGuid,   
                                       deviceNo,   
                                       &devInfoData))   
        {   
            ULONG  requiredLength = 0;   
            SetupDiGetInterfaceDeviceDetail(hDevInfo,   
                                            &devInfoData,   
                                            NULL,    
                                            0,   
                                            &requiredLength,   
                                            NULL);   
   
            PSP_INTERFACE_DEVICE_DETAIL_DATA devDetail = (SP_INTERFACE_DEVICE_DETAIL_DATA*) malloc (requiredLength);   
            devDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);   
   
            if(! SetupDiGetInterfaceDeviceDetail(hDevInfo,   
                                                 &devInfoData,   
                                                 devDetail,   
                                                 requiredLength,   
                                                 NULL,   
                                                 NULL))    
            {   
                AfxMessageBox("获取HID设备细节信息失败!");   
                free(devDetail);   
                return;   
            }   
   
            HANDLE hidHandle = CreateFile(devDetail->DevicePath,   
                                          GENERIC_READ | GENERIC_WRITE,   
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,   
                                          NULL,    
                                          OPEN_EXISTING,            
                                          FILE_FLAG_OVERLAPPED,    
                                          NULL);   
            free(devDetail);   
   
            if(hidHandle==INVALID_HANDLE_VALUE)   
            {   
                AfxMessageBox("获取HID设备句柄失败!");   
                return;   
            }   
   
            _HIDD_ATTRIBUTES hidAttributes;   
            if(!HidD_GetAttributes(hidHandle, &hidAttributes))   
            {   
                AfxMessageBox("获取HID设备属性失败!");   
                CloseHandle(hidHandle);   
                return;   
            }   
   
            if(USB_VID == hidAttributes.VendorID &&   
               USB_PID  == hidAttributes.ProductID &&   
               USBRS232_VERSION == hidAttributes.VersionNumber)   
   
            {   
                CString version;   
                version.Format("RS232-USB设备已连接, ver%X", hidAttributes.VersionNumber);   
                m_Status.SetWindowText(version);   
   
                m_HidHandle = hidHandle;   
   
                m_MaxDataLen = MAX_DATA_LEN;   
                   
                g_KeepGoing = true;   
                if(_beginthread(RecvThreadFunction, 0, m_HidHandle)  0)   
                {   
                    AfxMessageBox("启动接收数据线程失败!");   
                }   
                break;   
            }   
            else   
            {   
                CloseHandle(hidHandle);   
                ++deviceNo;   
            }   
        }   
    }   
   
    if(m_HidHandle == INVALID_HANDLE_VALUE)   
       m_Status.SetWindowText("RS232-USB设备未连接!");   
   
    SetupDiDestroyDeviceInfoList(hDevInfo);   
}   
   
void CUsbHostDlg::OnCut()    
{   
    // TODO: Add your control notification handler code here   
    if(m_HidHandle != INVALID_HANDLE_VALUE)   
    {   
        g_KeepGoing = false;    
   
        CloseHandle(m_HidHandle);   
   
        m_HidHandle = INVALID_HANDLE_VALUE;   
   
        m_Status.SetWindowText("RS232-USB设备已断开!");   
    }   
    else   
    {   
        m_Status.SetWindowText("RS232-USB设备未连接!");   
    }   
}   
   
void CUsbHostDlg::OnOpen()   
{   
    // TODO: Add your control notification handler code here   
    CFileDialog OpenDlg(TRUE,"txt","*.txt");   
    if (OpenDlg.DoModal()==IDOK)   
    {   
        CFile OpenFile;   
        if(!OpenFile.Open(OpenDlg.GetPathName(),CFile::modeRead))   
        {   
            AfxMessageBox("打开文件失败!");   
            return;   
        }   
        else   
        {              
            char* pBuf = new char[8*1024];   
            UINT nBytesRead = OpenFile.Read( pBuf, 8*1024-1 );   
   
            pBuf[nBytesRead]= '\0';   
   
            m_SendData.SetWindowText(pBuf);   
           
            m_SendData.LineScroll(m_SendData.GetLineCount());   
               
            OpenFile.Close();   
            delete pBuf;   
        }   
    }   
}   
   
void CUsbHostDlg::OnSend()   
{   
    // TODO: Add your control notification handler code here   
    if(m_HidHandle == INVALID_HANDLE_VALUE)   
    {   
        m_Status.SetWindowText("RS232-USB设备未连接");   
        return;   
    }   
   
    CString text;   
   
    m_SendData.GetWindowText(text);   
   
    text += "\r\n";   
   
    int len = text.GetLength();   
               
    BYTE* reportBuf = new BYTE[m_MaxDataLen+2];   
   
    reportBuf[0] = RID_TRANSMIT;   
   
    int offset = 0;   
   
    while(len > 0)   
    {   
        int thisLen = min(len, m_MaxDataLen);   
   
        reportBuf[1] = thisLen;   
           
        for(int i = 0; i  thisLen; i++)   
            reportBuf[i + 2] = text[i + offset];   
           
        if(!HidD_SetFeature(m_HidHandle, reportBuf, m_MaxDataLen+2))   
        {   
           AfxMessageBox("发送数据失败!");   
           delete reportBuf;   
           return;   
        }   
   
        len -= m_MaxDataLen;   
        offset += m_MaxDataLen;   
    }   
    delete reportBuf;   
}   
   
   
void CUsbHostDlg::OnClearOpen()    
{   
    // TODO: Add your control notification handler code here   
    m_SendData.SetWindowText("");   
}   
   
void __cdecl RecvThreadFunction(HANDLE hidHandle)   
{   
    char recvDataBuf[RECV_DATA_LEN];   
    CString msg;   
   
    HANDLE hIOWaiter = CreateEvent(NULL, TRUE, FALSE, NULL);   
   
    if(hIOWaiter == NULL)   
        goto exit2;   
   
    OVERLAPPED ol;   
    ol.Offset = 0;   
    ol.OffsetHigh = 0;   
    ol.hEvent = hIOWaiter;   
   
    for(;;)   
    {   
        DWORD recvdBytes;   
        ResetEvent(hIOWaiter);   
        if(!ReadFile(hidHandle, recvDataBuf, RECV_DATA_LEN, &recvdBytes, &ol))   
        {            
            DWORD err = GetLastError();   
            if(err != ERROR_IO_PENDING && err != ERROR_DEVICE_NOT_CONNECTED && g_KeepGoing)   
            {   
                msg.Format("读操作错误: %d", err);   
                AfxMessageBox(msg);   
                goto exit1;   
            }   
   
            while(WaitForSingleObject(hIOWaiter, 100) == WAIT_TIMEOUT)   
            {   
                if(!g_KeepGoing)   
                {   
                    if(hidHandle != INVALID_HANDLE_VALUE)    
                    {   
                        CancelIo(hidHandle);   
                    }   
                    goto exit1;   
                }   
            }   
   
            if(!GetOverlappedResult(hidHandle, &ol, &recvdBytes, false))   
            {    
                err = GetLastError();   
                if(err != ERROR_DEVICE_NOT_CONNECTED && g_KeepGoing)   
                {   
                    msg.Format("GetOverlappedResult例程错误: %d", err);    
                    AfxMessageBox(msg);    
                }   
                continue;   
            }   
        }   
   
        if(recvdBytes  RECV_DATA_LEN)         
        {   
            if(g_KeepGoing)   
            {   
                msg.Format("Only read %d bytes", recvdBytes);    
                AfxMessageBox(msg);   
            }   
            continue;   
        }   
   
        AfxGetMainWnd()->SendMessage(IDM_RECV_DATA, 0, (LPARAM)(recvDataBuf + 1));   
    }   
   
exit1:   
    CloseHandle(hIOWaiter);   
   
exit2:   
    _endthread();   
}   
   
LONG CUsbHostDlg::OnReceivedData(UINT wParam, LONG lParam)   
{   
   
    if(!g_KeepGoing)    
        return (LRESULT) 0;   
   
    int len = *(BYTE*)lParam;   
    char* buf = (char*)(lParam + 1);   
   
    buf[len] = '\0';   
   
    m_RecvDataTotal += buf;   
   
    if(buf[len - 1] == '\r')   
        m_RecvDataTotal += "\n";   
   
    m_RecvData.SetWindowText(m_RecvDataTotal);   
       
    m_RecvData.LineScroll(m_RecvData.GetLineCount());   
   
    return (LRESULT) 0;   
}   
   
void CUsbHostDlg::OnSave()    
{   
    // TODO: Add your control notification handler code here   
    CFileDialog SaveDlg(FALSE,"txt","*.txt");   
    if (SaveDlg.DoModal()==IDOK)   
    {   
        CFile SaveFile;   
        if(!SaveFile.Open(SaveDlg.GetPathName(),CFile::modeCreate|CFile::modeWrite))   
        {   
            AfxMessageBox("打开文件失败!");   
            return;   
        }   
        else   
        {   
            CString text;   
            m_RecvData.GetWindowText(text);   
            int len = text.GetLength();   
   
            char* pBuf = new char[8*1024];   
            if (len>=8*1024) len=8*1024-1;   
   
            for(int i = 0; i  len; i++)   
                pBuf[i] = text[i];               
               
            SaveFile.Write( pBuf, len );   
            SaveFile.Close();   
            delete pBuf;   
        }   
    }   
}   
   
void CUsbHostDlg::OnClearSave()    
{   
    // TODO: Add your control notification handler code here   
    m_RecvData.SetWindowText("");   
    m_RecvDataTotal.Empty();           
}   
   
void CUsbHostDlg::OnChangeRecvData()    
{   
    // TODO: Add your control notification handler code here   
    m_RecvData.GetWindowText(m_RecvDataTotal);   
}   
   
void CUsbHostDlg::OnSelchangeBaudrate()    
{   
    // TODO: Add your control notification handler code here   
    if(m_HidHandle == INVALID_HANDLE_VALUE)   
    {   
        m_Status.SetWindowText("RS232-USB设备未连接!");   
        return;   
    }   
   
    BYTE* reportBuf = new BYTE[3];   
   
    reportBuf[0] = RID_COMMAND;   
   
    int rateindex;   
   
    rateindex = m_BaudRate.GetCurSel();   
   
    reportBuf[1] = RateTable[rateindex].parameter;   
   
    if(!HidD_SetFeature(m_HidHandle, reportBuf, m_MaxDataLen + 2))   
        m_Status.SetWindowText("发送数据失败!");   
    else   
        m_Status.SetWindowText("波特率设置成功!");   
    delete reportBuf;   
}   
   
void CUsbHostDlg::OnEnd()    
{   
    // TODO: Add your control notification handler code here   
    OnCut();   
    OnOK();   
}   

