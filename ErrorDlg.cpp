// ErrorDlg.cpp : implementation file
//

#include "stdafx.h"
//#include "Metis.h"
#include "ErrorDlg.h"
#include ".\errordlg.h"

extern CString g_strWd;
// CErrorDlg dialog

IMPLEMENT_DYNAMIC(CErrorDlg, CDialog)
CErrorDlg::CErrorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CErrorDlg::IDD, pParent)
	, m_strErrors(_T(""))
{
}

CErrorDlg::~CErrorDlg()
{
}

void CErrorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ERRORS, m_strErrors);
}


BEGIN_MESSAGE_MAP(CErrorDlg, CDialog)
	ON_BN_CLICKED(IDC_METIS_HELP, OnBnClickedMetisHelp)
END_MESSAGE_MAP()


// CErrorDlg message handlers

BOOL CErrorDlg::OnInitDialog(void)
{

	CDialog::OnInitDialog();

	UpdateData(FALSE);
	return TRUE;
}

void CErrorDlg::OnBnClickedMetisHelp()
{
	
	ShellExecute(NULL, _T("open"), g_strWd + _T("\\docs\\Metis.chm"), NULL, NULL, SW_SHOW);
}
