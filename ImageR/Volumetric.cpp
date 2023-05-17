#include "StdAfx.h"
#include ".\volumetric.h"
#include "..\..\ImageRLib\TVolume.h"
#include "..\..\yUtils\XMLDump.h"

#include "Screens.h"
#include "DisplayRectangle.h"

CVolumetric::CVolumetric(const char *zName, CDisplayRectangle* pSrcDR)
: CMetaData(zName)
, mpSrcDR(pSrcDR)
, mbSliceIncrementInitialized(false)
, mbInvertUpDown(false)
{
    InitOnConstruction();
    CMetaData* pVolumeData = mpSrcDR->GetCurrentData();
    msName = pVolumeData->GetShortName() + msName;
}
CVolumetric::CVolumetric(const char *zName, class CXMLParseNode *pXmlNode)
: CMetaData(zName)
, mpSrcDR((CDisplayRectangle*)CNamedObject::GetParsed(pXmlNode, "src_dr_id"))
, mbSliceIncrementInitialized(false)
, mbInvertUpDown(false)
{
    InitOnConstruction();
}
CVolumetric::~CVolumetric(void)
{
	// mpVolumeData->DeleteVolume();
}
void CVolumetric::InitOnConstruction(void)
{
    CMetaData* pVolumeData = mpSrcDR->GetCurrentData();
    gScreens.mVolumeRefreshInterval = 1;
    pVolumeData->StartLoadVolume ();
    mnLinesInPage = pVolumeData->GetNVolumePages();
    mnCols = pVolumeData->GetNCols();
    mMmPerPixelWidth = pVolumeData->MmPerPixelWidth();
    SetCtImage(pVolumeData->IsCtImage(), true /*force*/);
	mRescaleIntercept = pVolumeData->mRescaleIntercept;
	mRescaleSlope = pVolumeData->mRescaleSlope;

	if (pVolumeData)
		SetDataType(pVolumeData->GetDataType());

    AddCheck(mpSrcDR);
    CVolume *pVolume = pVolumeData->GetVolume();
    if (pVolume)
        AddCheck(pVolume);
}
CMetaData* CVolumetric::GetSourceData(void) 
{
    if (!mpSrcDR)
        return NULL;
    return mpSrcDR->GetCurrentData();
}
CVolume* CVolumetric::GetSourceVolume()
{
    CMetaData* pVolumeData = GetSourceData();
    if (!pVolumeData)
        return NULL;
    CVolume *pVolume = pVolumeData->GetVolume();
    if (!pVolume)
    {
        pVolumeData->StartLoadVolume();
        pVolume = pVolumeData->GetVolume();
        if (pVolume)
            AddCheck(pVolume);
    }
    return pVolume;
}
float CVolumetric::GetBias(void)
{
    CMetaData* pVolumeData = GetSourceData();
    if (pVolumeData)
        return pVolumeData->GetBias();
    return 0.0f;
}
bool CVolumetric::VerifyMmPerPixelPage(void)
{
    if (mbSliceIncrementInitialized)
        return false;

    CMetaData* pVolumeData = GetSourceData();
    CVolume *pVolume = pVolumeData->GetVolume();
    if (pVolume && pVolume->mnLoaded > 1)
    {
        SetMmPerPixel(mMmPerPixelWidth, pVolumeData->MmPerPixelPage());
        mbSliceIncrementInitialized = true;
        if (mpfLog)
            fprintf (mpfLog, "<CVolumetric::VerifyMmPerPixelPage> %s volData %s mMmPerPixelHeight %f\n",
			(const char*)msName, (const char*)pVolumeData->msName, mMmPerPixelHeight);
        return true;
    }
    return false;
}
void CVolumetric::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("Volumetric",xmlDump);
    CMetaData::SaveState(xmlDump);
    if (mpSrcDR)
        xmlDump.Write("src_dr_id", mpSrcDR->mUId);
    xmlDump.Write("b_invert_up_down", mbInvertUpDown);
}
bool CVolumetric::ReadyForWindowFocus(void)
{
    CVolume *pVolume = GetSourceVolume();
    if (pVolume && pVolume->mnLoaded > mnLinesInPage / 2)
        return true;
    return false;
}
void CVolumetric::InvertUpDown(void)
{
	mbInvertUpDown = !mbInvertUpDown;
	mbPageUpdated = false;
	OnChange();
}
bool CVolumetric::IsComputedFrom(CMetaData *pData)
{
    CMetaData* pVolumeData = GetSourceData();
    return (pVolumeData == pData);
}
