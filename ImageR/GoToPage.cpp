// GoToPage.cpp : implementation file
//

#include "stdafx.h"
//#include "viewer.h"
#include "GoToPage.h"
#include "MetaData.h"
#include "Screens.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\yUtils\MyDicom.h"
#include "ArchivesFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoToPage dialog


CGoToPage::CGoToPage(CMetaData *pData)
	: CDialog(CGoToPage::IDD, NULL)
	, mpData(pData)
	, mpPosition(pData->GetPosition())
	, mpArchivesFile(NULL)
	, mbChanged(false)
	, mbUseImageNumbers(gScreens.mbEbwMode)
	, miFirstImage(0)
	, miSecondImage(0)
	, mStep(0)
{
	if (mpData->GetClass() == "CArchivesFile")
	{
		mpArchivesFile = (CArchivesFile *)mpData;
		bool bOK = mpArchivesFile->GetInstanceNumberByIndex(mpPosition->miFirst, miFirstImage);
		bOK &= mpArchivesFile->GetInstanceNumberByIndex(mpPosition->Relative2Index(1), miSecondImage);
		if (!bOK)
			mbUseImageNumbers = false;
		else
			mStep = miSecondImage - miFirstImage;
	}
	else
		mbUseImageNumbers = false;
}
void CGoToPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGoToPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGoToPage, CDialog)
	//{{AFX_MSG_MAP(CGoToPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoToPage message handlers

void CGoToPage::OnOK() 
{
	char zText[16];
	CWnd *pWnd=GetDlgItem(IDC_GOTOPAGE);
	pWnd->GetWindowText(zText, 15);
	int iWanted = atoi(zText);
	if (mbUseImageNumbers && (mStep != 0))
	{
		int deltaNum = iWanted - miFirstImage;
		int iRelative = deltaNum / mStep;
		iWanted = mpPosition->Relative2Index(iRelative);
	}
	if (mpPosition->miCurrent != iWanted)
	{
		mpPosition->ClipAndSetCurrent(iWanted);
		mbChanged = true;
	}

	CDialog::OnOK();
}

BOOL CGoToPage::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int iStart = mpPosition->miCurrent;
	if (mbUseImageNumbers)
	{
		CMyDicom *pDicom = mpData->GetDicom();
		pDicom->GetInstanceNumber(iStart);
	}
  char zText[32];
  sprintf_s(zText, 32, "%d", iStart);

  CEdit *pWnd=(CEdit *)GetDlgItem(IDC_GOTOPAGE);
  pWnd->SetWindowText(zText);
  pWnd->SetFocus();
  pWnd->SetSel(0, -1);

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
