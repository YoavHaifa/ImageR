#if !defined(AFX_GOTOPAGE_H__17E50920_E539_446E_B250_96186368C522__INCLUDED_)
#define AFX_GOTOPAGE_H__17E50920_E539_446E_B250_96186368C522__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GoToPage.h : header file
//
#include "resource.h"
#include "ImageViewDefs.h"

/////////////////////////////////////////////////////////////////////////////
// CGoToPage dialog

class CGoToPage : public CDialog
{
// Construction
public:
	CGoToPage(class CMetaData *pData);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGoToPage)
	enum { IDD = IDD_GOTOPAGE_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGoToPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGoToPage)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	class CMetaData *mpData;
	class CPosition *mpPosition;
	class CArchivesFile *mpArchivesFile;
public:
	bool mbChanged;
	bool mbUseImageNumbers;
	int miFirstImage;
	int miSecondImage;
	int mStep;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOTOPAGE_H__17E50920_E539_446E_B250_96186368C522__INCLUDED_)
