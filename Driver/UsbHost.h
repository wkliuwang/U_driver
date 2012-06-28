// UsbHost.h : main header file for the USBHOST application 
// 
 
#if !defined(AFX_USBHOST_H__AB341165_800F_11D7_A300_5254AB177364__INCLUDED_) 
#define AFX_USBHOST_H__AB341165_800F_11D7_A300_5254AB177364__INCLUDED_ 
 
#if _MSC_VER > 1000 
#pragma once 
#endif // _MSC_VER > 1000 
 
#ifndef __AFXWIN_H__ 
	#error include 'stdafx.h' before including this file for PCH 
#endif 
 
#include "resource.h"		// main symbols 
 
///////////////////////////////////////////////////////////////////////////// 
// CUsbHostApp: 
// See UsbHost.cpp for the implementation of this class 
// 
 
class CUsbHostApp : public CWinApp 
{ 
public: 
	CUsbHostApp(); 
 
// Overrides 
	// ClassWizard generated virtual function overrides 
	//{{AFX_VIRTUAL(CUsbHostApp) 
	public: 
	virtual BOOL InitInstance(); 
	//}}AFX_VIRTUAL 
 
// Implementation 
 
	//{{AFX_MSG(CUsbHostApp) 
		// NOTE - the ClassWizard will add and remove member functions here. 
		//    DO NOT EDIT what you see in these blocks of generated code ! 
	//}}AFX_MSG 
	DECLARE_MESSAGE_MAP() 
}; 
 
 
///////////////////////////////////////////////////////////////////////////// 
 
//{{AFX_INSERT_LOCATION}} 
// Microsoft Visual C++ will insert additional declarations immediately before the previous line. 
 
#endif // !defined(AFX_USBHOST_H__AB341165_800F_11D7_A300_5254AB177364__INCLUDED_) 

