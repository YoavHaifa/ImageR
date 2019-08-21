#include "StdAfx.h"
#include "ProcImage.h"
#include "..\..\yUtils\DynamicTimer.h"
#include "..\..\yUtils\ParametersControl.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"

CProcImage::CProcImage(CMetaData *pImage, const char *zName)
: CMetaData (zName ? zName : "ProcImage")
, mpImage(pImage)
, mpTempRaster(NULL)
, mpCurrentRaster(NULL)
, mSmoothFactor(0.5f)
, mSmoothFactorDone(0)
{
    InitOnConstruction();
}
CProcImage::CProcImage(class CXMLParseNode *pXmlNode)
: CMetaData (pXmlNode->GetFirstDeepString("name", "ProcImage"), pXmlNode)
, mpImage((CMetaData *)CNamedObject::GetParsed(pXmlNode, "source_id"))
, mpTempRaster(NULL)
, mpCurrentRaster(NULL)
, mSmoothFactor(0.5f)
, mSmoothFactorDone(0)
{
    CXMLParseNode * pSmoothNode = pXmlNode->GetFirstDeep("smooth");
    if (pSmoothNode)
        pSmoothNode->GetValue(mSmoothFactor);
    InitOnConstruction();
}
void CProcImage::InitOnConstruction(void)
{
    mnLinesInPage = mpImage->GetNLinesInPage();
    mnCols = mpImage->GetNCols();

    SetTypedDataHandler(DATA_TYPE_USHORT);

    AddCheck(mpImage);

    SetMmPerPixel(mpImage->MmPerPixelWidth(),mpImage->MmPerPixelHeight());

    gParametersControl.AddRef (&mSmoothFactor, "Smooth", 0.0f, 10.0f, 0.1f);

    SetPosition(mpImage->GetPosition());

    AddDualModeToName();
}

CProcImage::~CProcImage(void)
{
    if (mpTempRaster)
        delete mpTempRaster;
}
bool CProcImage::ShouldUpdate(void)
{
    if (mSmoothFactor != mSmoothFactorDone)
        return true;
    return CDependant::ShouldUpdate();
}
void CProcImage::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("ProcImage",xmlDump);
    CMetaData::SaveState(xmlDump);
    if (mpImage)
        xmlDump.Write("source_id", mpImage->mUId);
    xmlDump.Write("smooth", mSmoothFactor);
}

bool CProcImage::DoUpdatePageData(void)
{
    if (!mpImage->UpdatePageData())
        return false;

	static CEventTracer EvTracer ("UpdateProc");
	CDynamicTimer DynTimer (&EvTracer);

    unsigned short *pMyRaster = (unsigned short *)GetPageData();
    unsigned int nBytesInPage = mnLinesInPage * mnCols * sizeof (pMyRaster[0]);
    memset (pMyRaster, 0, nBytesInPage);
    Smooth();
    OnChange();
    return true;
}
void CProcImage::Smooth(void)
{
    mpCurrentRaster = (unsigned short *)mpImage->GetPageData();
    unsigned short *pMyRaster = (unsigned short *)GetPageData();
    AllocateTempRaster();

    float restFactor = mSmoothFactor;
    while (restFactor >= 1.0f)
    {
        unsigned short *pTargetRaster;
        if (mpCurrentRaster == pMyRaster)
            pTargetRaster = mpTempRaster;
        else
            pTargetRaster = pMyRaster;
        Smooth1(pTargetRaster, mpCurrentRaster);
        mpCurrentRaster = pTargetRaster;
        restFactor -= 1.0f;
    }
    if (restFactor > 0.0f)
    {
        unsigned short *pTargetRaster;
        if (mpCurrentRaster == pMyRaster)
            pTargetRaster = mpTempRaster;
        else
            pTargetRaster = pMyRaster;
        PartSmooth(pTargetRaster, mpCurrentRaster, restFactor);
        mpCurrentRaster = pTargetRaster;
    }
    if (mpCurrentRaster != pMyRaster)
        memcpy (pMyRaster, mpCurrentRaster, mnLinesInPage * mnCols * sizeof(pMyRaster[0]));
    if (mpfLog)
        fprintf (mpfLog, "<CProcImage::Smooth> %s factor %f\n",
            (const char *)mpImage->msName, mSmoothFactor);
    mSmoothFactorDone = mSmoothFactor;
}
void CProcImage::PartSmooth(unsigned short *pRaster, const unsigned short *pSrcRaster, float factor)
{
    CopyFrame(pRaster, pSrcRaster);

    float envWeight = factor / 9.0f;
    float myWeight = 1.0f - factor;

    // Smooth
    for (int iLine = 1; iLine < mnLinesInPage - 1; iLine++)
    {
        unsigned short *pLine = pRaster + iLine * mnCols;
        const unsigned short *pSrcLine = pSrcRaster + iLine * mnCols;
        const unsigned short *pPrevSrcLine = pSrcLine - mnCols;
        const unsigned short *pNextSrcLine = pSrcLine + mnCols;

        for (int iCol = 1; iCol < mnCols - 1; iCol++)
        {
            unsigned short env = pPrevSrcLine[iCol-1] + pPrevSrcLine[iCol] + pPrevSrcLine[iCol+1]
                            + pSrcLine[iCol-1] + pSrcLine[iCol] + pSrcLine[iCol+1]
                            + pNextSrcLine[iCol-1] + pNextSrcLine[iCol] + pNextSrcLine[iCol+1];
            pLine[iCol] = (unsigned short)(env * envWeight + pSrcLine[iCol] * myWeight + 0.5f);
        }
    }
}
void CProcImage::Smooth1(unsigned short *pRaster, const unsigned short *pSrcRaster)
{
    CopyFrame(pRaster, pSrcRaster);

    // Smooth
    for (int iLine = 1; iLine < mnLinesInPage - 1; iLine++)
    {
        unsigned short *pLine = pRaster + iLine * mnCols;
        const unsigned short *pSrcLine = pSrcRaster + iLine * mnCols;
        const unsigned short *pPrevSrcLine = pSrcLine - mnCols;
        const unsigned short *pNextSrcLine = pSrcLine + mnCols;

        for (int iCol = 1; iCol < mnCols - 1; iCol++)
        {
            pLine[iCol] = (pPrevSrcLine[iCol-1] + pPrevSrcLine[iCol] + pPrevSrcLine[iCol+1]
                            + pSrcLine[iCol-1] + pSrcLine[iCol] + pSrcLine[iCol+1]
                            + pNextSrcLine[iCol-1] + pNextSrcLine[iCol] + pNextSrcLine[iCol+1])
                                / 9;
        }
    }
}
void CProcImage::CopyFrame(unsigned short *pRaster, const unsigned short *pSrcRaster)
{
    // Copy first and last line
    int nBytesInLine = mnCols * sizeof(pRaster);
    memcpy (pRaster, pSrcRaster, nBytesInLine);
    int lastLineOffset = (mnLinesInPage - 1) * mnCols;
    memcpy (pRaster + lastLineOffset, pSrcRaster + lastLineOffset, nBytesInLine);

    for (int iLine = 1; iLine < mnLinesInPage - 1; iLine++)
    {
        unsigned short *pLine = pRaster + iLine * mnCols;
        const unsigned short *pSrcLine = pSrcRaster + iLine * mnCols;
        pLine[0] = pSrcLine[0];
        pLine[mnCols-1] = pSrcLine[mnCols-1];
    }
}
bool CProcImage::AllocateTempRaster(void)
{
    if (!mpTempRaster)
        mpTempRaster = new unsigned short [mnLinesInPage * mnCols];
    if (mpTempRaster)
        return true;
    return false;
}