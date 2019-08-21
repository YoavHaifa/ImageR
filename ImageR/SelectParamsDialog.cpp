// SelectParamsDialog.cpp : implementation file
//

#include "stdafx.h"
//#include "DataViewer.h"
#include "SelectParamsDialog.h"
#include ".\selectparamsdialog.h"
#include "MultiFile.h"
#include "..\..\yUtils\MyWindows.h"

// CSelectParamsDialog dialog

IMPLEMENT_DYNAMIC(CSelectParamsDialog, CDialog)
CSelectParamsDialog::CSelectParamsDialog(CWnd* pParent /*=NULL*/, const char *zfName)
	: CDialog(CSelectParamsDialog::IDD, pParent)
    , msFileName(zfName)
    , mpMultiFile (0)
    , mnParams (0)
{
    AnalyzeParameters();
}

CSelectParamsDialog::~CSelectParamsDialog()
{
}
void CSelectParamsDialog::SParamData::SetValueText()
{
    char zText[128];
    sprintf_s (zText, 128, "%d", mValue);
    mpWnd->SetWindowText(zText);
}
BOOL CSelectParamsDialog::OnInitDialog() 
{
	BOOL rc = CDialog::OnInitDialog();
    SetWindowText (msFileName);

    if (mnParams > 0)
    {
        maParams[0].mpWnd = GetDlgItem(IDC_EDIT_PAR1);
        CWnd *pWnd=GetDlgItem(IDC_STATIC_PAR1);
        pWnd->SetWindowText(maParams[0].msName);
        maParams[0].SetValueText();
    }

    if (mnParams > 1)
    {
        maParams[1].mpWnd = GetDlgItem(IDC_EDIT_PAR2);
        CWnd *pWnd=GetDlgItem(IDC_STATIC_PAR2);
        pWnd->SetWindowText(maParams[1].msName);
        maParams[1].SetValueText();
    }

    if (mnParams > 2)
    {
        maParams[2].mpWnd = GetDlgItem(IDC_EDIT_PAR3);
        CWnd *pWnd=GetDlgItem(IDC_STATIC_PAR3);
        pWnd->SetWindowText(maParams[2].msName);
        maParams[2].SetValueText();
    }

    return rc;
}
void CSelectParamsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSelectParamsDialog, CDialog)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SEL_PARAMS_SPIN1, OnDeltaposSpin1)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SEL_PARAMS_SPIN2, OnDeltaposSpin2)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SEL_PARAMS_SPIN3, OnDeltaposSpin3)
END_MESSAGE_MAP()


// CSelectParamsDialog message handlers
bool CSelectParamsDialog::OnSpin (SParamData &param, int iDelta)
{
    int save = param.mValue;
    for (int i = 0; i < 11; i++)
    {
        if (iDelta > 0)
            param.mValue--;
        else
            param.mValue++;

        if (ComposeName())
        {
            param.SetValueText();
            //if (mpMultiFile)
            //    mpMultiFile->set_name (msFileName, false, NULL);
            return true;
        }
    }
    param.mValue = save;
    return false;
}
void CSelectParamsDialog::OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
    OnSpin (maParams[0], pNMUpDown->iDelta);
    *pResult = 0;
}
void CSelectParamsDialog::OnDeltaposSpin2(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
    OnSpin (maParams[1], pNMUpDown->iDelta);
    *pResult = 0;
}
void CSelectParamsDialog::OnDeltaposSpin3(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
    OnSpin (maParams[2], pNMUpDown->iDelta);
    *pResult = 0;
}
bool CSelectParamsDialog::AnalyzeParameters(void)
{
    mnParams = 0;
    int iPrevSlash = -1;
    int iSlash = msFileName.FindOneOf ("\\/");
    if (iSlash < 0)
        return false;

    int len = msFileName.GetLength();
    for (int i = iSlash + 1; i < len; i++)
    {
        int iSl = msFileName.Find ("/", iSlash + 1);
        int iBSl = msFileName.Find ("\\", iSlash + 1);
        if (iSl < 0 && iBSl < 0)
            break;
        iPrevSlash = iSlash;
        if (iSl < 0)
            iSlash = iBSl;
        else if (iBSl < 0)
            iSlash = iSl;
        else if (iSl < iBSl)
            iSlash = iSl;
        else
            iSlash = iBSl;
    }
    if (iPrevSlash < 0)
        return false;

    msFileNamePrefix = msFileName.Left (iPrevSlash+1);
    msFileNamePostfix = msFileName.Right (len - iSlash);

    int iStart = iPrevSlash + 1;
    while (iStart < iSlash)
    {
        int iAdd = AnalyzeParameter ((const char *)msFileName + iStart);
        if (!iAdd)
            break;
        iStart += iAdd;
    }

    return mnParams > 0;
}
int CSelectParamsDialog::AnalyzeParameter(const char *zText)
{
    int i = 0;
    while (zText[i] != '_' && zText[i] != '\\' && zText[i] != '/')
        maParams[mnParams].msName += zText[i++];
    if (zText[i] != '_')
        return 0;
    i++;
    if (!isdigit(zText[i]))
        return 0;
        
    maParams[mnParams].mValue = atoi (&zText[i]);
    mnParams++;
    while (isdigit(zText[i]))
        i++;
    while (zText[i] == '_')
        i++;
    return i;
}
bool CSelectParamsDialog::ComposeName (void)
{
    CString msName (msFileNamePrefix);
    for (int i = 0; i < mnParams; i++)
    {
        msName += maParams[i].msName;
        msName += '_';
        char zText[128];
        sprintf_s (zText, 128, "%d", maParams[i].mValue);
        msName += zText;
        if (i < mnParams - 1)
            msName += '_';
    }
    msName += msFileNamePostfix;

    FILE *pf = MyFOpen (msName, "r");
    if (pf)
    {
        fclose (pf);
        msFileName = msName;
        SetWindowText (msFileName);
        return true;
    }
    return false;
}

