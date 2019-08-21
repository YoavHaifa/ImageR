#if !defined(AFX_DATAAREA_H__6E46377D_B026_473C_9B7C_29735C25817B__INCLUDED_)
#define AFX_DATAAREA_H__6E46377D_B026_473C_9B7C_29735C25817B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DataArea.h : header file
//
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CDataArea window



class CDataArea : public CStatic
{
// Construction
public:
	CDataArea();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataArea)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDataArea();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDataArea)
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	UINT mKeyboardMsg;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATAAREA_H__6E46377D_B026_473C_9B7C_29735C25817B__INCLUDED_)
