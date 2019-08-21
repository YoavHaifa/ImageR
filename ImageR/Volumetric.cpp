#include "StdAfx.h"
#include ".\volumetric.h"
#include "..\..\ImageRLib\TVolume.h"
#include "..\..\yUtils\XMLDump.h"

#include "Screens.h"

CVolumetric::CVolumetric(const char *zName, CMetaData *pData)
: CMetaData(zName)
, mpVolumeData(pData)
, mbSliceIncrementInitialized(false)
, mbInvertUpDown(false)
{
    InitOnConstruction();
    msName = mpVolumeData->GetShortName() + msName;
}
CVolumetric::CVolumetric(const char *zName, class CXMLParseNode *pXmlNode)
: CMetaData(zName)
, mpVolumeData((CMetaData *)CNamedObject::GetParsed(pXmlNode, "volume_id"))
, mbSliceIncrementInitialized(false)
{
    InitOnConstruction();
}
CVolumetric::~CVolumetric(void)
{
}
void CVolumetric::InitOnConstruction(void)
{
    gScreens.mVolumeRefreshInterval = 1;
    mpVolumeData->StartLoadVolume ();
    mnLinesInPage = mpVolumeData->GetNVolumePages();
    mnCols = mpVolumeData->GetNCols();
    mMmPerPixelWidth = mpVolumeData->MmPerPixelWidth();
    mbCtImage = mpVolumeData->IsCtImage();

	if (mpVolumeData)
		SetDataType(mpVolumeData->GetDataType());

    CVolume *pVolume = mpVolumeData->GetVolume();
    if (pVolume)
        AddCheck(pVolume);
}
bool CVolumetric::VerifyMmPerPixelPage(void)
{
    if (mbSliceIncrementInitialized)
        return false;

    CVolume *pVolume = mpVolumeData->GetVolume();
    if (pVolume && pVolume->mnLoaded > 1)
    {
        SetMmPerPixel(mMmPerPixelWidth, mpVolumeData->MmPerPixelPage());
        mbSliceIncrementInitialized = true;
        if (mpfLog)
            fprintf (mpfLog, "<CVolumetric::VerifyMmPerPixelPage> %s volData %s mMmPerPixelHeight %f\n",
                msName, mpVolumeData->msName, mMmPerPixelHeight);
        return true;
    }
    return false;
}
void CVolumetric::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("Volumetric",xmlDump);
    CMetaData::SaveState(xmlDump);
    if (mpVolumeData)
        xmlDump.Write("volume_id", mpVolumeData->mUId);
}
bool CVolumetric::ReadyForWindowFocus(void)
{
    CVolume *pVolume = mpVolumeData->GetVolume();
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
	return (mpVolumeData == pData);
}
