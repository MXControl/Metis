#pragma once
#include "afxcmn.h"

#include "resource.h"

// CSpyDlg dialog

class CSpyDlg : public CDialog
{
	DECLARE_DYNAMIC(CSpyDlg)

public:
	CSpyDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSpyDlg();

// Dialog Data
	enum { IDD = IDD_VARPSY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_wndVars;
	BOOL OnInitDialog(void);

	CMap<CString, LPCSTR, CString, CString&> *m_pVars;
	void LoadVars(void);
	afx_msg void OnBnClickedReload();
};
