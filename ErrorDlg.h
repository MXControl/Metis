#pragma once
#include "resource.h"

// CErrorDlg dialog

class CErrorDlg : public CDialog
{
	DECLARE_DYNAMIC(CErrorDlg)

public:
	CErrorDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CErrorDlg();

// Dialog Data
	enum { IDD = IDD_ERRORS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_strErrors;
protected:
	BOOL OnInitDialog(void);
public:
	afx_msg void OnBnClickedMetisHelp();
};
