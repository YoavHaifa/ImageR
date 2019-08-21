// SlicesDialog.cpp : implementation file
//

#include "stdafx.h"
//#include "DataViewer.h"
#include "SlicesDialog.h"


// CSlicesDialog dialog

IMPLEMENT_DYNAMIC(CSlicesDialog, CDialog)
CSlicesDialog::CSlicesDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSlicesDialog::IDD, pParent)
    , mnSlice(0)
    , miSlice(0)
    , mlHeaderBytes(0)
    , mlSlice(0)
    , mnFocalSpots(0)
    , miFocalSpot(0)
    , mbViewReadings(false)
{
}

CSlicesDialog::~CSlicesDialog()
{
}
    
void CSlicesDialog::set_text (int id, int value)
{
    char zText[20];
    sprintf_s (zText, 20, "%d", value);
    CWnd *pWnd = GetDlgItem(id);
    pWnd->SetWindowText(zText);
}

int CSlicesDialog::get_value (int id)
{
    char zText[20];
    CWnd *pWnd = GetDlgItem(id);
    pWnd->GetWindowText(zText, 19);
    return atoi(zText);
}

BOOL CSlicesDialog::OnInitDialog()
{
    set_text (IDC_EDIT_MNSLICES, mnSlice);
    set_text (IDC_EDIT_MLSLICE, mlSlice);
    set_text (IDC_EDIT_MLHEADER, mlHeaderBytes);
    set_text (IDC_EDIT_MISLICE, miSlice);
    set_text (IDC_EDIT_MNFS, mnFocalSpots);
    set_text (IDC_EDIT_MIFS, miFocalSpot);

    CButton *pWnd=(CButton *)GetDlgItem(IDC_CHECK_VREADINGS);
    if (mbViewReadings)
        pWnd->SetCheck(BST_CHECKED);
    else
        pWnd->SetCheck(BST_UNCHECKED);

    return TRUE;
}
void CSlicesDialog::OnOK()
{
    mnSlice = get_value (IDC_EDIT_MNSLICES);
    mlSlice = get_value (IDC_EDIT_MLSLICE);
    mlHeaderBytes = get_value (IDC_EDIT_MLHEADER);
    miSlice = get_value (IDC_EDIT_MISLICE);
    mnFocalSpots = get_value (IDC_EDIT_MNFS);
    miFocalSpot = get_value (IDC_EDIT_MIFS);

    CButton *pWnd=(CButton *)GetDlgItem(IDC_CHECK_VREADINGS);
    int ChkState=pWnd->GetCheck();
    if (ChkState == BST_CHECKED)
        mbViewReadings = true;
    else
        mbViewReadings = false;

	CDialog::OnOK();
}

void CSlicesDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //DDX_Control(pDX, IDC_CHECK_VREADINGS, m_VReadings);
}


BEGIN_MESSAGE_MAP(CSlicesDialog, CDialog)
END_MESSAGE_MAP()


// CSlicesDialog message handlers
