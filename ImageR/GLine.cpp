#include "StdAfx.h"
#include "GLine.h"
#include "GPoint.h"
#include "DisplayRectangle.h"
#include "MetaData.h"
#include "GraphicElementOnDR.h"
#include "DisplayGE.h"
#include "resource.h"
#include "..\..\yUtils\LogFile.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\MyStatistics.h"

bool CGLine::umbCreatOnly2Points = false;

CGLine::CGLine(CDisplayRectangle *pDispRect, const char *zName, 
        COLORREF color, int deltaMm)
: CDataLine (pDispRect->GetPosition(), zName, color)
, mbShowSlope(false)
, mpUpper(NULL)
, mpLower(NULL)
, mbExtra(false)
, mbTieOrigins(false)
, mbEnableDistanceDisplay(true)
{
    CMetaData *pData = pDispRect->GetCurrentData();
    CDataCoordinates dataCoo (pDispRect->GetSelectedDataCoo());

    int delta = deltaMm / 2;
    CMyImage *pPage = pData->PageData();
    mDataPoints.AddTail( new CDataCoordinates(*pPage, dataCoo.fx-delta, dataCoo.fy+delta));
    mDataPoints.AddTail( new CDataCoordinates(*pPage, dataCoo.fx+delta, dataCoo.fy-delta));
	if (umbCreatOnly2Points)
		umbCreatOnly2Points = false;
	else
	    mDataPoints.AddTail( new CDataCoordinates(*pPage, dataCoo.fx, dataCoo.fy));
}
CGLine::CGLine(class CDisplayRectangle *pDispRect, class CXMLParseNode *pGLineXml)
: CDataLine(pDispRect->GetCurrentData()->PageData(), pGLineXml)
, mbShowSlope(false)
, mpUpper(NULL)
, mpLower(NULL)
, mbExtra(false)
, mbTieOrigins(false)
, mbEnableDistanceDisplay(true)
{
}
CGLine::~CGLine(void)
{
	if (mpLower)
		mpLower->mpUpper = mpUpper;
	if (mpUpper)
		mpUpper->mpLower = mpLower;
}
void CGLine::SetAbove(CGLine *pLower)
{
	mpLower = pLower;
	if (pLower)
		pLower->mpUpper = this;
}
void CGLine::OnNewDisplay(CGraphicElementOnDR * pGEonDR)
{
	if (!msBelow.IsEmpty())
	{
		CDisplayRectangle *pDR = pGEonDR->GetDR();
		CGraphicElementOnDR *pOtherOnDR = pDR->FindGraphic(msBelow);
		CGraphicElement *pOther = pOtherOnDR->GetGE();
		if (pOther && (pOther->GetClass() == "CGLine"))
		{
			CGLine *pUpper = (CGLine *)pOther;
			pUpper->SetAbove(this);
		}
	}

    pGEonDR->mbAllowPermanentSelection = false;
    pGEonDR->mbDisplayInactivePoints = false;
    pGEonDR->mnSeenPoints = N_SEEN_POINTS;

    CGPoint **pPoints = pGEonDR->GetPointsMap();
    if (pPoints)
	{
		if (mDataPoints.GetSize() > N_SEEN_POINTS)
	        pPoints[N_SEEN_POINTS]->mbActivateAll = true;
	}
	if (mbExtra)
	{
		pGEonDR->mbAllowPermanentSelection = true;
		pGEonDR->mbDisplayInactivePoints = true;
	}
}
//static FILE *upfLog = NULL;

bool CGLine::Drag(CRect rect, CSize size, CGPoint *apPointsOnDR[])
{
	static int count = 0;
	//CLogFile logFile("GLine","Drag",count++ > 0);
	//upfLog = logFile.mpf;
	/*if (logFile)
        if ((apPointsOnDR[0]->mbActive) || (apPointsOnDR[1]->mbActive))
	{
		fprintf(logFile.mpf, "<CGLine::Drag> %s Start [x %d y %d] - [x %d y %d]\n", msName,
						apPointsOnDR[0]->x,apPointsOnDR[0]->y, 
						apPointsOnDR[1]->x,apPointsOnDR[1]->y
						);
		fprintf(logFile.mpf, "Size %d %d\n", size.cx, size.cy);
	} */

	const int MIN_ASCEND = 3;
    //bool bAllActive = apPointsOnDR[0]->mbActive && apPointsOnDR[1]->mbActive;

    CSize rectSize (rect.right - rect.left, rect.bottom - rect.top);
    CRect rectClip (0,0,rectSize.cx, rectSize.cy);
    bool bChange = false;

    for (int i = 0; i < N_SEEN_POINTS; i++)
    {
        if (apPointsOnDR[i]->mbActive)
        {
            CRect rClip(rectClip);
			/* if (logFile)
				fprintf(logFile.mpf, "rClip init x [%d - %d], y [%d - %d]\n",
					rClip.left, rClip.right, rClip.top, rClip.bottom); */
			if (mbAscending)
			{
				if (i == 0)
				{
					if (apPointsOnDR[0]->mbActive && !apPointsOnDR[1]->mbActive)
					{
						CGPoint *pLow = apPointsOnDR[0];
						CGPoint *pHigh = apPointsOnDR[1];
						if (pLow && pHigh)
							rClip.top = apPointsOnDR[1]->y + MIN_ASCEND;
						rClip.right = apPointsOnDR[1]->x - MIN_ASCEND;
					}
				}
				if (i == 1)
				{
					if (apPointsOnDR[1]->mbActive && !apPointsOnDR[0]->mbActive)
					{
						rClip.bottom = apPointsOnDR[0]->y - MIN_ASCEND;
						rClip.left = apPointsOnDR[0]->x + MIN_ASCEND;
					}
				}
				/* if (logFile)
					fprintf(logFile.mpf, "rClip bAsc x [%d - %d], y [%d - %d]\n",
						rClip.left, rClip.right, rClip.top, rClip.bottom); */
			}
            bChange = apPointsOnDR[i]->ClipDrag (rClip, size);
            if (!bChange)
                break;
        }
    }

    if (bChange)
    {
 		/* if (logFile)	
			fprintf(logFile.mpf, "%s Size %d %d\n", msName, size.cx, size.cy); */
       for (int i = 0; i < N_SEEN_POINTS; i++)
        {
            if (apPointsOnDR[i]->mbActive)
            {
				/* if (logFile)
					fprintf(logFile.mpf, "Point[%d] Start [x %d y %d]\n", i, 
									apPointsOnDR[i]->x,apPointsOnDR[i]->y); */
                *apPointsOnDR[i] += size;
				/* if (logFile)
					fprintf(logFile.mpf, "Point[%d] Moved [x %d y %d]\n", i, 
									apPointsOnDR[i]->x,apPointsOnDR[i]->y); */
				CGLine *pLineToTie = NULL;
				if (mpUpper || mpLower)
				{
					if (i == 0 && mbTieOrigins)
					{
						if (mpUpper)
							pLineToTie = mpUpper;
						else if (mpLower)
							pLineToTie = mpLower;
					}
					else
					{
						float x = (float)apPointsOnDR[i]->x;
						float y = (float)apPointsOnDR[i]->y;
						float yUp = y;
						float yLow = y;
						if (mpUpper)
							yUp = mpUpper->GetYOfXScreen(x) + 2;
						if (mpLower)
							yLow = mpLower->GetYOfXScreen(x) - 2;
						if (mpUpper && mpLower && (yUp > yLow))
						{
							float avg = (yUp + yLow) / 2;
							/* if (logFile)
								fprintf(logFile.mpf, "Avg %f & %f --> %f\n", 
												yUp, yLow, avg); */
							yUp = avg;
							yLow = avg;
						}
						if (y < yUp)
						{
							apPointsOnDR[i]->y = (LONG)yUp;
							/* if (logFile)
								fprintf(logFile.mpf, "Point[%d] Upped [x %d y %d] (yUp %f)\n", i, 
												apPointsOnDR[i]->x,apPointsOnDR[i]->y, yUp); */
						}
						if (y > yLow)
						{
							apPointsOnDR[i]->y = (LONG)yLow;
							/* if (logFile)
								fprintf(logFile.mpf, "Point[%d] Lowed [x %d y %d]\n", i, 
												apPointsOnDR[i]->x,apPointsOnDR[i]->y); */
						}
					}

				}
                apPointsOnDR[i]->ComputeDataPosition ();
				if (pLineToTie)
				{
					CDataCoordinates *pCoo = pLineToTie->GetFirstDataCoo();
					*pCoo = *apPointsOnDR[i]->mpData;
					pLineToTie->OnUserChange();
				}
			}
		}
		if (mDataPoints.GetSize() > N_SEEN_POINTS)
			apPointsOnDR[N_SEEN_POINTS]->Average(*apPointsOnDR[0], *apPointsOnDR[1]);
		OnUserChange();
		/* if (logFile.mpf)
			fprintf(logFile.mpf, "<CGLine::Drag> %s Changed [x %d y %d] - [x %d y %d]\n", msName,
							apPointsOnDR[0]->x,apPointsOnDR[0]->y, 
							apPointsOnDR[1]->x,apPointsOnDR[1]->y
							); */
    }

	/* if (logFile.mpf)
		fprintf(logFile.mpf, "---\n"); */
	//upfLog = NULL;
    return bChange;
}
void CGLine::OnUserChange()
{
	mbChangedByUser = true;
    OnChange();
}
float CGLine::GetYOfXScreen(float x)
{
	CGraphicElementOnDR * pGeOnDR = CDisplayGE::GetGEOnDR(this);
	CGPoint **apPointsOnDR = pGeOnDR->GetPointsMap();

	float dx = (float)(apPointsOnDR[1]->x - apPointsOnDR[0]->x);
	if (dx == 0.0f)
		return 0.0f;

	float dy = (float)(apPointsOnDR[1]->y - apPointsOnDR[0]->y);
	float pointXRelative = (x - apPointsOnDR[0]->x) / dx;

	float linyYAtPoint = apPointsOnDR[0]->y + dy * pointXRelative;
	/* if (upfLog)
		fprintf(upfLog, "<GetYOfXScreen> Line [x %d y %d] - [x %d y %d]\n",
						apPointsOnDR[0]->x,apPointsOnDR[0]->y,
						apPointsOnDR[1]->x,apPointsOnDR[1]->y);
		fprintf(upfLog, "<GetYOfXScreen> x %f dx %f dy %f xRel %f yAt %f\n",
						x, dx, dy, pointXRelative, linyYAtPoint); */
	return linyYAtPoint;
}
bool CGLine::GetStatisticsString(CMetaData *pData, CString &sValue, class CGraphicElementOnDR *pGEOnDR)
{
    if (!mbEnableDistanceDisplay)
        return false;

    CMyImage *pPage = pData->PageData();
    if (!pPage)
        return false;

    CDataCoordinates headCoo = *mDataPoints.GetHead();
	CDataCoordinates tailCoo = *GetSecondDataCoo();

	if ((pData != mpOriginalData) && pGEOnDR)
	{
		pGEOnDR->AdjustDataCoordinates(pData,0,headCoo);
		pGEOnDR->AdjustDataCoordinates(pData,1,tailCoo);
	}

	float dist = headCoo.DistanceFrom(tailCoo);
	if (umbShowMeasurementsInMm)
	{
		float dx = headCoo.fx - tailCoo.fx;
		float dy = headCoo.fy - tailCoo.fy;
		dx *= pData->MmPerPixelWidth();
		dy *= pData->MmPerPixelHeight();
		dist = sqrt(dx * dx + dy * dy);
	}

	char zBuf[256];
	sprintf_s(zBuf, "Dist %.2f %s", dist, umbShowMeasurementsInMm ? "mm" : "pix");

    sValue = zBuf;
    return true;
}
bool CGLine::GetDataDependantText(class CMetaData * /*pData*/, CString &s)
{
	if (!mbShowSlope)
		return false;

	float slope = GetSlope();

	s = msText;
	char zBuf[128];
	sprintf_s (zBuf, 128, " %.2f", slope);
	s += zBuf;
	return true;
}
void CGLine::SaveState(CXMLDump & xmlDump)
{
	CXMLDumpScope scope("GLine",xmlDump);
	CDataLine::SaveState(xmlDump);
	if (mbTieOrigins)
		xmlDump.Write("b_tie_origins",mbTieOrigins);
	if (mbExtra)
		xmlDump.Write("b_extra",mbExtra);
	if (mbShowSlope)
		xmlDump.Write("b_show_slope",mbShowSlope);
	if (mpUpper)
		xmlDump.Write("upper",mpUpper->Name());
	if (mpLower)
		xmlDump.Write("lower",mpLower->Name());
}
bool CGLine::EditContextMenu(CMenu *pMenu)
{
	pMenu->AppendMenu(MF_STRING, ID_PRINT_INFO, "Print Info");
	return true;
}
void CGLine::PrintInfo(CMetaData *pData)
{
	CString sfName;
	{
		CLogFile fLog(Name());
		if (!fLog)
			return;

		sfName = (CString)fLog.mName;
		CFileName fNameExcel(sfName);
		fNameExcel.ChangeType("xls");
		CLogFile logXls(fNameExcel);

		CDataCoordinates *pHeadCoo = mDataPoints.GetHead();
		CDataCoordinates *pTailCoo = GetSecondDataCoo();
		fprintf(fLog.mpf, "Start [x %d y %d]\n", (int)(pHeadCoo->fx + 0.5f), (int)(pHeadCoo->fy + 0.5f));
		fprintf(fLog.mpf, "End   [x %d y %d]\n", (int)(pTailCoo->fx + 0.5f), (int)(pTailCoo->fy + 0.5f));
		fprintf(fLog.mpf, "\n");

		CString sStat;
		mbDisplayStatisticsOnStatusBar = true;
		if (GetStatisticsString(pData, sStat, NULL))
			fprintf(fLog.mpf, "%s\n\n", (const char*)sStat);

		int nPoints = 0;
		float dx = pTailCoo->fx - pHeadCoo->fx;
		float dy = pTailCoo->fy - pHeadCoo->fy;
		if (abs(dx) > abs(dy))
			nPoints = (int)(abs(dx) + 0.5f) + 1;
		else
			nPoints = (int)(abs(dy) + 0.5f) + 1;
		int nDeltas = nPoints - 1;
		if (nDeltas > 1)
		{
			dx = dx / nDeltas;
			dy = dy / nDeltas;
		}
		CMyStatistics stat(nPoints);

		CDataCoordinates coo(*pHeadCoo);
		coo.mbOnRaster = true;
		CDataCoordinates cooMax(*pHeadCoo);
		CDataCoordinates cooMin(*pHeadCoo);
		float minVal = 0; 
		float maxVal = 0;
		for (int i = 0; i < nPoints; i++)
		{
			float value = pData->GetFloatValue(coo);
			stat.AddValue(value);
			if (i == 0)
			{
				minVal = value;
				maxVal = value;
			}
			else if (value > maxVal)
			{
				maxVal = value;
				cooMax = coo;
			}
			else if (value < minVal)
			{
				minVal = value;
				cooMin = coo;
			}

			//fprintf(fLog.mpf, "Coo [x %f y %f] value %f\n", coo.fx, coo.fy, value);
			coo.fx += dx;
			coo.fy += dy;
			if (logXls)
				fprintf(logXls.mpf,"%.3f\n",value);
		}
		stat.ComputeSDev();

		if (nPoints > 0)
		{
			fprintf(fLog.mpf, "Compute statistics on %d points\n", stat.mn);
			fprintf(fLog.mpf, "Average %f\n", stat.mAverage);
			fprintf(fLog.mpf, "Range %f - %f\n", stat.mMin, stat.mMax);
			fprintf(fLog.mpf, "SDev %f\n", stat.mStandardDeviation);
			fprintf(fLog.mpf, "\n");
			fprintf(fLog.mpf, "Max Vaue %f at [x %.1f - y %.1f]\n", maxVal, cooMax.fx, cooMax.fy);
			fprintf(fLog.mpf, "Min Vaue %f at [x %.1f - y %.1f]\n", minVal, cooMin.fx, cooMin.fy);
		}

		if (logXls)
			fprintf(fLog.mpf,"\nFor Displaying Value's Grap Open: %s\n", (const char*)logXls.Name());
	}
	CMyWindows::CreateProcess("NotePad",sfName);
}

