#pragma once
// WindParams.h : header file
//

#include "ViewParams.h"

#define WIND_PARAMS_PAGE_INCREMENT 128

#define WND_NUM_COLORS  256


/////////////////////////////////////////////////////////////////////////////
// CWindowDlg dialog

class CWindowDlg : public CDialog
{
// Construction
public:
	CWindowDlg(CMetaData *pData, CWindowParams *pWindParams);   // standard constructor
	~CWindowDlg(void);

	static CWindowDlg *umpCurrentDlg;
	static CWnd       *umpImageViewWindow;
	static void SetImageView(CWnd *pImageViewWindow);
	static int umDebug;


    //void set_on (CWindowParams *pWindParams, int *pOn);
    void update();
// Dialog Data
	//{{AFX_DATA(CWindowDlg)
	enum { IDD = IDD_WINDPARAMS_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWindowDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWindowDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillfocusEditWidth();
	afx_msg void OnKillfocusEditCenter();
	afx_msg void OnPaint();
	//virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CMetaData     *mpData;
    CWindowParams *mpWindowParams;
    CBitmap       *mpBitMap;
    FILE          *mpfLog;
	float mSavedCenter;
	float mSaveWidth;

    void UpdateWidthDisplay(void);
    void UpdateCenterDisplay(void);
    void SetWindowText (int id, const char *zText);
    float GetWindowValue (int id);
    void on_width_scroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    void on_center_scroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    void draw_bitmap(CDC &dc, RECT *pRect);
};

