#include "StdAfx.h"
#include "GRoi.h"
#include "DisplayRectangle.h"
#include "Screens.h"
#include "MetaData.h"
#include "Tissue.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyMath.h"
#include "DisplayGE.h"
#include "GraphicElementOnDR.h"

CGRoi::CGRoi(class CDisplayRectangle *pDispRect, const char *zName, COLORREF color, int deltaMm)
: CDataRoi(pDispRect->GetPosition(), zName, color)
{
    InitPoints(pDispRect, deltaMm);

    for (int i = 0; i < N_POINTS; i++)
        mDataPoints.AddTail(mapDataPoints[i]);
}
CGRoi::CGRoi(class CDisplayRectangle *pDispRect, class CXMLParseNode *pGRoiXml)
: CDataRoi(pDispRect->GetCurrentData()->PageData(), pGRoiXml)
{
}
CGRoi::~CGRoi(void)
{
}
void CGRoi::InitPoints(CDisplayRectangle *pDispRect, int deltaMm)
{
    CMetaData *pData = pDispRect->GetCurrentData();
    CDataCoordinates dataCoo (pDispRect->GetSelectedDataCoo());

    int delta = deltaMm / 2;
    CMyImage *pPage = pData->PageData();
    mapDataPoints[0] = new CDataCoordinates(*pPage, dataCoo.fx-delta, dataCoo.fy-delta);
    mapDataPoints[1] = new CDataCoordinates(*pPage, dataCoo.fx+delta, dataCoo.fy-delta);
    mapDataPoints[2] = new CDataCoordinates(*pPage, dataCoo.fx+delta, dataCoo.fy+delta);
    mapDataPoints[3] = new CDataCoordinates(*pPage, dataCoo.fx-delta, dataCoo.fy+delta);
    mapDataPoints[4] = new CDataCoordinates(*pPage, dataCoo.fx, dataCoo.fy);
}
void CGRoi::OnNewDisplay(CGraphicElementOnDR * pGEonDR)
{
    pGEonDR->mbAllowPermanentSelection = false;
    pGEonDR->mbConnectLastAndFirst = true;
    pGEonDR->mbDisplayInactivePoints = false;
    pGEonDR->mnSeenPoints = N_SEEN_POINTS;

    CGPoint **pPoints = pGEonDR->GetPointsMap();
    if (pPoints)
        pPoints[N_SEEN_POINTS]->mbActivateAll = true;
}
void CGRoi::SetRelativeClip(int i, CRect &rClip, CGPoint *apPointsOnDR[])
{
    if (i == 0 || i == 3)
        rClip.right = apPointsOnDR[1]->x - 5;
    else
        rClip.left = apPointsOnDR[0]->x + 5;

    if (i == 0 || i == 1)
        rClip.bottom = apPointsOnDR[2]->y - 5;
    else
        rClip.top = apPointsOnDR[0]->y + 5;
}
bool CGRoi::Drag(CRect rect, CSize size, CGPoint *apPointsOnDR[])
{
    bool bAllActive = apPointsOnDR[0]->mbActive && apPointsOnDR[1]->mbActive;
    if (bAllActive && mbFixedCenter)
        return false;

    CSize rectSize (rect.right - rect.left, rect.bottom - rect.top);
    CRect rectClip (0,0,rectSize.cx, rectSize.cy);
    bool bChange = false;
    for (int i = 0; i < N_SEEN_POINTS; i++)
    {
        if (apPointsOnDR[i]->mbActive)
        {
            CRect rClip(rectClip);
            if (!bAllActive)
                SetRelativeClip(i, rClip, apPointsOnDR);

            bChange = apPointsOnDR[i]->ClipDrag (rClip, size);
            if (!bChange)
                break;
        }
    }
    if (bChange)
    {
        for (int i = 0; i < N_SEEN_POINTS; i++)
        {
            if (apPointsOnDR[i]->mbActive)
            {
                *apPointsOnDR[i] += size;
                AdjustOtherPointsToDrag (i, apPointsOnDR);
                apPointsOnDR[i]->ComputeDataPosition ();
            }
        }
		mbChangedByUser = true;
        OnChange();
    }

    return bChange;
}
void CGRoi::AdjustOtherPointsToDrag (int i, CGPoint *apPointsOnDR[])
{
    // Keep rectangular shape
    int iNext = (i + 1) % N_SEEN_POINTS;
    int iPrev = (i + N_SEEN_POINTS - 1) % N_SEEN_POINTS;
    if (i & 1)
    {
        if (!apPointsOnDR[iPrev]->mbActive)
            apPointsOnDR[iPrev]->y = apPointsOnDR[i]->y;
        if (!apPointsOnDR[iNext]->mbActive)
            apPointsOnDR[iNext]->x = apPointsOnDR[i]->x;
    }
    else
    {
        if (!apPointsOnDR[iPrev]->mbActive)
            apPointsOnDR[iPrev]->x = apPointsOnDR[i]->x;
        if (!apPointsOnDR[iNext]->mbActive)
            apPointsOnDR[iNext]->y = apPointsOnDR[i]->y;
    }
    if (!apPointsOnDR[iPrev]->mbActive)
        apPointsOnDR[iPrev]->ComputeDataPosition ();
    if (!apPointsOnDR[iNext]->mbActive)
        apPointsOnDR[iNext]->ComputeDataPosition ();

	apPointsOnDR[4]->Average(*apPointsOnDR[0], *apPointsOnDR[2]);
}
bool CGRoi::SetDataMask (class CTImage<bool> &mask)
{
    CMetaData *pData = mpDisplayGE->GetFirstData();
    int nCols = pData->GetNCols();
    int nLines = pData->GetNLinesInPage();
    if (!mask.SetSize (nLines, nCols))
        return false;
    return CDataRoi::SetDataMask(mask);
}
bool CGRoi::GetStatisticsString(CMetaData *pData, CString &sValue, class CGraphicElementOnDR *pGEOnDR)
{
	if ((pData != mpOriginalData) && pGEOnDR)
	{
		for (int i = 0; i < N_SEEN_POINTS; i++)
			pGEOnDR->AdjustDataCoordinates(pData, i, maDataPointsForStat[i]);
		mbUseAdjustedPointForMask = true;
	}
	else
		mbUseAdjustedPointForMask = false;

    CTImage<bool> mask("Roi");
    SetDataMask(mask);

    CMetaData::SStatistics stat;
    if (!pData->ComputeStatistics(mask, stat))
        return false;

	int area = stat.mn;
	if (umbShowMeasurementsInMm)
	{
	    float voxelSizeMm = pData->MmPerPixelWidth() * pData->MmPerPixelHeight();
		area = (int)(stat.mn * voxelSizeMm + 0.5f);
	}
    float avg = pData->Rescale(stat.mAverage);
	float std = pData->RescaleWidth(stat.mStandardDeviation);

    //if (pData->IsCtImage())
    //    avg += pData->GetCtBias();

    char zBuf[256];
    sprintf_s (zBuf, 256, "Ar %d Av %s std %s", 
		area, CMyWindows::FormatByScale(avg), (const char*)CMyWindows::FormatByScale(std));
    sValue = zBuf;
    return true;
}
