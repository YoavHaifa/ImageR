#include "StdAfx.h"
#include "PflhDicomFile.h"

#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\XMLDump.h"

#include "..\..\ImageRLib\Position.h"


CPflhDicomFile::CPflhDicomFile(const char *zName, class CMyDicom *pDicom, class CXMLParseNode *pMetaDataXml)
: CMetaFile(zName, pMetaDataXml)
, mnPages(0)
, mFrameSizeBytes(0)
, mLastFrameSizeBytes(0)
, mLastReadOffset(0)
, mDicomDataGroup(0)
, mDicomDataNum(0)
{
	if (!pDicom)
		pDicom = CFileName::GetDicomFile(zName);

    if (!pDicom)
	{
		CMyWindows::MessBox("<CPflhDicomFile::CPflhDicomFile> Expecting Dicom File", "Bad Input");
		return;
	}

    SetDicom (pDicom);

	__int64 len = mpDicom->GetTagLastDataLen(mDicomDataGroup, mDicomDataNum);
	mFrameSizeBytes = mnLinesInPage * mnCols * sizeof(float);
	if (mFrameSizeBytes > 0)
	{
		mnPages = (int)(len / mFrameSizeBytes);
		__int64 nFullPagesLen = (__int64)mFrameSizeBytes * mnPages;
		if (nFullPagesLen < len)
		{
			mnPages++;
			mLastFrameSizeBytes = (unsigned int)(len - nFullPagesLen);
		}
	}

	if (mnPages > 1)
	{
		AddFirstPosition(new CPosition("MultiDataPos"));
		SetLeafRange(1, mnPages);
		GetPosition()->mbStayInLimits = true;
	}
	//CheckBedPosition(this, mNamePattern);
}
CPflhDicomFile::~CPflhDicomFile(void)
{
}
void CPflhDicomFile::SetDicom(class CMyDicom *pDicom)
{
	CMetaFile::SetDicom(pDicom);
    if (!mpDicom)
        return;

	SetTypedDataHandler(DATA_TYPE_FLOAT);

	if (mpDicom->HasTagRef(PFLH1_PIXEL_DATA_GROUP, PFLH1_PIXEL_DATA_NUM))
	{
	    mDicomDataGroup = PFLH1_PIXEL_DATA_GROUP;
		mDicomDataNum = PFLH1_PIXEL_DATA_NUM;
		mpDicom->GetIntTextField(PFLH1_NUM_SLICES_GROUP, PFLH1_NUM_SLICES_NUM, mnLinesInPage);
		mpDicom->GetIntTextField(PFLH1_NUM_DETS_GROUP, PFLH1_NUM_DETS_NUM, mnCols);
	}
	else
	{
	    mDicomDataGroup = PFLH_PIXEL_DATA_GROUP;
		mDicomDataNum = PFLH_PIXEL_DATA_NUM;
	}

}
bool CPflhDicomFile::DoUpdatePageData()
{
    if (!mpDicom || !mpPageData)
		return false;

	char *pBuf = (char *)mpPageData->GetDataStart();
	int iCurrent = GetPosition(0)->GetCurrentRelative();
	mLastReadOffset = iCurrent * mFrameSizeBytes;

	bool bOK = mpDicom->GetTagDataFromOffset(mDicomDataGroup, mDicomDataNum, 
                    pBuf, mpPageData->GetByteSize(), mLastReadOffset, true/*bLast*/);

	return bOK;
}
void CPflhDicomFile::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("PflhDicomFile",xmlDump);
    CMetaFile::SaveState(xmlDump);
    xmlDump.Write("last_read_offset",(int)mLastReadOffset);
	xmlDump.Write("n_pages",mnPages);
	xmlDump.Write("frame_size_bytes",mFrameSizeBytes);
	if (mLastFrameSizeBytes > 0)
		xmlDump.Write("last_frame_size_bytes",mFrameSizeBytes);
}
