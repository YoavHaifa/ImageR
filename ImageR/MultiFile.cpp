// MultiFile.cpp: implementation of the CMultiFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include ".\multifile.h"
#include "ImageView.h"
#include "..\..\yUtils\MyDicom.h"
#include <math.h>
#include "TypedData.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\TVolume.h"
#include "..\..\ImageRLib\CtImages.h"
#include "Oblique.h"
#include "SelectParamsDialog.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\LogFile.h"
#include "..\..\yUtils\MyProgress.h"
#include "..\..\yUtils\NameGetDialog.h"
#include "Screens.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#pragma warning( disable : 4355 )

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMultiFile::CMultiFile(const char *zName, CXMLParseNode *pMetaDataXml)
: CMetaFile (zName, pMetaDataXml)
, miFile (0)
, mbUpdateViewerOnSetName(false)
, mNamePattern(zName, this)
, mbIsMx8000Prep(false)
{
    mMatrix = 512;
    mbSingle = false;

	AddCheck(&mNamePattern);

	miFile = mNamePattern.GetInitialImageIndex();
	CFileNamePattern::SavePattern(&mNamePattern);
}
CMultiFile::~CMultiFile()
{
	CFileNamePattern::ReleasePattern(&mNamePattern);
}
void CMultiFile::SetDicom(class CMyDicom *pDicom) 
{
	CMetaFile::SetDicom(pDicom);
    if (!mpDicom)
        return;

	if (pDicom->IsRGB(mPlanarConfig))
		SetTypedDataHandler(DATA_TYPE_RGB);
	else if (pDicom->GetPixelRepresentation()) // 0-unsigned, 1-signed
		SetTypedDataHandler(DATA_TYPE_SHORT);
	else
		SetTypedDataHandler(DATA_TYPE_USHORT);


	if (mpCtImages)
		mpCtImages->SetDicomForCtImages(mpDicom);
}
//--------------------------------------------------------------------
bool CMultiFile::set_i(int iImage)
{
    if (mNamePattern.IsEmpty())
       return iImage == miFile;

	if (iImage < 0)
		return false;

    CString sName (GetFileNameOfPage(iImage));
    if (!CFileName::Exist(sName))
    {
        return false;
    }

	miFile = iImage;
	msName = sName;

	return true;
}
//--------------------------------------------------------------------
//--------------------------------------------------------------------
//--------------------------------------------------------------------
void CMultiFile::set_file_offset(ULONGLONG Offset)
{
	Offset += mOffsetBytes;
    LARGE_INTEGER li;
    li.QuadPart = Offset;
	SetFilePointerEx (h(), li, NULL, FILE_BEGIN);
}
bool CMultiFile::ContainsFile(const char *zName)
{
	if (mNamePattern.IsList())
		return mNamePattern.ContainsFile(zName);

    CFileName myName(msName);
    CString sMyPath = myName.Path();
    sMyPath.MakeLower();

    CFileName name(zName);
    CString sPath = name.Path();
    sPath.MakeLower();

    return sMyPath == sPath;
}
//--------------------------------------------------------------------
//--------------------------------------------------------------------
/*bool CMultiFile::read(FILE *pf)
{
    size_t nRead = fread((void *)&mId, sizeof(mId), 1, pf);
	if (nRead < 1)
		return false;

	AnalyzeFileName();

	if (open())
	{
		close();
		return true;
	}

	return false;
}*/
//--------------------------------------------------------------------
//--------------------------------------------------------------------
//--------------------------------------------------------------------
// Check for Mx8000 Prep (For GALAI)

/*bool CMultiFile::CheckForMx8000Prep()
{
    if (mName.msName.Right(2) == "11")
	//size_t privateNameLen = strlen(mzFilePrivateName);
	//if (mzFilePrivateName[privateNameLen-1] == '1' && mzFilePrivateName[privateNameLen-2] == '1')
	{
		char zNewName [256];
		strcpy (zNewName, mId.mzFileName);
		strcat (zNewName, ".Mx8000_prep");

		if (mpfLog)
			fprintf (mpfLog, "converting %s --> %s\n", mId.mzFileName, zNewName);
		convert_char (mId.mzFileName, zNewName);
		strcpy (mId.mzFileName, zNewName);
		mName.Set (mId.mzFileName);
		if (mpfLog)
			fprintf (mpfLog, "Finished converting %s\n", zNewName);
	}
	if (IsOfType ("Mx8000_prep"))
    {
		print_mx8000_dicom();
        return true;
    }
    return false;
} */
//--------------------------------------------------------------------
//--------------------------------------------------------------------
/*
bool CMultiFile::read_image (int iImage, MyImage &Image)
{
	short *pRaster = Image.p_raster();
	DWORD nBytesToRead = Image.byte_len();
	DWORD nBytesRead = 0;

	ULONGLONG OffsetBytes;
	if (IsMultiFile())
	{
		if (!open())
			return false;
		OffsetBytes = 0;
	}
	else
		OffsetBytes = nBytesToRead * iImage;

	set_file_offset (OffsetBytes);
	
	BOOL bOK = ReadFile (mhFile, pRaster, nBytesToRead, &nBytesRead, NULL);

	if (!bOK)
	{
		DWORD ErrorId = GetLastError ();
		char *lpMsgBuf;
		FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				ErrorId,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
		MessageBox (NULL, lpMsgBuf, "ReadFile Error", MB_OK);
	}
	if (nBytesRead < nBytesToRead)
		return false;

	return true;
} */
//--------------------------------------------------------------------
typedef struct SSegmentOrDataTag
{
	unsigned short group;
	unsigned short num;
	char	vr[2];
}SSegmentOrDataTag;


void CMultiFile::convert_char (const char *zfName, const char *zfOut)
{
	FILE *pfIn = MyFOpen (zfName, "rb");
	if (!pfIn)
		return;

	FILE *pfOut = MyFOpen (zfOut, "wb");
	if (!pfOut)
	{
		fclose (pfIn);
		return;
	}

	while (1)
		{
		unsigned char buf[2];
		unsigned char cbuf[2];
		size_t nRead = fread (buf, sizeof(unsigned char), 2, pfIn);
		if (nRead != 2)
			break;
		cbuf[1]=buf[0];
		cbuf[0]=buf[1];
		fwrite (cbuf, sizeof(unsigned char), 2, pfOut);
		}

	fclose (pfIn);
	fclose (pfOut);
}
/*
void CMultiFile::print_mx8000_dicom()
{
	CMyDicom MyDicom (z_name());
	MyDicom.PrintMx8000 ();

	if (MyDicom.mPrepOffset)
	{
		mOffsetBytes = MyDicom.mPrepOffset;
		mbIsMx8000Prep = true;
	}

} */
DWORD WINAPI CMultiFile::StaticSelectByParameters (LPVOID p)
{
    CMultiFile *pMultiFile = (CMultiFile *)p;
    pMultiFile->SelectByParameters();
    return 0;
}
bool CMultiFile::SelectByParameters(void)
{
    CSelectParamsDialog Dlg(NULL, z_name());
    Dlg.mpMultiFile = this;
    mbUpdateViewerOnSetName = true;
    INT_PTR rc = Dlg.DoModal();
    mbUpdateViewerOnSetName = false;
    if (rc == IDOK)
    {
        return true;
    }
    return false;
}
void CMultiFile::PrintId(FILE * pf, const char * zText)
{
    CMetaData::PrintId (pf, zText);
	if (IsMultiFile())
		fprintf (pf, "MultiFile <%s> i <%d>\n",
			(const char*)mNamePattern.GetPattern(), i());
}
bool CMultiFile::guess_matrix_and_offset()
{

	// Open to set file length
	if (!open ())
		return false;
	close ();
	if (mbDicomMatrix)
		return true;

	static int maMatrices[5] = {1024, 768, 512, 340, 256};
	mOffsetBytes = 0;

	for (int i = 0; i < 5; i++)
	{
		int ImgLen = maMatrices[i] * maMatrices[i] * sizeof(short);
		if (mLength >= ImgLen)
		{
			mOffsetBytes = mLength - ImgLen;
			mMatrix = maMatrices[i];
			mbDicomMatrix = true;
			break;
		}
	}

	if (mOffsetBytes > 0 && mOffsetBytes < 100000)
		return true;

	return false;
}

CString CMultiFile::GetFileNameOfPage(int iPage)
{
    if (mNamePattern.IsEmpty())
        return msName;

	return mNamePattern.GetFileNameOfPage(iPage);
}

void CMultiFile::ChangeDicomDateToAllFiles(CString sNewDate)
{
    CString sLogName = PrivateName() + ".change_date.log";
    for (int iFile = 0; iFile < 1000; iFile++)
    {
        CString sLog = CMyWindows::GetLogDir();
        sLog += "\\";
        sLog += sLogName;
        if (!CFileName::Exist(sLog))
            break;
        char zBuf[256];
        sprintf_s (zBuf, 256, "%s.%d.change_date.log", (const char*)PrivateName(), iFile);
		sLogName = zBuf;
    }
    CLogFile log (sLogName);

	int n = GetNDataUnits();
    int nChanged = 0;
    CMyDicom *pDicom = NULL;

	for (int i = StartLoopI(); i >= 0; i = LoopNext())
    {
		CString sName (mNamePattern.GetName());
        pDicom = CFileName::GetDicomFile(sName);
        if (!pDicom)
        {
            if (log)
                fprintf (log.mpf, "NOT DICOM: %s\n", (const char*)sName);
            MessageBox (NULL, sName, "Not Dicom", MB_OK);
            continue;
        }

        int nFieldsChanged = pDicom->ChangeDicomDate(sNewDate);
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
        pDicom = NULL;
        if ((nChanged % 10 == 0) || (i == n - 1))
        {
            char zBuf[64];
            sprintf_s (zBuf, 64, "%d updated", nChanged);
            CMyWindows::PrintStatus(zBuf);
        }
    }

    if (pDicom)
        delete pDicom;
    char zBuf[256];
    sprintf_s (zBuf, 256, "%d / %d images updated", nChanged, n);
    MessageBox(NULL, zBuf, "Dicom Date Change Finished", MB_OK);
}


void CMultiFile::SaveState(class CXMLDump & xmlDump)
{
    if (HasPosition())
        set_i(GetPosition()->miCurrent); // In case it is not currently displayed

    CXMLDumpScope graph("MultiFile",xmlDump);
    CMetaFile::SaveState(xmlDump);
    if (mDumpCount)
        return;

    mNamePattern.SaveState(xmlDump);
}
bool CMultiFile::Is(const char * zName)
{
	CString sPattern(mNamePattern.GetPattern());
    if (sPattern == zName)
        return true;
    return CNamedObject::Is(zName);
}
bool CMultiFile::DoUpdatePageData()
{
	void *pData = mpPageData->GetDataStart();
	unsigned long nBytes = mpPageData->GetByteSize();
	mpPageData->Zero();
	CString sfName(mNamePattern.GetName());
	FILE *pf = MyFOpen(sfName, "rb");
	if (!pf)
	{
		char zBuf[256];
		sprintf_s(zBuf, 256, "<CMultiFile::DoUpdatePageData> Failed to open <%s>", (const char*)sfName);
		MessageBox(NULL, zBuf, "File Open Error", MB_OK);
		return false;
	}
	fread (pData, 1, nBytes, pf);
	fclose (pf);

	if (mpfLog)
		fprintf (mpfLog, "<CMultiFile::DoUpdatePageData> pData %p, nBytes %d\n", pData, nBytes);

	return true;
}
bool CMultiFile::SetDir(int iDir)
{
	if (mNamePattern.GetPosition(1))
	{
		mNamePattern.SetDir(iDir);
		return true;
	}
	return false;
}
void CMultiFile::CheckDicomSeries(void)
{
	if (!mpDicom)
		return;
	CLogFile fLog("DicomSeries");
	if (!fLog)
		return;

	CMyDicomCheckSeries series;
	for (int i = StartLoopI(); i >= 0; i = LoopNext())
    {
		CString sName (mNamePattern.GetName());
        CMyDicom *pDicom = CFileName::GetDicomFile(sName);
        if (!pDicom)
        {
            MessageBox (NULL, sName, "Not Dicom", MB_OK);
            continue;
        }

		pDicom->CheckSeries(fLog.mpf, series);
	}
	fLog.CloseAndDisplay();

}
void CMultiFile::AmendAllByXml(void)
{
	if (!mpDicom)
		return;

	CFileDialog readDlg(TRUE);
	CString sConfPath = CMyWindows::GetLogDir("Config");
    readDlg.m_ofn.lpstrInitialDir = (const char *)sConfPath;
	readDlg.m_ofn.lpstrTitle = "Select XML File";
	readDlg.m_ofn.lpstrDefExt = "xml";
	readDlg.m_ofn.lpstrFilter = "Xml File\0*.xml\0\0\0";

	if (readDlg.DoModal() != IDOK)
		return;
	CFileName fNameXML(readDlg.m_ofn.lpstrFile);
	CXMLParse xmlParse(fNameXML);
	CXMLParseNode *pXmlRoot = xmlParse.GetRoot();
	if (!pXmlRoot)
	{
		CMyWindows::MessBox("Error: No Proper XML Data", fNameXML);
		return;
	}

	CString sPath = GetPath();
	CString sSubDir("Amended");
	CXMLParseNode *pXmlDirName = pXmlRoot->GetFirstDeep("dir");
	if (pXmlDirName)
		pXmlDirName->GetValue(sSubDir);
	sPath += sSubDir;
	if (!CMyWindows::VerifyDirectory(sPath))
	{
		CMyWindows::MessBox("Error: Failed to create directory", sPath);
		return;
	}

	int nChanged = 0;
	int n = GetNDataUnits();
	CMyProgress myProgress (0, (short)n); 

	for (int i = StartLoopI(); i >= 0; i = LoopNext())
    {
		CString sName (mNamePattern.GetName());
        CMyDicom *pDicom = CFileName::GetDicomFile(sName);
        if (!pDicom)
        {
            MessageBox (NULL, sName, "Not Dicom", MB_OK);
            continue;
        }

		CString sNewName (sPath + "\\");
		CFileName fName(sName);
		sNewName += fName.Private();
		pDicom->EditByXmlAndSave(pXmlRoot, sNewName);

        delete pDicom;
        pDicom = NULL;
		nChanged++;
		myProgress.SetPos(i);

        if ((nChanged % 10 == 0) || (i == n - 1))
        {
            char zBuf[64];
            sprintf_s (zBuf, 64, "%d updated", nChanged);
            CMyWindows::PrintStatus(zBuf);
        }
    }

    char zBuf[256];
    sprintf_s (zBuf, 256, "%d images updated", nChanged);
    MessageBox(NULL, zBuf, "Dicom Change Finished", MB_OK);
}
void CMultiFile::SetNewPosition(void)
{
	mNamePattern.SetNewPosition();
	SetPosition (mNamePattern.GetPosition(0));
	CMetaData::SetNewPosition();
}
CString CMultiFile::GetLastName(void)
{
	return mNamePattern.GetLastName();
}
void CMultiFile::SetSecondLeafingIndex()
{
	CPosition *pPos = mNamePattern.SetSecondLeafingIndex();
	if (pPos)
		mMultiPosition.Add(pPos);
}
void CMultiFile::ReverseFilesOrder(void)
{
	CPosition *pPos = GetPosition();
	ReorderFiles(pPos->miLast, pPos->miFirst, -pPos->mStep, 1, pPos->mStep, "reversed");
}
void CMultiFile::ReorderFiles(void)
{
	CPosition *pPos = GetPosition();
	int iFirst, iLast, step;
	if (!pPos->SelectSubset(iFirst, iLast, step))
		return;

	int iOut = 1;
	if (!CNameGetDialog::GetIntValue("Select first output index", iOut))
		return;

	int iOutStep = abs(step);
	if (!CNameGetDialog::GetIntValue("Select output step", iOutStep))
		return;

	ReorderFiles(iFirst, iLast, step, iOut, iOutStep);
}
void CMultiFile::ReorderFiles(int iFirst, int iLast, int step, int iOut, int iOutStep, const char *zPrefix)
{
	char zBuf[256];
	CPosition *pPos = GetPosition();

	CFileName fName(Name());
	int absStep = abs(step);
	if (absStep == 1)
		sprintf_s(zBuf,sizeof(zBuf),"%s\\ReOrdered_%d_%d", (const char*)fName.Path(), iFirst, iLast);
	else
		sprintf_s(zBuf,sizeof(zBuf),"%s\\ReOrdered_%d_%d_step%d", (const char*)fName.Path(), iFirst, iLast, absStep);
	CString sOutDir(zBuf);
	if (!CMyWindows::VerifyDirectory(sOutDir))
		return;

	CString sPrefix(_T("im"));
	if (zPrefix)
		sPrefix = zPrefix;
	if (!CNameGetDialog::GetStringValue("Select image name prefix", sPrefix))
		return;

	int n = abs(iLast - iFirst) / abs(step) + 1;
	CMyProgress prog(0, (short)n);
	int nSaved = 0;
	CLogFile log("CMultiFile__ReorderFiles");
	if (log)
	{
		fprintf(log.mpf,"reorder %d - %d step %d\n", iFirst, iLast, step);
	}

	pPos->SaveCurrentPosition();
	if (pPos->TrySetCurrent(iFirst))
	{
		while (nSaved < n)
		{
			CString sSrc = GetFileNameOfPage(pPos->miCurrent);
			sprintf_s(zBuf,sizeof(zBuf),"%s\\%s_%03d.dat", (const char*)sOutDir, (const char*)sPrefix, iOut);
			if (log)
				fprintf(log.mpf,"Copy %s --> %s\n", (const char*)sSrc, zBuf);
			if (!CMyWindows::MyCopyFile(sSrc,zBuf))
			{
				if (log)
					fprintf(log.mpf,"Copy Failed\n\n");
				break;
			}
			nSaved++;
			iOut += iOutStep;
			prog.SetPos(nSaved);

			if (iLast >= iFirst)
			{
				if (pPos->miCurrent >= iLast)
					break;
				int iNext = min(pPos->miCurrent + step, iLast);
				if (!pPos->TrySetCurrent(iNext))
					break;
			}
			else // (iLast < iFirst)
			{
				if (pPos->miCurrent <= iLast)
					break;
				int iNext = max(pPos->miCurrent + step, iLast);
				if (!pPos->TrySetCurrent(iNext))
					break;
			}
		}
	}
	if (log)
		fprintf(log.mpf,"\n%d files reordered\n", nSaved);
	pPos->RestoreSavedPosition();
	sprintf_s(zBuf,"%d  reordered files in %s", nSaved, (const char*)sOutDir);
	CMyWindows::MessBox(zBuf,"Notice");
}
bool CMultiFile::DoLoadVolume()
{
	if (!mpVolume)
		return false;

    unsigned char *pVolumePage = (unsigned char *) mpVolume->GetPageVP(GetPosition()->miFirst);
	if (!pVolumePage)
        return false;

    int nToLoad = GetNPages();
	mpVolume->mnLoaded = 0;
    unsigned int pageLenBytes = mnLinesInPage * mnCols * mDataUnitSize;
    //unsigned int pageSize = pageLen * sizeof(pVolumePage[0]);
    if (mpfLog)
    {
        fprintf (mpfLog, "<CMultiFile::DoLoadVolume> Start Loading %d\n", nToLoad);
        fflush (mpfLog);
    }

	//bool bLoop = StartLoop();
	//int iFirst = mpVolume->GetIFirst();
	//int iLast = mpVolume->GetILast();
	CPosition* pPos = mMultiPosition.GetPosition(0);
	for (int iLoad = StartLoopI(); iLoad >= 0 && iLoad < nToLoad; iLoad = LoopNext())
    {
		//int iLoad = pPos->GetCurrent();
		int iLoadRelative = pPos->GetCurrentRelative();

		if (!DoLoadVolumeImage(iLoadRelative, pVolumePage))
		{
			CString s;
			s.Format("%s Failed to load relative page %d", Name(), iLoadRelative);
			CMyWindows::MessBox(s, "Load Volume Error");
			return false;
		}

		//mpVolume->mnLoaded++;
        int ii = iLoadRelative + 1;
        if ((gScreens.mVolumeRefreshInterval && (ii % gScreens.mVolumeRefreshInterval == 0)) 
			|| (ii == nToLoad))
        {
            if (mpVolume->OnLoad(ii))
                gScreens.Refresh();
        }
        
        char zBuf[32];
        sprintf_s (zBuf, 32, "LV%03d", ii);
        gScreens.PrintStatusInBar(CScreens::SBAR_GENERAL,zBuf);
        pVolumePage += pageLenBytes;
    }
    if (mpfLog)
    {
        fprintf (mpfLog, "<CMultiFile::DoLoadVolume> %d loaded\n", nToLoad);
        fflush (mpfLog);
    }
	mbLoadingVolume = true;
	gScreens.Refresh(); // Re-display original current image
    return true;
}
bool CMultiFile::DoLoadVolumeImage(int /*iLoad*/,void * pVolumePage)
{
	mbPageUpdated = false;
	mbUpdateForLoadingVolume = true;
	bool bOK = UpdatePageData();
	mbUpdateForLoadingVolume = false;
	if (!bOK)
		return false;

	if (!mpPageData->CopyRasterTo(pVolumePage))
		return false;

	return true;

	/*
	static int unWarning = 0;
	if (!unWarning++)
	{
		char zBuf[256];
		sprintf_s(zBuf,sizeof(zBuf),"Class <%s> should implement \"DoLoadVolumeImage()\" to see volumetric data", GetClass());
		CMyWindows::MessBox(zBuf, "SW Warning");
	}
    if (mpPageData)
        mpPageData->mbUpdateFailed = true;
	return false; */
}
