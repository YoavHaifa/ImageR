#include "StdAfx.h"
#include ".\windowparams.h"
#include "ViewParams.h"
#include "MetaData.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\LogFile.h"
#include "..\..\yUtils\MyRegistry.h"
#include "..\..\ImageRLib\WindowAutoFocus.h"
#include "Screens.h"

int CWindowParams::umCreateIndex = 0;
int CWindowParams::umDebugWindow = 0;
FILE *CWindowParams::umpfLog = NULL;
float CWindowParams::umWindowAccelerationFactor = 1.0f;
bool CWindowParams::umbUseAllRangeForColorer = true;

CWindowParams::CWindowParams(class CMetaData *pData, bool bSetFocus)
: CDependant(pData ? pData->GetWindowName() : "WindowParams")
, mIndex (umCreateIndex++)
, mCenter (1024)
, mWidth (128)
, mbInitialized (false)
, mpCurrentData(pData)
, mMin (0)
, mMax (MAX_CT_VALUE)
, mRange (MAX_CT_VALUE)
, mbExternalRangeSet(false)
, mLastMinUsed(0)
, mLastMaxUsed(0)
{
	if (umDebugWindow && !umpfLog)
	{
		umpfLog = CMyWindows::FOpenLogFile("CWindowParams");
	}
	if (umpfLog)
	{
		fprintf(umpfLog,"<CWindowParams> pData %p %s focus %d\n", 
			pData, pData ? (const char*)pData->msName : "", (int)bSetFocus);
		fflush(umpfLog);
	}
    if (!ReadInitialWindowFromRegistry(pData))
    {
        if (bSetFocus && pData)
            SetAutoFocus(pData);
    }
    mRange = mMax - mMin;
	if (umpfLog)
		DebugPrint("End Constructor");
}
CWindowParams::CWindowParams(class CXMLParseNode *pWindowXml)
: CDependant(pWindowXml)
, mIndex (umCreateIndex++)
, mCenter (1024)
, mWidth (128)
, mbInitialized (false)
, mMin (0)
, mMax (MAX_CT_VALUE)
, mRange (MAX_CT_VALUE)
, mbExternalRangeSet(false)
, mLastMinUsed(0)
, mLastMaxUsed(0)
{
	if (umDebugWindow && !umpfLog)
	{
		umpfLog = CMyWindows::FOpenLogFile("CWindowParamsX");
	}
	if (umpfLog)
	{
		fprintf(umpfLog,"<CWindowParams> pWindowXml %p\n", pWindowXml);
		fflush(umpfLog);
	}
    if (pWindowXml->GetValue("center", mCenter)
        && pWindowXml->GetValue("width", mWidth))
        mbInitialized = true;
    mRange = mMax - mMin;
	if (umpfLog)
		DebugPrint("End Constructor from XML");
}
CWindowParams::~CWindowParams(void)
{
	DebugPrint("destructor");
}
void CWindowParams::UpdateByMouseMove(int deltaX, int deltaY, class CMetaData* pData)
{
	bool bCT = pData->IsCtImage();
	float range = pData->Range();

	mWidth += ScaleDelta(deltaX, range, bCT);
	mCenter -= ScaleDelta(deltaY, range, bCT);

	SetInRange(bCT, pData->Min(), pData->Max());
	if (umDebugWindow)
		DebugPrint("UpdateByMouseMove");
}
CString CWindowParams::Value2StringByScale(float value, bool bInt)
{
    static char zBuf[128];
    int intValue = (int)value;

    if (bInt || (float)intValue == value || value <= -100.0f || value >= 100.0f)
        sprintf_s (zBuf, 128, "%d", intValue);
    else if (value <= 0.01f || value >= 0.01f)
        sprintf_s (zBuf, 128, "%.3f", value);
    else
        sprintf_s (zBuf, 128, "%.6f", value);
	return CString(zBuf);
}
CString CWindowParams::GetLowText ()
{
    bool bUseInt = mpCurrentData && mpCurrentData->UseIntegerWindowStatus();
	float low = mLastMinUsed;
	if (mpCurrentData)
		low = mpCurrentData->Rescale(low);
	if (umpfLog)
	{
		static CString sLast;
		CString sNew = Value2StringByScale (low, bUseInt);
		if (sNew != sLast)
		{
			fprintf(umpfLog,"<GetLowText> %s\n", (const char*)sNew);
			sLast = sNew;
		}
	}
    return Value2StringByScale (low, bUseInt);
}
CString CWindowParams::GetHighText ()
{
    bool bUseInt = mpCurrentData && mpCurrentData->UseIntegerWindowStatus();
	float high = mLastMaxUsed;
	if (mpCurrentData)
		high = mpCurrentData->Rescale(high);

	if (umpfLog)
	{
		static CString sLast;
		CString sNew = Value2StringByScale (high, bUseInt);
		if (sNew != sLast)
		{
			fprintf(umpfLog,"<GetHighText> %s\n", (const char*)sNew);
			sLast = sNew;
		}
	}
    return Value2StringByScale (high, bUseInt);
}
CString CWindowParams::GetWidthText ()
{
	float width = mWidth;
	if (mpCurrentData)
		width = mpCurrentData->RescaleWidth(width);

    return Value2StringByScale (width);
}
CString CWindowParams::GetCenterText ()
{
    bool bUseInt = mpCurrentData && mpCurrentData->UseIntegerWindowStatus();
    float center = mCenter;
	if (mpCurrentData && mpCurrentData->IsCtImage())
		center = mpCurrentData->Rescale(center);

    return Value2StringByScale (center, bUseInt);
}
float CWindowParams::ScaleDelta (int delta, float range, int bInteger)
{
    if (delta == 0)
        return 0.0f;

    int absDelta = (delta >= 0) ? delta : - delta;
    float factor;

    static const int smallDelta = 5;
    static const int bigDelta = 50;

    float smallFactor = mWidth;
    if (mWidth < (range / 100.0f))
        smallFactor = range / 100.0f;

    if (absDelta <= smallDelta)
        factor = smallFactor;
    else if (absDelta >= bigDelta)
        factor = range;
    else
        factor = smallFactor + (range - smallFactor) * (absDelta - smallDelta) / (bigDelta - smallDelta);

    float fDelta = (float)delta * factor / 400.0f;
	fDelta *= umWindowAccelerationFactor;

    if (bInteger)
    {
        if (fDelta < 0.0f && fDelta > -1.0f)
            fDelta = -1.0f;
        else if (fDelta > 0.0f && fDelta < 1.0f)
            fDelta = 1.0f;
    }

    return fDelta;
}

void CWindowParams::SetCt(float minDataValue)
{
	if (minDataValue > 0)
		minDataValue = 0;

    float fMAX_CT_VALUE = (float)MAX_CT_VALUE;

    mWidth = (float)(int)mWidth;

	if (mWidth > fMAX_CT_VALUE)
		mWidth = fMAX_CT_VALUE;
    if (mWidth < 0.0f)
		mWidth = 0.0f;

    mCenter = (float)(int)mCenter;

	if (mCenter > fMAX_CT_VALUE)
		mCenter = fMAX_CT_VALUE;
	else if (mCenter < minDataValue)
		mCenter = minDataValue;
	DebugPrint("SetCt");
}

void CWindowParams::SetInRange(bool bCT, float min, float max)
{
    if (bCT && !mbExternalRangeSet)
	{
        SetCt(min);
		return;
	}
	if (mbExternalRangeSet)
	{
		min = mMin;
		max = mMax;
	}

    float range = max - min;
    float minWidth = 1.0f;
    if (range < 1000.0f)
        minWidth = range / 1000.0f;

    if (mWidth < minWidth)
		mWidth = minWidth;
    if (mWidth > 2.0f * range)
        mWidth = 2.0f * range;

    if (mCenter > max)
        mCenter = max;
    else if (mCenter < min)
        mCenter = min;
	DebugPrint("SetInRange");
}
bool CWindowParams::SetLocalFocus(CTImage<float> *pRaster, class CDRCoordinates coo)
{
    if (!pRaster || pRaster->IsEmpty())
        return false;
    CRect rect = pRaster->GetRectAround(coo, 20);
    return SetAutoFocus(pRaster, rect);
}
bool CWindowParams::SetAutoFocus(CMetaData *pData)
{
	if (umpfLog)
	{
		fprintf(umpfLog,"<SetAutoFocus> pData %p %s ready %s\n", 
			pData, pData ? (const char*)pData->msName : "", pData->ReadyForWindowFocus() ? "Yes" : "No");
		fflush(umpfLog);
	}
    if (!pData->ReadyForWindowFocus())
        return false;

    CTImage<float> *pRaster = pData->GetFloatZoomedRaster();
    if (!pRaster || pRaster->IsEmpty())
	{
		if (umpfLog)
		{
			fprintf(umpfLog,"<SetAutoFocus> pRaster %p NULL or EMPTY\n\n", pRaster);
			fflush(umpfLog);
		}
        return false;
	}
	if (!pData->ZoomedRasterIsReady())
	{
		if (umpfLog)
		{
			fprintf(umpfLog,"<SetAutoFocus> Zoom not ready\n\n");
			fflush(umpfLog);
		}
		return false;
	}

	//static FILE *pf = NULL;
	//if (!pf)
	//	pf = CMyWindows::FOpenLogFile("AutoFocus");

    float fraction = pData->IsDiff() ? 0.8f : 0.3f;
	CRect relevantRect (pData->GetZoomedRect());

	CRect rect = pRaster->GetCenterOfRect(fraction, relevantRect);
	float ratioNotZero = 0.0f;
    bool bOK = SetAutoFocus(pRaster, rect, &ratioNotZero);
	if (umpfLog)
	{
		fprintf(umpfLog,"<SetAutoFocus> bOK %s\n\n", bOK ? "Yes" : "No");
		fflush(umpfLog);
	}
	/* if (pf)
	{
		fprintf (pf, "<SetAutoFocus> %s\n", pData->Name());
		fprintf (pf, "relevantRect x %3d [%4d - %4d] y %3d [%4d - %4d]\n",
			relevantRect.right - relevantRect.left, relevantRect.left, relevantRect.right,
			relevantRect.bottom - relevantRect.top, relevantRect.top, relevantRect.bottom);
		fprintf (pf, "rect center  x %3d [%4d - %4d] y %3d [%4d - %4d] (fraction %f) ratioNotZero %f\n", 
			rect.right - rect.left, rect.left, rect.right,
			rect.bottom - rect.top, rect.top, rect.bottom, fraction, ratioNotZero);
		fflush(pf);
	} */
	if (ratioNotZero < 0.3f)
	{
		float ratioNotZeroFull = 0.0f;
		bOK = SetAutoFocus(pRaster, relevantRect, &ratioNotZeroFull);
		if (ratioNotZeroFull < ratioNotZero)
			bOK = SetAutoFocus(pRaster, rect, &ratioNotZero);
	}
	/* if (pf)
	{
		fprintf (pf, "mWidth %f mCenter %f\n", mWidth, mCenter);
		fprintf (pf, "\n");
		fflush(pf);
	} */
	DebugPrint("AutoFocus");
	return bOK;
}
bool CWindowParams::SetAutoFocus(CTImage<float> * pRaster, CRect rect, float *poRatioNotZero)
{
	CWindowAutoFocus windowAutoFocus;
	bool bOK = windowAutoFocus.RecommendAutoFocus(pRaster, rect, mCenter, mWidth, poRatioNotZero);
	if (bOK)
		mbInitialized = true;
	return bOK;
}
void CWindowParams::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("WindowParams",xmlDump);
    CNamedObject::SaveState(xmlDump);
    if (mDumpCount)
        return;

    xmlDump.Write("center", mCenter);
    xmlDump.Write("width", mWidth);
	if (mbExternalRangeSet)
	{
		xmlDump.Write("b_external_range_set", mbExternalRangeSet);
		xmlDump.Write("min_val", mMin);
		xmlDump.Write("max_val", mMax);
	}
}
void CWindowParams::SetRange (float min, float max)
{
	if (mbExternalRangeSet)
		return;

    mMin = min;
    mMax = max;
    mRange = mMax - mMin;
	if (umpfLog)
		DebugPrint("End SetRange");
}
void CWindowParams::ExternalSetRange (float min, float max)
{
	if (umpfLog)
	{
		fprintf(umpfLog,"<ExternalSetRange> min %f max %f\n", min, max);
	}
	mbExternalRangeSet = true;
    mMin = min;
    mMax = max;
    mRange = mMax - mMin;
	DebugPrint("ExternalSetRange");
}
bool CWindowParams::SetCenterFromGui(float center)
{
	if (mpCurrentData)
		center = mpCurrentData->Unscale(center);
    return SetCenter (center);
}
bool CWindowParams::SetRangeFromGui(float minVal, float maxVal)
{
	if (mpCurrentData)
	{
		minVal = mpCurrentData->Unscale(minVal);
		maxVal = mpCurrentData->Unscale(maxVal);
	}
	ExternalSetRange(minVal, maxVal);
	return true;
}
bool CWindowParams::SetWidthFromGui(float width)
{
	if (mpCurrentData)
		width = mpCurrentData->UnscaleWidth(width);
	return SetWidth(width);
}
bool CWindowParams::SetWidth(float width)
{
    if (width < 0.0f)
        width = 0;
    else if (width > 2 * mRange)
        width = 2 * mRange;

    if (width == mWidth)
        return false;

    mWidth = width;
	DebugPrint("SetWidth");
    OnChange();
	return true;
}
bool CWindowParams::SetCenter(float newCenter)
{
    if (newCenter < mMin)
        newCenter = mMin;
    else if (newCenter > mMax)
        newCenter = mMax;

    if (newCenter == mCenter)
        return false;

	mCenter = newCenter;
	DebugPrint("SetCenter");
    OnChange();
	return true;
}
bool CWindowParams::ReadInitialWindowFromRegistry(class CMetaData *pData)
{
    CMyRegistry myReg("SOFTWARE\\ImageR", KEY_READ);
    if (!myReg)
        return false;

    DWORD value;
	static bool ubOnce = false;
	if (!ubOnce)
	{
		ubOnce = true;
		if (myReg.Get("AutoFocusSharpness", value))
		{
			if (value > 0 && value < 10000)
			{
				float factor = CWindowAutoFocus::GetSharpnessFactor();
				factor *= (100.0f / value);
				CWindowAutoFocus::SetSharpnessFactor(factor);
			}
		}
	}

    int nRead = 0;
    CString sName (msName + "Center");
	
    if (myReg.Get(sName, value))
    {
		short center = (short)value; // Force it to get signed value from DWORD
        mCenter = (float)center;
		if (pData)
			mCenter = pData->Unscale(mCenter);
        nRead++;
    }

    sName = msName + "Width";
    if (myReg.Get(sName, value))
    {
        mWidth = (float)value;
 		if (pData)
			mWidth = pData->UnscaleWidth(mWidth);
        nRead++;
    }
    if (nRead == 2)
    {
        mbInitialized = true;
        return true;
    }
	DebugPrint("ReadInitialWindowFromRegistry");
    return false;
}
void CWindowParams::GetWindowMinMax(float &oMin, float &oMax, bool bColorer)
{
    float halfWidth = mWidth / 2.0f;
    oMin = mCenter - halfWidth;
    oMax = mCenter + halfWidth;

	if (bColorer && umbUseAllRangeForColorer && mbExternalRangeSet)
	{
		if (mMin < mMax)
		{
			bool bClipped = false;
			if (umpfLog)
				fprintf(umpfLog,"GetWindowMinMax(colorer use all range) --> %f - %f\n", oMin, oMax);
			if (oMin < mMin)
			{
				oMin = mMin;
				bClipped = true;
			}
			if (oMax > mMax)
			{
				oMax = mMax;
				bClipped = true;
			}
			if (bClipped && umpfLog)
				fprintf(umpfLog,"GetWindowMinMax(colorer use all range) CLIPPED --> %f - %f\n\n", oMin, oMax);
		}
	}
	mLastMinUsed = oMin;
	mLastMaxUsed = oMax;
	if (umpfLog)
		fprintf(umpfLog,"GetWindowMinMax --> %f - %f\n\n", oMin, oMax);
}
void CWindowParams::ReadRegistry(CMyRegistry &myReg)
{
	int windowAccelerationFactor = 0;
	if (myReg.Get("WindowingAccelerationFactorPC",windowAccelerationFactor) && windowAccelerationFactor > 0)
		umWindowAccelerationFactor = (float)windowAccelerationFactor / 100.0f;

	myReg.Get("DebugWindowing",umDebugWindow);
	myReg.Get("bWindowingUseAllRangeForColorer", umbUseAllRangeForColorer);
}
void CWindowParams::DebugPrint(const char *zText)
{
	if (!umpfLog)
		return;
	fprintf(umpfLog,"<CWindowParams::DebugPrint> %s %d: %s\n", (const char*)msName, mUId, zText);

	fprintf(umpfLog,"C %12f  -  W %12f  -  [%8.3f - %8.3f] range %.3f %s\n", mCenter, mWidth, mMin, mMax, mRange,
		mbExternalRangeSet ? "Ext Range Set" : "");

	if (mpCurrentData)
	{
		float rescaledC = mpCurrentData->Rescale(mCenter);
		float rescaledW = mpCurrentData->RescaleWidth(mWidth);
		float resMin = mpCurrentData->Rescale(mMin);
		float resMax = mpCurrentData->Rescale(mMax);
		if (rescaledW != mWidth || rescaledC != mCenter)
		{
			fprintf(umpfLog,"Rescaled: C %12f  -  W %12f  -  [%8.3f - %8.3f]\n", rescaledC, rescaledW, resMin, resMax);
		}
	}
	fprintf(umpfLog,"\n");
	fflush(umpfLog);
	if (mRange < 0)
	{
		static int count = 0;
		if (count++ < 1)
			CMyWindows::MessBox("< CWindowParams::DebugPrint> mRange < 0", "SW Error");
	}
}
