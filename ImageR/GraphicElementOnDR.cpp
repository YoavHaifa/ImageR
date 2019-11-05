#include "StdAfx.h"
#include "GraphicelEmentOnDR.h"
#include "DisplayRectangle.h"
#include "GPoint.h"
#include "MetaData.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\CsvFile.h"

bool CGraphicElementOnDR::umbCheckRange = false;

CGraphicElementOnDR::CGraphicElementOnDR(CGraphicElement *pGE, 
                                         CDisplayRectangle *pDispRect)
: CGraphicElementOnDRBase(pGE->msName + "OnDR")
, mpGE(pGE)
, mpDR(pDispRect)
, mbActive(false)
, mnSeenPoints(0)
, mapPointsOnDR(NULL)
, mnPointsOnDR(0)
, mbConnectPoints(pGE->mbConnectPoints)
, mbConnectPointsPairs(pGE->mbConnectPointsPairs)
, mbConnectLastAndFirst(pGE->mbConnectLastAndFirst)
, mbDisplayInactivePoints(pGE->mbDisplayInactivePoints)
, mbAllowPermanentSelection(false)
, mpZoomParams(pDispRect->GetZoomParams())
, mbDisabled(false)
{
	miDisp = pDispRect->mDRId;
    BuildPoints();

    AddCheck(pGE);
    AddCheck(pDispRect);
    mpDR->AddGraphicElement(this);
	mpGE->AddAppearance(this);
}

CGraphicElementOnDR::~CGraphicElementOnDR(void)
{
	if (!umDeleter.mbDoingGlobalDestruction)
		mpDR->RemoveGraphicElement(this);
    DeleteAllPoints();
	mpGE->RemoveAppearance(this);
}
void CGraphicElementOnDR::BuildPoints()
{
    mnSeenPoints = 0;
    POSITION pos = mpGE->mDataPoints.GetHeadPosition();
    while (pos)
    {
        CDataCoordinates *pDataCoo = mpGE->mDataPoints.GetNext(pos);
        CGPoint *pPoint = new CGPoint (pDataCoo, mpDR, mpZoomParams);
		if (umbCheckRange)
		{
			if (pPoint->y < 0 || pPoint->y > 10000)
			{
				static int unErr = 0;
				if (unErr++ < 1)
					CMyWindows::MessBox("<CGraphicElementOnDR::BuildPoints> Point out of range","Warning");
			}
		}
        pPoint->mColor = mpGE->mColor;
        mPoints.AddTail (pPoint);
        mnSeenPoints++;
    }

	//LogAllPoints("BuildPoints");
	mpGE->OnPointsUpdated(this);
	mnPointsOnDR = 0;
}
void CGraphicElementOnDR::DeleteAllPoints(void)
{
    while (!mPoints.IsEmpty())
    {
        CGPoint *pPoint = mPoints.GetTail();
        mPoints.RemoveTail();
        delete pPoint;
    }
	if (mapPointsOnDR)
	{
		delete mapPointsOnDR;
		mapPointsOnDR = NULL;
	}
}

CGPoint **CGraphicElementOnDR::GetPointsMap(void)
{
    int nPoints = (int)mPoints.GetSize();
    if (mnPointsOnDR < nPoints || !mapPointsOnDR)
    {
        if (mapPointsOnDR)
            delete mapPointsOnDR;
        mnPointsOnDR = (int)mPoints.GetSize();
		if (nPoints > 0)
			mapPointsOnDR = new CGPoint *[mnPointsOnDR];
		else
			mapPointsOnDR = NULL;
    }

    int i = 0;
    POSITION pos = mPoints.GetHeadPosition();
    while (pos)
        mapPointsOnDR[i++] = mPoints.GetNext(pos);
    mnPointsOnDR = nPoints;

    return mapPointsOnDR;
}
void CGraphicElementOnDR::SetPointsColor(COLORREF color)
{
    POSITION pos = mPoints.GetHeadPosition();
    while (pos)
    {
        CGPoint *pPoint = mPoints.GetNext(pos);
        pPoint->mColor = color;
    }
}
CGPoint *CGraphicElementOnDR::FindPointNear(CDRCoordinates &drCoo, int *pDistance)
{
    CGPoint *pNearestPoint = NULL;
    int bestDistance = INT_MAX;

    POSITION pos = mPoints.GetHeadPosition();
    while (pos)
    {
        CGPoint *pPoint = mPoints.GetNext(pos);
		if (pPoint->mbSelectable)
		{
			if (pPoint->IsNear(drCoo))
			{
				int dist = pPoint->DistanceFrom(drCoo);
				if (dist < bestDistance)
				{
					bestDistance = dist;
					pNearestPoint = pPoint;
				}
			}
		}
    }
    if (pDistance)
        *pDistance = bestDistance;
    return pNearestPoint;
}
bool CGraphicElementOnDR::Drag(CSize size)
{
	if (mpGE->mbDisableDrag)
		return false;

    GetPointsMap();
	if (mpGE->NewDrawOnDrag(mpDR))
	{
		mpGE->OnPointsUpdated(this);
		return true;
	}
    bool bChange = mpGE->Drag (*mpDR, size, mapPointsOnDR);
	if (bChange)
		mpGE->OnPointsUpdated(this);
	return bChange;
}
bool CGraphicElementOnDR::ComputePointsScreenPosition (void)
{
	GetPointsMap();

    bool bOnRect = false;
    CZoomPanParams *pZoomParams = mpDR->GetZoomParams();
	if (mpGE->mpfLog && (umDebug & 0x100))
	{
		fprintf(mpGE->mpfLog,"<ComputePointsScreenPosition>  zoom mm/pix %.2f shift (x %.2f, y %.2f)\n", 
			pZoomParams->MmPerPixel(), pZoomParams->XShiftMM(), pZoomParams->YShiftMM());
	}
    for (int i = 0; i < mnPointsOnDR; i++)
    {
        mapPointsOnDR[i]->ComputeScreenPosition(pZoomParams);
        bOnRect |= mapPointsOnDR[i]->mbOnRect;
		if (mpGE->mpfLog && (umDebug & 0x100))
		{
			fprintf(mpGE->mpfLog,"<ComputePointsScreenPosition>: (x %.2f, y %.2f) --> (x %d, y %d)\n", 
				mapPointsOnDR[i]->mpData->fx, mapPointsOnDR[i]->mpData->fy,
				mapPointsOnDR[i]->x, mapPointsOnDR[i]->y);
		}
    }
    return bOnRect;
}
void CGraphicElementOnDR::Draw (CDC &dc)
{
	if (mpGE->mpfLog)
	{
		fprintf(mpGE->mpfLog,"<Draw> on %s\n", (const char*)mpDR->Name());
		if (mpDR->GetCurrentData())
			fprintf(mpGE->mpfLog,"    Data %s\n", (const char*)mpDR->GetCurrentData()->Name());
		fflush(mpGE->mpfLog);
	}
	static int unDebug = 0;
	int nSeenPoints = 0;
	if (mpGE->mbDebug)
		unDebug++;
	mpGE->AutoUpdateForDraw();
	if (mpGE->DrawPointless(mpDR, dc))
	{
		UpdatePoints();
		ComputePointsScreenPosition();
	}
	else
	{
		if (!ComputePointsScreenPosition())
		{
			if (mpGE->mbDisplayOnlyIfPointsInDispRect)
				return;
		}

		nSeenPoints = mnSeenPoints ? mnSeenPoints : mnPointsOnDR;

		if (!mpGE->SpecialDraw(*mpDR, dc, mapPointsOnDR))
		{
			if (mbConnectPoints)
			{
				if (mbConnectPointsPairs)
				{
					for (int i = 0; i < nSeenPoints - 1; i += 2)
						mpDR->DrawLine (dc, *mapPointsOnDR[i], *mapPointsOnDR[i+1], mpGE->mColor);
				}
				else
				{
					for (int i = 0; i < nSeenPoints - 1; i++)
						mpDR->DrawLine (dc, *mapPointsOnDR[i], *mapPointsOnDR[i+1], mpGE->mColor);
				}
			}

			if (mbConnectLastAndFirst)
				mpDR->DrawLine (dc, *mapPointsOnDR[nSeenPoints - 1], *mapPointsOnDR[0], mpGE->mColor);
		}

		for (int i = 0; i < nSeenPoints; i++)
		{
			if (mapPointsOnDR[i]->mbActive || mbDisplayInactivePoints)
				mapPointsOnDR[i]->DrawCursor(*mpDR, dc);
		}
	}

	if (mpGE->mpfLog)
	{
		CMetaData *pData = mpDR->GetCurrentData();
		CDataCoordinates dataCoo (pData->CurPage());
		mapPointsOnDR[0]->UpdateData(pData->GetZoomParams(), dataCoo);
		fprintf(mpGE->mpfLog,"==> <Draw> point[0] (x %.2f, y %.2f)\n", dataCoo.fx, dataCoo.fy);
	}
	bool bImageText = mpGE->GetStatOrDataDependantText(mpDR->GetCurrentDataForWindowing(), msImageText, this);
	if (bImageText || !mpGE->msText.IsEmpty())
	{
		if (mpGE->mbActive || !mpGE->mbTextOnlyWhileActive)
		{
			int i = mpGE->miTextPoint;
			if (i < 0 || i > nSeenPoints)
				i = 0;
			int textLeft = mpDR->left + mapPointsOnDR[i]->x;
			int textTop = mpDR->top + mapPointsOnDR[i]->y - 20;
			CString sText(mpGE->msText);
			if (bImageText)
			{
				if (!sText.IsEmpty())
					sText += " ";
				sText += msImageText;
			}
			int textWidth = sText.GetLength() * 20;;
			int textHeight = 20;
			CRect rect (textLeft, textTop, textLeft + textWidth, textTop + textHeight);
			dc.DrawText(sText, rect, 0);
		}
	}
	if (mpGE->mpfLog)
	{
		fprintf(mpGE->mpfLog,"<Draw> END\n\n");
		fflush(mpGE->mpfLog);
	}
}
bool CGraphicElementOnDR::GetStatOrDataDependantText(CString &s)
{
	bool bSave = mpGE->mbDisplayStatisticsAsText;
	mpGE->mbDisplayStatisticsAsText = true;
	CMetaData *pData = mpDR->GetCurrentDataForWindowing();
	bool bTextReady = mpGE->GetStatOrDataDependantText(pData, s, this);
	mpGE->mbDisplayStatisticsAsText = bSave;
	return bTextReady;
}
float CGraphicElementOnDR::GetDataValue(CMetaData *pData, int iPoint)
{
	CDataCoordinates dataCoo (pData->CurPage());
	mapPointsOnDR[iPoint]->UpdateData(pData->GetZoomParams(), dataCoo);
    CMyImage *pPage = pData->PageData();
	return pPage->GetFloatValue((int)(dataCoo.fy+0.5f), (int)(dataCoo.fx+0.5f));
}
void CGraphicElementOnDR::AdjustDataCoordinates(CMetaData *pData, int iPoint, CDataCoordinates &oCoo)
{
	CDataCoordinates dataCoo (pData->CurPage());
	mapPointsOnDR[iPoint]->UpdateData(pData->GetZoomParams(), dataCoo);
	oCoo = dataCoo;
}
bool CGraphicElementOnDR::OnLButtonDown(UINT nFlags, CDRCoordinates &drCoo)
{
	GetPointsMap();
    CGPoint *pPoint = FindPointNear(drCoo);
    if (!pPoint)
        return false;
    mbActive = true;
    mpGE->OnSetActive();

    if (pPoint->mbActivateAll)
    {
        if (pPoint->IsNear(drCoo))
        {
            for (int i = 0; i < mnSeenPoints; i++)
                mapPointsOnDR[i]->mbActive = true;
            mbAllActive = true;
            return true;
        }
        return false;
    }
	bool bActivated = pPoint->OnLButtonDown (nFlags, drCoo);
	return bActivated || pPoint->mbActive;
}
bool CGraphicElementOnDR::OnLButtonUp(UINT nFlags, CDRCoordinates &drCoo)
{
	GetPointsMap();
	bool bChanged = false;
    if (mbActive)
    {
        mbActive = false;
        bChanged = true;
        mpGE->OnResetActive();
    }
    for (int i = 0; i < mnPointsOnDR; i++)
    {
		if (mapPointsOnDR[i]->mbSelectable)
	        bChanged |= mapPointsOnDR[i]->OnLButtonUp (nFlags, drCoo);
    }
    mbAllActive = false;
    return bChanged;
}
bool CGraphicElementOnDR::OnRButtonDblClk(UINT nFlags, CDRCoordinates &drCoo)
{
    if (!mbAllowPermanentSelection)
        return false;

    CGPoint *pPoint = FindPointNear(drCoo);
    if (!pPoint)
        return false;
    return pPoint->OnRButtonDblClk (nFlags, drCoo);
}

bool CGraphicElementOnDR::OnKillFocus(void)
{
    if (mbActive || mbAllActive)
    {
        mbActive = false;
        mbAllActive = false;
        for (int i = 0; i < mnPointsOnDR; i++)
        {
            mapPointsOnDR[i]->mbActive = false;
            mapPointsOnDR[i]->mbSelectedActive = false;
        }
        return true;
    }
    return false;
}
bool CGraphicElementOnDR::OnDataUpdate(void)
{
	bool bChanged = mpGE->OnDataUpdate();
	if (bChanged)
		UpdatePoints();
	return bChanged;
}
void CGraphicElementOnDR::UpdatePoints(void)
{
    DeleteAllPoints();
    BuildPoints();
}

bool CGraphicElementOnDR::IsRelevant(CMetaData *pData)
{
	if (mbDisabled)
		return false;
	if (!mpGE->IsValid())
		return false;

    if (mpGE->mbLockedToPosition)
	{
		if (mpGE->miPosition2d >= 0)
		{
			CPosition *pSecondDataPos = pData->GetPosition(1);
			if (pSecondDataPos && (mpGE->miPosition2d != pSecondDataPos->miCurrent))
				return false;
		}

		CPosition *pDataPos = pData->GetPosition();
        return pDataPos->miCurrent == mpGE->miPosition;
	}

    return true;
}
bool CGraphicElementOnDR::Disable(void)
{
	if (mbDisabled)
		return false;
	mbDisabled = true;
	return true;
}
bool CGraphicElementOnDR::Enable(void)
{
	if (!mbDisabled)
		return false;
	mbDisabled = false;
	return true;
}
bool CGraphicElementOnDR::FixCenter(void)
{
	return mpGE->FixCenter();
}
void CGraphicElementOnDR::SaveState(CXMLDump & xmlDump)
{
    CXMLDumpScope graph("graphic_on_dr",xmlDump);
    mpGE->SaveState(xmlDump);
	if (mbDisabled)
		xmlDump.Write("disabled", mbDisabled);
}
void CGraphicElementOnDR::Print(FILE *pf)
{
	fprintf (pf, "<CGraphicElementOnDR::Print> %s nPoints %d\n", (const char*)msName, mnPointsOnDR);
    for (int i = 0; i < mnPointsOnDR; i++)
    {
        mapPointsOnDR[i]->Print(pf);
    }
	fprintf (pf, "\n");
}
void CGraphicElementOnDR::FixPoint(int i, float x, float y)
{
	if (!mapPointsOnDR)
		GetPointsMap();
	if (i >= mnPointsOnDR)
		return;
	mapPointsOnDR[i]->mpData->Set(x,y);
	mapPointsOnDR[i]->mbSelectable = false;
	mapPointsOnDR[i]->mbActive = false;
}
void CGraphicElementOnDR::FreePoint(int i)
{
	if (!mapPointsOnDR)
		GetPointsMap();
	if (i < 0 || i >= mnPointsOnDR)
		return;
	mapPointsOnDR[i]->mbSelectable = true;
}
void CGraphicElementOnDR::FreeAndActivatePoint(int i)
{
	if (!mapPointsOnDR)
		GetPointsMap();
	if (i < 0 || i >= mnPointsOnDR)
		return;
	mapPointsOnDR[i]->mbSelectable = true;
	mapPointsOnDR[i]->mbActive = true;
	mapPointsOnDR[i]->mbSelectedActive = true;
}
void CGraphicElementOnDR::FixAllPoints(void)
{
	//LogAllPoints("FixAllPoints_Start");

	if (!mapPointsOnDR)
		GetPointsMap();
	for (int i = 0; i < mnPointsOnDR; i++)
	{
		mapPointsOnDR[i]->mbSelectable = false;
		mapPointsOnDR[i]->mbActive = false;
		mapPointsOnDR[i]->mbSelectedActive = false;
	}
	//LogAllPoints("FixAllPoints_End");
}
void CGraphicElementOnDR::LogAllPoints(const char *zText)
{
	static int count = 0;

	CLogFile log;
	char zBuf[128];
	sprintf_s(zBuf,sizeof(zBuf),"Log_%02d_%s", ++count, zText);
	log.Open("CGraphicElementOnDR", zBuf);
	if (!log)
		return;
	if (!mapPointsOnDR)
		GetPointsMap();

	fprintf(log.mpf,"mnPointsOnDR %d\n\n", mnPointsOnDR);

	for (int i = 0; i < mnPointsOnDR; i++)
	{
		fprintf(log.mpf,"%3d: %p\n              ", i, mapPointsOnDR[i]);
		fflush(log.mpf);
		mapPointsOnDR[i]->PrintMore(log.mpf);
		fflush(log.mpf);
	}
}
CMetaDataBase *CGraphicElementOnDR::GetData()
{
	return mpDR->GetCurrentDataForWindowing();
}
void CGraphicElementOnDR::PrintExcelHeader(FILE *pf)
{
	CMetaData *pData = mpDR->GetCurrentDataForWindowing();
	if (pData)
		pData->PrintExcelHeader(pf);
	else
		fprintf(pf,"None\t");
}
void CGraphicElementOnDR::PrintCsvHeader(CCsvFile &csv)
{
	CMetaData *pData = mpDR->GetCurrentDataForWindowing();
	if (pData)
		pData->PrintCsvHeader(csv);
	else
		csv.PrintCell("None");
}
void CGraphicElementOnDR::PrintValue2Excel(FILE *pf, int iLine, int iCol)
{
	CMetaData *pData = mpDR->GetCurrentDataForWindowing();
	if (pData)
		pData->PrintValue2Excel(pf, iLine, iCol);
	else
		fprintf(pf,"???\t");
}
void CGraphicElementOnDR::PrintValue2Csv(CCsvFile &csv, int iLine, int iCol)
{
	CMetaData *pData = mpDR->GetCurrentDataForWindowing();
	if (pData)
		pData->PrintValue2Csv(csv, iLine, iCol);
	else
		csv.PrintCell("???");
}
void CGraphicElementOnDR::SaveStateForGE(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("graphic_on_dr",xmlDump);

	CMetaData *pData = mpDR->GetCurrentDataForWindowing();
	if (pData)
	{
		xmlDump.Write("disp", mpDR->mDRId);
		xmlDump.Write("data", pData->Name());
		float value = GetDataValue(pData, 0);
		xmlDump.Write("value", value);
		float dispValue = pData->Rescale(value);
		xmlDump.Write("dvalue", dispValue);
		if (!msImageText.IsEmpty())
			xmlDump.Write("text", msImageText);
	}
}
