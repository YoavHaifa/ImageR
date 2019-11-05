#include "StdAfx.h"
#include "Sinograms.h"
#include "Sinograms.h"
#include "..\..\yUtils\DynamicTimer.h"
#include "..\..\yUtils\MyMath.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\ParametersControl.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"

#include <math.h>
#include ".\sinograms.h"
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923

CSinograms::CBeamHardeningConfig CSinograms::umConfig;

CSinograms::CBeamHardeningConfig::CBeamHardeningConfig(void)
: mbAddedToParamsControl(false)
, mXmlDumpId(0)
{
    mnSinograms = 145;
    mCorrectionThreshold = 1024;
    mCorrectionFactor = 0.001f;

    mbUseSpecialParamsForL1 = false;
    mCorrectionThresholdL1 = mCorrectionThreshold;
    mCorrectionFactorL1 = mCorrectionFactor;
}
void CSinograms::CBeamHardeningConfig::AddToParamsControl(int maxNSinograms)
{
    if (mbAddedToParamsControl)
        return;
    mbAddedToParamsControl = true;

    gParametersControl.AddRef (&mnSinograms, "n Sinograms", 32, maxNSinograms, 1);
    gParametersControl.AddRef (&mCorrectionThreshold, "BH Threshold", 900, 1200, 1);
    gParametersControl.AddRef (&mCorrectionFactor, "BH Factor", 0.0001F, 0.01F, 0.0001F);

    gParametersControl.AddRef (&mbUseSpecialParamsForL1, "L1 Specal Params");
    gParametersControl.AddRef (&mCorrectionThresholdL1, "L1 BH Threshold", 900, 1200, 1);
    gParametersControl.AddRef (&mCorrectionFactorL1, "L1 BH Factor", 0.0001F, 0.01F, 0.0001F);
}
void CSinograms::CBeamHardeningConfig::SaveState(CXMLDump & xmlDump)
{
    if (mXmlDumpId == xmlDump.mId)
        return;
    mXmlDumpId = xmlDump.mId;

    CXMLDumpScope graph("config",xmlDump);
    xmlDump.Write("n_sinograms", mnSinograms);
    xmlDump.Write("correction_threshold", mCorrectionThreshold);
    xmlDump.Write("correction_factor", mCorrectionFactor);
    xmlDump.Write("b_l1_special_params", mbUseSpecialParamsForL1);
    xmlDump.Write("correction_threshold_l1", mCorrectionThresholdL1);
    xmlDump.Write("correction_factor_l1", mCorrectionFactorL1);
}
void CSinograms::CBeamHardeningConfig::ResumeState(class CXMLParseNode *pXmlNode)
{
    pXmlNode->GetValue("n_sinograms", mnSinograms);
    pXmlNode->GetValue("correction_threshold", mCorrectionThreshold);
    pXmlNode->GetValue("correction_factor", mCorrectionFactor);
    pXmlNode->GetValue("b_l1_special_params", mbUseSpecialParamsForL1);
    pXmlNode->GetValue("correction_threshold_l1", mCorrectionThresholdL1);
    pXmlNode->GetValue("correction_factor_l1", mCorrectionFactorL1);
}
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
CSinograms::CSinograms(CMetaData *pImage)
: CMetaData("Sinograms")
, meType (SINO_SINOGRAMS)
, mpImage(pImage)
, mpSinograms(NULL)
, mDeltaTheta(0)
, mnImageLines(0)
, mImageLineCenter(0)
, mnImageCols(0)
, mCount(0)
, mbPrint(false)
, mXStart(0)
, mXEnd(0)
, mMidSino(0)
, mnOut(0)
, mImageColCenter(0)
, mnSinogramsComputed(0)
, miFirstSinogramLine(0)
, mCorrectThresholdDone(0)
, mCorrectionFactorDone(0.0f)
{
    InitOnConstruction();
}
CSinograms::CSinograms(CSinograms *pSinograms)
: CMetaData("Attenuation")
, meType (SINO_ATTENUATION)
, mpImage(pSinograms->mpImage)
, mpSinograms(pSinograms)
, mDeltaTheta(mpSinograms->mDeltaTheta)
, mnImageLines(0)
, mImageLineCenter(0)
, mnImageCols(0)
, mCount(0)
, mbPrint(false)
, mXStart(0)
, mXEnd(0)
, mMidSino(0)
, mnOut(0)
, mImageColCenter(0)
, mnSinogramsComputed(0)
, miFirstSinogramLine(0)
, mCorrectThresholdDone(0)
, mCorrectionFactorDone(0.0f)
{
    InitOnConstruction();
}
CSinograms::CSinograms(CMetaData *pImage, CSinograms *pSinograms)
: CMetaData("BHCorrectedImage")
, meType (SINO_CORRECTED_IMAGE)
, mpImage(pImage)
, mpSinograms(pSinograms)
, mDeltaTheta(0)
, mnImageLines(0)
, mImageLineCenter(0)
, mnImageCols(0)
, mCount(0)
, mbPrint(false)
, mXStart(0)
, mXEnd(0)
, mMidSino(0)
, mnOut(0)
, mImageColCenter(0)
, mnSinogramsComputed(0)
, miFirstSinogramLine(0)
, mCorrectThresholdDone(0)
, mCorrectionFactorDone(0.0f)
{
    InitOnConstruction();
}
CSinograms::CSinograms(class CXMLParseNode *pXmlNode)
: CMetaData (pXmlNode->GetFirstDeepString("name", "BHCorrection"), pXmlNode)
, meType (SINO_UNDEF)
, mpImage((CMetaData *)CNamedObject::GetParsed(pXmlNode, "source_id"))
, mpSinograms((CSinograms *)CNamedObject::GetParsed(pXmlNode, "sinograms_id"))
, mDeltaTheta(0)
, mnImageLines(0)
, mImageLineCenter(0)
, mnImageCols(0)
, mCount(0)
, mbPrint(false)
, mXStart(0)
, mXEnd(0)
, mMidSino(0)
, mnOut(0)
, mImageColCenter(0)
, mnSinogramsComputed(0)
, miFirstSinogramLine(0)
, mCorrectThresholdDone(0)
, mCorrectionFactorDone(0.0f)
{
    CString sType = pXmlNode->GetFirstDeepString("type", "CORRECTED_IMAGE");
    if (sType == "SINOGRAMS")
        meType = SINO_SINOGRAMS;
    else if (sType == "ATTENUATION")
        meType = SINO_ATTENUATION;
    else
        meType = SINO_CORRECTED_IMAGE;

    InitOnConstruction();

    CXMLParseNode *pXmlConfigNode = pXmlNode->GetFirst("config");
    if (pXmlConfigNode)
        umConfig.ResumeState(pXmlConfigNode);
}
void CSinograms::InitOnConstruction(void)
{
    mbCtImage = false;
    if (mpImage)
    {
        mnLinesInPage = mpImage->GetNLinesInPage();
        mnCols = mpImage->GetNCols();
        mMmPerPixelWidth = mpImage->MmPerPixelWidth();
        SetPosition(mpImage->GetPosition());
    }

    switch (meType)
    {
    case SINO_SINOGRAMS:
        mnLinesInPage = 580;
        mnCols = 800;
        SetTypedDataHandler(DATA_TYPE_FLOAT);
        AddCheck(mpImage);
        mMmPerPixelWidth = mpImage->MmPerPixelWidth() * mpImage->GetNCols() / mnCols;
        umConfig.AddToParamsControl(mnLinesInPage);
        break;

    case SINO_ATTENUATION:
        SetTypedDataHandler(DATA_TYPE_FLOAT);
        AddCheck(mpSinograms);
        break;

    case SINO_CORRECTED_IMAGE:
        mbCtImage = true;
        SetTypedDataHandler(DATA_TYPE_USHORT);
        AddCheck(mpSinograms);
        break;
    }

    SetMmPerPixel(mMmPerPixelWidth, mMmPerPixelWidth);
}


CSinograms::~CSinograms(void)
{
}
void CSinograms::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("BHCorrection",xmlDump);
    CMetaData::SaveState(xmlDump);
    if (mpImage)
        xmlDump.Write("source_id", mpImage->mUId);
    if (mpSinograms)
        xmlDump.Write("sinograms_id", mpSinograms->mUId);

    const char *zType;
    switch (meType)
    {
    case SINO_SINOGRAMS:
        zType = "SINOGRAMS";
        break;
    case SINO_ATTENUATION:
        zType = "ATTENUATION";
        break;
    case SINO_CORRECTED_IMAGE:
    default:
        zType = "CORRECTED_IMAGE";
        break;
    }
    xmlDump.Write("type", zType);

    umConfig.SaveState(xmlDump);
}

CString CSinograms::GetWindowName(void)
{
    switch (meType)
    {
    case SINO_SINOGRAMS:
        return CString("SinoWindow");
    case SINO_ATTENUATION:
        return CString("AttenWindow");
    default:
        break;
    }
    return CMetaData::GetWindowName();
}
CString CSinograms::GetZoomName(void)
{
    switch (meType)
    {
    case SINO_SINOGRAMS:
        return CString("SinoZoom");
    default:
        break;
    }
    return CMetaData::GetWindowName();
}
bool CSinograms::ShouldUpdate(void)
{

    switch (meType)
    {
    case SINO_SINOGRAMS:
        if (umConfig.mnSinograms != mnSinogramsComputed)
            return true;
        break;
    case SINO_ATTENUATION:
        break;
    case SINO_CORRECTED_IMAGE:
        if (mCorrectionThreshold != mCorrectThresholdDone)
            return true;
        if (mCorrectionFactor != mCorrectionFactorDone)
            return true;
        break;
    default:
        break;
    }
    return CDependant::ShouldUpdate();
}
bool CSinograms::UpdateCorrection(void)
{
	static CEventTracer EvTracer ("UpdateCorrection");
	CDynamicTimer DynTimer (&EvTracer);

    unsigned short *pCorrectedImage = (unsigned short *)GetPageData();
    unsigned short *pOriginalImage = (unsigned short *)mpImage->GetPageData();
    float *pAttenuation = (float *)mpSinograms->GetPageData();
    int nPixels = mnLinesInPage * mnCols;

    while (nPixels--)
    {
        unsigned short originalValue = *pOriginalImage++;
        if (originalValue <= mCorrectionThreshold)
            *pCorrectedImage++ = originalValue;
        else
        {
            int delta = originalValue - mCorrectionThreshold;
            float attenuation = *pAttenuation;
            delta += (unsigned short)(delta * mCorrectionFactor * attenuation);
            *pCorrectedImage++ = (unsigned short)(mCorrectionThreshold + delta);
        }
        pAttenuation++;
    }
    mCorrectionFactorDone = mCorrectionFactor;
    mCorrectThresholdDone = mCorrectionThreshold;
    if (mpfLog)
        fprintf (mpfLog, "<CSinograms::UpdateCorrection> Threshold %d Factor %f\n",
            mCorrectionThreshold, mCorrectionFactor);
    return true;
}
bool CSinograms::UpdateAttenuation(void)
{
	static CEventTracer EvTracer ("UpdateAttenuation");
	CDynamicTimer DynTimer (&EvTracer);

    float *pPage = (float *)mpPageData->GetDataStart();
    int nPixels = mnLinesInPage * mnCols;
    unsigned int nBytesInPage = nPixels * sizeof (pPage[0]);
    memset (pPage, 0, nBytesInPage);

    mMidSino = (mpSinograms->mnCols - 1) / 2.0f;
    float *pSinogram = (float *)mpSinograms->GetPageData();
    pSinogram += mpSinograms->miFirstSinogramLine * mpSinograms->mnCols;
    mDeltaTheta = mpSinograms->mDeltaTheta;

    for (int iSino = 0; iSino < umConfig.mnSinograms; iSino++)
    {
        float theta = mDeltaTheta * iSino;
        float dsdx = (float)cos(theta);
        float dsdy = (float)sin(theta);

        float sinPosStartMidLine = mMidSino + mXStart * dsdx;

        for (int iLine = 0; iLine < mnLinesInPage; iLine++)
        {
            float y = iLine - mImageLineCenter;
            float sinPos = sinPosStartMidLine + y * dsdy;
            float *pLine = pPage + iLine * mnCols;

            for (int iCol = 0; iCol < mnCols; iCol++)
            {
                *pLine++ += pSinogram[(int)sinPos];
                sinPos += dsdx;
            }
        }
        pSinogram += mpSinograms->mnCols;
    }

	//mpPageData->Dump("Attenuation_initial");

    // Divide to avoid high values
    unsigned short *pOriginalImage = (unsigned short *)mpImage->GetPageData();
    int threshold = mCorrectionThreshold / 2;
    float divider = (float)mpSinograms->mnLinesInPage;
    float *pPixels = pPage;
    for (int i = 0; i < nPixels; i++)
    {
        unsigned short originalValue = *pOriginalImage++;
        if (originalValue <= threshold)
            *pPixels++ = 0;
        else
            *pPixels++ /= divider;
    }

	//mpPageData->Dump("Attenuation__final");
    return true;
}
bool CSinograms::DoUpdatePageData(void)
{
    if (mpImage && !mpImage->UpdatePageData())
        return false;
    if (mpSinograms && !mpSinograms->UpdatePageData())
        return false;

    mnImageLines = mpImage->GetNLinesInPage();
    mnImageCols = mpImage->GetNCols();
    mImageLineCenter = (mnImageCols - 1) / 2.0f;
    mImageColCenter = (mnImageLines - 1) / 2.0f;
    mXStart = - (mnImageCols - 1) / 2.0f;
    mXEnd = (mnImageCols - 1) / 2.0f;

    // Special types
    if (meType == SINO_ATTENUATION)
        return UpdateAttenuation();
    if (meType == SINO_CORRECTED_IMAGE)
        return UpdateCorrection();

    // Compute Sinograms
	static CEventTracer EvTracer ("CSinograms::DoUpdatePageData");
	CDynamicTimer DynTimer (&EvTracer);

    float *pPage = (float *)mpPageData->GetDataStart();
    unsigned int nBytesInPage = mnLinesInPage * mnCols * sizeof (pPage[0]);
    memset (pPage, 0, nBytesInPage);
    miFirstSinogramLine = (mnLinesInPage - umConfig.mnSinograms) / 2;
    float *pLine = pPage + miFirstSinogramLine * mnCols;
    mDeltaTheta = (float)(M_PI / umConfig.mnSinograms);

    mMidSino = (mnCols - 1) / 2.0f;

    mnOut = 0;

    int quarter = umConfig.mnSinograms / 4;
    int quarter3 = quarter * 3;
    mCount = 0;
    for (int i = 0; i < umConfig.mnSinograms; i++)
    {
        mbPrint = mpfLog && ((mCount++ % 58) == 0);

        if (i < quarter || i > quarter3)
            ComputeSinogramFromLines(pLine, i * mDeltaTheta);
        else
            ComputeSinogramFromCols(pLine, i * mDeltaTheta);

        pLine += mnCols;
    }

    mnSinogramsComputed = umConfig.mnSinograms;
    if (mpfLog)
        fprintf (mpfLog, "<CSinograms::DoUpdatePageData> nSinograms %d\n", umConfig.mnSinograms);
    return true;
}
//----------------------------------------------------------------------------------------
// Sinogram computation algorithm
// ------------------------------
// Assume center of the image goes to center of the sinogram
// 
//             Image Center
//     ---------*---------------------
//              |\   \
//              | \   \
//     ---------|--\---\---------------
//              |   \   \
//              |    \   \
//              |Theta\   \
//              |      \   \
//              |       \   \
//              |        \  ***
//                      ****                   
//                  ****   Sino-Center                   
//
// ds / dx = sin(90-theta) = cos(theta)
// dx / ds = 1 / sin(90-theta)
// dx / dy = tan(theta)
// dy / dx = 1 / tan(theta)
// ds / dy = sin (theta)
// 
//  Multiply each sinogram by a constant
//  ------------------------------------
//  The distance between two sampling points is the hypotenuse in the triangle 
//  where dy==1 and dx==dxdy
//  dxdy / hypotenuse = sin (theta)
//  hypotenuse = tan(theta) / sin(theta)
// 
//----------------------------------------------------------------------------------------
void CSinograms::ComputeSinogramFromLines(float * pSinogram, float theta)
{
	static CEventTracer EvTracer ("ComputeSinogramFromLines");
	CDynamicTimer DynTimer (&EvTracer);

    if (mbPrint)
        fprintf (mpfLog, "\n <CSinograms::ComputeSinogramFromLines> %d  theta %f\n", mCount, theta);

    float dsdx = (float)cos(theta);
    float dxds = 1.0f / dsdx;
    float dsdy = (float)sin(theta);

    float dxdy = (float)tan(theta);

    float xStartOnSino = mMidSino + mXStart * dsdx;
    float xEndOnSino = mMidSino + mXEnd * dsdx;

    unsigned short *pImageLine = (unsigned short *)(mpImage->GetPageData());

    for (int iLine = 0; iLine < mnImageLines; iLine++)
    {
        float y = iLine - mImageColCenter;
        float xStartLineOnSino = xStartOnSino + y * dsdy;
        float xEndLineOnSino = xEndOnSino + y * dsdy;

        if (xStartLineOnSino > xEndLineOnSino)
            CMyMath::Swap (xStartLineOnSino, xEndLineOnSino);
        int firstSino = (int)ceil (xStartLineOnSino);
        int lastSino = (int)floor (xEndLineOnSino);

        float linePos = mImageLineCenter 
            + (firstSino - mMidSino) * dxds 
            - y * dxdy;

        int count = lastSino - firstSino + 1;
        float *pAccum = pSinogram + firstSino;
        while (count--)
        {
            *pAccum++ += pImageLine[(int)linePos];
            linePos += dxds;
        }
        /*for (int iSin = firstSino; iSin <= lastSino; iSin++)
        {
            //int iInLine = (int)linePos;
            //pSinogram[iSin] += pImageLine[iInLine];
            pSinogram[iSin] += pImageLine[(int)linePos];

            linePos += dxds;
        } */
        
        pImageLine += mnImageCols;
    }

    float mult = 1.0f;
    if (theta > 0.0f)
    {
        mult = dxdy / dsdy;
        if (mult < 0.0f)
            mult = - mult;
    }
    mult /= mnImageCols;
    for (int i = 0; i < mnCols; i++)
        pSinogram[i] *= mult;
    if (mbPrint)
        fprintf (mpfLog, "mult %6.3f nOut %d\n", mult, mnOut);
}
void CSinograms::ComputeSinogramFromCols(float * pSinogram, float theta)
{
	static CEventTracer EvTracer ("ComputeSinogramFromCols");
	CDynamicTimer DynTimer (&EvTracer);

    if (mbPrint)
        fprintf (mpfLog, "\n <CSinograms::ComputeSinogramFromCols> %d  theta %f\n", mCount, theta);

    float dsdx = cosf(theta);
//    float dxds = (float)(1.0f / dsdx);
    float dsdy = sinf(theta);
    float dyds = (float)(1.0f / dsdy);

    float dxdy = (float)tan(theta);
    float dydx = (float)(1.0f / dxdy);

    float yStartOnSino = mMidSino + mXStart * dsdy;
    float yEndOnSino = mMidSino + mXEnd * dsdy;

    unsigned short *pImageRaster = (unsigned short *)(mpImage->GetPageData());
    unsigned short *pImageRasterCol = pImageRaster;

    for (int iCol = 0; iCol < mnImageCols; iCol++)
    {
        float x = iCol - mImageLineCenter;
        float yStartLineOnSino = yStartOnSino + x * dsdx;
        float yEndLineOnSino = yEndOnSino + x * dsdx;

        if (yStartLineOnSino > yEndLineOnSino)
            CMyMath::Swap (yStartLineOnSino, yEndLineOnSino);
        int firstSino = (int)ceil (yStartLineOnSino);
        int lastSino = (int)floor (yEndLineOnSino);

        float colPos = mImageLineCenter 
            + (firstSino - mMidSino) * dyds 
            - x * dydx;

        int count = lastSino - firstSino + 1;
        float *pAccum = pSinogram + firstSino;
        while (count--)
        {
            *pAccum++ += pImageRasterCol[(int)colPos * mnImageCols];
            colPos += dyds;
        }
        /*for (int iSin = firstSino; iSin <= lastSino; iSin++)
        {
            //int iInCol = (int)colPos;
            //pSinogram[iSin] += pImageRaster[iInCol * mnImageCols + iCol];
            pSinogram[iSin] += pImageRaster[(int)colPos * mnImageCols + iCol];

            colPos += dyds;
        } */
        pImageRasterCol++;
    }

    float mult = 1.0f;
    if (dsdx > 0.0001f || dsdx < 0.0001f)
    {
        mult = dydx / dsdx;
        if (mult < 0.0f)
            mult = - mult;
    }
    mult /= mnImageCols;
    for (int i = 0; i < mnCols; i++)
        pSinogram[i] *= mult;
    if (mbPrint)
        fprintf (mpfLog, "mult %6.3f nOut %d\n", mult, mnOut);
}


