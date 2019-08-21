#pragma once
#include "resource.h"

// CSelectParamsDialog dialog

class CSelectParamsDialog : public CDialog
{
	DECLARE_DYNAMIC(CSelectParamsDialog)

public:
	CSelectParamsDialog(CWnd* pParent = NULL, const char *zfName = NULL);   // standard constructor
	virtual ~CSelectParamsDialog();

// Dialog Data
	enum { IDD = IDD_SEL_PARAMS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

    typedef struct SParamData
    {
        CString msName;
        int mValue;
        int mMin;
        int mMax;
        CWnd *mpWnd;

        void SetValueText(void);
    } SParamData;

    static const int umMaxParams = 10;
    SParamData maParams[umMaxParams];
    int mnParams;

    bool AnalyzeParameters(void);
    int AnalyzeParameter(const char *zText);
    bool ComposeName (void);
    bool OnSpin (SParamData &param, int iDelta);

public:
    CString msFileName;
    CString msFileNamePrefix;
    CString msFileNamePostfix;
    afx_msg void OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDeltaposSpin2(NMHDR *pNMHDR, LRESULT *pResult);
    class CMultiFile *mpMultiFile;
    afx_msg void OnDeltaposSpin3(NMHDR *pNMHDR, LRESULT *pResult);
};
