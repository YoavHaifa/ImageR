#pragma once
#include "readingsfile.h"

class CFloatPrep :
    public CReadingsFile
{
public:
    CFloatPrep(const char *zName);
    ~CFloatPrep(void);
};
