#pragma once
#include "ReadingsFile.h"

class CPipeBufReadings :
    public CReadingsFile
{
public:
	CPipeBufReadings(const char *zName);
	~CPipeBufReadings(void);
};



