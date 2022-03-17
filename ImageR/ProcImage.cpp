#include "StdAfx.h"
#include "ProcImage.h"
#include "..\..\yUtils\DynamicTimer.h"
#include "..\..\yUtils\ParametersControl.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"

static void Debug(const char* zText)
{
    int* pFirst = new int[509];
    delete pFirst;
    CMyWindows::PrintStatus(zText);
}

float CProcImage::umSmoothFactor = 1;
float CProcImage::umMaxSmoothFactor = 10;

// Static function
void CProcImage::SetSmoothFactor(float factor)
{
    if (factor <= 0)
        return;
    umSmoothFactor = min(factor, umMaxSmoothFactor);
}

CProcImage::CProcImage(CMetaData *pImage, const char *zName)
: CMetaData (zName ? zName : "ProcImage")
, mpImage(pImage)
, mpTempRaster(NULL)
, mSmoothFactorDone(0)
{
    InitOnConstruction();
}
CProcImage::CProcImage(class CXMLParseNode *pXmlNode)
: CMetaData (pXmlNode->GetFirstDeepString("name", "ProcImage"), pXmlNode)
, mpImage((CMetaData *)CNamedObject::GetParsed(pXmlNode, "source_id"))
, mpTempRaster(NULL)
, mSmoothFactorDone(0)
{
    CXMLParseNode * pSmoothNode = pXmlNode->GetFirstDeep("smooth");
    if (pSmoothNode)
        pSmoothNode->GetValue(umSmoothFactor);
    InitOnConstruction();
}
void CProcImage::InitOnConstruction(void)
{
    mnLinesInPage = mpImage->GetNLinesInPage();
    mnCols = mpImage->GetNCols();

    SetTypedDataHandler(mpImage->GetDataType());

    AddCheck(mpImage);

    SetMmPerPixel(mpImage->MmPerPixelWidth(),mpImage->MmPerPixelHeight());

    gParametersControl.AddRef (&umSmoothFactor, "Smooth", 0.0f, umMaxSmoothFactor, 0.1f);

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
    if (umSmoothFactor != mSmoothFactorDone)
        return true;
    return CDependant::ShouldUpdate();
}
void CProcImage::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("ProcImage",xmlDump);
    CMetaData::SaveState(xmlDump);
    if (mpImage)
        xmlDump.Write("source_id", mpImage->mUId);
    xmlDump.Write("smooth", umSmoothFactor);
}

bool CProcImage::DoUpdatePageData(void)
{
    if (!mpImage->UpdatePageData())
        return false;

	static CEventTracer EvTracer ("UpdateProc");
	CDynamicTimer DynTimer (&EvTracer);

    Debug("DoUpdatePageData");
    unsigned short *pMyRaster = (unsigned short *)GetPageData();
    unsigned int nBytesInPage = mnLinesInPage * mnCols * sizeof (pMyRaster[0]);
    memset (pMyRaster, 0, nBytesInPage);
    Smooth();
    Debug("DoUpdatePageData smoothed");
    OnChange();
    return true;
}
void CProcImage::Smooth(void)
{
    if (mDataType == DATA_TYPE_USHORT)
    {
        unsigned short*  pCurrentRasterUSHORT = (unsigned short*)mpImage->GetPageData();
        Smooth(pCurrentRasterUSHORT);
    }
    else if (mDataType == DATA_TYPE_SHORT)
    {
        short* pCurrentRasterSHORT = (short*)mpImage->GetPageData();
        Smooth(pCurrentRasterSHORT);
    }
    else
    {
        CMyWindows::MessBox("Smooth is not supported for this type", "Warning");
    }
}
template <typename T>
void CProcImage::Smooth(T *pCurrentRaster)
{
    //pCurrentRaster = (unsigned short *)mpImage->GetPageData();
    T *pMyRaster = (T *)GetPageData();
    AllocateTempRaster();
    Debug("AllocateTempRaster");

    float restFactor = umSmoothFactor;
    while (restFactor >= 1.0f)
    {
        T *pTargetRaster;
        if (pCurrentRaster == pMyRaster)
            pTargetRaster = (T *)mpTempRaster;
        else
            pTargetRaster = pMyRaster;
        Debug("Call Smooth1");
        Smooth1(pTargetRaster, pCurrentRaster);
        Debug("After Smooth1");
        pCurrentRaster = pTargetRaster;
        restFactor -= 1.0f;
    }
    if (restFactor > 0.0f)
    {
        T *pTargetRaster;
        if (pCurrentRaster == pMyRaster)
            pTargetRaster = (T *)mpTempRaster;
        else
            pTargetRaster = pMyRaster;
        Debug("Call PartSmooth");
        PartSmooth(pTargetRaster, pCurrentRaster, restFactor);
        pCurrentRaster = pTargetRaster;
    }
    if (pCurrentRaster != pMyRaster)
        memcpy (pMyRaster, pCurrentRaster, mnLinesInPage * mnCols * sizeof(pMyRaster[0]));
    if (mpfLog)
        fprintf (mpfLog, "<CProcImage::Smooth> %s factor %f\n",
            (const char *)mpImage->msName, umSmoothFactor);
    mSmoothFactorDone = umSmoothFactor;
}
template <typename T>
void CProcImage::PartSmooth(T *pRaster, const T *pSrcRaster, float factor)
{
    CopyFrame(pRaster, pSrcRaster);

    float envWeight = factor / 9.0f;
    float myWeight = 1.0f - factor;

    // Smooth
    for (int iLine = 1; iLine < mnLinesInPage - 1; iLine++)
    {
        int lineOffset = iLine * mnCols;
        T *pLine = pRaster + lineOffset;
        const T *pSrcLine = pSrcRaster + lineOffset;
        const T *pPrevSrcLine = pSrcLine - mnCols;
        const T *pNextSrcLine = pSrcLine + mnCols;

        for (int iCol = 1; iCol < mnCols - 1; iCol++)
        {
            T env = pPrevSrcLine[iCol-1] + pPrevSrcLine[iCol] + pPrevSrcLine[iCol+1]
                            + pSrcLine[iCol-1] + pSrcLine[iCol] + pSrcLine[iCol+1]
                            + pNextSrcLine[iCol-1] + pNextSrcLine[iCol] + pNextSrcLine[iCol+1];
            pLine[iCol] = (T)(env * envWeight + pSrcLine[iCol] * myWeight + 0.5f);
        }
    }
}
template <typename T>
void CProcImage::Smooth1(T *pRaster, const T *pSrcRaster)
{
    CopyFrame(pRaster, pSrcRaster);
    Debug("After CopyFrame");

    // Smooth
    for (int iLine = 1; iLine < mnLinesInPage - 1; iLine++)
    {
        int lineOffset = iLine * mnCols;
        T *pLine = pRaster + lineOffset;
        const T *pSrcLine = pSrcRaster + lineOffset;
        const T *pPrevSrcLine = pSrcLine - mnCols;
        const T *pNextSrcLine = pSrcLine + mnCols;

        for (int iCol = 1; iCol < mnCols - 1; iCol++)
        {
            pLine[iCol] = (pPrevSrcLine[iCol-1] + pPrevSrcLine[iCol] + pPrevSrcLine[iCol+1]
                            + pSrcLine[iCol-1] + pSrcLine[iCol] + pSrcLine[iCol+1]
                            + pNextSrcLine[iCol-1] + pNextSrcLine[iCol] + pNextSrcLine[iCol+1])
                                / 9;
        }
    }
    Debug("End Smooth1");
}
template <typename T>
void CProcImage::CopyFrame(T *pRaster, const T *pSrcRaster)
{
    // Copy first and last line
    int nBytesInLine = mnCols * sizeof(pRaster[0]);
    memcpy (pRaster, pSrcRaster, nBytesInLine);
    int lastLineOffset = (mnLinesInPage - 1) * mnCols;
    memcpy (pRaster + lastLineOffset, pSrcRaster + lastLineOffset, nBytesInLine);

    for (int iLine = 1; iLine < mnLinesInPage - 1; iLine++)
    {
        int lineOffset = iLine * mnCols;
        T *pLine = pRaster + lineOffset;
        const T *pSrcLine = pSrcRaster + lineOffset;
        pLine[0] = pSrcLine[0];
        pLine[mnCols-1] = pSrcLine[mnCols-1];
    }
}
bool CProcImage::AllocateTempRaster(void)
{
    if (!mpTempRaster)
    {
        int len = mnLinesInPage * mnCols;
        if (mDataType == DATA_TYPE_USHORT)
            mpTempRaster = new unsigned short[len];
        else if (mDataType == DATA_TYPE_SHORT)
            mpTempRaster = new short[len];
        else
            return false;
    }
    if (mpTempRaster)
        return true;
    return false;
}