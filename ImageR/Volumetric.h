#pragma once
#include "MetaData.h"

class CVolumetric :
    public virtual CMetaData
{
public:
    CVolumetric(const char *zName, CMetaData *pData);
    CVolumetric(const char *zName, class CXMLParseNode *pXmlNode);
    virtual ~CVolumetric(void);

    virtual CMetaData *GetSourceData(void){return mpVolumeData;}
    virtual float GetCtBias (void) {return (mbCtImage && mpVolumeData) ? mpVolumeData->GetCtBias() : 0.0f;}
    virtual bool IsVolumetric(void){return true;}
    virtual void SaveState(class CXMLDump & xmlDump);
    virtual bool ReadyForWindowFocus(void);

protected:
    CMetaData *mpVolumeData;

public:
    bool mbSliceIncrementInitialized;
    void InitOnConstruction(void);
    bool VerifyMmPerPixelPage(void);
protected:
	bool mbInvertUpDown;
public:
	void InvertUpDown(void);
	bool IsInvertedUpDown(void) {return mbInvertUpDown;}
	bool IsComputedFrom(CMetaData *pData);
};
