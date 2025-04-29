#include "StdAfx.h"
#include ".\MultiDataFile.h"
#include "..\..\yUtils\MyWindows.h"
#include "Screens.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\TVolume.h"
#include <math.h>
#include "..\..\yUtils\XmlParse.h"
#include "..\..\yUtils\XMLDump.h"
#include "ImageParams.h"
#include "TypedData.h"
#include "FileTypes.h"

CMultiDataFile::CMultiDataFile(const char *zfName, 
	CXMLParseNode *pMetaDataXml, 
	unsigned int fileHeaderBytes, 
	EMyImageDataType eDefaultDataType,
	bool bComputeNPages)
: CMultiFile (zfName, pMetaDataXml)
, mFilePageLenNeto(0)
, mDisplayPageLenNeto(0)
, mFilePageLenBruto(0)
, mnPages(1)
, mpPageHeader(NULL)
{
	if (fileHeaderBytes > 0)
		mPageHeaderLength = fileHeaderBytes;

	if (mName.IsOfType("ximg"))
		GetInfoFromXMLHeader();

	if (!mnCols || !mnLinesInPage)
	{
		if (!TryToGetImageSizeFromXml())
			gFileTypes.SearchForType(zfName, this);
	}

	

    mLength = CMyWindows::GetFileLength(msName);

	if (mDataUnitSize == 0)
	{
		if (eDefaultDataType)
			SetTypedDataHandler(eDefaultDataType);
		else
			SetTypedDataHandler(DATA_TYPE_SHORT);
	}

    if (!mnLinesInPage || !mnCols)
		GuessMatrix();

	if (bComputeNPages)
		ComputeNPages();

	CMetaData* pSizeData = gScreens.GetCurrentFileData(true/*bOrAny*/);
	if (pSizeData)
        SetMmPerPixel(*pSizeData);
	Verify("<CMultiDataFile> On Construct");
}

CMultiDataFile::~CMultiDataFile(void)
{
	if (mpPageHeader)
		delete mpPageHeader;
}
void CMultiDataFile::ComputeFilePageLenNeto()
{
	mDisplayPageLenNeto = mnLinesInPage * mnCols * GetUnitSize();
	mFilePageLenNeto = mDisplayPageLenNeto;
}
unsigned int CMultiDataFile::ComputeNPages()
{
	ComputeFilePageLenNeto();
	mFilePageLenBruto = mFilePageLenNeto + mPageHeaderLength;
    ULONGLONG allPagesSize = mLength - mOffsetBytes;

	if (mFilePageLenBruto > 0)
	    mnPages = (unsigned int)((allPagesSize + mFilePageLenBruto - 1) / mFilePageLenBruto);
	else
		mnPages = 0;

	if (mnPages > 1)
	{
		AddFirstPosition(new CPosition("MultiDataPos"));
		SetLeafRange(1, mnPages);
		GetPosition()->mbStayInLimits = true;
	}

	return mnPages;
}
void CMultiDataFile::LoadPrologue()
{
	ComputeFilePageLenNeto();
	mFilePageLenBruto = mFilePageLenNeto + mPageHeaderLength;
    unsigned long nBytes = mpPageData->GetByteSize();
    if (mDisplayPageLenNeto > nBytes)
        mDisplayPageLenNeto = nBytes;

    open();
}
bool CMultiDataFile::SetOffset(ULONGLONG offset)
{
	if (offset > 0)
	{
		LARGE_INTEGER liOffset = { 0 };
		liOffset.QuadPart = offset;
		return SetFilePointerEx(mhFile, liOffset, NULL, FILE_BEGIN) != 0;
	}
	return true;
}
DWORD CMultiDataFile::ReadPage(unsigned char* pDest)
{
	DWORD nBytesRead = 0;
	BOOL bOK = ReadFile(mhFile, pDest, mFilePageLenNeto, &nBytesRead, NULL);
	if (!bOK)
	{
		char zBuf[256];
		sprintf_s(zBuf, 256, "Read File failed: %s", (const char*)msName);
		CMyWindows::DisplayLastError(GetLastError(), zBuf);
	}
	if (nBytesRead < mFilePageLenNeto)
	{
		mpPageData->mnRealPixels = nBytesRead / mDataUnitSize;
		pDest += nBytesRead;
		memset(pDest, 0, mFilePageLenNeto - nBytesRead);
	}
	return nBytesRead;
}
bool CMultiDataFile::DoUpdatePageData(void)
{
	LoadPrologue();

    ULONGLONG offset = mOffsetBytes;
	CPosition *pPos = GetPosition();
	if (mnPages > 1)
		offset += pPos->GetCurrentRelative() * (ULONGLONG)mFilePageLenBruto;

	if (mPageHeaderLength > 0)
	{
		offset += mPageHeaderLength; // Skip header of current page
		if (mpPageHeader)
			ReadPageHeader();
	}

    unsigned char *pDest = (unsigned char *)mpPageData->GetDataStart();

    if (SetOffset(offset)) // Set File Offset
		ReadPage(pDest);
	else // SetOffset failed!
		memset(pDest, 0, mFilePageLenNeto);

    close();
	if (mbZeroNanOnRead)
		ZeroNan();
    return true;
}
bool CMultiDataFile::DoLoadVolume()
{
	Verify("<CMultiDataFile> DoLoadVolume");

	if (!mpVolume)
		return false;

	int nToLoad = GetNPages();
	if (nToLoad == 1)
		return CMultiFile::DoLoadVolume();

	LoadPrologue();

	CPosition* pPos = GetPosition();
	unsigned char* pVolumePage = (unsigned char*)mpVolume->GetPageVP(pPos->miFirst);
	if (!pVolumePage)
		return false;

	mpVolume->mnLoaded = 0;
	ULONGLONG offset = mOffsetBytes;
	if (mPageHeaderLength > 0)
		offset += mPageHeaderLength; // Skip header of first page

	if (mpfLog)
	{
		fprintf(mpfLog, "<CMultiDataFile::DoLoadVolume> Start Loading %d\n", nToLoad);
		fflush(mpfLog);
	}

	for (int iLoad = StartLoopI(); iLoad >= 0 && iLoad < nToLoad; iLoad = LoopNext())
	{
		int iLoadRelative = pPos->GetCurrentRelative();

		/*
		if (!DoLoadVolumeImage(iLoadRelative, pVolumePage))
		{
			CString s;
			s.Format("%s Failed to load relative page %d", Name(), iLoadRelative);
			CMyWindows::MessBox(s, "Load Volume Error");
			return false;
		}*/
		if (SetOffset(offset)) // Set File Offset
			ReadPage(pVolumePage);
		else // SetOffset failed!
			memset(pVolumePage, 0, mFilePageLenNeto);

		int ii = iLoadRelative + 1;
		if ((gScreens.mVolumeRefreshInterval && (ii % gScreens.mVolumeRefreshInterval == 0))
			|| (ii == nToLoad))
		{
			if (mpVolume->OnLoad(ii))
				gScreens.Refresh();
		}

		char zBuf[32];
		sprintf_s(zBuf, 32, "LV%03d", ii);
		gScreens.PrintStatusInBar(CScreens::SBAR_GENERAL, zBuf);
		pVolumePage += mDisplayPageLenNeto;
		offset += mFilePageLenBruto;
	}
	if (mpfLog)
	{
		fprintf(mpfLog, "<CMultiDataFile::DoLoadVolume> %d loaded\n", nToLoad);
		fflush(mpfLog);
	}
	mbLoadingVolume = true;
	gScreens.Refresh(); // Re-display original current image
	return true;
}
bool CMultiDataFile::ReadPageHeader(void)
{
    DWORD nBytesRead;
	BOOL bOK = ReadFile (mhFile, mpPageHeader, mPageHeaderLength, &nBytesRead, NULL);
    if (!bOK)
    {
        char zBuf[256];
        sprintf_s (zBuf, 256, "Read File header failed: %s", (const char*)msName);
        CMyWindows::DisplayLastError(GetLastError(), zBuf);
		return false;
    }
    if (nBytesRead < (unsigned int)mPageHeaderLength)
    {
        char *pDest = mpPageHeader + nBytesRead;
        memset (pDest, 0, mPageHeaderLength - nBytesRead);
    }
	return true;
}
bool CMultiDataFile::TryToGetImageSizeFromXml(void)
{
    CFileName xmlFileName ("ImageR.xml");
    xmlFileName.ChangePath(GetPath());
    CXMLParse parseFile (xmlFileName);
    CXMLParseNode *pRoot = parseFile.GetRoot();
    if (!pRoot)
        return false;

    bool bRead = false;
    CXMLParseNode *pRows = pRoot->GetFirst("rows");
    if (pRows)
        bRead = pRows->GetValue(mnLinesInPage);
    CXMLParseNode *pCols = pRoot->GetFirst("columns");
    if (pCols)
        bRead = pCols->GetValue(mnCols);

    return bRead;
}
unsigned long CMultiDataFile::ReadXMLOffset()
{
	FILE *pf = MyFOpen(mName,"rb");
	if (!pf)
		return 0;

	// Read Offset
	unsigned long offset = 0;
	if (fread(&offset,sizeof(offset),1,pf))
		mOffsetBytes = offset;
	fclose(pf);
	return offset;
}
void CMultiDataFile::GetInfoFromXMLHeader()
{
	// Read Offset
	ReadXMLOffset();

	CXMLParse xmlParse(mName, 4);
	CXMLParseNode *pRoot = xmlParse.GetRoot();
	if (!pRoot)
		return;

	pRoot->GetValue("n_cols", mnCols);
	pRoot->GetValue("n_lines", mnLinesInPage);
	bool bFloat = false;
	if (pRoot->GetValue("b_float", bFloat) && bFloat)
		SetTypedDataHandler(DATA_TYPE_FLOAT);
	bool bDouble = false;
	if (pRoot->GetValue("b_double", bFloat) && bDouble)
		SetTypedDataHandler(DATA_TYPE_DOUBLE);
}
bool CMultiDataFile::DesplayXmlHeader(void)
{
	CFileName fName(msName);
	if (!fName.IsOfType("ximg"))
		return false;

	// Read Offset
	unsigned long offset = ReadXMLOffset();
	if (offset == 0)
		return false;

	FILE *pf = MyFOpen(fName,"rb");
	if (!pf)
		return 0;

	CString sfName(CMyWindows::GetLogDir() + "\\" + fName.Private() + ".xml");
	FILE *pfWrite = MyFOpen(sfName,"wb");
	if (!pfWrite)
	{
		fclose(pf);
		return false;
	}

	char ch;
	unsigned int nSkip = sizeof(offset);
	for (unsigned int i = 0; i < offset; i++)
	{
		fread (&ch, sizeof(ch), 1, pf);
		if (i >= nSkip)
			fwrite (&ch, sizeof(ch), 1, pfWrite);
	}
	fclose(pf);
	fclose(pfWrite);
				
	CMyWindows::CreateProcess("notepad",sfName);
	return true;
}
bool CMultiDataFile::GuessMatrix(void)
{
	if (!mDataUnitSize)
	{
		mDataUnitSize = 1;
		CMyWindows::MessBox("<CMultiDataFile::GuessMatrix> mDataUnitSize not defined","SW Error");
	}
	ULONGLONG dataLengthInFile = mLength - mPageHeaderLength;
    double fileLen = (double)dataLengthInFile / mDataUnitSize;
	if (fileLen <= 128)
	{
		mnCols = (int)fileLen;
		mnLinesInPage = 1;
		return true;
	}

    double side = sqrt (fileLen);
    int iSide = (int)side;
    if (side == (double)iSide && (iSide <= 1024))
    {
        if (!mnLinesInPage)
            mnLinesInPage = iSide;
        if (!mnCols)
            mnCols = iSide;
    }
	else if (fileLen < 512 * 512) // Smal File
	{
		if (fileLen <= 64 * 64)
			mnCols = 64;
		else if (fileLen <= 256 * 256)
			mnCols = 256;
		else
			mnCols = 512;
		mnLinesInPage = ((int)fileLen + mnCols - 1) / mnCols;
	}
    else
    {
        if (!mnLinesInPage)
			GetDefaultNLines();
        if (!mnCols)
            mnCols = 512;
    }
	return true;
}
/*
bool CMultiDataFile::GuessMatrix(void)
{
    const int nOptions = 4;
    int maOptions [nOptions] = {512, 256, 768, 1024};
    unsigned int maDiff[nOptions];

    // Read first image
    int max_len = 1024 * 1024;
    if (max_len > mLength)
        max_len = (int)mLength;

    short *pTmp = new short [max_len];
    FILE *pf = MyFOpen (msName, "rb");
    if (!pf)
        return false;
    size_t nRead = fread (pTmp, sizeof(short), max_len, pf);
    fclose (pf);

    int nTry = 1000;
    int iBest = - 1;
    unsigned int bestDiff = 0;
    for (int iOption = 0; iOption < nOptions; iOption++)
    {
        int matrix = maOptions[iOption];
        if (matrix * matrix > (int)nRead)
            continue;
        size_t nLinesRead = nRead / matrix;

        unsigned int sumDiff = 0;
        for (int iTry = 0; iTry < nTry; iTry++)
        {
            int iLine = rand() % ((int)nLinesRead - 1);
            int iPix = rand() % matrix;
            int diff = pTmp[iLine * matrix + iPix] - pTmp[(iLine + 1) * matrix + iPix];
            if (diff < 0)
                diff = - diff;
            sumDiff += diff;
        }
        if (iBest < 0 || (sumDiff < (unsigned int)(bestDiff * 0.8)))
        {
            iBest = iOption;
            bestDiff = sumDiff;
        }
        maDiff[iOption] = sumDiff;
    }

    if (iBest >= 0)
    {
        mnCols = maOptions[iBest];
        mnLinesInPage = maOptions[iBest];
        return true;
    }
    return false;
} */
DWORD WINAPI CMultiDataFile::StaticSetImageSize (LPVOID p)
{
    CImageParams ImgParams((CMultiDataFile *)p);

    if (ImgParams.DoModal() == IDOK)
	{
		gScreens.Refresh();
	}
    return 0;
}
bool CMultiDataFile::set_i (int /*i*/)
{
	CPosition *pPos = mNamePattern.GetPosition(0);
	if (!pPos)
		return true;
		
	return CMultiFile::set_i(pPos->miCurrent);
}
void CMultiDataFile::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("MultiDataFile",xmlDump);
    CMultiFile::SaveState(xmlDump);
	xmlDump.Write("n_pages",mnPages);
	xmlDump.Write("file_page_length_neto", mFilePageLenNeto);
	xmlDump.Write("display_page_length_neto", mDisplayPageLenNeto);
	if (mFilePageLenBruto != mFilePageLenNeto)
		xmlDump.Write("page_length_bruto",mFilePageLenBruto);
}
void CMultiDataFile::EditContextMenu(CMenu *pPopup)
{
	pPopup->RemoveMenu(ID_MULTIDATA_SHOWCHESSHEADER, MF_BYCOMMAND);
}
bool CMultiDataFile::AddToTitleText(CString &iosTitle)
{
	if (iosTitle.IsEmpty())
		return false;

	if (mnPages <= 1)
		return false;

	CPosition *pPos = GetPosition();
	int iPage = pPos->GetCurrentRelative() + 1;

	char zBuf[128];
	sprintf_s(zBuf,sizeof(zBuf)," (p%d)", iPage);
	iosTitle += zBuf;
	return true;
}
