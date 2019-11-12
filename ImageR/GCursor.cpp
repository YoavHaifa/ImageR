#include "StdAfx.h"
#include "GCursor.h"
#include "DisplayRectangle.h"
#include "MetaData.h"
#include "GraphicelEmentOnDR.h"

CGCursor::CGCursor(CMetaData *pData, CDisplayRectangle *pDispRect, const char *zName, COLORREF color)
: CDataCursor (pDispRect->GetPosition(), zName, color)
, mLen (10)
{
	mbDisplayInactivePoints = true;
    CDataCoordinates dataCoo (pDispRect->GetSelectedDataCoo());
    mDataPoints.AddTail(new CDataCoordinates(*pData->PageData(), dataCoo.fx, dataCoo.fy));
	if (mpfLog)
	{
		fprintf(mpfLog,"<CGCursor> (x %.2f, y %.2f)\n\n", dataCoo.fx, dataCoo.fy);
		fflush(mpfLog);
	}
}
CGCursor::CGCursor(class CDisplayRectangle *pDispRect, class CXMLParseNode *pCursorXml)
: CDataCursor(pDispRect->GetCurrentData()->PageData(), pCursorXml)
, mLen (10)
{
	mbDisplayInactivePoints = true;
}
CGCursor::CGCursor(const char *zName)
: CDataCursor(zName)
, mLen (10)
{
	mbDisplayInactivePoints = true;
}
CGCursor::~CGCursor(void)
{
    if (msName == "MipRef2Im")
    {
        MessageBox(NULL, "Who the hell is destroying my cursor?", "Error", MB_OK);
    }
}
bool CGCursor::SpecialDraw (CDisplayRectangle &displayRect, CDC &dc, CGPoint *apPointsOnDR[])
{
	if (!IsActive() && !mbDisplayInactivePoints)
		return false;

    if (mbValid && apPointsOnDR[0]->mbOnRect)
    {
        apPointsOnDR[0]->mColor = mColor;
        apPointsOnDR[0]->DrawCursor(displayRect, dc);
    }
    return true;
}
bool CGCursor::Drag(CRect rect, CSize size, CGPoint *apPointsOnDR[])
{
	if (!apPointsOnDR || !apPointsOnDR[0])
		return false;

    CSize rectSize (rect.right - rect.left, rect.bottom - rect.top);
    CRect rectClip (0,0,rectSize.cx, rectSize.cy);
    bool bChange = false;
    if (apPointsOnDR[0]->mbActive)
        bChange = apPointsOnDR[0]->ClipDrag (rectClip, size);

    if (bChange)
    {
        *apPointsOnDR[0] += size;
        apPointsOnDR[0]->ComputeDataPosition ();
		mbChangedByUser = true;
        OnChange();
    }
    return bChange;
}
bool CGCursor::GetStatisticsString(CMetaData *pData, CString &sValue, class CGraphicElementOnDR *pGEOnDR)
{
    CMyImage *pPage = pData->PageData();
    if (!pPage)
        return false;

    CDataCoordinates *pDataCoo = mDataPoints.GetHead();
    float value = pPage->GetFloatValue((int)(pDataCoo->fy+0.5f), (int)(pDataCoo->fx+0.5f));

	if ((pData != mpOriginalData) && pGEOnDR)
	{
		float save = value;
		value = pGEOnDR->GetDataValue(pData, 0);
		if (mpfLog)
			fprintf(mpfLog,"DISPLAY ON NON-OROGINAL DATA %s, value %f --> %f\n\n", (const char*)pData->Name(), save, value);
	}

	value = pData->Rescale(value);
    //if (pData->IsCtImage())
    //    value += pData->GetCtBias();
    int iValue = (int)value;
 
    char zBuf[256];
    if ((float)iValue == value)
        sprintf_s (zBuf, 256, "%s %d", (const char *)msName, iValue);
    else if (value <= -1.0f || value >= 1.0f)
        sprintf_s (zBuf, 256, "%s %.3f", (const char *)msName, value);
    else
        sprintf_s (zBuf, 256, "%s %.6f", (const char *)msName, value);

    sValue = zBuf;
	if (mpfLog)
	{
		fprintf(mpfLog,"<GetStatisticsString> (fx %.2f fy %.2f) %s\n", pDataCoo->fx, pDataCoo->fy, (const char*)sValue);
		fflush(mpfLog);
	}
    return true;
}
