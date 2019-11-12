#include "StdAfx.h"
#include "HybridFile.h"
#include "FileFactory.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\FileName.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\ZoomPanParams.h"
#include "Screens.h"
#include "ImageView.h"
#include "DisplayRectangle.h"
#include "TypedData.h"

bool CHybridFile::umbAutoFocusOnNewFile = true;
int CHybridFile::unInstances = 0;
bool CHybridFile::umbSkipMyParams = true;

CHybridFile::CHybridFile(const char *zDirName, CXMLParseNode *pMetaDataXml)
: CMetaData (zDirName, pMetaDataXml)
, msDir(zDirName)
, mpCurrentFile(NULL)
, miFile (-1)
{
	if (!mpfLog)
	{
		static int i = 0;
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf),"CHybridFile%d",++i);
		mpfLog = CMyWindows::FOpenLogFile(zBuf);
	}

    CString sPattern(zDirName);
    sPattern += "\\*.*";
    CMyWindows::ListFiles(sPattern, mFileNamesList,true /*bRecursive*/);
	RemoveSomeFiles();
    if ((int)mFileNamesList.GetSize() < 1)
        return;

	int firstPos = 1;
	CString sPositionName("HybridPos");
	CPosition *pPosition = 	(CPosition *)CNamedObject::GetNamed(sPositionName);
	if (pPosition)
		firstPos = pPosition->miCurrent;
	else
		pPosition = new CPosition(sPositionName);
	SetPosition(pPosition);
	SetLeafRange(1, (int)mFileNamesList.GetSize());
    GetPosition()->miCurrent = 1;

    msName = zDirName;
	set_i(firstPos);

	if (unInstances == 0)
		gScreens.GetImageView()->AddMenuItem("Window", ID_WINDOW_HYBRID_STOP_AUTO, "Suspend Auto Focus");
	unInstances++;
}
CHybridFile::~CHybridFile(void)
{
	unInstances--;
	if (unInstances == 0 && !umDeleter.mbDoingGlobalDestruction)
		gScreens.GetImageView()->RemoveMenuItem("Window", ID_WINDOW_HYBRID_STOP_AUTO);
}
bool CHybridFile::set_i(int iImage)
{
    if (miFile == iImage)
        return true;

    if ((iImage <= 0) || (iImage > mFileNamesList.GetSize()))
        return false;

	miFile = iImage;
	if (mpCurrentFile)
	{
		delete mpCurrentFile;
		mpCurrentFile = NULL;
	}
	mnLinesInPage = 0;
	mnCols = 0;
	mpPageData = NULL;
	mpTypedData = NULL;
	CFileNamePattern::DisableFilesArray();

    POSITION pos = mFileNamesList.FindIndex(iImage - 1);
    if (pos)
    {
        msName = *mFileNamesList.GetNext(pos);
		CFileFactory factory(mpfLog);
		mpCurrentFile = factory.CreateFileObject(msName);
		if (mpCurrentFile)
		{
			mnLinesInPage = mpCurrentFile->GetNLinesInPage();
			mnCols = mpCurrentFile->GetNCols();
			mpPageData = mpCurrentFile->PageData();
			mMmPerPixelHeight = mpCurrentFile->MmPerPixelHeight();
			mMmPerPixelWidth = mpCurrentFile->MmPerPixelWidth();
			SetDataType(mpCurrentFile->GetDataType());
			mpTypedData = mpCurrentFile->GetTypedData();
			if (GetZoomParams())
				GetZoomParams()->SetMmPerPixel(MmPerPixelWidth());
			mpCurrentFile->AssignDisplayRectangle(*mpDR);
			if (mpCommonWindowParams && umbAutoFocusOnNewFile)
				mpCommonWindowParams->mbInitialized = false;

			if (gScreens.mbShowScanCenter && mpDR)
			{
				mpDR->DoGraphicCommand(GCOM_REMOVE, "ScanCenter");
				mpDR->AddScanCenter();
			}

			return true;
		}
    }
    return false;
}
bool CHybridFile::DoUpdatePageData(void)
{
	static FILE *upfLog = NULL;
	if (!upfLog)
		upfLog = CMyWindows::FOpenLogFile("CHybridFile__DoUpdatePageData");

	if (!mpCurrentFile)
		return false;
	if (mpDR && !mpCurrentFile->GetDR())
		mpCurrentFile->AssignDisplayRectangle(*mpDR);

	bool bOK = mpCurrentFile->UpdatePageData();
	if (upfLog)
	{
		fprintf (upfLog, "this %p %4d: %s\n", this, mUId, (const char*)mpCurrentFile->Name());
		fprintf (upfLog, "typed %p Avg %f\n", mpTypedData, mpTypedData->Average());
		fprintf (upfLog, "\n");
		fflush (upfLog);
	}
	return bOK;
}
void CHybridFile::RemoveSomeFiles(void)
{
	static int i = 0;
	char zBuf[128];
	sprintf_s(zBuf,sizeof(zBuf),"CHybridFile%d_Files",++i);
    CLogFile log (zBuf);
	int nInit = (int)mFileNamesList.GetSize();

	int nRemoved = 0;
	int nOK = 0;
	int nPrinted = 0;
	bool bFound = false;
	do
	{
		int i = 0;
		bFound = false;
		POSITION pos = mFileNamesList.GetHeadPosition();
		while (pos)
		{
			POSITION lastPos = pos;
			CFileName fName(*mFileNamesList.GetNext(pos));
			CString sPrivate(fName.Private());
			ULONG64 length = CMyWindows::GetFileLength(fName);
			bool bPrint = i++ >= nOK;
			if (log.mpf && bPrint)
				fprintf (log.mpf, "%3d: %s", ++nPrinted, (const char *)fName);

			if (fName.IsOfType("xml") 
				|| fName.IsOfType("txt") 
				|| fName.IsOfType("xls") 
				|| fName.IsOfType("doc")
				|| fName.IsOfType("cmel")
				|| (sPrivate == "DIRFILE")
				|| (umbSkipMyParams && fName.GetLastDirName() == "MyParams")
				|| (length < 100)
				)
			{
				if (log.mpf)
					fprintf (log.mpf, " Removed %d\n", ++nRemoved);
				mFileNamesList.RemoveAt(lastPos);
				bFound = true;
				break;
			}

			if (log.mpf && bPrint)
				fprintf (log.mpf, " OK %d\n", ++nOK);
		}
	} while (bFound);
	if (log.mpf)
		fprintf (log.mpf, "\n%d files found, %d removed, %d OK\n", nInit, nRemoved, nOK);
}
void CHybridFile::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("HybridFile",xmlDump);
    CMetaData::SaveState(xmlDump);
	xmlDump.Write("dir_name", msDir);
	xmlDump.Write("i_file", miFile);
	{
		CXMLDumpScope graph("files_list",xmlDump);

		POSITION pos = mFileNamesList.GetHeadPosition();
		while (pos)
		{
			CString sfName(*mFileNamesList.GetNext(pos));
			xmlDump.Write("name", sfName);
		}
	}
	if (mpCurrentFile)
		mpCurrentFile->SaveState(xmlDump);
}
CMetaData *CHybridFile::GetDataWithText(void)
{
	if (mpCurrentFile)
		return mpCurrentFile;
	return this;
}
void CHybridFile::SetNewDate(const char * zNewDate)
{
    char zBuf[512];
	int n = (int)mFileNamesList.GetSize();
    sprintf_s (zBuf, sizeof(zBuf), "First Image: %s\nNew Date %s\nAre you sure you want to changed data for %d images?",
        (const char*)Name(), zNewDate, n);
    int rc = MessageBox(NULL, zBuf, "Change Dicom Date Confirmation", MB_OKCANCEL);
    if (rc != IDOK)
		return;

    int nChanged = 0;
    CLogFile log ("CHybridFile_SetNewDate");

	POSITION pos = mFileNamesList.GetHeadPosition();
    while (pos)
    {
        CString sName = *mFileNamesList.GetNext(pos);
		CMyDicom *pDicom = CFileName::GetDicomFile(sName);
		if (pDicom)
		{
			int nFieldsChanged = pDicom->ChangeDicomDate(zNewDate);
			if (nFieldsChanged)
			{
				if (log)
					fprintf (log.mpf, "CHANGED %d: %s\n", nFieldsChanged, (const char*)sName);
				nChanged++;
			}
			else
			{
				if (log)
					fprintf (log.mpf, "NO CHANGE: %s\n", (const char*)sName);
				MessageBox (NULL, sName, "NO CHANGE", MB_OK);
			}

			delete pDicom;
		}

	}
    sprintf_s (zBuf, sizeof(zBuf), "%d / %d images updated", nChanged, n);
    MessageBox(NULL, zBuf, "Dicom Date Change Finished", MB_OK);
}
bool CHybridFile::GetPageDataByIndex(CMyImage * pResultImage, int iImage, CString *psName)
{
	set_i(iImage);
	if (mpCurrentFile)
		return mpCurrentFile->GetPageDataByIndex(pResultImage, 1, psName);
    return false;
}
bool CHybridFile::Is(const char * zName)
{
	POSITION pos = mFileNamesList.GetHeadPosition();
    while (pos)
    {
        CString sName = *mFileNamesList.GetNext(pos);
		if (sName == zName)
			return true;
	}
    return CNamedObject::Is(zName);
}

