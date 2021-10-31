#include "StdAfx.h"
#include ".\metafile.h"
#include ".\TypedData.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyTracer.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include <math.h>
#include "Screens.h"

const char CMetaFile::caDATA_VIEW_PAR_FILE_NAME[] = "dvpar\\dataview.dvpar";


CMetaFile::CMetaFile(const char *zName, CXMLParseNode *pMetaDataXml)
: CMetaData (zName, pMetaDataXml)
, mName (zName)
, mpDicom (NULL)
, mOffsetBytes(0)
, mLength(0)
, mbSpecPar (false)
, mhFile (INVALID_HANDLE_VALUE)
, mCtBias (CMyDicom::GetDefaultCtBias())
, mbCtBiasSet (false)
, mbDicomMatrix (false)
, mPlanarConfig(0) // Keep RGB in 3 separate consecutive plans
, mbDicomSetOnce(false)
, mbZeroNanOnRead(false)
{
	int offset = 0;
    CFileName fName(msName);
    CString sPrivate(fName.Private());
    if (CMyWindows::GetValueFromString(sPrivate, "_ofs", offset))
		mOffsetBytes = offset;

	int nanValue = -1;
    if (CMyWindows::GetValueFromString(sPrivate, "_nan", nanValue))
		if (nanValue == 0)
			mbZeroNanOnRead = true;

	GetFileLength();

	if (mnCols > 0 && !mnLinesInPage)
	{
		ULONGLONG dataLength = mLength - mOffsetBytes;
		unsigned int lineLen = mnCols * GetUnitSize();
		if (dataLength > 0 && lineLen)
		{
			mnLinesInPage = (int) ((dataLength + lineLen - 1) / lineLen);
			if (mnLinesInPage > 512)
				GetDefaultNLines();
		}
	}
}

CMetaFile::~CMetaFile(void)
{
    if (mpDicom)
        delete mpDicom;
}
float CMetaFile::GetCtBias (void) 
{
	if (!mbCtImage)
		return 0.0f;

	if (!mbCtBiasSet)
	{
		if (mpDicom)
		{
			mCtBias = mpDicom->GetCtBias();
			mbCtBiasSet = true;
		}
	}
	return mCtBias;
}
bool CMetaFile::GetMmPerPixel(double &mmppWidth, double &mmppHeight)
{
	if (!mpDicom)
		return false;

    double dx = 0.0, dy = 0.0;
    if (mpDicom->GetPixelSpacing(dx, dy) && dx > 0.01 && dy > 0.01)
	{
		mmppWidth = dx;
		mmppHeight = dy;
		return true;
	}
	return false;
}
void CMetaFile::SetOffsetBytes(ULONGLONG offset) 
{
    if (mOffsetBytes == offset)
        return;

    mOffsetBytes = offset;
    mbPageUpdated = false;
}
void CMetaFile::OnStartUpdate (void)
{
	static int count = 0;
	CMyTracer myTracer("CMetaFile::OnStartUpdate", ++count);
    FILETIME modTime = CFileName::GetModificationTime(z_name());
    mLastModificationRead = modTime;
    if (umpfUpdatesLog)
    {
        CTime time(modTime);
        CString sTime (time.Format("%c"));
        fprintf (umpfUpdatesLog, "OnStartUpdate: (File Time <%s>) %s(%d)\n", 
            (const char *)sTime, (const char *)msName, mUId);
        fflush (umpfUpdatesLog);
    }
}
bool CMetaFile::ShouldUpdate(void)
{
    if (CDependant::ShouldUpdate())
        return true;

    FILETIME modTime = CFileName::GetModificationTime(z_name());
    if (mLastModificationRead.dwLowDateTime != modTime.dwLowDateTime)
    {
        if (umpfUpdatesLog)
        {
            SYSTEMTIME sysTime;
            FileTimeToSystemTime(&modTime, &sysTime);
            CTime time(sysTime);
            //time.GetAsSystemTime(sysTime);
            CString sTime (time.Format("%c"));
            fprintf (umpfUpdatesLog, "Diff File Time <%s>--> %s(%d)\n", 
                (const char *)sTime, (const char *)msName, mUId);
            fflush (umpfUpdatesLog);
        }
        mLastModificationRead = modTime;
        return true;
    }

    return false;
}
void CMetaFile::ReReadDicom(void)
{
	if (mpDicom)
		delete mpDicom;
	CMyDicom * pDicom = CFileName::GetDicomFile(Name());
	SetDicom(pDicom);
}
bool CMetaFile::open()
{
	if (mhFile != INVALID_HANDLE_VALUE)
		close();

    mhFile = CreateFile(z_name(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, 0, NULL);

    if (mhFile == INVALID_HANDLE_VALUE)
	{
		CString sErrText("<CMetaFile::open> Can't open file\n");
        sErrText += msName;
        MessageBox(NULL, (const char *)sErrText, "ERROR", MB_ICONERROR | MB_OK);
        return false;
    }

    GetFileLength();

	OnOpen(); // Virtual function for type-specific extra measures

	if (mpfLog)
		fprintf (mpfLog, "<CMultiFile::open> %s Offset %d\n", z_name(), (int)mOffsetBytes);
	return true;
}
void CMetaFile::GetFileLength(void)
{
    mLength = CMyWindows::GetFileLength(msName);
	if (mLength < 100)
	{
		static int unWarn = 0;
		if (!unWarn++)
		{
			CString s;
			if (mLength < 1)
				s = "<CMetaFile::GetFileLength> EMPTY file <" + msName + ">";
			else
			{
				char zBuf[128];
				_itoa_s((int)mLength, zBuf, 128, 10);
				s = "<CMetaFile::GetFileLength> SHORT file <" + msName + "> len " + zBuf;

			}
				//s.Format("<CMetaFile::GetFileLength> SHORT file <%s> len %d", msName, (int));
			CMyWindows::MessBox(s,"Warning");
		}
	}
}
void CMetaFile::OnOpen(void)
{
    if (!mDataType)
        SetDataType(DATA_TYPE_USHORT);

    if (mnCols == 0)
    {
        long n = (long)(mLength / sizeof (unsigned short));
        mnCols = (int)sqrt((float)n);
        mnLinesInPage = mnCols;
    }
}

void CMetaFile::close()
{
	if (mhFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle (mhFile);
		mhFile = INVALID_HANDLE_VALUE;
        if (mpTypedData)
        {
            if (mpTypedData->Min() < MIN_CT_VALUE || mpTypedData->Max() > MAX_CT_VALUE)
                mbCtImage = false;
        }
	}
}
//--------------------------------------------------------------------
void CMetaFile::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("MetaFile",xmlDump);
    CMetaData::SaveState(xmlDump);
    xmlDump.Write("length",mLength);
    xmlDump.Write("offset",mOffsetBytes);
}
bool CMetaFile::GetDefaultNLines(void)
{
	if (CFileName::StaticIsOfType(msName,"prep"))
        mnLinesInPage = 580; // (int)(mLength / (mnCols * sizeof(short)));
	else
        mnLinesInPage = 512; // (int)(mLength / (mnCols * sizeof(short)));
	return true;
}
void CMetaFile::SetDicom(class CMyDicom *pDicom)
{
	mpDicom = pDicom;
	if (!mpDicom)
		return;

	ReadRescaleAndSlope();

	if (!mbDicomSetOnce)
	{
		mbDicomSetOnce = true;
		SetDicomOnce();
	}
}
void CMetaFile::ReadRescaleAndSlope(void)
{
	if (!mpDicom->GetFloatTextField(RESCALE_INTERCEPT_GROUP, RESCALE_INTERCEPT_NUM, mRescaleIntercept))
		mRescaleIntercept = 0;
	if (!mpDicom->GetFloatTextField(RESCALE_SLOPE_GROUP, RESCALE_SLOPE_NUM, mRescaleSlope))
		mRescaleSlope = 1;
}
void CMetaFile::SetDicomOnce() 
{
    CPoint imageShape(0,0);
    if (mpDicom->GetImageShape(imageShape))
    {
        mnLinesInPage = imageShape.y;
        mnCols = imageShape.x;
    }
    else
    {
       
            mnLinesInPage = 512;
            mnCols = 512;
    }

    double dx = 0.0, dy = 0.0;
    if (GetMmPerPixel(dx, dy))
	{
		//if (!mpXmlNode)
	    SetMmPerPixel((float)dx, (float)dy, true /*bOnly Once*/);
		CMetaData *pCurData = gScreens.GetCurrentData();
		if (pCurData && !pCurData->mbMmPerPixelSet)
			pCurData->SetMmPerPixel(*this);
	}

}
void CMetaFile::ZeroNan()
{
	if (mDataType == DATA_TYPE_FLOAT)
	{
		float* pData = (float*)mpPageData->GetDataStart();
		int pageLen = mnLinesInPage * mnCols;
		for (int i = 0; i < pageLen; i++)
		{
			float value = pData[i];
			if (_isnanf(value) || !_finitef(value))
				pData[i] = 0;
		}
	}
	else if (mDataType == DATA_TYPE_DOUBLE)
	{
		double* pData = (double*)mpPageData->GetDataStart();
		int pageLen = mnLinesInPage * mnCols;
		for (int i = 0; i < pageLen; i++)
		{
			double value = pData[i];
			if (_isnan(value) || !_finite(value))
				pData[i] = 0;
		}
	}
}
