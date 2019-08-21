#pragma once
#include "resource.h"
#include "..\..\yUtils\MyDialogEx.h"
#include "..\..\ImageRLib\MyImage.h"

// CDataTypeDialog dialog

class CDataTypeDialog : public CMyDialogEx
{
	DECLARE_DYNAMIC(CDataTypeDialog)

public:
	CDataTypeDialog(EMyImageDataType eType, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDataTypeDialog();

	EMyImageDataType meWantedDataType;

// Dialog Data
	enum { IDD = IDD_DIALOG_DATA_TYPE };

protected:
	EMyImageDataType meInitialType;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	EMyImageDataType GetSelectedDataType(void);

	DECLARE_MESSAGE_MAP()

};
