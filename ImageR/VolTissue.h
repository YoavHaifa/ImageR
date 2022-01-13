#pragma once
#include "volumetric.h"

class CVolTissue :
    public CVolumetric
{
public:
    CVolTissue(class CDisplayRectangle* pSrcDR);
    virtual ~CVolTissue(void);

    virtual bool ShouldUpdate(void);
    virtual bool DoUpdatePageData(void);

    // Colorer Interface
    virtual bool IsColorer(void){return true;}
    virtual bool UpdateColorer(class CDisplayRectangle &displayRect);

    bool IsCommand(const char * zCommand);
    bool * GetImageMask(int iImage);
    unsigned int miLastComputed;
};
