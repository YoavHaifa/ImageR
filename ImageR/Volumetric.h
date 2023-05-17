#pragma once
#include "MetaData.h"

class CVolumetric :
    public virtual CMetaData
{
public:
    CVolumetric(const char *zName, class CDisplayRectangle *pSrcDR);
    CVolumetric(const char *zName, class CXMLParseNode *pXmlNode);
    virtual ~CVolumetric();

    virtual CMetaData* GetSourceData();
    CVolume* GetSourceVolume();
    virtual float GetBias();
    virtual bool IsVolumetric(){return true;}
    virtual void SaveState(class CXMLDump & xmlDump);
    virtual bool ReadyForWindowFocus();

protected:
    class CDisplayRectangle* mpSrcDR;

public:
    bool mbSliceIncrementInitialized;
    void InitOnConstruction();
    bool VerifyMmPerPixelPage();
protected:
	bool mbInvertUpDown;
public:
	void InvertUpDown();
	bool IsInvertedUpDown() {return mbInvertUpDown;}
	bool IsComputedFrom(CMetaData *pData);
};
