#if !defined(AFX_ZOOMPARAMS_H__EF427605_3DB2_41E7_A4D6_F04A319BD972__INCLUDED_)
#define AFX_ZOOMPARAMS_H__EF427605_3DB2_41E7_A4D6_F04A319BD972__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ZoomParams.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CZoomParamsDlg dialog

class CZoomParamsDlg : public CDialog
{
// Construction
public:
	CZoomParamsDlg(class CMetaData *pData, class CZoomPanParams *pZoomParams, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CZoomParamsDlg)
	enum { IDD = IDD_ZOOM_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CZoomParamsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CZoomParamsDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  class CZoomPanParams *mpZoomParams;
  class CMetaData *mpData;
public:
    afx_msg void OnBnClickedRadioInterp();
    afx_msg void OnBnClickedRadioDup();
    afx_msg void OnEnKillfocusEditzoom();
	afx_msg void OnBnClickedRadioBicubic();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ZOOMPARAMS_H__EF427605_3DB2_41E7_A4D6_F04A319BD972__INCLUDED_)
