#pragma once
#include "metafile.h"

class CBmpFile :
    public CMetaFile
{
public:
    CBmpFile(const char *zName);
    ~CBmpFile(void);
};
