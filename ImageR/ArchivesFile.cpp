#include "StdAfx.h"
#include "ArchivesFile.h"
#include "..\..\yUtils\LogFile.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\ImageRLib\TVolume.h"
#include "Screens.h"
#include <math.h>
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyTracer.h"
#include "..\..\yUtils\DynamicTimer.h"

#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\RingLevelInfo.h"
#include "DRText.h"
#include "DisplayRectangle.h"

bool CArchivesFile::umbSaveRaster = false;
bool CArchivesFile::umbNeedDataText = true;

CArchivesFile::CArchivesFile(const char *zName, CMyDicom *pDicom, CXMLParseNode *pMetaDataXml)
: CMultiFile (zName, pMetaDataXml)
, mnPixelsLen(0)
, mpRli(NULL)
, mbLookedForRli(false)
{
	mpCtImages = this;

	static int count = 0;
	CMyTracer myTracer("CArchivesFile::CArchivesFile", ++count);

	if (!pDicom)
		pDicom = CFileName::GetDicomFile(GetFileName());

    if (pDicom)
	{
        SetDicom (pDicom);
		CheckBedPosition(this, mNamePattern);
	}
	else
		SetTypedDataHandler(DATA_TYPE_USHORT);
}
CArchivesFile::~CArchivesFile(void)
{
	static int count = 0;
	CMyTracer myTracer("CArchivesFile::~CArchivesFile", ++count);
}
    
ULONGLONG CArchivesFile::GetLength(void)
{
    if (HasPageData())
        return mpPageData->GetByteSize();
    return mLength;
}
   

typedef struct SSegmentOrDataTagWithLength
{
	unsigned short group;
	unsigned short num;
	char	vr[2];
	short s_len;
	long long_len;
}SSegmentOrDataTagWithLength;

void CArchivesFile::UpdateDicom()
{
	static int count = 0;
	CMyTracer myTracer("CArchivesFile::UpdateDicom", ++count);
    if (mpDicom)
	{
        delete mpDicom;
		mpDicom = NULL;
	}
	CMetaFile::SetDicom(CFileName::GetDicomFile (msName));

    if (mpDicom)
        mOffsetBytes = mpDicom->GetTagDataOffset (mDicomDataGroup, mDicomDataNum);

    if (mpfLog)
    {
        fprintf (mpfLog, "<CArchivesFile::UpdateDicom> %s offset %llu\n", (const char*)msName, mOffsetBytes);
        fflush (mpfLog);
    }
}
bool CArchivesFile::GetMmPerPixel(double &mmppWidth, double &mmppHeight)
{
	if (GetCirsMmPerPixel(mpDicom, mmppWidth, mmppHeight))
		return true;

	return CMultiFile::GetMmPerPixel(mmppWidth, mmppHeight);
}
bool CArchivesFile::DoUpdatePageData()
{
	static CEventTracer EvTracer ("CArchFile_Update");
	CDynamicTimer DynTimer (&EvTracer);

	static int count = 0;
	CMyTracer myTracer("CArchivesFile::DoUpdatePageData", ++count);
    int nRetry = 0;
	if (mpDicom)
 		mpDicom->PrintSequences("Before UpdateDicom");
	UpdateDicom();
	mpDicom->PrintSequences("After UpdateDicom");
    while (!mpDicom && nRetry++ < 10)
    {
        Sleep(100);
        UpdateDicom();
    }
	if (!mpDicom)
	{
		CMyWindows::MessBox("Faild to get Dicom data", Name());
		return false;
	}

	OnImageLoaded(mpDicom, GetPosition());

    if (meCompressed)
    {
		mpDicom->PrintSequences("Before Uncompressing");
        unsigned short *pDest = (unsigned short *)mpPageData->GetDataStart();
        const unsigned char *pSource = mpDicom->GetTagData(mDicomDataGroup, mDicomDataNum);
        while (!pSource && nRetry++ < 10)
        {
            Sleep(100);
            UpdateDicom();
            pSource = mpDicom->GetTagData(mDicomDataGroup, mDicomDataNum);
        }
        if (!pSource)
        {
            static int nError = 0;
            if (!nError++)
                MessageBox (NULL, msName, "MISSING COMPRESSED DATA", MB_OK);
        }
        if (!pDest || !pSource)
        {
            if (mpfLog)
                fprintf (mpfLog, "pSource %p pDest %p\n", pSource, pDest);
            return false;
        }
        unsigned long compressedDataLen = mpDicom->GetTagDataLen(mDicomDataGroup, mDicomDataNum);
        bool bOK = CtDecode (pSource, compressedDataLen, pDest, mpPageData->GetByteSize(),
			GetNCols(), GetNLinesInPage(), msName, NULL /*pDicom*/, mpfLog);

        /*if (bOK)
        {
            FILE *pf = MyFOpen ("d:\\tmp\\rle1.img","wb");
            if (pf)
            {
                fwrite (pDest, 1, mpPageData->GetByteSize(), pf);
                fclose (pf);
            }
        } */
        return bOK;
    }

    if (mpDicom && mpPageData)
    {
		char *pBuf = (char *)mpPageData->GetDataStart();
		if (mPlanarConfig == 1)
			pBuf = new char[mpPageData->GetByteSize()];

        bool bOK = mpDicom->GetTagData(mDicomDataGroup, mDicomDataNum, 
                    pBuf, mpPageData->GetByteSize(), true/*bLast*/);
        while (!bOK && (mnPixelsLen != mpDicom->mnTagDataRead) && nRetry++ < 10)
        {
			size_t nLastRead = mpDicom->mnTagDataRead;
            Sleep(100);
            UpdateDicom();
            bOK = mpDicom->GetTagData(mDicomDataGroup, mDicomDataNum, 
                    pBuf, mpPageData->GetByteSize());
			if (nLastRead == mpDicom->mnTagDataRead)
				mnPixelsLen = mpDicom->mnTagDataRead;
        }
		if (bOK && (mPlanarConfig == 1))
		{
			ReorderRgbData(pBuf);
			delete pBuf;
		}

        return bOK || (mnPixelsLen == mpDicom->mnTagDataRead);
    }
    return false;
}
CString CArchivesFile::GetPageDataText(int /*i*/)
{
	static int count = 0;
	CMyTracer myTracer("CArchivesFile::GetPageDataText", ++count);
	return GetBedPosText(mpDicom, GetPosition());
}
bool CArchivesFile::GetInstanceNumberByIndex(int iImage, int &number)
{
    CString sName (GetFileNameOfPage(iImage));
    if (!CFileName::Exist(sName))
        return false;

    CMyDicom *pDicom = CFileName::GetDicomFile(sName);
    if (!pDicom)
        return false;
    
	bool bOK = pDicom->GetInstanceNumber(number);
    delete pDicom;
    return bOK;
}
bool CArchivesFile::GetPageDataByIndex(CMyImage * pResultImage, int iImage, CString *psName)
{
	static CEventTracer EvTracer ("CArchFile_GetPageBI");
	CDynamicTimer DynTimer (&EvTracer);

    CString sName (GetFileNameOfPage(iImage));
    if (!CFileName::Exist(sName))
        return false;
	if (psName)
		*psName = sName;

    CMyDicom *pDicom = CFileName::GetDicomFile(sName);
    if (!pDicom)
        return false;
    
    bool bOK = GetPageDataFromDicom(pDicom, (unsigned short *)pResultImage->GetDataStart());
    delete pDicom;
    return bOK;
}
bool CArchivesFile::GetPageDataFromDicom(CMyDicom *pDicom, unsigned short *pPage)
{
    int pageLen = mnLinesInPage * mnCols;
    unsigned int pageSize = pageLen * sizeof(pPage[0]);

    if (meCompressed)
    {
        const unsigned char *pSource = pDicom->GetTagData(mDicomDataGroup, mDicomDataNum);
        if (!pSource)
            return false;

        // if (mpfLog)
        //     fprintf (mpfLog, "Compressed Len %d\n", 
        //         pDicom->GetTagDataLen(mDicomDataGroup, mDicomDataNum));
        return CtDecode (pSource, pDicom->GetTagDataLen(mDicomDataGroup, mDicomDataNum), 
                pPage, pageSize,
				GetNCols(), GetNLinesInPage(), 
				msName, NULL /*pDicom*/, mpfLog);
    }

    return pDicom->GetTagData(mDicomDataGroup, mDicomDataNum, 
            pPage, pageSize);
}
CString CArchivesFile::GetPageText (void)
{
    return CMetaData::GetPageText();
}
void CArchivesFile::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("ArchivesFile",xmlDump);
    CMultiFile::SaveState(xmlDump);
    if (mDumpCount)
        return;

    if (mSecondBedPos != 0.0f || mFirstBedPos != 0.0f)
    {
        xmlDump.Write("first_bed_pos", mFirstBedPos);
        xmlDump.Write("second_bed_pos", mSecondBedPos);
        xmlDump.Write("last_bed_pos", mLastBedPos);
    }

    if (meCompressed)
    {
        switch (meCompressed)
        {
        case CMyDicom::COMP_RLE1:
            xmlDump.Write ("compression", "RLE1");
            break;
        case CMyDicom::COMP_RGB1:
            xmlDump.Write ("compression", "RGB1");
            break;
        case CMyDicom::COMP_RICE:
            xmlDump.Write ("compression", "RICE");
            break;
        default:
            break;
        }

        if (umbSaveRaster && mpPageData)
        {
			CString sDumpDir(CMyImage::GetDumpDir());
            CMyWindows::VerifyDirectory(sDumpDir);
            static int count = 0;
            char zName[256];
            sprintf_s (zName, 256, "%s\\im%03d.img", (const char*)sDumpDir, count++);
            FILE *pf = MyFOpen (zName, "wb");
            if (pf)
            {
                unsigned short *pRaster = (unsigned short *)mpPageData->GetDataStart();
                int n = mnLinesInPage * mnCols;
                fwrite (pRaster, sizeof(pRaster[0]), n, pf);
                fclose (pf);
                xmlDump.Write("raster_dump", zName);
            }
        }
    }
}
CRingLevelInfo *CArchivesFile::GetRli(void)
{
	if (!mbLookedForRli)
	{
		if (!mpRli)
			mpRli = new CRingLevelInfo(this);
	}
	if (mpRli && mpRli->IsValid())
		return mpRli;
	return NULL;
}
bool CArchivesFile::SetCurrent(int iCurrent)
{
	if (!GetPosition()->TrySetCurrent(iCurrent))
		return false;

	mbPageUpdated = false;
	return UpdatePageData();
}
bool CArchivesFile::SetCurrentPositionOnly(int iCurrent)
{
	if (!GetPosition()->TrySetCurrent(iCurrent))
		return false;
	return true;
}
CMyDicom * CArchivesFile::GetDicomByIndex(int iImage)
{
	if (!SetCurrent(iImage))
		return NULL;
	return GetDicom();
}
void CArchivesFile::ReorderRgbData(char * pBuf)
{
	S_RGB *pRgb = (S_RGB *)mpPageData->GetDataStart();
	int n3 = mpPageData->GetByteSize();
	int n = n3 / 3;
	int iRed = 0;
	int iGreen = n;
	int iBlue = n * 2;
	for (int i = 0; i < n; i++)
	{
		pRgb[i].RGB[0] = pBuf[iRed++];
		pRgb[i].RGB[1] = pBuf[iGreen++];
		pRgb[i].RGB[2] = pBuf[iBlue++];
	}
}
bool CArchivesFile::DoLoadVolumeImage(int iLoad, void *pVolumePage)
{
	unsigned short *pRead = (unsigned short *)pVolumePage;
	CString sName (mNamePattern.GetName());
    CMyDicom *pDicom = CFileName::GetDicomFile(sName);
    if (!pDicom)
        return false;

    if (!GetPageDataFromDicom(pDicom, pRead))
    {
        delete pDicom;
        return false;
    }

    if (iLoad == 0)
		OnFirstImageLoaded(pDicom);
    else if (iLoad == 1)
	{
		OnSecondImageLoaded(pDicom);
		mMmPerPixelPage = mSecondBedPos - mFirstBedPos;
		if (mMmPerPixelPage > -0.001 && mMmPerPixelPage < 0.001)
		{
			static int nNotice = 0;
			if (!nNotice++)
				CMyWindows::MessBox ("Bed Position is Constant", msName);
			mMmPerPixelPage = -1.0f;
		}
	}
        
    delete pDicom;
	return true;
}
