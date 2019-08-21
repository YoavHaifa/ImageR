#if !defined(AFX_PRINTEXCEL_H__031C071B_358E_4AEB_BA79_F676219DC2F6__INCLUDED_)
#define AFX_PRINTEXCEL_H__031C071B_358E_4AEB_BA79_F676219DC2F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrintExcel.h : header file
//
#include "resource.h"
#include "MultiFile.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\LogFile.h"

/////////////////////////////////////////////////////////////////////////////
// CPrintExcel dialog

class CPrintExcel : public CDialog
{
// Construction
public:
	CPrintExcel(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPrintExcel)
	enum { IDD = IDD_PRINT_EXCEL };
	UINT	m_Position;
	UINT	m_From;
	UINT	m_Till;
	UINT	m_Step;
	BOOL	m_SwapWords;
	//}}AFX_DATA

    template <typename T>
    static CString PrintScreen (T *pData, const char *zFormat, 
                             int nLines, int nCols, int nColsLoaded, const char *zfName,
                             const CString &sFileText);
    template <typename T>
    static CString Value2s (T *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias);
    void PrintFromFile (class CMetaData &file, int nColsInFile, EMyImageDataType eDataType, FILE *pfLog);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrintExcel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPrintExcel)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

template <typename T>
CString CPrintExcel::PrintScreen (T *pData, const char *zFormat, int nLines, int nCols, int nColsLoaded,
                         const char *zfName, const CString &sFileText)
{
    char zBuf[256];

    strcpy_s (zBuf, 256, zfName);
    if (!sFileText.IsEmpty())
        strcat_s (zBuf, 256, sFileText);

    strcat_s (zBuf, 256, ".xls");

    CLogFile log(zBuf);
    if (!log)
        return CString("");

    CString sName(log.mName);
    fprintf (log.mpf, "iCol\\iLine\t");
    for (int i = 0; i < nLines; i++)
        fprintf (log.mpf, "%d\t", i+1);
    fprintf (log.mpf, "\n");

    for (int iCol = 0; iCol < nCols; iCol++)
    {

        fprintf (log.mpf, "%d\t", iCol);
        for (int iLine = 0; iLine < nLines; iLine++)
        {
            fprintf (log.mpf, zFormat, pData[iLine*nColsLoaded+iCol]);
            fprintf (log.mpf, "\t");
        }
        fprintf (log.mpf, "\n");
    }
    return sName;
}

#endif // !defined(AFX_PRINTEXCEL_H__031C071B_358E_4AEB_BA79_F676219DC2F6__INCLUDED_)
