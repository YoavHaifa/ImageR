#pragma once
#include "..\..\yUtils\LogFile.h"
#include "..\..\yUtils\CompVector.h"

class CPrintStat
{
public:
	CPrintStat(void);
	~CPrintStat(void);

	CLogFile mLog;
	CLogFile mLogForExcel;
	int mn;

	static float umFractionToSample;
	static int umnSamples;
	static int umSize;
	static int umThreshMinRange;
	static int umThreshMaxRange;
	static int umThreshMinAvg;
	static int umThreshMaxAvg;
	static int umHistDelta;
	static int umTailSizePC;
	static int umBias;
	static bool umbOpenStatFile;
	static bool umbShowMedian;
	static bool umbPutMedianInClipboard;

	CCompVector mSDevVec;
	CCompVector mTails;
	bool ComputeAndPrint(class CMyImage *pImage);
	static void ReadRegistry(void);
};
