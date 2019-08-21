#if !defined(AFX_STATISTICS_H__8F15E05C_BCC1_4FCD_B88A_5E2B5DC2A748__INCLUDED_)
#define AFX_STATISTICS_H__8F15E05C_BCC1_4FCD_B88A_5E2B5DC2A748__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "resource.h"
#include "ViewParams.h"
// Statistics.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatistics dialog

class CStatistics : public CDialog
{
// Construction
public:
	CStatistics(class CImageView *pImageView, SCommonParams *pCommonParams,
              CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CStatistics)
	enum { IDD = IDD_STATISTICS_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatistics)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStatistics)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusPrecedits();
	afx_msg void OnDeltaposPrecspins(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  int mRowFrom;
  int mRowTo;
  int mColFrom;
  int mColTo;

  float mMin;
  float mMax;
  float mAver;
  float mStdDev;

  int mPrecision;

  SCommonParams *mpCommonParams;
  class CImageView *mpImageView;

  void calculate();
  void get_params();
  void results_out();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATISTICS_H__8F15E05C_BCC1_4FCD_B88A_5E2B5DC2A748__INCLUDED_)
