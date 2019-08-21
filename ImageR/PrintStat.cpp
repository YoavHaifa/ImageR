#include "StdAfx.h"
#include "PrintStat.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\ImageRLib\TImage.h"
#include "..\..\yUtils\MyRegistry.h"
#include "..\..\yUtils\Histogram.h"

float CPrintStat::umFractionToSample(0.5f);
int CPrintStat::umnSamples(25);
int CPrintStat::umSize(3);
int CPrintStat::umThreshMinRange(0);
int CPrintStat::umThreshMaxRange(4095);
int CPrintStat::umThreshMinAvg(974);
int CPrintStat::umThreshMaxAvg(1500);
int CPrintStat::umHistDelta(5);
int CPrintStat::umTailSizePC(20);
int CPrintStat::umBias(1024);
bool CPrintStat::umbOpenStatFile(false);
bool CPrintStat::umbShowMedian(true);
bool CPrintStat::umbPutMedianInClipboard(true);

CPrintStat::CPrintStat(void)
: mn(0)
, mSDevVec("SDev")
, mTails("Tails")
{
	static bool bOnce = false;
	if (!bOnce)
	{
		ReadRegistry();
		bOnce = true;
	}
	mLog.Open("PrintStat");
	if (umTailSizePC > 0)
	{
		CFileName fName(mLog.Name());
		fName.ChangeType("xls");
		mLogForExcel.Open(fName);
	}
	if (mLog)
	{
		fprintf(mLog.mpf,"Fraction to sample    %.2f\n", umFractionToSample);
		fprintf(mLog.mpf,"Number of samples     %d\n", umnSamples);
		fprintf(mLog.mpf,"Size (on each side)   %d\n", umSize);
		fprintf(mLog.mpf,"Range HU              [%d - %d]\n", umThreshMinRange - umBias, umThreshMaxRange - umBias);
		fprintf(mLog.mpf,"Range Average HU      [%d - %d]\n", umThreshMinAvg - umBias, umThreshMaxAvg - umBias);
		fprintf(mLog.mpf,"Histogram Delta       %d\n", umHistDelta);
		fprintf(mLog.mpf,"Histogram Tail (%%)   %d\n", umTailSizePC);
		fprintf(mLog.mpf,"Bias                  %d\n", umBias);
		fprintf(mLog.mpf,"bOpenStatFile         %s\n", umbOpenStatFile ? "Yes" : "No");
		fprintf(mLog.mpf,"bShowMedian           %s\n", umbShowMedian ? "Yes" : "No");
		fprintf(mLog.mpf,"bPutMedianInClipboard %s\n", umbPutMedianInClipboard ? "Yes" : "No");
		fprintf(mLog.mpf,"\n");
	}
}
CPrintStat::~CPrintStat(void)
{
	float median = mTails.SortAndComputeMedian();
	if (mLog)
		fprintf(mLog.mpf,"\nMedian Tail %f\n",median);
	if (umbShowMedian || umbPutMedianInClipboard)
	{
		char zBuf[256];
		sprintf_s(zBuf,sizeof(zBuf),"Median of all histograms' %d%% tails: %f\r\n%d images\n", 
			umTailSizePC, median, mn);
		if (umbShowMedian)
			CMyWindows::MessBox(zBuf,"Statistics Result");
		if (umbPutMedianInClipboard)
			CMyWindows::PutTextInClipboard(zBuf);
	}
	if (mLog)
	{
		mLog.Close();
		if (umbOpenStatFile)
			CMyWindows::CreateProcess("notepad",mLog.Name());
	}
}
bool CPrintStat::ComputeAndPrint(class CMyImage *pImage)
{
	if (!mLog)
		return false;

	mn++;
	fprintf(mLog.mpf,"Image %3d:\n", mn);
	CHistogram hist("SDev",1000);
	hist.SetDelta((float)umHistDelta);
	mSDevVec.SetEmpty();

	float xStart = pImage->mnCols/2.0f * ( 1 - umFractionToSample);
	float xDelta = pImage->mnCols * umFractionToSample / (umnSamples - 1);
	float yStart = pImage->mnLines/2.0f * ( 1 - umFractionToSample);
	float yDelta = pImage->mnLines * umFractionToSample / (umnSamples - 1);

	float avg, minVal, maxVal, sDev;

	float y = yStart;
	for (int i = 0; i < umnSamples; i++)
	{
		float x = xStart;
		for (int j = 0; j < umnSamples; j++)
		{
			CDataCoordinates coo(*pImage, x,y);
			if (pImage->GetStatisticsPerCoo(coo,umSize,avg, minVal, maxVal, sDev))
			{
				fprintf(mLog.mpf,"P[%3d,%3d] = R[%4d -%4d] avg %12.3f sd %12.3f\n",
					(int)y, (int)x, (int)minVal-umBias, (int)maxVal-umBias, avg-umBias, sDev);

				if (minVal >= umThreshMinRange && maxVal <= umThreshMaxRange
					&& avg >= umThreshMinAvg && avg <= umThreshMaxAvg)
				{
					hist.IncValue(sDev);
					mSDevVec.AddAtEnd(sDev);
				}
			}

			x += xDelta;
		}
		y += yDelta;
	}

	fprintf(mLog.mpf,"\n");
	hist.Print(mLog.mpf);

	// Print Tail
	if (umTailSizePC > 0)
	{
		float tail = mSDevVec.SortAndAverageLowest(umTailSizePC / 100.0f);
		fprintf(mLog.mpf,"Image %3d: Average of lowest %d%% - %f\n", mn, umTailSizePC, tail);
		fprintf(mLog.mpf,"\n");

		mTails.AddAtEnd(tail);

		if (mLogForExcel)
			fprintf(mLogForExcel.mpf,"%d \t %f \n", mn, tail);
	}
	return true;
}
void CPrintStat::ReadRegistry(void)
{
	CMyRegistry reg;
	int fractionPC = -1;

	reg.Get("PrintStatFractionPC",fractionPC);
	if (fractionPC > 0)
		umFractionToSample = fractionPC / 100.0f;

	reg.Get("PrintStatNSamples",umnSamples);
	reg.Get("PrintStatSize",umSize);
	if (reg.Get("PrintStatThreshMinRange",umThreshMinRange))
		umThreshMinRange += umBias;
	if (reg.Get("PrintStatThreshMaxRange",umThreshMaxRange))
		umThreshMaxRange += umBias;
	if (reg.Get("PrintStatThreshMinAvg",umThreshMinAvg))
		umThreshMinAvg += umBias;
	if (reg.Get("PrintStatThreshMaxAvg",umThreshMaxAvg))
		umThreshMaxAvg += umBias;
	reg.Get("PrintStatHistDelta",umHistDelta);
	reg.Get("PrintStatTailSizePC",umTailSizePC);
	reg.Get("PrintStatBias",umBias);
	reg.Get("bOpenStatFile",umbOpenStatFile);
	reg.Get("bShowMedian",umbShowMedian);
	reg.Get("bPutMedianInClipboard",umbPutMedianInClipboard);
}
