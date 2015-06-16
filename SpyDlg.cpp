// SpyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SpyDlg.h"
#include ".\spydlg.h"


// CSpyDlg dialog

IMPLEMENT_DYNAMIC(CSpyDlg, CDialog)
CSpyDlg::CSpyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpyDlg::IDD, pParent)
{

	m_pVars = NULL;
}

CSpyDlg::~CSpyDlg()
{
}

void CSpyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VARS, m_wndVars);
}


BEGIN_MESSAGE_MAP(CSpyDlg, CDialog)
	ON_BN_CLICKED(IDC_RELOAD, OnBnClickedReload)
END_MESSAGE_MAP()


// CSpyDlg message handlers

BOOL CSpyDlg::OnInitDialog(void)
{

	CDialog::OnInitDialog();

	m_wndVars.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	m_wndVars.InsertColumn(0, _T("Variable name"), LVCFMT_LEFT, 200);
	m_wndVars.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 200);

	LoadVars();

	return TRUE;
}

void CSpyDlg::LoadVars(void)
{

	if(m_pVars){

		m_wndVars.DeleteAllItems();

		CString strVar, strValue;
		POSITION pos = m_pVars->GetStartPosition();
		while(pos){

			m_pVars->GetNextAssoc(pos, strVar, strValue);

			int nPos = m_wndVars.InsertItem(0, strVar, 0);
			m_wndVars.SetItemText(nPos, 1, strValue);
		}
	}
}

void CSpyDlg::OnBnClickedReload()
{

	LoadVars();
}
