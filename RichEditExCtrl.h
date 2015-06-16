/*
** Copyright (C) 2003-2004 Thees Winkler
**  
4** Metis is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** Metis is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#if !defined(AFX_RICHEDITEXCTRL_H__4714BC01_81C2_4626_919B_262F32C149B5__INCLUDED_)
#define AFX_RICHEDITEXCTRL_H__4714BC01_81C2_4626_919B_262F32C149B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RichEditExCtrl.h : header file
//

class _AFX_RICHEDITEX_STATE
{
public:
    _AFX_RICHEDITEX_STATE();
    virtual ~_AFX_RICHEDITEX_STATE();

    HINSTANCE m_hInstRichEdit20 ;
};

BOOL PASCAL AfxInitRichEditEx();

///////////////////////////////////////////////////////////////////////////////
// CRichEditExCtrl window

class CRichEditExCtrl : public CRichEditCtrl
{
// Construction
public:
	CRichEditExCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRichEditExCtrl)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRichEditExCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CRichEditExCtrl)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

///////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RICHEDITEXCTRL_H__4714BC01_81C2_4626_919B_262F32C149B5__INCLUDED_)
