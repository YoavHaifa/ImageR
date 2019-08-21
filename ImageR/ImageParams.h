#pragma once

#include "ImageViewDefs.h"
#include "ViewParams.h"
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CImageParams dialog

class CImageParams : public CDialog
{
// Construction
public:
	CImageParams(class CMultiDataFile *pData,
               CWnd* pParent = NULL);   // standard constructor

    // Dialog Data
	//{{AFX_DATA(CImageParams)
	enum { IDD = IDD_IMAGEPARAMS_DIALOG };
	//}}AFX_DATA


    // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageParams)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    // Implementation
protected:
	void set_int_param (int idc, int value);
	int get_int_param (int idc, int minValue = 0, int defaultValue = 0);

	// Generated message map functions
	//{{AFX_MSG(CImageParams)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnTry();
	afx_msg void OnCreatePattern();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void get_params();
    void set_params();
    bool check_params();

    float mnPages;
    class CMultiDataFile *mpFile;
    int mnLinesSave;
    int mnColsSave;
    EMyImageDataType meDataTypeSave;

    template <typename T>
    void CreatePattern (T first, T delta, const char *zName);

public:
    int mnLines;
    int mnCols;
    EMyImageDataType meDataType;
    void SetNPages(void);
    ULONGLONG mOffsetBytes;
    ULONGLONG mOffsetBytesSave;
};

