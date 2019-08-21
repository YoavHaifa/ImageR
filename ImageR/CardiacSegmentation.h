#pragma once
#include "metafile.h"

class CCardiacSegmentation :
    public CMetaFile
{
public:
    CCardiacSegmentation(const char *zName, CMyDicom *pDicom);
    virtual ~CCardiacSegmentation(void);
    virtual bool DoUpdatePageData(void);

private:
    int mnSlices;
    unsigned char *mpData;
};
