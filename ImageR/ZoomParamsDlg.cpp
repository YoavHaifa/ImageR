// ZoomParamsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ZoomParamsDlg.h"
#include "..\..\ImageRLib\ZoomPanParams.h"
#include "MetaData.h"
#include "Screens.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CZoomParamsDlg dialog


CZoomParamsDlg::CZoomParamsDlg(CMetaData *pData, CZoomPanParams *pZoomParams, CWnd* pParent /*=NULL*/)
: CDialog(CZoomParamsDlg::IDD, pParent)
, mpData (pData)
, mpZoomParams (pZoomParams)
{
	//{{AFX_DATA_INIT(CZoomParamsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CZoomParamsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CZoomParamsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CZoomParamsDlg, CDialog)
	//{{AFX_MSG_MAP(CZoomParamsDlg)
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_RADIO_INTERP, OnBnClickedRadioInterp)
    ON_BN_CLICKED(IDC_RADIO_DUP, OnBnClickedRadioDup)
    ON_EN_KILLFOCUS(IDC_EDITZOOM, OnEnKillfocusEditzoom)
	ON_BN_CLICKED(IDC_RADIO_BICUBIC, &CZoomParamsDlg::OnBnClickedRadioBicubic)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CZoomParamsDlg message handlers

BOOL CZoomParamsDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    char zStr[32];

    CEdit *pWnd=(CEdit *)GetDlgItem(IDC_EDITZOOM);
    sprintf_s(zStr, 32, "%.2f", mpZoomParams->GetZoomFactor(mpData));
    pWnd->SetWindowText(zStr);
    pWnd->SetFocus();
    pWnd->SetSel(0, -1);

	if (gScreens.mbEbwMode)
	{
		CWnd *pWnd = GetDlgItem(IDC_RADIO_DISABLE);
		pWnd->ShowWindow(SW_HIDE);
		pWnd = GetDlgItem(IDC_RADIO_DUP);
		pWnd->ShowWindow(SW_HIDE);
		pWnd = GetDlgItem(IDC_RADIO_INTERP);
		pWnd->ShowWindow(SW_HIDE);

		RECT WndRectOK;


		RECT WndRect;
		GetWindowRect(&WndRect);
		MoveWindow(WndRect.left,WndRect.top,180,130);

		pWnd = GetDlgItem(IDOK);
		pWnd->GetWindowRect(&WndRectOK);
		pWnd->MoveWindow(WndRectOK.left-WndRect.left,WndRectOK.top-WndRect.top - 70,
			WndRectOK.right-WndRectOK.left,WndRectOK.bottom-WndRectOK.top);

		pWnd = GetDlgItem(IDCANCEL);
		pWnd->GetWindowRect(&WndRectOK);
		pWnd->MoveWindow(WndRectOK.left-WndRect.left,WndRectOK.top-WndRect.top - 70,
			WndRectOK.right-WndRectOK.left,WndRectOK.bottom-WndRectOK.top);
	}
	else
	{
		CButton *pButton;
		if (mpZoomParams->IsDisabled())
			pButton=(CButton *)GetDlgItem(IDC_RADIO_DISABLE);
		else if (mpZoomParams->IsDuplicate())
			pButton=(CButton *)GetDlgItem(IDC_RADIO_DUP);
		else if (mpZoomParams->IsBiCubic())
			pButton=(CButton *)GetDlgItem(IDC_RADIO_BICUBIC);
		else
			pButton=(CButton *)GetDlgItem(IDC_RADIO_INTERP);
		pButton->SetCheck(1);
	}
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CZoomParamsDlg::OnOK() 
{
    char zStr[16];
    CWnd *pWnd=GetDlgItem(IDC_EDITZOOM);
    pWnd->GetWindowText(zStr, 15);
    mpZoomParams->SetZoomFactor (mpData, (float)atof(zStr));

    CButton *pButton=(CButton *)GetDlgItem(IDC_RADIO_DISABLE);;
    if (pButton->GetCheck())
    {
        mpZoomParams->mbEnableZoom = false;
    }
    else
    {
        CButton *pButton=(CButton *)GetDlgItem(IDC_RADIO_DUP);;
        if (pButton->GetCheck())
            mpZoomParams->SetMode(ZOOM_DUP);
        else
		{
			CButton *pButton=(CButton *)GetDlgItem(IDC_RADIO_BICUBIC);;
			if (pButton->GetCheck())
				mpZoomParams->SetMode(ZOOM_BICUBIC);
			else
	            mpZoomParams->SetMode(ZOOM_BILINEAR);
		}
    }

	CDialog::OnOK();
    gScreens.Refresh();
}

void CZoomParamsDlg::OnBnClickedRadioInterp()
{
    mpZoomParams->mbEnableZoom = true;
    mpZoomParams->SetMode(ZOOM_BILINEAR);
    gScreens.Refresh();
}

void CZoomParamsDlg::OnBnClickedRadioDup()
{
    mpZoomParams->mbEnableZoom = true;
    mpZoomParams->SetMode(ZOOM_DUP);
    gScreens.Refresh();
}

void CZoomParamsDlg::OnEnKillfocusEditzoom()
{
    char zStr[16];
    CWnd *pWnd=GetDlgItem(IDC_EDITZOOM);
    pWnd->GetWindowText(zStr, 15);
    mpZoomParams->SetZoomFactor (mpData, (float)atof(zStr));
    gScreens.Refresh();
}

void CZoomParamsDlg::OnBnClickedRadioBicubic()
{
    mpZoomParams->mbEnableZoom = true;
    mpZoomParams->SetMode(ZOOM_BICUBIC);
    gScreens.Refresh();
}
