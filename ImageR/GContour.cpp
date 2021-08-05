#include "StdAfx.h"
#include "GContour.h"
#include "DisplayRectangle.h"
#include "MetaData.h"
#include "GPoint.h"
#include "SharedMemFile.h"
#include "..\..\ImageRLib\Rebm.h"
#include "..\..\ImageRLib\DataVolume.h"
#include "..\..\yUtils\XMLDump.h"

// Drawing edge of a REBM on zoomed images
// ---------------------------------------
// 1) Create bitmap of all the pixels that are on the REBM's edge
//    Check only 4 neighbors for each pixel.
//    If it is on, it has at least one neighbor that is off
// 2) Create Full Bitmap
// 3) Draw edge lines on zoomed raster
//    For each pixel on the edge, check 4 out of 8 neighbors
//    If a neighbor has a common outsider neighbor, draw the line...
//    Only the 4 (closest) outsider neighbors should be checked 

CGContour::CGContour(class CDisplayRectangle *pDispRect, const char *zName, COLORREF color)
: CGraphicElement(pDispRect->GetCurrentData()->GetPosition(), zName, color)
, mpRebm(NULL)
, mpEdges(NULL)
, mpFull(NULL)
, mpDataVolume(NULL)
, mpSharedMemFile(NULL)
{
	mbContour = true;
	CMetaData *pData = pDispRect->GetCurrentData();
	if (!pData)
		return;

	mpRebm = new CRebm(pData->GetNLinesInPage(), pData->GetNCols());
	InitEdgesRaster(pDispRect);
	mpEdges->AddCheck(mpRebm);
	mpFull->AddCheck(mpRebm);

    mSeed = pDispRect->GetSelectedDataCoo();
	ComputeRebmFromSeed(pDispRect);
	SetContextMenu(0);
}
CGContour::CGContour(class CDisplayRectangle *pDispRect, CDataVolume *pVolume, COLORREF color)
: CGraphicElement(pDispRect->GetCurrentData()->GetPosition(), pVolume->Name() + "Contour", color)
, mpRebm(NULL)
, mpEdges(NULL)
, mpFull(NULL)
, mpDataVolume(pVolume)
, mpSharedMemFile(NULL)
{
	mbContour = true;
	InitEdgesRaster(pDispRect);
	CPosition *pPos = pDispRect->GetPosition();
	mpEdges->AddCheck(pPos);
	mpFull->AddCheck(pPos);
	SetContextMenu(0);
}
CGContour::CGContour(CDisplayRectangle *pDispRect, CXMLParseNode *pGRoiXml)
: CGraphicElement(pDispRect->GetCurrentData()->PageData(), pGRoiXml)
, mpRebm(NULL)
, mpEdges(NULL)
, mpFull(NULL)
, mpDataVolume(NULL)
, mpSharedMemFile(NULL)
{
	mbContour = true;
	SetContextMenu(0);
}
CGContour::CGContour(CDisplayRectangle *pDispRect, CSharedMemFile *pSharedMemFile)
: CGraphicElement(pDispRect->GetCurrentData()->GetPosition(), pSharedMemFile->GetNameFromHeader())
, mpRebm(NULL)
, mpEdges(NULL)
, mpFull(NULL)
, mpDataVolume(NULL)
, mpSharedMemFile(pSharedMemFile)
{
	mbContour = true;
	InitEdgesRaster(pDispRect);
	mpRebm = new CRebm (mpSharedMemFile->GetNLinesInPage(), mpSharedMemFile->GetNCols());
	mpRebm->AddCheck(mpSharedMemFile);
	mpEdges->AddCheck(mpRebm);
	mpFull->AddCheck(mpRebm);
	mbDisableDrag = true;
	CString sCommand (mpSharedMemFile->GetCommand());
	if (sCommand == "all")
		mbDisplayOnAll = true;
	SetContextMenu(0);

	if (!pSharedMemFile->GetColor(mColor))
		mColor = 0xff00ff;
}
CGContour::~CGContour(void)
{
	if (mpRebm && !mpDataVolume)
		delete mpRebm;
	if (mpEdges)
		delete mpEdges;
	if (mpFull)
		delete mpFull;
	DeleteEdges();

	if (!umDeleter.mbDoingGlobalDestruction)
	{
		if (mpSharedMemFile)
			delete mpSharedMemFile;
	}
}
void CGContour::InitEdgesRaster(class CDisplayRectangle *pDispRect)
{
	CMetaData *pData = pDispRect->GetCurrentData();
	if (!pData)
		return;

	mpEdges = new CTImage<bool>("RebmEdges", pData->GetNLinesInPage(), pData->GetNCols());
	mpFull = new CTImage<bool>("RebmFull", pData->GetNLinesInPage(), pData->GetNCols());
}
void CGContour::DeleteEdges(void)
{
	while (!mEdgesToDraw.IsEmpty())
	{
		delete mEdgesToDraw.GetTail();
		mEdgesToDraw.RemoveTail();
	}
}
bool CGContour::ComputeEdges(void)
{
	if (!mpRebm || !mpEdges || !mpFull)
		return false;

	if (mpSharedMemFile && mpRebm->ShouldUpdate())
		UpdateRebmFromShared();

	if (mpEdges->ShouldUpdate())
	{
		mpRebm->SetFull(mpFull);
		mpRebm->SetEdges(mpEdges);
	}

	return true;
}
void CGContour::AddEdgeToDraw(int line1, int pix1, int line2, int pix2)
{
	CGContourEdge *pEdge = new CGContourEdge(line1, pix1, line2, pix2);
	mEdgesToDraw.AddTail(pEdge);
}
void CGContour::AddGraphPoints(CDisplayRectangle *pDispRect)
{
	DeleteDataPoints();
	CRect rect;
	if (!mpRebm->GetRect(rect))
		return;

	CMetaData *pData = pDispRect->GetCurrentData();
	CMyImage *pPage = pData->PageData();
	for (int iLine = rect.top; iLine <= rect.bottom; iLine++)
	{
		bool *pLine = mpEdges->GetLine(iLine);
		for (int iPix = rect.left; iPix <= rect.right; iPix++)
		{
			if (pLine[iPix])
				mDataPoints.AddTail( new CDataCoordinates(*pPage, (float)iPix, (float)iLine));
		}
	}
}
bool CGContour::ComputeEdgesToDraw(void)
{
	DeleteEdges();

	CRect rect;
	if (!mpRebm->GetRect(rect))
		return true; // No edges in this case is the real state of the REBM

	// Each pixel may add lines to 4 of its neighbors - from up to lower right

	// First Line, no prev... may have next line
	bool *pLine1 = mpEdges->GetLine(rect.top);
	bool *pFullLine1 = mpFull->GetLine(rect.top);
	bool *pNext1 = NULL;
	bool *pFullNext1 = NULL;
	if (rect.top + 1 <= rect.bottom)
	{
		pNext1 = mpEdges->GetLine(rect.top+1);
		pFullNext1 = mpFull->GetLine(rect.top+1);
	}	
	for (int iPix = rect.left; iPix < rect.right; iPix++)
	{
		if (pLine1[iPix])
		{
			if (pLine1[iPix + 1])
				AddEdgeToDraw(rect.top, iPix, rect.top, iPix + 1);
			if (pNext1 && pNext1[iPix + 1])
			{
				if (!pFullLine1[iPix + 1] || !pFullNext1[iPix])
					AddEdgeToDraw(rect.top, iPix, rect.top+1, iPix + 1);
			}
		}
	}

	// All middle lines
	for (int iLine = rect.top + 1; iLine < rect.bottom; iLine++)
	{
		bool *pPrev = mpEdges->GetLine(iLine-1);
		bool *pLine = mpEdges->GetLine(iLine);
		bool *pNext = mpEdges->GetLine(iLine+1);
		bool *pFullPrev = mpFull->GetLine(iLine-1);
		bool *pFullLine = mpFull->GetLine(iLine);
		bool *pFullNext = mpFull->GetLine(iLine+1);
		// First
		int iPix = rect.left;
		if (pLine[iPix])
		{
			if (pPrev[iPix])
				AddEdgeToDraw(iLine, iPix, iLine-1, iPix);
			if (pPrev[iPix+1] && (!pFullPrev[iPix] || !pFullLine[iPix+1]))
				AddEdgeToDraw(iLine, iPix, iLine-1, iPix+1);
			if (pLine[iPix+1] && (!pFullPrev[iPix+1] || !pFullNext[iPix+1]))
				AddEdgeToDraw(iLine, iPix, iLine, iPix+1);
			if (pNext[iPix+1] && (!pFullLine[iPix+1] || !pFullNext[iPix]))
				AddEdgeToDraw(iLine, iPix, iLine+1, iPix+1);
		}
		// Middle
		for (iPix++; iPix < rect.right; iPix++)
		{
			if (pLine[iPix])
			{
				if (pPrev[iPix] && (!pFullPrev[iPix-1] || !pFullPrev[iPix+1]))
					AddEdgeToDraw(iLine, iPix, iLine-1, iPix);
				if (pPrev[iPix+1] && (!pFullPrev[iPix] || !pFullLine[iPix+1]))
					AddEdgeToDraw(iLine, iPix, iLine-1, iPix+1);
				if (pLine[iPix+1] && (!pFullPrev[iPix+1] || !pFullNext[iPix+1]))
					AddEdgeToDraw(iLine, iPix, iLine, iPix+1);
				if (pNext[iPix+1] && (!pFullLine[iPix+1] || !pFullNext[iPix]))
					AddEdgeToDraw(iLine, iPix, iLine+1, iPix+1);
			}
		}
		// Last
		if (pLine[iPix])
		{
			if (pPrev[iPix])
				AddEdgeToDraw(iLine, iPix, iLine-1, iPix);
		}
	}

	// Last Line - if not also first - no next
	if (rect.bottom > rect.top)
	{
		int iLine = rect.bottom;
		bool *pPrev = mpEdges->GetLine(iLine-1);
		bool *pLine = mpEdges->GetLine(iLine);
		bool *pFullPrev = mpFull->GetLine(iLine-1);
		bool *pFullLine = mpFull->GetLine(iLine);
		int iPix = rect.left;
		if (pLine[iPix])
		{
			if (pPrev[iPix])
				AddEdgeToDraw(iLine, iPix, iLine-1, iPix);
			if (pPrev[iPix+1] && (!pFullPrev[iPix] || !pFullLine[iPix+1]))
				AddEdgeToDraw(iLine, iPix, iLine-1, iPix+1);
			if (pLine[iPix+1])
				AddEdgeToDraw(iLine, iPix, iLine, iPix+1);
		}
		// Middle
		for (iPix++; iPix < rect.right; iPix++)
		{
			if (pLine[iPix])
			{
				if (pPrev[iPix] && (!pFullPrev[iPix-1] || !pFullPrev[iPix+1]))
					AddEdgeToDraw(iLine, iPix, iLine-1, iPix);
				if (pPrev[iPix+1] && (!pFullPrev[iPix] || !pFullLine[iPix+1]))
					AddEdgeToDraw(iLine, iPix, iLine-1, iPix+1);
				if (pLine[iPix+1])
					AddEdgeToDraw(iLine, iPix, iLine, iPix+1);
			}
		}
		// Last
		if (pLine[iPix])
		{
			if (pPrev[iPix])
				AddEdgeToDraw(iLine, iPix, iLine-1, iPix);
		}
	}

	return true;
}
void CGContour::DrawEdges(CDisplayRectangle *pDispRect, CDC &dc)
{
	COLORREF color = mColor;
	if (mbActive)
		color = 0xffff00;
	CMetaData *pData = pDispRect->GetCurrentData();
	CMyImage *pPage = pData->PageData();
	CZoomPanParams *pZoomPar = pDispRect->GetZoomParams();

	POSITION pos = mEdgesToDraw.GetHeadPosition();
	while (pos)
	{
		CGContourEdge *pEdge = mEdgesToDraw.GetNext(pos);
		pEdge->dc1.mpImage = pPage;
		pEdge->dc2.mpImage = pPage;
		CGPoint p1(&pEdge->dc1, pDispRect, pZoomPar);
		CGPoint p2(&pEdge->dc2, pDispRect, pZoomPar);
		p1.ComputeScreenPosition(pZoomPar);
		p2.ComputeScreenPosition(pZoomPar);
        pDispRect->DrawLine (dc, p1, p2, color);

	}

	AddGraphPoints(pDispRect);
}
bool CGContour::DrawPointless(CDisplayRectangle *pDispRect, CDC &dc)
{
	if (mpDataVolume)
	{
		CPosition *pPos = pDispRect->GetPosition();
		int iRel = pPos->GetCurrentRelative();
		mpRebm = mpDataVolume->GetRebm(iRel);
		if (!mpRebm)
			return true; // Do nothing - but That's it!
	}

	if (!ComputeEdges())
		return false;

	if (!ComputeEdgesToDraw())
		return false;

	DrawEdges(pDispRect, dc);
	return true;
}
void CGContour::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("CGContour",xmlDump);
    
	CGraphicElement::SaveState(xmlDump);
	{
		CXMLDumpScope graph1("seed",xmlDump);
		xmlDump.Write("seed_x", mSeed.fx);
		xmlDump.Write("seed_y", mSeed.fy);
	}
	if (mpRebm)
		mpRebm->SaveState(xmlDump);
	if (mpSharedMemFile)
		mpSharedMemFile->SaveState(xmlDump);
}
bool CGContour::NewDrawOnDrag(CDisplayRectangle *pDisplayRectangle)
{
	if (!pDisplayRectangle->UpdateMousePosToNewScreenCoo())
		return false;

	CLogFile log("CGContour_Drag");
	fprintf (log.mpf, "seed %f %f \n", mSeed.fx, mSeed.fy);
	mSeed = pDisplayRectangle->GetMouseDataPos();
	fprintf (log.mpf, "seed %f %f \n", mSeed.fx, mSeed.fy);
	ComputeRebmFromSeed(pDisplayRectangle);
	return true;
}
bool CGContour::ComputeRebmFromSeed(CDisplayRectangle *pDisplayRectangle)
{
	CMetaData *pData = pDisplayRectangle->GetCurrentData();
	if (!pData )
		return false;
	if (!mpFull)
		return false;

	CMyImage *pPage = pData->PageData();
	mpFull->Zero();
	pPage->InitRebm(mSeed, *mpRebm);
	mpRebm->SetFull(mpFull);
	//double newSum = mpFull->Sum();
	//mpRebm->Print();
	return true;
}
bool CGContour::UpdateRebmFromShared(void)
{
	mpRebm->DeleteAllRuns();
	mpSharedMemFile->UpdatePageData();

	CMyImage *pPageData = mpSharedMemFile->PageData();
	int mContourThreshold = mpSharedMemFile->GetContourThreshold();

	CTImage<short> *pShort = (CTImage<short> *)pPageData;
	pShort->InitRebmByThreshold(*mpRebm, (short)mContourThreshold);
	mpRebm->OnChange();

	CString sDesc(_T("<CGContour::UpdateRebmFromShared> Print Rebm of shared image"));
	CPosition *pPosition = mpSharedMemFile->GetPosition();
	if (pPosition)
	{
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf), " %d", pPosition->miCurrent);
		sDesc += zBuf;
	}
	mpRebm->Print(false, (const char *)sDesc);

	CString sText (mpSharedMemFile->GetDescription());
	msText = sText;
	return true;
}

