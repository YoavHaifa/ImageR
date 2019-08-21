// Statistics.cpp : implementation file
//

#include "stdafx.h"
#include <math.h>
#include "Statistics.h"
#include "ImageView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatistics dialog


CStatistics::CStatistics(CImageView *pImageView, SCommonParams *pCommonParams,
                         CWnd* pParent /*=NULL*/)
	: CDialog(CStatistics::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStatistics)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

  mpCommonParams=pCommonParams;
  mpImageView = pImageView;

  mRowFrom=0;
  mRowTo=pCommonParams->nRowsInFile-1;
  mColFrom=0;
  mColTo=pCommonParams->nColsInFile-1;

  mMin=0.0f;
  mMax=0.0f;
  mAver=0.0f;
  mStdDev=0.0f;

  mPrecision=2;
}


void CStatistics::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatistics)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatistics, CDialog)
	//{{AFX_MSG_MAP(CStatistics)
	ON_EN_KILLFOCUS(IDC_PRECEDITS, OnKillfocusPrecedits)
	ON_NOTIFY(UDN_DELTAPOS, IDC_PRECSPINS, OnDeltaposPrecspins)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatistics message handlers

void CStatistics::calculate()
{
  char zText[20];
  CWnd *pWnd;

  if (mRowTo < mRowFrom)
    mRowTo=mRowFrom;
  if (mColTo < mColFrom)
    mColTo=mColFrom;

  if (mRowFrom >= mpCommonParams->nRowsInFile) {
    mRowFrom=mpCommonParams->nRowsInFile-1;
    sprintf_s(zText, 20, "%d", mRowFrom);
    pWnd=GetDlgItem(IDC_FROMROW);
    pWnd->SetWindowText(zText);
  }
  if (mRowTo >= mpCommonParams->nRowsInFile) {
    mRowTo=mpCommonParams->nRowsInFile-1;
    sprintf_s(zText, 20, "%d", mRowTo);
    pWnd=GetDlgItem(IDC_TOROW);
    pWnd->SetWindowText(zText);
  }

  if (mColFrom >= mpCommonParams->nColsInFile) {
    mColFrom=mpCommonParams->nColsInFile-1;
    sprintf_s(zText, 20, "%d", mColFrom);
    pWnd=GetDlgItem(IDC_FROMCOL);
    pWnd->SetWindowText(zText);
  }
  if (mColTo >= mpCommonParams->nColsInFile) {
    mColTo=mpCommonParams->nColsInFile-1;
    sprintf_s(zText, 20, "%d", mColTo);
    pWnd=GetDlgItem(IDC_TOCOL);
    pWnd->SetWindowText(zText);
  }

  int iRow, iCol;
  float CurVal, Sum=0.0f;

  mMin=mMax=mpImageView->get_data_value(mRowFrom, mColFrom);
  for (iRow=mRowFrom; iRow <= mRowTo; iRow++) {
    for (iCol=mColFrom; iCol <= mColTo; iCol++) {
      CurVal=mpImageView->get_data_value(iRow, iCol);
      if (CurVal < mMin)
        mMin=CurVal;
      else if (CurVal > mMax)
        mMax=CurVal;
      Sum += CurVal;
    }
  }
  float NumVals=(float)(mRowTo-mRowFrom+1)*(mColTo-mColFrom+1);
  mAver=Sum/NumVals;

  Sum=0.0f;
  if (NumVals == 1) {
    mStdDev=0.0f;
  } else {
    for (iRow=mRowFrom; iRow <= mRowTo; iRow++) {
      for (iCol=mColFrom; iCol <= mColTo; iCol++) {
        CurVal=mpImageView->get_data_value(iRow, iCol);
        Sum += (CurVal-mAver)*(CurVal-mAver);
      }
    }
    mStdDev=(float)sqrt(Sum/(NumVals-1));
  }
  results_out();
}

void CStatistics::results_out()
{
  char zText[20];
  CWnd *pWnd;
  char zFormat[16];
  sprintf_s(zFormat, 16, "%%.%df", mPrecision);

  if (mpCommonParams->DataType == DATA_TYPE_FLOAT) {
    sprintf_s(zText, 20, zFormat, mMin);
  } else {
    sprintf_s(zText, 20, "%.0f", mMin);
  }
  pWnd=GetDlgItem(IDC_MIN);
  pWnd->SetWindowText(zText);

  if (mpCommonParams->DataType == DATA_TYPE_FLOAT) {
    sprintf_s(zText, 20, zFormat, mMax);
  } else {
    sprintf_s(zText, 20, "%.0f", mMax);
  }
  pWnd=GetDlgItem(IDC_MAX);
  pWnd->SetWindowText(zText);

  sprintf_s(zText, 20, zFormat, mAver);
  pWnd=GetDlgItem(IDC_AVER);
  pWnd->SetWindowText(zText);

  sprintf_s(zText, 20, zFormat, mStdDev);
  pWnd=GetDlgItem(IDC_STDDEV);
  pWnd->SetWindowText(zText);
}

void CStatistics::get_params()
{
  char zText[20];
  CWnd *pWnd;

  pWnd=GetDlgItem(IDC_FROMROW);
  pWnd->GetWindowText(zText, 19);
  mRowFrom=atoi(zText);
  pWnd=GetDlgItem(IDC_TOROW);
  pWnd->GetWindowText(zText, 19);
  mRowTo=atoi(zText);

  pWnd=GetDlgItem(IDC_FROMCOL);
  pWnd->GetWindowText(zText, 19);
  mColFrom=atoi(zText);
  pWnd=GetDlgItem(IDC_TOCOL);
  pWnd->GetWindowText(zText, 19);
  mColTo=atoi(zText);
}

void CStatistics::OnOK() 
{
  get_params();
  calculate();
}

BOOL CStatistics::OnInitDialog() 
{
  CDialog::OnInitDialog();

  calculate();

  char zText[20];
  CWnd *pWnd;

  sprintf_s(zText, 20, "%d", mRowFrom);
  pWnd=GetDlgItem(IDC_FROMROW);
  pWnd->SetWindowText(zText);

  sprintf_s(zText, 20, "%d", mRowTo);
  pWnd=GetDlgItem(IDC_TOROW);
  pWnd->SetWindowText(zText);

  sprintf_s(zText, 20, "%d", mColFrom);
  pWnd=GetDlgItem(IDC_FROMCOL);
  pWnd->SetWindowText(zText);

  sprintf_s(zText, 20, "%d", mColTo);
  pWnd=GetDlgItem(IDC_TOCOL);
  pWnd->SetWindowText(zText);

  sprintf_s(zText, 20, "%d", mPrecision);
  pWnd=GetDlgItem(IDC_PRECEDITS);
  pWnd->SetWindowText(zText);

  CSpinButtonCtrl *pSpin=(CSpinButtonCtrl *)GetDlgItem(IDC_PRECSPINS);
  pSpin->SetBuddy(pWnd);
  pSpin->SetRange(0, 7);
  pSpin->SetPos(mPrecision);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

//----------------------------------------------------------------

void CStatistics::OnKillfocusPrecedits() 
{
  CWnd *pWnd=GetDlgItem(IDC_PRECEDITS);
  char zText[20];
  pWnd->GetWindowText(zText, 15);
  int CurVal=atoi(zText);
  if (CurVal > 7) {
    CurVal=7;
    sprintf_s(zText, 20, "%d", CurVal);
    pWnd->SetWindowText(zText);
  }
  CSpinButtonCtrl *pSpin=(CSpinButtonCtrl *)GetDlgItem(IDC_PRECSPINS);
  pSpin->SetPos(CurVal);
  mPrecision=CurVal;
  results_out();
}

void CStatistics::OnDeltaposPrecspins(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

  char zText[20];

  CWnd *pWnd=GetDlgItem(IDC_PRECEDITS);
  pWnd->GetWindowText(zText, 15);
  int CurVal=atoi(zText);
  if ((CurVal > 0 && pNMUpDown->iDelta < 0) ||
      (CurVal < 7 && pNMUpDown->iDelta > 0))
    CurVal += pNMUpDown->iDelta;

  sprintf_s(zText, 20, "%d", CurVal);
  pWnd->SetWindowText(zText);
  CSpinButtonCtrl *pSpin=(CSpinButtonCtrl *)GetDlgItem(IDC_PRECSPINS);
  pSpin->SetPos(CurVal);

  mPrecision=CurVal;
  results_out();

	*pResult = 0;
}
