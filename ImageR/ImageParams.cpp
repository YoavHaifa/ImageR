// ImageParams.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include ".\imageparams.h"
#include "MultiDataFile.h"
#include "..\..\yUtils\MyDicom.h"
#include "Screens.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\yUtils\MyWindows.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CImageParams dialog


CImageParams::CImageParams(CMultiDataFile *pFile,
                           CWnd* pParent /*=NULL*/)
	: CDialog(CImageParams::IDD, pParent)
    , mpFile (pFile)
    , mnLinesSave(pFile->GetNLinesInPage())
    , mnColsSave(pFile->GetNCols())
    , meDataTypeSave(pFile->GetDataType())
    , mnLines(pFile->GetNLinesInPage())
    , mnCols(pFile->GetNCols())
    , meDataType(pFile->GetDataType())
    , mOffsetBytesSave(pFile->GetOffsetBytes())
    , mOffsetBytes(pFile->GetOffsetBytes())
{
	//{{AFX_DATA_INIT(CImageParams)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CImageParams::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImageParams)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImageParams, CDialog)
	//{{AFX_MSG_MAP(CImageParams)
	ON_BN_CLICKED(IDC_SAVE, OnTry)
	ON_BN_CLICKED(IDC_AUTO, OnCreatePattern)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImageParams message handlers

BOOL CImageParams::OnInitDialog() 
{
  CDialog::OnInitDialog();

  set_params();
  SetNPages();

  CWnd *pWnd = GetDlgItem(IDC_STATIC_FILE_LENGTH);
  char zBuf[128];
  sprintf_s (zBuf, 128, "%I64d", mpFile->GetLength());
  pWnd->SetWindowText(zBuf);

  return FALSE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

//---------------------------------------------------------------------

int CImageParams::get_int_param(int idc, int minValue, int defaultValue)
{
  char zText[32];
  CWnd *pWnd = GetDlgItem(idc);
  pWnd->GetWindowText(zText, 31);
  int value = atoi(zText);
  if (minValue && (value < minValue))
    value = defaultValue;
	return value;
}

//---------------------------------------------------------------------

void CImageParams::get_params()
{
	mnLines = get_int_param (IDC_ROWS, 1, 512);
    mnCols = get_int_param (IDC_COLUMNS, 1, 512);
    mnPages = (float)get_int_param (IDC_PAGES, 1, 1);

    CComboBox *pBox = (CComboBox *)GetDlgItem(IDC_DTYPE);
    meDataType = EMyImageDataType(pBox->GetCurSel() + 1);

    CWnd *pWnd=GetDlgItem(IDC_OFFS);
    char zText[128];
    pWnd->GetWindowText(zText, 127);
    mOffsetBytes =_atoi64(zText);
}
//----------------------------------------------------------
void CImageParams::set_int_param(int idc, int value)
{
  char zText[32];
  sprintf_s (zText, 32, "%d", value);
  CWnd *pWnd = GetDlgItem (idc);
  pWnd->SetWindowText (zText);
}

//----------------------------------------------------------

void CImageParams::set_params()
{
    set_int_param (IDC_ROWS, mpFile->GetNLinesInPage());
	set_int_param (IDC_COLUMNS, mpFile->GetNCols());

	set_int_param (IDC_INCR, 0);


	CWnd *pWnd = GetDlgItem (IDC_ROWS);
    pWnd->SetFocus();

    char zText[128];
    sprintf_s(zText, 128, "%I64d", mpFile->GetOffsetBytes());
    pWnd = GetDlgItem(IDC_OFFS);
    pWnd->SetWindowText(zText);

    CComboBox *pBox=(CComboBox *)GetDlgItem(IDC_DTYPE);
    pBox->SetCurSel(max(0, mpFile->GetDataType()-1));
}

//----------------------------------------------------------

void CImageParams::OnOK() 
{
    OnTry();
	CDialog::OnOK();
}
void CImageParams::OnCancel() 
{
    mpFile->SetSize(mnLinesSave, mnColsSave);
    mpFile->SetTypedDataHandler(meDataTypeSave);
    mpFile->SetOffsetBytes(mOffsetBytesSave);
    SetNPages();
    gScreens.Refresh();
    CDialog::OnCancel();
}
void CImageParams::OnTry() 
{
    get_params();

    mpFile->SetSize(mnLines, mnCols);
    if (meDataType >= 0)
        mpFile->SetTypedDataHandler(meDataType);

    mpFile->SetOffsetBytes(mOffsetBytes);

    SetNPages();

    gScreens.Refresh();
}
void CImageParams::OnCreatePattern() 
{
    get_params();
    if (mnPages > 128)
        mnPages = 128;

    switch (meDataType)
    {
    case DATA_TYPE_USHORT:
        CreatePattern((unsigned short)0, (unsigned short)1, "ushort");
        break;
    case DATA_TYPE_SHORT:
        CreatePattern((short)-20, (short)1, "short");
        break;
    case DATA_TYPE_UINT:
        CreatePattern((unsigned int)0, (unsigned int)1, "uint");
        break;
    case DATA_TYPE_INT:
        CreatePattern((int)-20, (int)1, "int");
        break;
    case DATA_TYPE_FLOAT:
        CreatePattern(-10.0f, 0.1f, "float");
        break;
    case DATA_TYPE_DOUBLE:
        CreatePattern(-10.0f, 0.1f, "double");
        break;
    default:
        MessageBox("Data Type not supported", "Note", MB_OK);
        break;
    }
}
template <typename T>
void CImageParams::CreatePattern (T first, T delta, const char *zTypeName)
{
    char zTmp[128];
	sprintf_s (zTmp, 128, "%s\\ImagePattern_lines%d_cols%d", 
		(const char*)CMyWindows::GetLogDir(), mnLines, mnCols);

    CString sfName(zTmp);
    if (mOffsetBytes)
    {
        sprintf_s (zTmp, 128, "_offset%I64d", mOffsetBytes);
        sfName += zTmp;
    }
    sfName += "_";
    sfName += zTypeName;
    sfName += ".dat";

    FILE *pf = MyFOpen (sfName, "wb");
    if (!pf)
        return;

    // Write Offset data
    ULONGLONG nOffsetUnits = mOffsetBytes / CMyImage::maDataSizes[meDataType];
    T value1 = (T)1234;
    T value2 = (T)21;
    for (ULONGLONG iOffset = 0; iOffset < nOffsetUnits; iOffset++)
    {
        int iInCol = (int)(iOffset % mnCols);
        if (iInCol % 32 < 16)
            fwrite (&value1, sizeof(value1), 1, pf);
        else
            fwrite (&value2, sizeof(value2), 1, pf);
    }

    // Write pages
    int topCol = mnCols/2;
    for (int iPage = 0; iPage < mnPages; iPage++)
    {
        T firstInLine = first;
        for (int iLine = 0; iLine < mnLines; iLine++)
        {
            T value = firstInLine;

            for (int iCol = 0; iCol < mnCols; iCol++)
            {
                if (mnPages > 1 && iCol > 10 && iCol < 20 && (iLine & 1) && iLine < 2 * iPage)
                {
                    T page = (T)1000;
                    fwrite (&page, sizeof(page), 1, pf);
                }
                else
                    fwrite (&value, sizeof(value), 1, pf);
                if (iCol < topCol)
                    value = (T)(value + delta);
                else
                    value = (T)(value - delta);
            }
            if (iLine < mnLines/2)
                firstInLine = (T)(firstInLine + delta);
            else
                firstInLine = (T)(firstInLine - delta);
        }
        // For Next Page
        topCol++;
    }
    fclose(pf);
}
void CImageParams::SetNPages(void)
{
	int nPages = mpFile->ComputeNPages();
	unsigned int pageSizeBruto = mpFile->GetPageSizeBruto();
	ULONGLONG allPagesSize = mpFile->GetLength() - mOffsetBytes;
	mnPages = (float)((double)allPagesSize / pageSizeBruto);
	char zBuf[128];
    if (nPages == (float)mnPages)
        sprintf_s (zBuf, 128, "%d", nPages);
    else
        sprintf_s (zBuf, 128, "%.3f", mnPages);

    CWnd *pWnd = GetDlgItem (IDC_PAGES);
    pWnd->SetWindowText (zBuf);
}
