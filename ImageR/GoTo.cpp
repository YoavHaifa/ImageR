// GoTo.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "GoTo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoTo dialog


CGoTo::CGoTo(SGoTo *pGoTo, CWnd* pParent /*=NULL*/)
	: CDialog(CGoTo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGoTo)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

  mpGoTo=pGoTo;
}


void CGoTo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGoTo)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGoTo, CDialog)
	//{{AFX_MSG_MAP(CGoTo)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoTo message handlers

BOOL CGoTo::OnInitDialog() 
{
  CDialog::OnInitDialog();

  CWnd *pWnd;
  char zStr[32];

  pWnd=GetDlgItem(IDC_GOTO_ROW);
  sprintf_s(zStr, 32, "%d", mpGoTo->Row);
  pWnd->SetWindowText(zStr);

  pWnd=GetDlgItem(IDC_GOTO_COL);
  sprintf_s(zStr, 32, "%d", mpGoTo->Col);
  pWnd->SetWindowText(zStr);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CGoTo::OnOK() 
{
  CWnd *pWnd;
  char zStr[32];

  pWnd=GetDlgItem(IDC_GOTO_ROW);
  pWnd->GetWindowText(zStr, 31);
  mpGoTo->Row=atoi(zStr);

  pWnd=GetDlgItem(IDC_GOTO_COL);
  pWnd->GetWindowText(zStr, 31);
  mpGoTo->Col=atoi(zStr);

	CDialog::OnOK();
}

void CGoTo::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);

  CWnd *pWnd=GetDlgItem(IDC_GOTO_ROW);
  GotoDlgCtrl(pWnd);	
}
