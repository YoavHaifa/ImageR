#pragma once
#include "MetaData.h"

class CTissue :
    public CMetaData
{
public:
    CTissue(int nLines, int nCols, const char *zName);
    ~CTissue(void);

    CTImage<bool> &Raster(void);
    virtual void AdjustColors(class CDisplayRectangle &dr);
    bool SetShape(CDisplayRectangle &displayRect);
    virtual bool UpdateColorer(class CDisplayRectangle &){return false;}
    virtual bool IsColorer(void){return true;}
    //float Average(void){return mAverage;}
    //int N(void){return mn;}
    //float StandardDeviation(void){return mStandardDeviation;}

protected:
public:
    bool * GetMaskStart(void);
};
