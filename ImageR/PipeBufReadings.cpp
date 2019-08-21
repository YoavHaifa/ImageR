#include "StdAfx.h"
#include "PipeBufReadings.h"
#include "..\..\yUtils\MyWindows.h"

CPipeBufReadings::CPipeBufReadings(const char *zName)
: CReadingsFile (zName)
{
    CFileName fName(zName);
    CString sPrivate(fName.Private());

    mnSlices = 1;
    mnFFS = 2;
    mlFanHeaderBytes = 128;
    mSliceLen = 680;
    mnCols = mSliceLen;
    mSliceLenWithoutFooter = 672;
		
	CMyWindows::GetValueFromString(sPrivate, "_header", mlFanHeaderBytes);
	CMyWindows::GetValueFromString(sPrivate, "_width", mSliceLen);
	CMyWindows::GetValueFromString(sPrivate, "_height", mnSlices);

    mDetectorSize = sizeof(short);

    ComputeReadingSize();

    mnFrames = (unsigned long)CMyWindows::GetFileLength(msName) / mFrameSize;
    mnLinesInPage = min(mnFrames, 580);
    SetTypedDataHandler(DATA_TYPE_USHORT);
}


CPipeBufReadings::~CPipeBufReadings(void)
{
}
