// PrintExcel.cpp : implementation file
//

#include "stdafx.h"
//#include "viewer.h"
#include "PrintExcel.h"
#include "ImageView.h"
#include "..\..\yUtils\MyWindows.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrintExcel dialog


CPrintExcel::CPrintExcel(CWnd* pParent /*=NULL*/)
	: CDialog(CPrintExcel::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrintExcel)
	m_Position = 0;
	m_From = 0;
	m_Till = 0;
	m_Step = 1;
	m_SwapWords = FALSE;
	//}}AFX_DATA_INIT
}


void CPrintExcel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrintExcel)
	DDX_Text(pDX, IDC_EDIT_POS, m_Position);
	DDX_Text(pDX, IDC_EDIT_FROM, m_From);
	DDX_Text(pDX, IDC_EDIT_TILL, m_Till);
	DDX_Text(pDX, IDC_EDIT_STEP, m_Step);
	DDX_Check(pDX, IDC_SWAP_WORDS, m_SwapWords);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrintExcel, CDialog)
	//{{AFX_MSG_MAP(CPrintExcel)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPrintExcel::PrintFromFile (CMetaData &file, int nColsInFile, EMyImageDataType eDataType, FILE *pfLog)
{
	if (!file.open())
	{
		fprintf (pfLog, "\n<CPrintExcel::PrintFromFile> FAILED TO OPEN %s\n", file.z_name());
		return;
	}

	char zBuf[256];
	unsigned short *pUSh = (unsigned short *)zBuf;
	unsigned short tUSh;

	sprintf_s (zBuf, 256, "%s.col_%d_range_%d_%d_%d%s.xls",
		file.PrivateName(), m_Position,
		m_From, m_Till, m_Step,
		m_SwapWords ? "_sw" : "");

    FILE *pf = CMyWindows::FOpenLogFile (zBuf);
	if (!pf)
    {
        file.close();
		return;
    }

	DWORD SizeOfUnit = CMyImage::maDataSizes[eDataType];
	DWORD nBytesRead;

	for (UINT i = m_From; i < m_Till; i += m_Step)
	{
		ULONGLONG offset;
		offset = (nColsInFile * i + m_Position) * SizeOfUnit;
		file.set_file_offset (offset);

		ReadFile(file.h(), zBuf, SizeOfUnit, &nBytesRead, NULL);
		if (nBytesRead != SizeOfUnit)
			break;

		switch (eDataType)
		{
		case DATA_TYPE_SHORT:
			{
				short *pShBuf=(short *)zBuf;
				fprintf (pf, "%d\n", *pShBuf);
			}
			break;
		case DATA_TYPE_USHORT:
			{
				unsigned short *pUshBuf=(unsigned short *)zBuf;
				fprintf (pf, "%u\n", *pUshBuf);
			}
			break;
		case DATA_TYPE_INT:
			{
				if (m_SwapWords)
				{
					tUSh = pUSh[0];
					pUSh[0] = pUSh[1];
					pUSh[1] = tUSh;
				}
				int *pIntBuf=(int *)zBuf;
				if (m_SwapWords)
				{
					tUSh = pUSh[0];
					pUSh[0] = pUSh[1];
					pUSh[1] = tUSh;
				}
				fprintf (pf, "%d\n", *pIntBuf);
			}
			break;
		case DATA_TYPE_UINT:
			{
				if (m_SwapWords)
				{
					tUSh = pUSh[0];
					pUSh[0] = pUSh[1];
					pUSh[1] = tUSh;
				}
				unsigned int *pUintBuf=(unsigned int *)zBuf;
				fprintf (pf, "%u\n", *pUintBuf);
			}
			break;
		case DATA_TYPE_FLOAT:
			{
				if (m_SwapWords)
				{
					tUSh = pUSh[0];
					pUSh[0] = pUSh[1];
					pUSh[1] = tUSh;
				}
				float *pFlBuf=(float *)zBuf;
				fprintf (pf, "%f\n", *pFlBuf);
			}
			break;
		case DATA_TYPE_DOUBLE:
			{
				/*if (m_SwapWords)
				{
					tUSh = pUSh[0];
					pUSh[0] = pUSh[1];
					pUSh[1] = tUSh;
				} */
				double *pFlBuf=(double *)zBuf;
				fprintf (pf, "%f\n", *pFlBuf);
			}
			break;
		case DATA_TYPE_BOOL:
			{
				bool *pShBuf=(bool *)zBuf;
				fprintf (pf, "%d\n", (int)*pShBuf);
			}
			break;
        default:
            break;
		}
	}

	file.close();
	fclose (pf);
}
template <>
CString CPrintExcel::Value2s<unsigned short> (unsigned short *pData, const char * /*zFormat*/, 
                             int nCols, int iLine, int iCol, float slope, float bias)
{
	CString s;
	unsigned short value = pData[iLine*nCols+iCol];
	if (slope != 0 || bias != 0)
	{
		double dValue = value * slope + bias;
		s = CMyWindows::FormatByScale(dValue);
	}
	else
		s.Format("%u",value);
	return s;
}
template <>
CString CPrintExcel::Value2s<unsigned int> (unsigned int *pData, const char * /*zFormat*/, 
                             int nCols, int iLine, int iCol, float slope, float bias)
{
	CString s;
	unsigned int value = pData[iLine*nCols+iCol];
	if (slope != 0 || bias != 0)
	{
		double dValue = value * slope + bias;
		s = CMyWindows::FormatByScale(dValue);
	}
	else
		s.Format("%u",value);
	return s;
}
template <>
CString CPrintExcel::Value2s<unsigned long> (unsigned long *pData, const char * /*zFormat*/, 
                             int nCols, int iLine, int iCol, float slope, float bias)
{
	CString s;
	unsigned long value = pData[iLine*nCols+iCol];
	if (slope != 0 || bias != 0)
	{
		double dValue = value * slope + bias;
		s = CMyWindows::FormatByScale(dValue);
	}
	else
		s.Format("%u",value);
	return s;
}
template <>
CString CPrintExcel::Value2s<long> (long *pData, const char * /*zFormat*/, 
                             int nCols, int iLine, int iCol, float slope, float bias)
{
	CString s;
	long value = pData[iLine*nCols+iCol];
	if (slope != 0 || bias != 0)
	{
		double dValue = value * slope + bias;
		s = CMyWindows::FormatByScale(dValue);
	}
	else
		s.Format("%d",value);
	return s;
}
template <>
CString CPrintExcel::Value2s<bool> (bool *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias)
{
	char zBuf[256];
	bool value = pData[iLine*nCols+iCol];
	if (slope != 0 || bias != 0)
		value = ((value * slope + bias) != 0);

	sprintf_s(zBuf,sizeof(zBuf),zFormat,value);
	CString s(zBuf);
	return s;
}
template <>
CString CPrintExcel::Value2s<S_RGB> (S_RGB *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias)
{
	char zBuf[256];
	S_RGB value = pData[iLine*nCols+iCol];
	if (slope != 0 || bias != 0)
		value = (S_RGB)(long)(value * slope + bias);

	sprintf_s(zBuf,sizeof(zBuf),zFormat,value);
	CString s(zBuf);
	return s;
}
template <typename T>
CString CPrintExcel::Value2s (T *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias)
{
	char zBuf[256];
	T value = pData[iLine*nCols+iCol];
	if (slope != 0 || bias != 0)
		value = (T)(value * slope + bias);

	sprintf_s(zBuf,sizeof(zBuf),zFormat,value);
	CString s(zBuf);
	return s;
}
template CString CPrintExcel::Value2s<float> (float *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias);
template CString CPrintExcel::Value2s<short> (short *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias);
template CString CPrintExcel::Value2s<int> (int *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias);
template CString CPrintExcel::Value2s<char> (char *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias);
template CString CPrintExcel::Value2s<double> (double *pData, const char *zFormat, 
                             int nCols, int iLine, int iCol, float slope, float bias);

/////////////////////////////////////////////////////////////////////////////
// CPrintExcel message handlers
