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
    static void SetSmoothFactor(float factor);

private:
    CMetaData *mpImage;
    void *mpTempRaster;
    static float umSmoothFactor;
    static float umMaxSmoothFactor;
    float mSmoothFactorDone;

    void Smooth();
    template <typename T>
    void Smooth(T *pRaster);
    template <typename T>
    void CopyFrame(T *pRaster, const T *pSrcRaster);
    template <typename T>
    void Smooth1(T *pRaster, const T *pSrcRaster);
    template <typename T>
    void PartSmooth(T *pRaster, const T *pSrcRaster, float factor);
    bool AllocateTempRaster(void);
    virtual float GetCtBias (void) {return mbCtImage && mpImage ? mpImage->GetCtBias() : 0.0f;}

public:

    void InitOnConstruction(void);
};
