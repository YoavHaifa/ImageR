#include "StdAfx.h"
#include "DisplayRectangle.h"
#include "MetaData.h"
#include "Screens.h"
#include "DisplayGE.h"
#include "GraphicElementOnDR.h"

#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyRegistry.h"
#include "..\..\yUtils\UserTextDialog.h"
#include "..\..\yUtils\NameGetDialog.h"

#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\GraphicToExcel.h"
#include "..\..\ImageRLib\GraphicToExcel.h"

#include "ZoomParamsDlg.h"
#include "FileFactory.h"
#include "GRoi.h"
#include "GEllipse.h"
#include "RingID.h"
#include "DRText.h"
#include "GGraph.h"
#include "GLine.h"
#include "GContour.h"
#include "GoToPage.h"
#include "SharedMemFile.h"
#include "TextController.h"
#include "ImageView.h"

bool CDisplayRectangle::umbPrintDataCoo = true;
DWORD CDisplayRectangle::umDebug = 0;
DWORD CDisplayRectangle::umDebugText = 0;
bool CDisplayRectangle::umbSaveZoomedRaster = false;
bool CDisplayRectangle::umbSaveZoomedRasterAsShort = false;
bool CDisplayRectangle::umbWrapOnNextAndPrev = true;
bool CDisplayRectangle::umbDisplayTitles = true;
bool CDisplayRectangle::umbDisplayDataRange = true;
float CDisplayRectangle::umCharFracPerTextWidth = 0.1f;
int CDisplayRectangle::umId = 0;
bool CDisplayRectangle::umbReverseRGBOrder = true;

CDisplayRectangle::CDisplayRectangle(FILE *pfLog, CXMLParseNode *pDRXml)
: CRect (0,0,0,0)
, CDependant ("DisplayRectangle",umId)
, mDRId(umId)
, mpCurrentData(NULL)
, mGrayRaster("GrayRaster")
, mGrayRasterForColorer("ColGrayRaster")
, mColorRaster("ColorRaster")
, mbColorRasterUpdated(false)
, mpColors(NULL)
, mColorsLineLen(0)
, mpfLog (pfLog)
, mpCurrentGE (NULL)
, mpDRXml (pDRXml)
, mpSelectedPoint(NULL)
, mbEnableColorers(true)
, mbEnableText(true)
, mbEnableGraphics(true)
, mbDisableStateRestore(false)
, mbBroadcastCursorLocation(false)
, mbIsCurrent(true)
, mbExternalFrame(0)
, mbShouldBroadcastIfLoosingMouse(false)
, mbUseColorMap(false)
, mnPaint(0)
, mLastDrCoo(*this,0,0)
, msUserDefinedTitle(_T(""))
//, mId(0)
, mnTitleLinesDisplayed(0)
{
	umId++;
	if (pDRXml)
		SetParseId(pDRXml);

	//static int count = 0;
	//mId = ++count;
	if (umDebug || umDebugText)
	{
		char zBuf[32];
		sprintf_s(zBuf,32,"DisplayRect_%d", mDRId + 1);
		mpfLog = CMyWindows::FOpenLogFile(zBuf);
	}
    if (pDRXml)
        InitFromXml(pDRXml);
}
CDisplayRectangle::~CDisplayRectangle(void)
{
}
void CDisplayRectangle::ToggleColors(void)
{
	if (mColorersList.IsEmpty() && gScreens.mGlobalColorersList.IsEmpty())
		mbUseColorMap = !mbUseColorMap;
	else
	    mbEnableColorers = !mbEnableColorers;
	gScreens.UpdateStatusBarOnNextPaint();
    gScreens.Refresh();
}
void CDisplayRectangle::ToggleText(void)
{
    mbEnableText = !mbEnableText;
    gScreens.Refresh();
}
void CDisplayRectangle::ToggleGraphics(void)
{
    mbEnableGraphics = !mbEnableGraphics;
    gScreens.Refresh();
}
void CDisplayRectangle::CheckXmlForDisabledGE(CGraphicElement *pGE, CXMLParseNode *pGraphicsXml)
{
	if (!pGE)
		return;
	if (!pGE->GetDisplayGE())
		return;
	CXMLParseNode *pDisabledXml = pGraphicsXml->GetFirst("disabled");
	if (!pDisabledXml)
		return;
	CGraphicElementOnDR *pGEonDR = pGE->GetDisplayGE()->FindGeOnDr(this, NULL);
	if (pGEonDR)
		pDisabledXml->GetValue(pGEonDR->mbDisabled);
}
bool CDisplayRectangle::InitGraphicsFromXml(CXMLParseNode *pGraphicsXml,
            CList<class CGraphicElement*,class CGraphicElement*> *pList)
{
    bool bChange = false;
    CXMLParseNode *pGEXml = pGraphicsXml->GetFirst(NULL);
    while (pGEXml)
    {
		CString sClass = pGEXml->GetFirstDeepString("class","unknown");
		CString sName = pGEXml->GetFirstDeepString("name","");
		bool bSeparationGraph = ((sName.Find("SeparationGraph")) >= 0 && (sClass == "CDataLine"));

		bool bEllipse = false;
		bool bRectangle = false;
		if (sClass == "CDataRoi")
		{
			CXMLParseNode *pbEllipse = pGraphicsXml->GetFirstDeep("b_ellipse");
			if (pbEllipse)
				pbEllipse->GetValue(bEllipse);
			if (!bEllipse)
				bRectangle = true;
		}
		if (sClass == "CGEllipse" || bEllipse)
        {
            CGEllipse *pEllipse = (CGEllipse *)GetParsed(pGEXml);
            if (!pEllipse)
			{
				CString sName = pGEXml->GetFirstDeepString("name","unknown");
				if (sName == "RingID")
				{
					CRingID *pRingID = AddScanCenter();
					if (pRingID)
					{
						if (pList)
							pList->AddTail(pRingID);
						return true;
					}
				}
                pEllipse = new CGEllipse (this, pGEXml);
			}
            if (AddGraphicElement (pEllipse))
			{
	            if (pList)
		            pList->AddTail(pEllipse);
				CheckXmlForDisabledGE(pEllipse, pGEXml);
				bChange = true;
			}
        }
        else if (sClass == "CGRoi" || bRectangle)
        {
            CGRoi *pRoi = (CGRoi *)GetParsed(pGEXml);
            if (!pRoi)
                pRoi = new CGRoi (this, pGEXml);
            if (AddGraphicElement (pRoi))
			{
	            if (pList)
		            pList->AddTail(pRoi);
				CheckXmlForDisabledGE(pRoi, pGEXml);
				bChange = true;
			}
        }
        else if (sClass == "CDataLine" || sClass == "CGLine")
        {
            CGLine *pLine = (CGLine *)GetParsed(pGEXml);
            if (!pLine)
                pLine = new CGLine (this, pGEXml);
            if (AddGraphicElement (pLine))
			{
	            if (pList)
		            pList->AddTail(pLine);
				CheckXmlForDisabledGE(pLine, pGEXml);
				bChange = true;
			}
        }
        else if (sClass == "CGCursor" || sClass == "CDataCursor")
        {
            CGCursor *pCursor = (CGCursor *)GetParsed(pGEXml);
            if (!pCursor)
                pCursor = new CGCursor (this, pGEXml);
            if (AddGraphicElement (pCursor))
			{
	            if (pList)
		            pList->AddTail(pCursor);
				CheckXmlForDisabledGE(pCursor, pGEXml);
				bChange = true;
			}
			//if (mpfLog)
			//{
			//	fprintf (mpfLog, "<CDisplayRectangle::InitGraphicsFromXml> pCursor %p\n", pCursor);
			//	pCursor->Print(mpfLog);
			//}
        }
		else if (sClass == "CDRText")
		{
			CDRText *pText = (CDRText *)GetParsed(pGEXml);
            if (!pText)
                pText = new CDRText (this, pGEXml);
            if (AddGraphicElement (pText))
			{
	            if (pList)
		            pList->AddTail(pText);
				CheckXmlForDisabledGE(pText, pGEXml);
				bChange = true;
			}
		}
        else if (sClass == "CGGraph")
        {
            CGGraph *pGraph = (CGGraph *)GetParsed(pGEXml);
            if (!pGraph)
                pGraph = new CGGraph (this, pGEXml);
            if (AddGraphicElement (pGraph))
			{
	            if (pList)
		            pList->AddTail(pGraph);
				CheckXmlForDisabledGE(pGraph, pGEXml);
				bChange = true;
			}
        }
		else if (sClass = "CRingID")
		{
			AddScanCenter(pGEXml);
		}
        pGEXml = pGraphicsXml->GetNext(NULL, pGEXml);
    }
    return bChange;
}
void CDisplayRectangle::InitDataFromXml(CXMLParseNode *pDataXml)
{
    CXMLParseNode *pMetaDataXml = pDataXml->GetFirst(NULL);
    while (pMetaDataXml)
    {
        CFileFactory fileFactory(NULL);
        CMetaData *pData = fileFactory.CreateFileObject(pMetaDataXml);
        if (pData)
            AddData(pData);

        pMetaDataXml = pDataXml->GetNext(NULL, pMetaDataXml);
    }
}
void CDisplayRectangle::InitTextsFromXml(CXMLParseNode *pTextsXml)
{
    CXMLParseNode *pTextXml = pTextsXml->GetFirst(NULL);
    while (pTextXml)
    {
		if (!pTextXml->GetFirstDeep("disable_state_restore"))
		{
	        CDRText *pText = new CDRText(pTextXml);
			AddGraphicElement(pText);
		}
        pTextXml = pTextsXml->GetNext(NULL, pTextXml);
    }
}
void CDisplayRectangle::InitFromXml(CXMLParseNode * pDRXml)
{
    CXMLParseNode *pDataXml = pDRXml->GetFirst("data");
    if (!pDataXml)
        return;
    InitDataFromXml(pDataXml);

    CXMLParseNode *pGraphicsXml = pDRXml->GetFirst("graphic_elements");
    if (pGraphicsXml)
        InitGraphicsFromXml(pGraphicsXml);

    CXMLParseNode *pTextsXml = pDRXml->GetFirst("texts");
    if (pTextsXml)
        InitTextsFromXml(pTextsXml);
}
void CDisplayRectangle::AddData(class CMetaData * pData)
{
    if (!mDataList.IsEmpty())
        OnDataEclipse();

    mDataList.AddTail (pData);
    pData->SetDefaultLogFile (mpfLog);
    mpCurrentData = pData;

    pData->AssignDisplayRectangle (*this);
}
CMetaData * CDisplayRectangle::GetCurrentData(void)
{
    return mpCurrentData;
}
CMetaData * CDisplayRectangle::GetCurrentDataForWindowing(void)
{
	if (mbEnableColorers)
	{
		if (!mColorersList.IsEmpty())
		{
			CMetaData *pData = mColorersList.GetHead();
			if (pData->IsDataForColoring())
				return pData;
		}
	}
    return mpCurrentData;
}
bool CDisplayRectangle::OnViewPrevfile(void)
{
    if (mDataList.IsEmpty())
        return false;
    if ((int)mDataList.GetSize() < 2)
        return false;
    OnDataEclipse();

    POSITION pos = mDataList.Find(mpCurrentData);
    mDataList.GetPrev(pos);
    if (!pos)
	{
		if (umbWrapOnNextAndPrev)
	        pos = mDataList.GetTailPosition();
		else
			return false;
	}
	CMetaData *pData = mDataList.GetAt(pos);
	pData->PropagateMultiPos(mpCurrentData);
    mpCurrentData = pData;
    OnChange();
    return true;
}
bool CDisplayRectangle::OnViewNextfile(void)
{
    if (mDataList.IsEmpty())
        return false;
    if ((int)mDataList.GetSize() < 2)
        return false;
    OnDataEclipse();

    POSITION pos = mDataList.Find(mpCurrentData);
    mDataList.GetNext(pos);
    if (!pos)
	{
		if (umbWrapOnNextAndPrev)
			pos = mDataList.GetHeadPosition();
		else
			return false;
	}
	CMetaData *pData = mDataList.GetAt(pos);
	pData->PropagateMultiPos(mpCurrentData);
    mpCurrentData = pData;
    OnChange();
    return true;
}
void CDisplayRectangle::DrawGraphics(CDC &dc)
{
	if (!mpCurrentData || !mbEnableGraphics)
        return;

	//if (mpfLog && !mGraphicList.IsEmpty())
	//	fprintf (mpfLog, "\n<CDisplayRectangle::DrawGraphics> on %s\n", mpCurrentData->Name());

    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pElement = mGraphicList.GetNext(pos);
        if (pElement->IsRelevant(mpCurrentData))
		{
            pElement->Draw (dc);
			//if (mpfLog)
			// pElement->Print(mpfLog);
		}
    }
    if (mbIsCurrent && gScreens.GetNDR() > 1)
        DrawFrame(dc, 0 , 0xffffff);
    if (mbExternalFrame)
        DrawFrame(dc, 1 , 0xff80ff);
}
CGraphicElementOnDR * CDisplayRectangle::FindGraphicNear(CDRCoordinates &drCoo)
{
    int bestDistance = INT_MAX;
    mpCurrentGE = NULL;

    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pElement = mGraphicList.GetNext(pos);
		if (pElement->IsRelevant(mpCurrentData) && pElement->IsSelectable())
        {
            int distance;
            if (pElement->IsValid() && pElement->FindPointNear (drCoo, &distance))
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    mpCurrentGE = pElement;
                }
        }
    }
    return mpCurrentGE;
}
CDataCoordinates CDisplayRectangle::GetSelectedDataCoo(void)
{
    if (!mpSelectedPoint)
    {
        CDRCoordinates drCoo (*this, (right - left) / 2, (bottom - top) / 2);
        SetSelectedPoint(drCoo);
    }

    CDataCoordinates dataCoo (mpCurrentData->CurPage());
    mpSelectedPoint->UpdateData(GetZoomParams(), dataCoo);
    return dataCoo;
}
void CDisplayRectangle::SetSelectedPoint(CDRCoordinates &drCoo)
{
    if (mpSelectedPoint)
        *mpSelectedPoint = drCoo;
    else
        mpSelectedPoint = new CDRCoordinates (*this, drCoo.x, drCoo.y);
}
bool CDisplayRectangle::OnLButtonDown(UINT nFlags, CScreenCoordinates screenCoo)
{
    CDRCoordinates drCoo (*this, screenCoo);
    SetSelectedPoint(drCoo);

    CGraphicElementOnDR * pGE = FindGraphicNear(drCoo);
    if (!pGE)
        return false;

    bool bChange = pGE->OnLButtonDown (nFlags, drCoo);
    if (bChange)
	{
		gScreens.mbLeftMouseDoingGraphics = true;
        PrintStatusBar(drCoo);
	}
    return bChange;
}
bool CDisplayRectangle::OnLButtonUp(UINT nFlags, CScreenCoordinates screenCoo)
{
    CDRCoordinates drCoo (*this, screenCoo);
    bool bChanged = false;
    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pElement = mGraphicList.GetNext(pos);
        bool bThisChanged = pElement->OnLButtonUp (nFlags, drCoo);
		if (bThisChanged)
		{
			pElement->GetGE()->InformDataOnLUp();
			bChanged = true;
		}
    }
    return bChanged;
}
bool CDisplayRectangle::OnRButtonDblClk(UINT nFlags, CScreenCoordinates screenCoo)
{
    CDRCoordinates drCoo (*this, screenCoo);
    bool bChanged = false;
    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pElement = mGraphicList.GetNext(pos);
        bChanged |= pElement->OnRButtonDblClk (nFlags, drCoo);
    }
    return bChanged;
}
bool CDisplayRectangle::UpdateMousePosToNewScreenCoo(void)
{
	return UpdateMousePos(gScreens.GetNewMousePos());
}
bool CDisplayRectangle::UpdateMousePos(CScreenCoordinates screenCoo)
{
	if (!mpCurrentData || !mpCurrentData->IsValid())
        return false;
    mMouseDataCoo.mpImage = mpCurrentData->PageData();
    if (!mMouseDataCoo.mpImage)
        return false;

    CDRCoordinates drCoo (*this, screenCoo);
    drCoo.UpdateData(GetZoomParams(), mMouseDataCoo);
    PrintStatusBar(drCoo);
	return true;
}
bool CDisplayRectangle::OnMousePos(CScreenCoordinates screenCoo)
{
	if (mpfLog && ((umDebug & 8) != 0))
		fprintf (mpfLog, "<OnMousePos> screenCoo x %d y %d\n", screenCoo.x, screenCoo.y);

	if (!UpdateMousePos(screenCoo))
		return false;

	bool bChanged = mpCurrentData->OnMousePos(mMouseDataCoo);

    // Call all Reference Graphic Elements
    POSITION pos = mRefList.GetHeadPosition();
    while (pos)
    {
        CMetaData *pData = mRefList.GetNext(pos);
        bChanged |= pData->OnMousePosOnRef (mMouseDataCoo);
        if (bChanged)
            gScreens.SetWaitKillFocus (this);
    }
    pos = gScreens.mGlobalRefList.GetHeadPosition();
    while (pos)
    {
        CGraphicElement *pElement = gScreens.mGlobalRefList.GetNext(pos);
        bChanged |= pElement->OnMousePosAnywhere (mpCurrentData, mMouseDataCoo);
    }
    if (mbBroadcastCursorLocation)
    {
        WPARAM wParam = (WPARAM)mMouseDataCoo.fx;
        LPARAM lParam = (LPARAM)mMouseDataCoo.fy;
		CMyWindows::PostMessageToClient(gScreens.CursorLocationMsg(), wParam, lParam);
		mbShouldBroadcastIfLoosingMouse = true;
    }
    return bChanged;
}
void CDisplayRectangle::OnLoosingMouse(void)
{
    if (mbBroadcastCursorLocation && mbShouldBroadcastIfLoosingMouse)
    {
        WPARAM wParam = 0x80000000;
        LPARAM lParam = 0x80000000;
		CMyWindows::PostMessageToClient(gScreens.CursorLocationMsg(), wParam, lParam);
		mbShouldBroadcastIfLoosingMouse = false;
    }
}
bool CDisplayRectangle::AddRef(CMetaData *pData)
{
    mRefList.AddTail(pData);
	pData->AddDependant(this);
    return true;
}
void CDisplayRectangle::Bye(CDependant* pDependant)
{
    POSITION pos = mRefList.Find((CMetaData *)pDependant);
    if (pos)
        mRefList.RemoveAt(pos);
}
bool CDisplayRectangle::AddGraphicElementFromSharedMemImage(class CSharedMemFile *pSharedMemFile)
{
    bool bChange = false;
   CGContour *pContour = new CGContour(this, pSharedMemFile);
    // If there is a GE with same name - remove it
    bChange |= DoGraphicCommand(GCOM_REMOVE, pContour->msName);
    bChange |= AddGraphicElement(pContour);
    if (pContour->DisplayOnAll())
    {
        bChange |= gScreens.RemoveGEFromAllOtherDR(pContour->msName, this);
        gScreens.DisplayOnAllRectangles(pContour);
    }
	return bChange;
}
bool CDisplayRectangle::AddGraphicElementFromXml(CXMLParseNode *pNode)
{
    CString sClass = pNode->GetFirstDeepString("class","unknown");
    CString sName = pNode->GetFirstDeepString("name","");
    bool bChange = false;
    if ((sClass == "CDataRoi")
		|| (sClass == "CDataCursor")
		|| (sClass == "CDataLine")
		|| (sClass == "CGLine")
		|| (sClass == "CGGraph")
		|| (sClass == "CGEllipse")
		|| (sClass == "CGCursor")
		|| (sClass == "CDRText")
		|| (sClass == "CRingID")
		)
    {
        if (!sName.IsEmpty() && (sName.Find("SeparationGraph") < 0))
            bChange |= DoGraphicCommand(GCOM_REMOVE, sName);

        CList<class CGraphicElement*,class CGraphicElement*> list;
        bChange |= InitGraphicsFromXml(pNode, &list);
		if (list.IsEmpty())
			return false;

        CGraphicElement *pGE = list.GetHead();
		if (pGE->DisplayInSelectedPosition())
		{
			if (pGE->GetClass() == "CGEllipse")
			{
				CDataCoordinates dataCoo (GetSelectedDataCoo());
				CGEllipse *pEllipse = (CGEllipse *)pGE;
				pEllipse->InitEllipse(dataCoo, (float)ELLIPSE_DEFAULT_SIZE_MM);
				pGE->ProtectFromDataEclipse();
				if (mpCurrentData && mpCurrentData->GetPosition())
				{
					pGE->InitPosition(mpCurrentData->GetPosition());
					pGE->LockToCurrentPosition();
				}
			}
		}
        if (pGE->DisplayOnAll())
        {
            bChange |= gScreens.RemoveGEFromAllOtherDR(sName, this);
            gScreens.DisplayOnAllRectangles(pGE);
        }
    }
    else
    {
		CXMLParseNode *pGEXml = pNode->GetFirst(NULL);
       CGContour *pContour = new CGContour(this, pGEXml);
        // If there is a GE with same name - remove it
        bChange |= DoGraphicCommand(GCOM_REMOVE, pContour->msName);
        bChange |= AddGraphicElement(pContour);
    }
    return bChange;
}
bool CDisplayRectangle::AddGraphicElement(CGraphicElement * pElement)
{
	if (pElement->GetNPoints() < 1 && !pElement->mbContour)
	{
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf),"<CDisplayRectangle::AddGraphicElement> %s with no points", (const char*)pElement->Name());
		CMyWindows::MessBox(zBuf, "SW Error");
		return false;
	}

	if (pElement->GetSaveToXml())
		gScreens.InitLeftMouseUpMsg();

    //if (pElement->GetDataPosition())
    //{
    //    if (mpCurrentData && !pElement->IsOn(mpCurrentData->GetPosition()))
    //        return false;
    //}

    CDisplayGE *pDisplayGE = pElement->GetDisplayGE();
    if (!pDisplayGE)
        pDisplayGE = new CDisplayGE(pElement);

    if (!pDisplayGE->AddDisplay(this))
		return false;

	if (mpCurrentData)
	{
		mpCurrentData->OnNewGE(pElement);
		pElement->SetOriginalData(mpCurrentData);
	}
	return true;
}
bool CDisplayRectangle::RemoveGraphicElement(CGraphicElementOnDR * pElement)
{
    POSITION pos = mGraphicList.Find(pElement);
    if (pos)
    {
        mGraphicList.RemoveAt(pos);
        return true;
    }
    return false;
}
CGraphicElementOnDR *CDisplayRectangle::FindGraphic(const char * zName)
{
    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR * pElement = mGraphicList.GetNext(pos);
        if (pElement->Is(zName))
			return pElement;
    }
    return NULL;
}
bool CDisplayRectangle::DoGraphicCommand(EGraphicCommand eGCom, const char * zName)
{
	bool bChanged = false;

    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR * pElement = mGraphicList.GetNext(pos);
        if (pElement->IsPrefixedBy(zName))
		{
			switch (eGCom)
			{
			case GCOM_REMOVE:
				bChanged |= RemoveGraphicElement(pElement);
				break;
			case GCOM_DISABLE:
				bChanged |= pElement->Disable();
				break;
			case GCOM_ENABLE:
				bChanged |=  pElement->Enable();
				break;
			case GCOM_FIX_CENTER:
				bChanged |=  pElement->FixCenter();
				break;
			default:
				break;
			}
		}
    }
    return bChanged;
}
bool CDisplayRectangle::AddGraphicElement(CGraphicElementOnDR * pElement)
{
    if (mGraphicList.Find(pElement))
        return false;

    mGraphicList.AddTail (pElement);
    return true;
}

bool CDisplayRectangle::DragGraphics(CSize size)
{
    bool bChanged = false;
    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pElement = mGraphicList.GetNext(pos);
		size_t pInt = (size_t)pElement;
		if (pInt == 0xdcdcdcdc || pInt == 0xcdcdcdcd)
			break;
		else
	        bChanged |= pElement->Drag (size);
    }
    return bChanged;
}

void CDisplayRectangle::SetShape(int setLeft, int setTop, int setRight, int setBottom)
{
    SetRect(setLeft, setTop, setRight, setBottom);
    mGrayRaster.SetSize(bottom - top + 1, right - left + 1);
    OnChange();
    //mCenter.x = (right - left) / 2;
    //mCenter.y = (bottom - top) / 2;
}
void CDisplayRectangle::SetColorsRaster(COLORREF * pStart, int lineLen)
{
    mpColors = pStart;
    mColorsLineLen = lineLen;
}
bool CDisplayRectangle::UpdateColorRaster(void)
{
    if (!mbColorRasterUpdated)
    {
        mColorRaster.SetSize(bottom - top + 1, right - left + 1);

        COLORREF *pRGBTable = gScreens.GetGrayRgbTable();
        unsigned char *pGray = mGrayRaster.GetData();
        COLORREF *pColor = mColorRaster.GetData();
        int n = mGrayRaster.GetNPixels();

        while (n--)
            *pColor++ = pRGBTable[*pGray++];
        mbColorRasterUpdated = true;
    }
    return true;
}
void CDisplayRectangle::ClearScreen(void)
{
    int nInLine = right - left + 1;
    COLORREF *pScreenLine = mpColors;
    for (int iLine=top; iLine <= bottom; iLine++) 
	{
        int n = nInLine;
        while (n--)
            *pScreenLine++ = 0;
        pScreenLine += (mColorsLineLen - nInLine);
    }
}
void CDisplayRectangle::GrayToScreenColors(void)
{
    int nInLine = right - left + 1;
    const unsigned char *pGray = mGrayRaster.GetData();
    COLORREF *pScreenLine = mpColors;
    COLORREF *pRGBTable = NULL;
	if (mbUseColorMap)
		pRGBTable = gScreens.GetColorsRgbTable();
	else
		pRGBTable = gScreens.GetGrayRgbTable();

    for (int iLine=top; iLine <= bottom; iLine++) 
	{
        int n = nInLine;
        while (n--)
            *pScreenLine++ = pRGBTable[*pGray++];
        pScreenLine += (mColorsLineLen - nInLine);
    }
}
void CDisplayRectangle::ColorRasterToScreenColors(void)
{
    int nInLine = right - left + 1;
    const COLORREF *pColor = mColorRaster.GetData();
    COLORREF *pScreenLine = mpColors;
    int nBytesInLine = nInLine * sizeof(pColor[0]);

    for (int iLine=top; iLine <= bottom; iLine++) 
	{
        memcpy (pScreenLine, pColor, nBytesInLine);
        pScreenLine += mColorsLineLen;
        pColor += nInLine;
    }
}
void CDisplayRectangle::Paint(void)
{
    if (!mpCurrentData)
	{
		ClearScreen();
        return;
	}

	if (mpfLog)
		fprintf (mpfLog, "<Paint> %d data %s\n", mnPaint, (const char*)mpCurrentData->Name());
	if (left == right)
	{
		static int unErr = 0;
		if (!gScreens.mbIgnoreServerTimingProblems && !unErr++)
			CMyWindows::MessBox("<CDisplayRectangle::Paint> Empty Rect", mpCurrentData->Name());
		return;
	}

    mbColorRasterUpdated = false;

    if (mpCurrentData->FillZoomedRaster (*this))
    {
		if (umbSaveZoomedRaster)
			mpCurrentData->SaveZoomedRaster();

		if (FillGrayRaster(mpCurrentData, mGrayRaster))
		{
			if (mbEnableColorers)
				ApplyColorers();

			// Set grey data to COLOREF
			if (mbColorRasterUpdated)
				ColorRasterToScreenColors();
			else
				GrayToScreenColors();
		}
		else
		{
			CopyZoomedRgbToScreen();
		}
    }
    else
    {
        static int nErrors = 0;
		if (!gScreens.mbIgnoreServerTimingProblems && !nErrors++)
			CMyWindows::MessBox("FillZoomedRaster failed", mpCurrentData->msName);
    }
	if (mpfLog)
		fprintf (mpfLog, "<Paint> %d End\n\n", mnPaint++);
}
void CDisplayRectangle::CopyZoomedRgbToScreen(void)
{
	S_RGB * pRgb = mpCurrentData->GetRgbZoomedRasterData();
	if (pRgb)
	{
		CMyImage *pZoomed = mpCurrentData->GetZoomedRaster();
		mColorRaster.SetSize(pZoomed->GetNLines(),pZoomed->GetNCols());
		COLORREF *pColor = mColorRaster.GetData();
		int n = pZoomed->GetNPixels();

		if (umbReverseRGBOrder)
		{
			while (n--)
			{
				S_RGB rgb = *pRgb++;
				unsigned char *pCol = (unsigned char *)pColor;
				pCol[0] = rgb.RGB[2];
				pCol[1] = rgb.RGB[1];
				pCol[2] = rgb.RGB[0];
				pColor++;
				//pRgb++;
			}
		}
		else
		{
			while (n--)
			{
				S_RGB rgb = *pRgb++;
				COLORREF col = rgb;
				*pColor++ = col;
			}
		}

		ColorRasterToScreenColors();
	}
}
void CDisplayRectangle::ApplyColorer(CMetaData *pColorer, FILE *pfLog)
{
    if (pColorer->IsValid())
    {
        if (pColorer->UpdateColorer(*this))
        {
			if (pfLog)
				fprintf (pfLog, "Colorer %s Average %f\n", (const char*)pColorer->Name(), pColorer->Average());
            UpdateColorRaster();
            pColorer->AdjustColors(*this);
			if (pfLog)
				fprintf (pfLog, "Colorer %s displayed\n", (const char*)pColorer->Name());
        }
		else if (pfLog)
			fprintf (pfLog, "Colorer %s UpdateColorer failed\n", (const char*)pColorer->Name());
    }
	else if (pfLog)
		fprintf (pfLog, "Colorer %s is not valid\n", (const char*)pColorer->Name());
}
void CDisplayRectangle::ApplyColorers(void)
{
	static int count = 0;
	static FILE *upf = NULL;
	static CFileName uLogFileName;
	if (!upf && gScreens.mDebugColor)
	{
		if (!count++)
			uLogFileName.Set(CFileName::BuildFileName(CMyWindows::GetLogDir(), "ApplyColorers", "log"));
		upf = CMyWindows::FOpenLogFile(uLogFileName, count > 1);
		CSharedMemFile::SetColorerDebugLog(upf);
	}

	if (mpCurrentData && !mpCurrentData->AllowColors())
		return;
	if (upf)
		fprintf (upf, "<ApplyColorers> %d on %s\n", count, (const char*)mpCurrentData->msName);

    POSITION pos = mColorersList.GetHeadPosition();
    while (pos)
    {
        CMetaData *pColorer = mColorersList.GetNext(pos);
		if (!pColorer->IsOnTop())
		{
			if (upf)
				fprintf (upf, "Background Colorer %s\n", (const char*)pColorer->msName);
			ApplyColorer(pColorer, upf);
		}
    }
    pos = gScreens.mGlobalColorersList.GetHeadPosition();
    while (pos)
    {
        CMetaData *pColorer = gScreens.mGlobalColorersList.GetNext(pos);
		if (upf)
			fprintf (upf, "Global Colorer %s\n", (const char*)pColorer->msName);
		ApplyColorer(pColorer, upf);
    }
    pos = mColorersList.GetHeadPosition();
    while (pos)
    {
        CMetaData *pColorer = mColorersList.GetNext(pos);
		if (pColorer->IsOnTop())
		{
			if (upf)
				fprintf (upf, "Top Colorer %s\n", (const char*)pColorer->msName);
			ApplyColorer(pColorer, upf);
		}
    }
	if (upf)
	{
		fprintf (upf, "<ApplyColorers> %d on %s End\n\n", count, (const char*)mpCurrentData->msName);
		fclose(upf);
		upf = NULL;
		CSharedMemFile::SetColorerDebugLog(NULL);
	}
}   
void CDisplayRectangle::DisplayWindowStatus(void)
{
	bool bDebugPrintStatus = ((umDebug & 4) != 0);
	CMetaData *pData = GetCurrentDataForWindowing();
    if (!pData)
        return;
	gScreens.DisplayWindowStatus(pData, bDebugPrintStatus ? mpfLog : NULL);
}
void CDisplayRectangle::PrintStatusStatistics(CDataCoordinates &dataCoo)
{
	CMetaData *pDataForDisplay = GetCurrentDataForWindowing();
    CString sValue;
    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pGEonDR = mGraphicList.GetNext(pos);
		if (mpCurrentData && !pGEonDR->IsRelevant(mpCurrentData))
			continue;

        CGraphicElement *pGE = pGEonDR->GetGE();
        if (pGE->GetStatisticsStringForStatus(pDataForDisplay, sValue, pGEonDR))
        {
			if (!gScreens.mbEbwMode)
				gScreens.PrintStatusInBar (CScreens::SBAR_GENERAL, pGE->msName);
            break;
        }
		if (pGEonDR == mpCurrentGE && !gScreens.mbEbwMode)
            gScreens.PrintStatusInBar (CScreens::SBAR_GENERAL, pGE->msName);
    }
    if (sValue.IsEmpty())
        sValue = pDataForDisplay->GetStatisticsStringPerCoo(dataCoo);
    gScreens.PrintStatusInBar (CScreens::SBAR_STATISTICS, sValue);
}
void CDisplayRectangle::PrintStatusBarWithLastCoo(void)
{
	PrintStatusBar(mLastDrCoo);
}
void CDisplayRectangle::PrintStatusBar(CDRCoordinates drCoo)
{
	mLastDrCoo = drCoo;
	CMetaData *pDataForDisplay = GetCurrentDataForWindowing();
	bool bDebugPrintStatus = ((umDebug & 4) != 0);
    if (bDebugPrintStatus && mpfLog)
        fprintf (mpfLog, "<CDisplayRectangle::PrintStatus> point [%d, %d]\n", drCoo.x, drCoo.y);

    CDataCoordinates dataCoo (pDataForDisplay->CurPage());
    drCoo.UpdateData(GetZoomParams(), dataCoo);

    char zBuf[128];
	if (umbPrintDataCoo)
	    sprintf_s (zBuf, 128, "X %3d Y %3d", dataCoo.X(), dataCoo.Y());
	else
	    sprintf_s (zBuf, 128, "Xs %3d Y %3d", drCoo.x, drCoo.y);

    gScreens.PrintStatusInBar(CScreens::SBAR_POS, zBuf);

    if (bDebugPrintStatus && mpfLog)
        fprintf (mpfLog, "<CDisplayRectangle::PrintStatusBar> dataCoo [%.2f, %.2f] %s\n", 
        dataCoo.fx, dataCoo.fy, dataCoo.IsOnRaster() ? "In" : "Out");

    CString specValue = pDataForDisplay->GetSpecialString(dataCoo);
    gScreens.PrintStatusInBar(CScreens::SBAR_TYPED, specValue);

	if (!gScreens.mbEbwMode || !mpCurrentData->IsCtImage())
	{
		CString sValue(_T(" "));
		if (pDataForDisplay)
			sValue = pDataForDisplay->GetValueString(dataCoo);
		gScreens.PrintStatusInBar (CScreens::SBAR_DATA, sValue);
	    if (bDebugPrintStatus && mpfLog)
		    fprintf (mpfLog, "<CDisplayRectangle::PrintStatusBar> value %s\n", (const char *)sValue);
	}

    PrintStatusStatistics(dataCoo);

    if (!gScreens.mbEbwMode && pDataForDisplay->GetPosition())
    {
        sprintf_s(zBuf, 128, "%s %s", (const char*)pDataForDisplay->GetPageText(), (const char*)pDataForDisplay->GetPosition()->GetText());
        gScreens.PrintStatusInBar (CScreens::SBAR_PAGE, zBuf);
    }

    DisplayWindowStatus();
    if (bDebugPrintStatus && mpfLog)
    {
        fprintf (mpfLog, "\n");
        fflush (mpfLog);
    }
}
CDRCenteredCoordinates CDisplayRectangle::GetStartCoo()
{
    int centerX = (right - left) / 2;
    int centerY = (bottom - top) / 2;
    return CDRCenteredCoordinates (mpCurrentData->GetZoomParams(), -centerX, -centerY);
}
CDRCenteredCoordinates CDisplayRectangle::GetEndCoo()
{
    int centerX = (right - left - 1) / 2;
    int centerY = (bottom - top - 1) / 2;
    return CDRCenteredCoordinates (mpCurrentData->GetZoomParams(), centerX, centerY);
}
CDataCoordinates CDisplayRectangle::GetDataStartCoo()
{
    CDRCenteredCoordinates drStart = GetStartCoo();
    CDataCoordinates dataStart (mpCurrentData->CurPage());
    drStart.UpdateData(dataStart);
    return dataStart;
}
CDataCoordinates CDisplayRectangle::GetDataEndCoo()
{
    CDRCenteredCoordinates drEnd = GetEndCoo();
    CDataCoordinates dataEnd (mpCurrentData->CurPage());
    drEnd.UpdateData(dataEnd);
    return dataEnd;
}
CRect CDisplayRectangle::GetDisplayRect(void)
{
    return CRect (0,0, right-left, bottom-top);
}
int CDisplayRectangle::GetRasterLen(void)
{
    return (right - left) * (bottom - top);
}
void CDisplayRectangle::DrawLineOnScreen(CDC &dc, CScreenCoordinates p1, CScreenCoordinates p2, COLORREF color)
{
    CPen Pen(PS_SOLID, 1, color);
    dc.SelectObject(&Pen);

	POINT aPoints[2];

    aPoints[0].x = p1.x;
    aPoints[0].y = p1.y;

	aPoints[1].x = p2.x;
    aPoints[1].y = p2.y;

    dc.Polyline (aPoints, 2);
}
void CDisplayRectangle::DrawLine(CDC &dc, CDRCoordinates p1, CDRCoordinates p2, COLORREF color)
{
    // Clip to rectangle
    if (!ClipCoo (p1.x, p1.y, p2.x, p2.y, right - left))
        return;
    if (!ClipCoo (p2.x, p2.y, p1.x, p1.y, right - left))
        return;
    if (!ClipCoo (p1.y, p1.x, p2.y, p2.x, bottom - top))
        return;
    if (!ClipCoo (p2.y, p2.x, p1.y, p1.x, bottom - top))
        return;

    CScreenCoordinates prev (*this, p1);
    CScreenCoordinates next (*this, p2);
    DrawLineOnScreen(dc, prev, next, color);
}
void CDisplayRectangle::DrawFrame(CDC &dc, int delta, COLORREF color)
{
    int xLen = (right - left) / 3;
    int yLen = (bottom - top) / 3;
    int left1 = left +  delta;
    int top1 = top + delta;
    int bottom1 = bottom - delta - 2;
    int right1 = right - delta - 1;
    DrawLineOnScreen(dc, CScreenCoordinates(left1,top1), CScreenCoordinates(left1+xLen,top1), color);
    DrawLineOnScreen(dc, CScreenCoordinates(left1,top1), CScreenCoordinates(left1,top1+yLen), color);
    DrawLineOnScreen(dc, CScreenCoordinates(right1-xLen,bottom1), CScreenCoordinates(right1,bottom1), color);
    DrawLineOnScreen(dc, CScreenCoordinates(right1,bottom1-yLen), CScreenCoordinates(right1,bottom1), color);

}
bool CDisplayRectangle::ClipCoo(LONG & x, LONG & y, const LONG x1, const LONG y1, const LONG maxX)
{
    if (x < 0)
    {
        if (x1 < 0)
            return false;

        LONG dx = x1 - x;
        LONG dy = y1 - y;
        float ratio = (float)(0 - x) / (float)dx;
        y += (LONG)(dy * ratio +0.5f);
        x = 0;
    }
    else if (x >= maxX)
    {
        if (x1 >= maxX)
            return false;

        LONG dx = x1 - x;
        LONG dy = y1 - y;
        float ratio = (float)((maxX - 1) - x) / (float)dx;
        y += (LONG)(dy * ratio +0.5f);
        x = maxX - 1;
    }
    return true;
}
bool CDisplayRectangle::ScrollLineLeft(void)
{
    if (mpCurrentData && mpCurrentData->GetPosition())
    {
        bool bChange = mpCurrentData->GetPosition()->LineLeft();
        return bChange;
    }
    return false;
}
bool CDisplayRectangle::ScrollLineRight(void)
{
    if (mpCurrentData && mpCurrentData->GetPosition())
    {
        bool bChange = mpCurrentData->GetPosition()->LineRight();
        return bChange;
    }
    return false;
}
void CDisplayRectangle::OnViewGotopage(void)
{
    if (!mpCurrentData || !mpCurrentData->GetPosition())
        return;

    CPosition *pPos = mpCurrentData->GetPosition();
    if (pPos->UseSpecialGoto())
    {    
        pPos->OnViewGotopage();
        return;
    }

    CGoToPage goToPage (mpCurrentData);
    if (goToPage.DoModal() == IDOK)
    {
		if (goToPage.mbChanged)
            gScreens.Refresh();
	}
}
CDataCoordinates CDisplayRectangle::SetSelectedPoint(CPoint point)
{
    CDRCoordinates drCoo(*this, CScreenCoordinates(point));
    SetSelectedPoint(drCoo);

    CDataCoordinates dataCoo (mpCurrentData->CurPage());
    drCoo.UpdateData(GetZoomParams(), dataCoo);
    return dataCoo;
}

void CDisplayRectangle::OnViewZoom(void)
{
    CZoomParamsDlg dlg (mpCurrentData, mpCurrentData->GetZoomParams());
    dlg.DoModal();
}

void CDisplayRectangle::SaveState(CXMLDump & xmlDump)
{
    CXMLDumpScope scope("DisplayRectangle",xmlDump);
    CDependant::SaveState(xmlDump);

    {
        CXMLDumpScope graph("rectangle",xmlDump);
        xmlDump.Write("left", left);
        xmlDump.Write("right", right);
        xmlDump.Write("top", top);
        xmlDump.Write("bottom", bottom);
    }

    if (mbDisableStateRestore || mDataList.IsEmpty())
        xmlDump.Write("bDisableStateRestore", mbDisableStateRestore);

    xmlDump.Write("n_datas",(int)mDataList.GetSize());

    {
        CXMLDumpScope graph("data",xmlDump);
        POSITION pos = mDataList.GetHeadPosition();
        while (pos)
        {
            CMetaData *pData = mDataList.GetNext(pos);
            pData->SaveState(xmlDump);
        }
    }

    if (mGraphicList.GetSize() > 0)
    {
        CXMLDumpScope graph("graphic_elements",xmlDump);
        POSITION pos = mGraphicList.GetHeadPosition();
        while (pos)
        {
            CGraphicElementOnDR *pGEonDR = mGraphicList.GetNext(pos);
            pGEonDR->SaveState(xmlDump);
        }
    }
    if (mColorersList.GetSize() > 0)
    {
        CXMLDumpScope graph("colorers",xmlDump);
        POSITION pos = mColorersList.GetHeadPosition();
        while (pos)
        {
            CMetaData *pColorer = mColorersList.GetNext(pos);
            pColorer->SaveState(xmlDump);
        }
    }
    if (mTextList.GetSize() > 0)
    {
        CXMLDumpScope graph("texts",xmlDump);
        POSITION pos = mTextList.GetHeadPosition();
        while (pos)
        {
            CDRText *pText = mTextList.GetNext(pos);
            pText->SaveState(xmlDump);
        }
    }
}
CGCursor * CDisplayRectangle::AddCursor(const char * zName, COLORREF color)
{
    CGCursor *pCursor = new CGCursor(mpCurrentData, this, zName, color);
    if (AddGraphicElement(pCursor))
	    return pCursor;
	delete pCursor;
	return NULL;
}
CGLine * CDisplayRectangle::AddLine(const char * zName, COLORREF color)
{
    CGLine *pLine = new CGLine(this, zName, color);
    AddGraphicElement(pLine);
    return pLine;
}
CGRoi * CDisplayRectangle::AddRoi(const char * zName, COLORREF color, int sizeMm)
{
    CGRoi *pRoi = new CGRoi(this, zName, color, sizeMm);
    if (AddGraphicElement(pRoi))
	    return pRoi;
	delete pRoi;
	return NULL;
}
CGEllipse * CDisplayRectangle::AddEllipse(const char * zName, COLORREF color, bool bCircle, int sizeMm)
{
    CGEllipse *pEllipse = new CGEllipse(this, zName, color, sizeMm);
    if (bCircle)
        pEllipse->SetCircle();
    if (AddGraphicElement(pEllipse))
	    return pEllipse;
	delete pEllipse;
	return NULL;
}
CRingID * CDisplayRectangle::AddScanCenter(CXMLParseNode *pGraphicsXml)
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData || !pData->IsDicom())
	{
		pData = mpCurrentData;
		if (!pData || !pData->IsDicom())
		{
			pData = gScreens.GetFirstData();
			if (!pData || !pData->IsDicom())
				return NULL;
		}
	}

	CRingID *pRingID = CRingID::CreateRingID(this, pData);
	if (!pRingID)
		return NULL;

	if (pGraphicsXml)
		pRingID->InitFromXml(NULL,pGraphicsXml);
    AddGraphicElement(pRingID);
	return pRingID;
}
void CDisplayRectangle::AddUserText(void)
{
    if (!mpSelectedPoint)
        GetSelectedDataCoo();
    if (!mpSelectedPoint)
        return;

    CUserTextDialog dlg;
    if (dlg.DoModal() == IDOK)
    {
		CDRText *pText = new CDRText(mpCurrentData, this);
		pText->SetText(dlg.GetText());
		AddGraphicElement(pText);
        gScreens.Refresh();
    }
}
void CDisplayRectangle::AddContour(const char * zName, COLORREF color)
{
    if (!mpSelectedPoint)
        GetSelectedDataCoo();
    if (!mpSelectedPoint)
	{
		CMyWindows::MessBox("Please click on the contour's starting point before choosing 'Add Contour'",
			"Operational Advice");
        return;
	}
    CGContour *pContour = new CGContour(this, zName, color);
    AddGraphicElement(pContour);
}
void CDisplayRectangle::DeleteGE(CGraphicElement *pGE)
{
	if (mGraphicList.IsEmpty())
		return;

	bool bFound = false;
	POSITION pos = mGraphicList.GetHeadPosition();
	while (pos)
	{
		CGraphicElementOnDR *pGEOnDR = mGraphicList.GetNext(pos);
		if (pGEOnDR->GetGE() == pGE)
		{
			bFound = true;
			break;
		}
	}
	if (!bFound)
		return;

    CDisplayGE *pDisplayGE = pGE->GetDisplayGE();
    if (pDisplayGE && pDisplayGE->RemoveDisplay(this) == 0)
        delete pDisplayGE;
}
void CDisplayRectangle::OnGraphicDelete(void)
{
    if (mpCurrentGE)
    {
        DeleteGE(mpCurrentGE->GetGE());
        mpCurrentGE = NULL;
        gScreens.Refresh();
    }
}
void CDisplayRectangle::OnGraphicDisplayonallimages(void)
{
    if (mpCurrentGE)
    {
        if (gScreens.DisplayOnAllRectangles(mpCurrentGE->GetGE()))
            gScreens.Refresh();
    }
}
void CDisplayRectangle::OnGraphicSetcolor(void)
{
    if (mpCurrentGE)
    {
        CColorDialog dlg;
        if (dlg.DoModal() == IDOK)
        {
            COLORREF color = dlg.GetColor();
            mpCurrentGE->GetGE()->SetColor(color);
            gScreens.Refresh();
        }
    }
}
void CDisplayRectangle::OnGraphicRename(void)
{
    if (!mpCurrentGE)
		return;
	CUserTextDialog dlg("Enter new name");
	dlg.msText = mpCurrentGE->GetName();
	if (dlg.DoModal() == IDOK)
	{
		mpCurrentGE->SetName(dlg.GetText());
	}
}
void CDisplayRectangle::OnGraphicSetText(void)
{
    if (!mpCurrentGE)
		return;
	CGraphicElement *pGE = mpCurrentGE->GetGE();
	CUserTextDialog dlg("Enter text");
	dlg.msText = pGE->GetText();
	if (dlg.DoModal() == IDOK)
	{
		pGE->SetText(dlg.GetText());
		gScreens.Refresh();
	}
}
bool CDisplayRectangle::OnGraphicSavetofile(void)
{
    if (!mpCurrentGE)
		return false;
	CGraphicElement *pGE = mpCurrentGE->GetGE();
    if (!pGE)
		return false;
	return pGE->SaveToFile();
}
bool CDisplayRectangle::OnGraphicSaveData2Excel(void)
{
    if (!mpCurrentGE)
		return false;
	CGraphicElement *pGE = mpCurrentGE->GetGE();
    if (!pGE)
		return false;
	CGraphicsToExcel g2e(pGE);
	return g2e.Print();
}
void CDisplayRectangle::OnGraphicDisplayStatistics(void)
{
    if (!mpCurrentGE)
		return;
	CGraphicElement *pGE = mpCurrentGE->GetGE();
	pGE->ToggleDisplayStatistics();
	gScreens.Refresh();
}
void CDisplayRectangle::AddRefBetween(CDisplayRectangle * pOtherDR)
{
    if (mpCurrentData && pOtherDR->mpCurrentData)
    {
        mpCurrentData->AddRefBetween (pOtherDR->mpCurrentData);
        pOtherDR->mpCurrentData->AddRefBetween (mpCurrentData);
    }
}
void CDisplayRectangle::DeleteGraphics(bool bOnDataEclipse)
{
    int nSave = 0;
    while (mGraphicList.GetSize() > nSave)
    {
        nSave = 0;
        POSITION pos = mGraphicList.GetHeadPosition();
        while (pos)
        {
            CGraphicElementOnDR *pGEOnDR = mGraphicList.GetNext(pos);
            CGraphicElement *pGE = pGEOnDR->GetGE();
            if (!pGE)
				nSave++;
			else if (!bOnDataEclipse || pGE->DeleteOnDataEclipse())
            {
                DeleteGE(pGE);
                break;
            }
            else
                nSave++;
        }
    }
}
void CDisplayRectangle::OnDataEclipse(void)
{
	DeleteGraphics(true);
}
float CDisplayRectangle::GetMinScreenSizeMm(void)
{
    float mmPerPixel = mpCurrentData->GetZoomParams()->MmPerPixel();
    int width = right - left;
    int height = bottom - top;
    int minSize = min (width, height);
    return minSize * mmPerPixel;
}

bool CDisplayRectangle::OnKillFocus(void)
{
    bool bChanged = false;
    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pElement = mGraphicList.GetNext(pos);
        bChanged |= pElement->OnKillFocus ();
    }

    pos = mRefList.GetHeadPosition();
    while (pos)
    {
        CMetaData *pData = mRefList.GetNext(pos);
        bChanged |= pData->OnMousePosNotOnRef ();
    }
    return bChanged;
}
CZoomPanParams * CDisplayRectangle::GetZoomParams(void)
{
    if (mpCurrentData)
        return mpCurrentData->GetZoomParams();
    return NULL;
}
CZoomPanParams * CDisplayRectangle::GetCommonZoomParams(void)
{
    if (mpCurrentData)
        return mpCurrentData->GetCommonZoomParams();
    return NULL;
}
class CPosition * CDisplayRectangle::GetPosition(void)
{
    if (!mpCurrentData)
        return NULL;
    return mpCurrentData->GetPosition();
}

void CDisplayRectangle::AddColorer(CMetaData * pColorer)
{
    mColorersList.AddTail(pColorer);
}

void CDisplayRectangle::AddText(CDRText * pText)
{
    POSITION pos = mTextList.GetHeadPosition();
    while (pos)
    {
        CDRText *pOldText = mTextList.GetNext(pos);
        if (*pOldText == *pText)
        {
            delete pText;
            return;
        }
    }
    mTextList.AddTail(pText);

	static int ubOnce = false;
	if (!ubOnce)
	{
		ubOnce = true;
		gScreens.mpImageView->AddMenuItem("&Graphic", ID_GRAPHIC_DELETEALLTEXTS, "Delete All Texts");
	}
}
bool CDisplayRectangle::DeleteText(int id)
{
	CString sText;
	sText.LoadString(id);
	return DeleteText(sText);
}
bool CDisplayRectangle::DeleteText(const char * zPrefix)
{
    int len = (int)strlen(zPrefix);

    POSITION pos = mTextList.GetHeadPosition();
    while (pos)
    {
        POSITION textPos = pos;
        CDRText *pText = mTextList.GetNext(pos);
        if (pText->msText.Left(len) == zPrefix)
        {
            delete pText;
            mTextList.RemoveAt(textPos);
            return true;
        }
    }
    return false;
}
void CDisplayRectangle::DisplayTextItem(CDRText *pText, CDC * pDC)
{
	if (mpfLog && umDebugText)
		fprintf(mpfLog,"<DisplayTextItem> text is <%s>\n", (const char*)pText->msText);
	if (pText->msText.IsEmpty())
		return;

    int textLeft = left + 10;
    int textTop = top + 10;
	bool bTextIsTopAligned = true;

    if (pText->miLine > 0)
        textTop = top + pText->miLine;
    else if (pText->miLine < 0)
	{
        textTop = bottom - pText->miLine - umTextHeight - umTextHeight;
		bTextIsTopAligned = false;
	}

	if (bTextIsTopAligned)
	{
		if (umbDisplayTitles)
			textTop += umTextHeight;

		if (umbDisplayDataRange)
			textTop += umTextHeight;
	}

	CString sText;
	if (!pText->msTitle.IsEmpty())
	{
		sText = pText->msTitle + ": ";
		sText += pText->msText;
	}
	else if (pText->miTitle)
	{
		CString sTitle;
		sTitle.LoadString(pText->miTitle);
		sText = sTitle + ": ";
		sText += pText->msText;
	}
	else if (pText->miRightTitle)
	{
		CString sTitle;
		sTitle.LoadString(pText->miRightTitle);
		sText = pText->msText + " ";
		sText += sTitle;
	}
	else
		sText = pText->msText;

	// Compute text rectangle
    int textWidth = sText.GetLength() * umTextCharWidth;
    if (pText->miCol > 0)
        textLeft = left + pText->miCol;
    else if (pText->miCol < 0)
	{
		if (pText->mbAbsoluteColumn)
			textLeft = right + pText->miCol;
		else
	        textLeft = right + pText->miCol - textWidth;
	}
    CRect rect (textLeft, textTop, textLeft + textWidth, textTop + umTextHeight);

    pDC->DrawText(sText, rect, 0);
	if (mpfLog && umDebugText)
		fprintf(mpfLog,"<DisplayTextItem> Drawt Text <%s>\n", (const char*)sText);
}
void CDisplayRectangle::DisplayTitleText(CDC * pDC, const char *zText)
{
	int textRignt = max(left + 10, right - 10);
	CString sText(zText);
	int textWidth = textRignt - left + 1;
	int maxChars = (int)(textWidth * umCharFracPerTextWidth);
	if (sText.GetLength() > maxChars)
		sText = sText.Right(maxChars);

	int textTop = top + umTextHeight * mnTitleLinesDisplayed;
    CRect rect (left, textTop, textRignt, textTop + umTextHeight);

    pDC->DrawText(sText, rect, 0);
	mnTitleLinesDisplayed++;
	if (mpfLog && umDebugText)
		fprintf(mpfLog,"<DisplayTitleText> %d <%s>\n", mnTitleLinesDisplayed, (const char*)sText);
}
bool CDisplayRectangle::DisplayTextOfCurrentImage(CDC * pDC)
{
	if (!mpCurrentData->IsFile())
	{
		msNameOfCurrentImageWithText.Empty();
		return false;
	}

	CString sfName = mpCurrentData->Name();
	if (sfName != msNameOfCurrentImageWithText)
	{
		gTexts.SetPathImageText(mpCurrentData, mCurrentImageTextList);
		msNameOfCurrentImageWithText = sfName;
	}

	if (mCurrentImageTextList.IsEmpty())
		return false;

	POSITION pos = mCurrentImageTextList.GetHeadPosition();
	while (pos)
	{
		CDRText *pText = mCurrentImageTextList.GetNext(pos);
		DisplayTextItem(pText, pDC);
	}
	return true;
}
void CDisplayRectangle::DisplayDataRange(CDC * pDC)
{
	CString sText;
	if (mpCurrentData)
		if (mpCurrentData->GetDataRangeText(sText))
		{
			DisplayTitleText(pDC, sText);
		}
}
void CDisplayRectangle::DisplayTitle(CDC * pDC)
{
	CString sTitle = msUserDefinedTitle;
	if (mpCurrentData)
		sTitle = mpCurrentData->GetDRTitle();
	if (sTitle.IsEmpty())
	{
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf),"Image %d", mDRId + 1);
		sTitle = zBuf;
	}
	if (!sTitle.IsEmpty())
		DisplayTitleText(pDC, sTitle);
}
bool CDisplayRectangle::DisplayText(CDC * pDC)
{
	if (mpfLog && umDebugText)
		fprintf(mpfLog,"<DisplayText>\n");

	if (!mbEnableText)
		return false;

    int n = 0;
	mnTitleLinesDisplayed = 0;
	if (umbDisplayTitles)
		DisplayTitle(pDC);
	if (umbDisplayDataRange)
		DisplayDataRange(pDC);

	if (mpCurrentData)
	{
		if (DisplayTextOfCurrentImage(pDC))
		{
			if (mpfLog && umDebugText)
				fprintf(mpfLog,"<DisplayTextOfCurrentImage> returned yes -> return\n");
			return true;
		}
	}

    POSITION pos = mTextList.GetHeadPosition();
    while (pos)
    {
        CDRText *pText = mTextList.GetNext(pos);
		if (!pText->mpData || (pText->mpData == mpCurrentData))
		{
			DisplayTextItem(pText, pDC);
		    n++;
		}
    }
	if (mpCurrentData)
	{
		CMetaData *pData = mpCurrentData->GetDataWithText();

		CList<class CDRText*,class CDRText*> *pDataTextList = pData->GetTextList();
		if (!pDataTextList->IsEmpty())
		{
			if (mpfLog && umDebugText)
				fprintf(mpfLog,"pDataTextList %d items\n", (int)pDataTextList->GetSize());
			pos = pDataTextList->GetHeadPosition();
			while (pos)
			{
				CDRText *pText = pDataTextList->GetNext(pos);
				DisplayTextItem(pText, pDC);
				n++;
			}
		}
	}
	if (mpfLog && umDebugText)
		fprintf(mpfLog,"<DisplayText> END n %d\n\n", n);
    return n > 0;
}
CMyImage * CDisplayRectangle::GetCurrentDataPage(void)
{
    if (mpCurrentData)
        return mpCurrentData->PageData();
    return NULL;
}
bool CDisplayRectangle::SetDRCurrent(CPoint point)
{
    bool bCurrent = (PtInRect (point) != 0);
    if (bCurrent == mbIsCurrent)
        return false;
    mbIsCurrent = bCurrent;
    return true;
}
bool CDisplayRectangle::DeleteCurrentData(void)
{
	POSITION pos = mDataList.GetHeadPosition();
	while (pos)
	{
		POSITION lastPos = pos;
		CMetaData *pData = mDataList.GetNext(pos);
		if (pData == mpCurrentData)
		{
			delete pData;
			mDataList.RemoveAt(lastPos);
			if (pos)
				mpCurrentData = mDataList.GetNext(pos);
			else if (!mDataList.IsEmpty())
				mpCurrentData = mDataList.GetHead();
			else
				mpCurrentData = NULL;
		}
	}
	return mpCurrentData != NULL;
}
bool CDisplayRectangle::DeleteData(const char * zName)
{
	POSITION pos = mDataList.GetHeadPosition();
	while (pos)
	{
		POSITION lastPos = pos;
		CMetaData *pData = mDataList.GetNext(pos);
		if (pData->Is(zName))
		{
			delete pData;
			mDataList.RemoveAt(lastPos);
			if (pData == mpCurrentData)
			{
				if (pos)
					mpCurrentData = mDataList.GetNext(pos);
				else if (!mDataList.IsEmpty())
					mpCurrentData = mDataList.GetHead();
				else
					mpCurrentData = NULL;
			}
			return true;
		}
	}
	return false;
}
void CDisplayRectangle::UpdateDataCheck(void)
{
	if (mpCurrentData)
	{
		if (mpfLog)
			fprintf(mpfLog,"<UpdateDataCheck> %s\n", (const char*)mpCurrentData->Name());

		unsigned long updateId = mpCurrentData->miUpdate;
		mpCurrentData->UpdatePageData ();
		if (updateId != mpCurrentData->miUpdate)
		{
			POSITION pos = mRefList.GetHeadPosition();
			while (pos)
				mRefList.GetNext(pos)->OnRefDataUpdate();

			pos = mGraphicList.GetHeadPosition();
			while(pos)
				mGraphicList.GetNext(pos)->OnDataUpdate();

			OnChange();
		}
	}
}
void CDisplayRectangle::DeleteTexts(void)
{
	while (!mTextList.IsEmpty())
    {
        CDRText *pText = mTextList.GetTail();
        delete pText;
        mTextList.RemoveTail();
    }
}
bool CDisplayRectangle::SpecialMouseScroll(CPoint point, bool & obChanged)
{
	if (!mpCurrentData)
		return false;
	return mpCurrentData->SpecialMouseScroll(point,obChanged);
}
void CDisplayRectangle::OnMultiDimScroll(int iDim)
{
	POSITION pos = mDataList.GetHeadPosition();
	while (pos)
	{
		CMetaData *pData = mDataList.GetNext(pos);
		if (pData != mpCurrentData)
			pData->PropagateLeaf(mpCurrentData,iDim);
	}
}
bool CDisplayRectangle::FillGrayRaster(CMetaData *pData, CTImage<unsigned char> &grayRaster, bool bColorer)
{
	float *pZoomed = pData->GetFloatZoomedRasterData();
	if (!pZoomed)
		return false;

	unsigned char *pGray = grayRaster.GetData();

	// Compute gray level according to windiow
	SRunTimeWindowParams runTimeWindowParams;
	runTimeWindowParams.Init(*pData->GetWindowParams(), bColorer);
	runTimeWindowParams.SetGrayRaster(pGray, pZoomed, grayRaster.GetNPixels());
	return true;
}
CTImage<unsigned char> &CDisplayRectangle::GetGrayRasterForColorer(void)
{
	mGrayRasterForColorer.SetSizeLike(mGrayRaster);
	return mGrayRasterForColorer;
}
bool CDisplayRectangle::HasDataColoring(void)
{
	if (mbEnableColorers)
	{
		if (!mColorersList.IsEmpty())
		{
			CMetaData *pData = mColorersList.GetHead();
			if (pData->IsDataForColoring())
				return true;
		}
	}
    return false;
}
void CDisplayRectangle::ReadRegistry(CMyRegistry &myReg)
{
	myReg.Get("bDataCoo", umbPrintDataCoo);
	myReg.Get("DisplayRectangleDebug", umDebug);
	myReg.Get("DisplayRectangleDebugText", umDebugText);
	myReg.Get("bSaveZoomedRasterAsShort", umbSaveZoomedRasterAsShort);
	myReg.Get("WrapOnNextAndPrev", umbWrapOnNextAndPrev);
	myReg.Get("bReverseRGBOrder", umbReverseRGBOrder);
}
bool CDisplayRectangle::SetDisplayWidthToAllData(int n)
{
	bool bAny = false;
    POSITION pos = mDataList.GetHeadPosition();
    while (pos)
    {
        CMetaData *pData = mDataList.GetNext(pos);
		if (!pData->IsDiff())
		{
			if (pData->SetDisplayWidth(n))
				bAny = true;
		}
	}
	return bAny;
}
void CDisplayRectangle::SetTitleByUser(bool bToData)
{
	CString sText;
	if (bToData)
	{
		if (!mpCurrentData)
			return;
		sText = mpCurrentData->msUserDefinedTitle;
	}
	else
		sText = msUserDefinedTitle;

	CString sTitle(_T("Select title for "));
	if (bToData)
		sTitle += "current data";
	else
		sTitle += "rectanle";

	if (!CNameGetDialog::GetStringValue(sTitle, sText))
		return;

	while (!sText.IsEmpty() && (sText[0] == ' '))
		sText = sText.Right(sText.GetLength()-1);

	//if (sText.IsEmpty())
	//	return;
	// Empty text removes special titles!

	if (bToData)
	{
		if (mpCurrentData)
			mpCurrentData->msUserDefinedTitle = sText;
	}
	else
		msUserDefinedTitle = sText;
	umbDisplayTitles = true;
	gScreens.Refresh();
}
int CDisplayRectangle::AddGESToList(CGraphicsToExcel &g2e, unsigned int listId)
{
	int nAdded = 0;
    POSITION pos = mGraphicList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pElement = mGraphicList.GetNext(pos);
		if (pElement->IsRelevant(mpCurrentData))
        {
			CGraphicElement *pGE = pElement->GetGE();
			if (pGE->IsRoi())
			{
				if (pGE->mListId != listId)
                {
                    pGE->mListId = listId;
					g2e.Add(pGE);
					nAdded++;
                }
			}
        }
    }
	return nAdded;
}