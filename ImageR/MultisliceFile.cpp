#include "StdAfx.h"
#include "MultisliceFile.h"


CMultisliceFile::CMultisliceFile(const char* zfName, CXMLParseNode* pMetaDataXml,
	unsigned int fileHeaderBytes)
	: CMultiDataFile(zfName, pMetaDataXml, fileHeaderBytes, DATA_TYPE_FLOAT, false)
{
	mnCols = 688;
	mnLinesInPage = 192;

	mnLRLeft = 40;
	mnHR = 368;
	mnLRRight = 40;

	mnMultiSlices = mnLinesInPage / 4;

	mnMultiSliceSamplesInFile = mnLRLeft + mnHR * 4 + mnLRRight;
	mnMultiSliceSizeInFileBytes = mnMultiSliceSamplesInFile * mDataUnitSize;

	mnPageSizeInFile = mnMultiSliceSizeInFileBytes * mnMultiSlices;

	ComputeNPages();
	mpReadBuffer = new float[mnPageSizeInFile];
}
CMultisliceFile::~CMultisliceFile()
{
	if (mpReadBuffer)
		delete[] mpReadBuffer;
}
void CMultisliceFile::ComputeFilePageLenNeto()
{
	mDisplayPageLenNeto = mnLinesInPage * mnCols * GetUnitSize();
	mFilePageLenNeto = mnPageSizeInFile;
}
bool CMultisliceFile::DoUpdatePageData(void)
{
	LoadPrologue();

	ULONGLONG offset = mOffsetBytes;
	CPosition* pPos = GetPosition();
	if (mnPages > 1)
		offset += pPos->GetCurrentRelative() * (ULONGLONG)mFilePageLenBruto;

	if (mPageHeaderLength > 0)
	{
		offset += mPageHeaderLength; // Skip header of current page
		if (mpPageHeader)
			ReadPageHeader();
	}

	if (SetOffset(offset)) // Set File Offset
		ReadPage((unsigned char *)mpReadBuffer);
	else // SetOffset failed!
		memset(mpReadBuffer, 0, mFilePageLenNeto);

	close();
	ConvertAllMultislices();

	if (mbZeroNanOnRead)
		ZeroNan();
	return true;
}
void CMultisliceFile::ConvertAllMultislices()
{
	float* pIn = mpReadBuffer;
	float *pOut = (float*)mpPageData->GetDataStart();

	for (int iMS = 0; iMS < mnMultiSlices; iMS++)
	{
		ExpandMultislice(pIn, pOut);
		pIn += mnMultiSliceSamplesInFile;
		pOut += mnCols * 4;
	}
}
void CMultisliceFile::ExpandMultislice(float* pIn, float* pOut)
{
	float* pInLeft = pIn;
	float* pInHR = pIn + mnLRLeft;
	float* pInRight = pInHR + mnHR;

	for (int iSL = 0; iSL < 4; iSL++)
	{
		ExpandLine(pInLeft, pInHR, pInRight, pOut);

		pInHR += mnHR;
		if (iSL == 0)
			pInHR += mnLRRight;

		pOut += mnCols;
	}
}
void CMultisliceFile::ExpandLine(float* pInLeft, float* pInHR, float* pInRight, float* pOut)
{
	ExpandLR(pInLeft, pOut, mnLRLeft);
	pOut += mnLRLeft * 4;

	memcpy(pOut, pInHR, mnHR * sizeof(pInHR[0]));
	pOut += mnHR;

	ExpandLR(pInRight, pOut, mnLRRight);
}
void CMultisliceFile::ExpandLR(float* pIn, float* pOut, int n)
{
	for (int iLR = 0; iLR < n; iLR++)
	{
		for (int i = 0; i < 4; i++)
			*pOut++ = pIn[iLR];
	}
}