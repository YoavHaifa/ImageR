#if !defined(AFX_GOTO_H__D08844C6_33FD_4665_9991_B26ED5C131E5__INCLUDED_)
#define AFX_GOTO_H__D08844C6_33FD_4665_9991_B26ED5C131E5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GoTo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGoTo dialog

struct SGoTo
{
  int Row;
  int Col;
};

class CGoTo : public CDialog
{
// Construction
public:
	CGoTo(SGoTo *pGoTo, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGoTo)
	enum { IDD = IDD_GOTO_DIALOG };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGoTo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGoTo)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  SGoTo *mpGoTo;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOTO_H__D08844C6_33FD_4665_9991_B26ED5C131E5__INCLUDED_)
