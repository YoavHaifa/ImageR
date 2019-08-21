// DataTypeDialog.cpp : implementation file
//

#include "stdafx.h"
#include "DataTypeDialog.h"
#include "afxdialogex.h"
#include "..\..\ImageRLib\MyImage.h"


// CDataTypeDialog dialog

IMPLEMENT_DYNAMIC(CDataTypeDialog, CDialogEx)

CDataTypeDialog::CDataTypeDialog(EMyImageDataType eType, CWnd* pParent /*=NULL*/)
	: CMyDialogEx(CDataTypeDialog::IDD, pParent)
	, meWantedDataType(eType)
	, meInitialType(eType)
{

}

CDataTypeDialog::~CDataTypeDialog()
{
}

void CDataTypeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDataTypeDialog, CDialogEx)
END_MESSAGE_MAP()

BOOL CDataTypeDialog::OnInitDialog(void)
{
	switch (meInitialType)
	{
	case DATA_TYPE_SHORT:
		SetCheck(IDC_RADIO_SHORT, true);
		break;

	case DATA_TYPE_USHORT:
		SetCheck(IDC_RADIO_USHORT, true);
		break;

	case DATA_TYPE_UINT:
		SetCheck(IDC_RADIO_UINT, true);
		break;

	case DATA_TYPE_INT:
		SetCheck(IDC_RADIO_INT, true);
		break;

	case DATA_TYPE_FLOAT:
		SetCheck(IDC_RADIO_FLOAT, true);
		break;

	case DATA_TYPE_DOUBLE:
		SetCheck(IDC_RADIO_DOUBLE, true);
		break;

	case DATA_TYPE_RGB:
		SetCheck(IDC_RADIO_RGB, true);
		break;

	case DATA_TYPE_COLORREF:
		SetCheck(IDC_RADIO_COLORREF, true);
		break;

	case DATA_TYPE_BOOL:
		SetCheck(IDC_RADIO_BOOL, true);
		break;

	case DATA_TYPE_CHAR:
		SetCheck(IDC_RADIO_CHAR, true);
		break;
	}

    return CMyDialogEx::OnInitDialog();
}

// CDataTypeDialog message handlers
EMyImageDataType CDataTypeDialog::GetSelectedDataType(void)
{
	if (IsChecked(IDC_RADIO_SHORT))
		return DATA_TYPE_SHORT;

	if (IsChecked(IDC_RADIO_USHORT))
		return DATA_TYPE_USHORT;

	if (IsChecked(IDC_RADIO_UINT))
		return DATA_TYPE_UINT;

	if (IsChecked(IDC_RADIO_INT))
		return DATA_TYPE_INT;

	if (IsChecked(IDC_RADIO_FLOAT))
		return DATA_TYPE_FLOAT;

	if (IsChecked(IDC_RADIO_DOUBLE))
		return DATA_TYPE_DOUBLE;

	if (IsChecked(IDC_RADIO_RGB))
		return DATA_TYPE_RGB;

	if (IsChecked(IDC_RADIO_COLORREF))
		return DATA_TYPE_COLORREF;

	if (IsChecked(IDC_RADIO_BOOL))
		return DATA_TYPE_BOOL;

	if (IsChecked(IDC_RADIO_CHAR))
		return DATA_TYPE_CHAR;

	return meInitialType;
}
void CDataTypeDialog::OnOK()
{
	meWantedDataType = GetSelectedDataType();
    CMyDialogEx::OnOK();
}