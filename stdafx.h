#if !defined(AFX_STDAFX_H__5ED155A2_847A_45B4_9C44_3994BE255BE6__INCLUDED_)
#define AFX_STDAFX_H__5ED155A2_847A_45B4_9C44_3994BE255BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Windows

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0500	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#include <afxwin.h>         // MFC 
#include <afxext.h>         // MFC
#include <afxdisp.h>        // MFC Automatisierungsklassen

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC 
#include <afxodlgs.h>       // MFC 
#include <afxdisp.h>        // MFC 
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO 
#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h>		// MFC ‚
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC 
#endif // _AFX_NO_AFXCMN_SUPPORT


#pragma comment(lib, "Shlwapi.lib")

#include <afxtempl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 

#endif // !defined(AFX_STDAFX_H__5ED155A2_847A_45B4_9C44_3994BE255BE6__INCLUDED_)
