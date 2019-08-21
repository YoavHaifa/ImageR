#pragma once
#include "MetaData.h"

class CProcImage :
    public CMetaData
{
public:
    CProcImage(CMetaData *pImage, const char *zName);
    CProcImage(class CXMLParseNode *pXmlNode);
    ~CProcImage(void);

    virtual bool DoUpdatePageData(void);
    virtual bool ShouldUpdate(void);
    virtual void SaveState(class CXMLDump & xmlDump);

private:
    CMetaData *mpImage;
    unsigned short *mpTempRaster;
    unsigned short *mpCurrentRaster;
    float mSmoothFactor;
    float mSmoothFactorDone;

    void Smooth(void);
    void CopyFrame(unsigned short *pRaster, const unsigned short *pSrcRaster);
    void Smooth1(unsigned short *pRaster, const unsigned short *pSrcRaster);
    void PartSmooth(unsigned short *pRaster, const unsigned short *pSrcRaster, float factor);
    bool AllocateTempRaster(void);
    virtual float GetCtBias (void) {return mbCtImage && mpImage ? mpImage->GetCtBias() : 0.0f;}

public:

    void InitOnConstruction(void);
};
