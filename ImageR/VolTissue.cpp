#include "StdAfx.h"
#include ".\voltissue.h"
#include "DisplayRectangle.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\ImageRLib\TVolume.h"

CVolTissue::CVolTissue(CDisplayRectangle* pSrcDR)
: CMetaData(pSrcDR->GetCurrentData()->msName + "_VolTissue")
, CVolumetric (pSrcDR->GetCurrentData()->msName + "_VolTissue", pSrcDR)
, miLastComputed(0)
{
    CMetaData* pVolumeData = GetSourceData();
    SetTypedDataHandler(DATA_TYPE_BOOL);
    mColor = pVolumeData->mColor;
}

CVolTissue::~CVolTissue(void)
{
}
bool CVolTissue::UpdateColorer(class CDisplayRectangle &displayRect)
{
    if (!IsCommand("Display"))
        return false;

    if (!UpdatePageData())
        return false;
    
    CMetaData *pData = displayRect.GetCurrentData();
    SetCommonZoom (pData->GetZoomParams());
    bool bOK = FillZoomedRaster(displayRect);
    return bOK;
}
bool CVolTissue::ShouldUpdate(void)
{
    CVolume *pVolume = GetSourceVolume();
    if (!pVolume)
        return false;

    unsigned int iLastComputed = pVolume->GetHeader()->mSync.miWriteEnd;
    if (iLastComputed != miLastComputed)
        return true;

    return CMetaData::ShouldUpdate();
}
bool CVolTissue::DoUpdatePageData()
{
    if (!mpfLog)
        mpfLog = CMyWindows::FOpenLogFile (CNamedObject::GetUniqueName(msName));

    CMetaData* pVolumeData = GetSourceData();
    if (pVolumeData->msName.Find("BitVolume") >= 0)
        return false;

    CVolume *pVolume = GetSourceVolume();
    if (!pVolume)
        return false;
    VerifyMmPerPixelPage();

    mpPageData->Zero();
    bool *pOutLine = (bool *)mpPageData->GetDataStart();

    int nImageLines = pVolumeData->GetNLinesInPage();
    int nImagePixels = pVolumeData->GetNCols();
    int nLinesToCompute = pVolume->mnLoaded;
    int iFirstImage = pVolumeData->GetVolume()->First();
    if (mpfLog)
    {
        fprintf (mpfLog, "<CVolTissue::DoUpdatePageData> iFirstImage %d nLinesToCompute %d\n",
            iFirstImage, nLinesToCompute);
        //fprintf (mpfLog, "<CMip::DoUpdatePageData> miCurrent %d pageOffset %d\n", 
        //    mpPosition->miCurrent, pageOffset);
        fprintf (mpfLog, "pVolHeader %p\n", pVolume->GetHeader());
    }
    int n = 0;
    for (int iMipLine = 0; iMipLine < nLinesToCompute; iMipLine++)
    {
        bool *pVolPage = (bool *) pVolume->GetPageVP (iFirstImage + iMipLine);

        for (int iImLine = 0; iImLine < nImageLines; iImLine++)
        {
            for (int iPix = 0; iPix < nImagePixels; iPix++)
            {
                bool value = *pVolPage++;
                pOutLine[iPix] |= value;
                if (value)
                    n++;
            }
        }
        pOutLine += mnCols;
    }
    
    //mpPageData->Dump();
    miLastComputed = pVolume->GetHeader()->mSync.miWriteEnd;

    OnChange();
    if (mpfLog)
    {
        fprintf (mpfLog, "<CVolTissue::DoUpdatePageData> %s n %d i %d\n", 
			(const char*)msName, n, miLastComputed);
        fprintf (mpfLog, "\n");
        fflush (mpfLog);
    }
    return true;
}

bool CVolTissue::IsCommand(const char * zCommand)
{
    CVolume *pVolume = GetSourceVolume();
    if (!pVolume)
        return false;
    CMyImage::SSharedMemHeader *pHeader = pVolume->GetHeader();
    if (!pHeader)
        return false;
    return _stricmp (pHeader->mzCommand, zCommand) == 0;
}

bool * CVolTissue::GetImageMask(int iImage)
{
    CVolume *pVolume = GetSourceVolume();
    if (!pVolume)
        return NULL;
    return (bool *)pVolume->GetPageVP(iImage);
}
