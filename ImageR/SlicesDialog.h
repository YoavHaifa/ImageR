#pragma once
#include "afxwin.h"
#include "resource.h"


// CSlicesDialog dialog

class CSlicesDialog : public CDialog
{
	DECLARE_DYNAMIC(CSlicesDialog)

public:
	CSlicesDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSlicesDialog();

// Dialog Data
	enum { IDD = IDD_SLICESDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    BOOL OnInitDialog();
    void OnOK();
    void set_text (int id, int value);
    int get_value (int id);

	DECLARE_MESSAGE_MAP()
public:
    int mnSlice;
    int miSlice;
    int mlHeaderBytes;
    int mlSlice;
    int mnFocalSpots;
    int miFocalSpot;
    //CButton m_VReadings;
    unsigned int mbViewReadings;
};
