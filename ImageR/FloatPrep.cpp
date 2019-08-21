#include "StdAfx.h"
#include "FloatPrep.h"
#include "..\..\yUtils\MyWindows.h"
#include "ReadingPosition.h"

CFloatPrep::CFloatPrep(const char *zName)
: CReadingsFile (zName)
{
    mnSlices = 1;
    mnFFS = 1;
    mlFanHeaderBytes = 0;
    mSliceLen = 672;
    mnCols = mSliceLen;
    mSliceLenWithoutFooter = mSliceLen;

    mDetectorSize = sizeof(float);

    ComputeReadingSize();

    mnFrames = (unsigned long)CMyWindows::GetFileLength(msName) / mFrameSize;
    mnLinesInPage = min(mnFrames, 580);
    SetTypedDataHandler(DATA_TYPE_FLOAT);
}

CFloatPrep::~CFloatPrep(void)
{
}
