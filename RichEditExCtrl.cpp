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

#include "stdafx.h"
#include "RichEditExCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


_AFX_RICHEDITEX_STATE::_AFX_RICHEDITEX_STATE()
{
	m_hInstRichEdit20 = NULL ;
}

_AFX_RICHEDITEX_STATE::~_AFX_RICHEDITEX_STATE()
{
	if( m_hInstRichEdit20 != NULL )
	{
		::FreeLibrary( m_hInstRichEdit20 ) ;
	}
}

_AFX_RICHEDITEX_STATE _afxRichEditStateEx ;

BOOL PASCAL AfxInitRichEditEx()
{
	if( ! ::AfxInitRichEdit() )
	{
		return FALSE ;
	}

	_AFX_RICHEDITEX_STATE* l_pState = &_afxRichEditStateEx ;

	if( l_pState->m_hInstRichEdit20 == NULL )
	{
		l_pState->m_hInstRichEdit20 = LoadLibraryA( "RICHED20.DLL" ) ;
	}

	return l_pState->m_hInstRichEdit20 != NULL ;
}

/////////////////////////////////////////////////////////////////////////////
// CRichEditExCtrl

CRichEditExCtrl::CRichEditExCtrl()
{
}

CRichEditExCtrl::~CRichEditExCtrl()
{
}


BEGIN_MESSAGE_MAP(CRichEditExCtrl, CRichEditCtrl)
	//{{AFX_MSG_MAP(CRichEditExCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRichEditExCtrl message handlers


BOOL CRichEditExCtrl::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, 
							 DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, 
							 UINT nID, CCreateContext* pContext) 
{

	
	if (!AfxInitRichEditEx())
		return FALSE;
	
	AfxMessageBox(".-)");
	CWnd* pWnd = this;
	return pWnd->Create(_T("RichEdit20A"), NULL, dwStyle, rect, pParentWnd, nID);

	//return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}
