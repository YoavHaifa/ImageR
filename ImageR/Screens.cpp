#include "StdAfx.h"
#include "Screens.h"
#include ".\DisplayRectangle.h"
#include "MetaData.h"
#include "TypedData.h"
#include "ImageView.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\ExactTimer.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\MyTracer.h"
#include "..\..\yUtils\DicomCompare.h"
#include "..\..\yUtils\CodeFile.h"
#include "..\..\yUtils\DeleteDlg.h"
#include "..\..\yUtils\MyRegistry.h"
#include "..\..\yUtils\MyMenu.h"

#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\GraphicElement.h"
#include "..\..\ImageRLib\GraphicToExcel.h"

#include "RecentFiles.h"
#include "GraphicElementOnDR.h"
#include "ArchivesFile.h"
#include "ContextMenu.h"
#include "Oblique.h"
#include "DiffData.h"

CString CScreens::umsScreenTitle("ImageR");
CString CScreens::umsVersion("V1.4.4");

CScreens gScreens;
float CScreens::MOUSE_ZOOM_FACTOR = 500.0f;
int CScreens::umDebug = 0;

CString CScreens::umsRawScreenTitle("");
CString CScreens::umsImageScreenTitle("");
CString CScreens::umsDisclaimer("Beta Version");
bool CScreens::umbDisplayDisclaimer = true;
bool CScreens::umbDebugColorMap = false;

CScreens::CScreens(void)
: CImageRIFBase ("Screens")
, mpfLog(NULL)
, mpCurrentDR(NULL)
, mpColorsRaster(NULL)
, mnAllocated(0)
, mpStatusBar (NULL)
, mbMouseOnMe(false)
, mbDebugData(false)
, mDebugColor(0)
, mPrevMouseFlags (0)
, mnRectsInRow(1)
, mnRectsRows(1)
, mbDebugMouse(false)
, mpXmlParse(NULL)
, mMainThreadId(GetCurrentThreadId())
, mnDatasUpdated(0)
, mpWaitKillFocusDR(NULL)
, mbCaptureSet(false)
, mbDataSwitched(false)
, mbSaveOnDataChange(false)
, mVolumeRefreshInterval(0)
, mbServer(false)
, mbReplace(false)
, mWantedSize(0,0)
, mbShowScanCenter(false)
, mnMaxRectanglesInRow(0)
, mbMaxRectanglesInRowSetByUser(false)
, mbIgnoreServerTimingProblems(true)
, mbKeepScreenSizeConstant(false)
, mCritical("MVScreenCS")
, mbLeftMouseDoingGraphics(false)
, mbEbwMode(false)
, mbOpenContextMenuOnRButtonUp(false)
, mbDisablePaint(false)
, miPipeMessageToHandle(0)
, miPipeMessageHandled(0)
, mbAllowPaging(true)
, mbCallGlobalClean(false)
, mnBitsPerPixel(0)
, mbSetNewPositionOnSameSeries(true)
, mbDisplayMinMaxAtWindowStatus(false)
, mbUpdateStatusBarOnNextPaint(false)
, mbCommonDisplayWidth(true)
, mnCommonDisplayWidth(1)
{
	mRButtonDownPoint.x = -1000;
	mRButtonDownPoint.y = -1000;

    mImageSize.cx = 0;
    mImageSize.cy = 0;

    mSaveDoneMsg = RegisterWindowMessage("ImageRSaveDone");
    mCursorLocationMsg = RegisterWindowMessage("ImageRCursorLocation");
	mImageRLeftMouseUpMsg = 0;
    mWindowMsg = RegisterWindowMessage("ImageRWindowMsg");

    for (int iCol = 0; iCol <= MAX_COLORS; iCol++)
        maGrayRGBTable[iCol] = RGB(iCol, iCol, iCol);

    for (int iCol = 0; iCol <= MAX_COLORS; iCol++)
	{
		int green = abs (iCol * 2 - MAX_COLORS);
        maColorsRGBTable[iCol] = RGB(MAX_COLORS - iCol, 
			green, 
			iCol);
	}
}
CScreens::~CScreens(void)
{

    if (mpColorsRaster)
        delete mpColorsRaster;

    if (mpfLog)
        fclose (mpfLog);

	CDicomCodeFile::Clean();

	if (mbCallGlobalClean)
		CMyWindows::GlobalCleanOnExit();
}
void CScreens::InitLeftMouseUpMsg(void)
{
	if (!mImageRLeftMouseUpMsg)
		mImageRLeftMouseUpMsg = RegisterWindowMessage("ImageRLeftMouseUp");
}
void CScreens::OnNewRect(void)
{
    SetCurrentDR(mRectanglesList.GetTail());
    CMetaData *pData = mpCurrentDR->GetCurrentData();
    int nRectangles = (int)mRectanglesList.GetSize();

    if (mpfLog)
    {
        fprintf (mpfLog, "<OnNewRect> %s - n %d\n", pData ? pData->z_name() : "NO DATA", nRectangles);
        fflush (mpfLog);
    }

    if (nRectangles == 1)
        SetFirstImageSize();
    else
         OnExtraNewRectangleAdjustSize(1); // Set Screen Size

    if (mpfLog)
    {
        fprintf (mpfLog, "<OnNewRect> End %s\n", pData ? pData->z_name() : "NO DATA");
        fflush (mpfLog);
    }
}
void CScreens::OpenLogFile(void)
{
	if (mpfLog)
		return;

	CString sfLogName("MVScreen_");
	sfLogName += umsVersion;
	sfLogName += ".log";
	mpfLog = CMyWindows::FOpenLogFile (sfLogName);
}
void CScreens::AddData(CMetaData *pNewData, bool bAddDisplay)
{
    if (!pNewData)
        return;
	if (mpCurrentDR)
		CheckForRepeatedDataOrAdjustPosition(pNewData);

	CDisplayRectangle *pRect = mpCurrentDR;
    if (bAddDisplay || !mpCurrentDR)
    {
        pRect = new CDisplayRectangle(mpfLog);
        mRectanglesList.AddTail(pRect);
        pRect->AddData(pNewData);
        OnNewRect();
		DisplayGlobalGraphicsOnNewRect(pRect);
    }
    else
	{
		if (mbReplace)
			mpCurrentDR->DeleteCurrentData();
	    mpCurrentDR->AddData(pNewData);
	}

    if (mRectanglesList.GetSize() > 1)
        AddRefToNewData();

    if (pNewData->SaveToRecentFiles())
		gRecentFiles.update(pNewData->NameToRecentFiles());

	if (mbShowScanCenter)
		pRect->AddScanCenter();

	CMyDicom *pDicom = pNewData->GetDicom();
	if (pDicom)
	{
		if (pDicom->IsCompressed())
		{
			static bool bOnce = false;
			if (!bOnce)
			{
				bOnce = true;
				mpImageView->AddMenuItem("Dicom", ID_DICOM_SAVEUNCOMPRERSSED, "Save Uncompressed");
			}
		}
		if (mRectanglesList.GetSize() >= 2)
		{
			CMetaData *pFirstData = mRectanglesList.GetHead()->GetCurrentData();
			if (pFirstData && pFirstData->GetDicom())
			{
				static bool bOnce = false;
				if (!bOnce)
				{
					bOnce = true;
					mpImageView->AddMenuItem("&Dicom", ID_DICOM_COMPARE, "Compare");
				}
			}
		}
	}

	if (mbCommonDisplayWidth && (mnCommonDisplayWidth > 1))
		pNewData->SetDisplayWidth(mnCommonDisplayWidth);
}
void CScreens::PrintWindowRect(const char *zText, FILE *pf)
{
	if (!pf)
		pf = mpfLog;
	if (pf)
	{
		RECT WndRect;
		mpImageView->GetWindowRect(&WndRect);
		fprintf (pf, "<%s> Window Rect x[%d - %d] y[%d - %d] size (x %d y %d)\n", zText,
			WndRect.left,WndRect.right, WndRect.top, WndRect.bottom,
			WndRect.right - WndRect.left, WndRect.bottom - WndRect.top);
		fprintf (pf, "cx image %d screen %d (%d)\n",
			mImageSize.cx, mScreenSize.cx, mScreenSize.cx - mImageSize.cx);
	}
}
void CScreens::OnExtraNewRectangleAdjustSize(int delta)
{
	static FILE *upfLog = NULL;
	if (!upfLog && ((umDebug & 0x10) != 0))
		upfLog = CMyWindows::FOpenLogFile("OnExtraNewRectangleAdjustSize");
	if (upfLog)
	{
		fprintf (upfLog, "\n<OnExtraNewRectangleAdjustSize> %+d\n", delta);
		PrintWindowRect("Before Change", upfLog);
	}

    int xOfRect = mImageSize.cx / mnRectsInRow;
    int yOfRect = mImageSize.cy / mnRectsRows;
	int oldNRectsInRow = mnRectsInRow;
	int oldNRectsRows = mnRectsRows;

	int nRects = (int)mRectanglesList.GetSize();
	if (nRects == 2 && delta == 1)
	{
		static bool bOnce = false;
		if (!bOnce)
		{
			bOnce = true;
			mpImageView->EnableMenuItem("Settings", ID_SETTINGS_NEWPOSITION);
		}
	}

    if (mnMaxRectanglesInRow && (nRects > mnMaxRectanglesInRow))
	{
		mnRectsRows = (nRects + mnMaxRectanglesInRow - 1) / mnMaxRectanglesInRow;
		mnRectsInRow = mnMaxRectanglesInRow;
	}
	else
	{
		mnRectsRows = 1;
		mnRectsInRow = nRects;
	}
	if (upfLog)
	{
		fprintf (upfLog, "\nnRects %d  mnRectsRows %d mnRectsInRow %d\n", 
			nRects, mnRectsRows, mnRectsInRow);
	}

    RECT WndRect;
    mpImageView->GetWindowRect(&WndRect);

    SIZE fullScreenSize;
    CMyWindows::GetPrimaryScreenSizeInPixels(fullScreenSize);

	// When ImageR winodw is integrated in another application
	//
	//CWnd* parentWindow = mpImageView->GetParent();
	HWND parentWindow = mpImageView->GetParentWindowHandle();
	if (parentWindow != NULL)
	{
		RECT parentRect;
		//parentWindow->GetWindowRect(&parentRect);
		if (::GetWindowRect(parentWindow, &parentRect))
		{
			fullScreenSize.cx = parentRect.right - parentRect.left;
			fullScreenSize.cy = parentRect.bottom - parentRect.top;
		}
	}
	
	fullScreenSize.cy -= 32; // Don't go to the lower end of the screen (tool bar)

	int wantedX = mScreenSize.cx;
	if (mnRectsInRow != oldNRectsInRow)
		wantedX = min (mScreenSize.cx + delta * xOfRect + 8, fullScreenSize.cx);
	int wantedY = mScreenSize.cy;
	if (mnRectsRows != oldNRectsRows)
		wantedY = min (mScreenSize.cy + delta * yOfRect + 8, fullScreenSize.cy);
	if (upfLog)
	{
		fprintf (upfLog, "mScreenSize x %d y %d \n", 
			mScreenSize.cx, mScreenSize.cy);
		fprintf (upfLog, "Wanted x %d y %d \n", 
			wantedX, wantedY);
	}

    if (WndRect.left + wantedX > fullScreenSize.cx)
        WndRect.left = max (0, fullScreenSize.cx - wantedX);
    if (WndRect.top + wantedY > fullScreenSize.cy)
        WndRect.top = max (0, fullScreenSize.cy - wantedY);
	if (upfLog)
	{
		fprintf (upfLog, "<OnExtraNewRectangleAdjustSize> Wanted x[%d %d] y[%d - %d] wanted %d %d\n",
			WndRect.left, WndRect.right, WndRect.top, WndRect.bottom, wantedX, wantedY);
	}

    if (WndRect.left + wantedX == WndRect.right) // && WndRect.top + wantedY == WndRect.bottom)
        SetScreenDivision();
	else
	{
		if (upfLog)
		{
			fprintf (upfLog, "<OnExtraNewRectangleAdjustSize> ERROR: Wanted x[%d %d] y[%d - %d] wanted %d %d\n",
				WndRect.left, WndRect.right, WndRect.top, WndRect.bottom, wantedX, wantedY);
		}
	}

	if (upfLog)
	{
		fprintf (upfLog, "<OnExtraNewRectangleAdjustSize> mImageSize.cx %d xOfRect %d wantedX %d (full.cx %d)\n",
			mImageSize.cx, xOfRect, wantedX, fullScreenSize.cx);
		fprintf (upfLog, "<OnExtraNewRectangleAdjustSize> mImageSize.cy %d yOfRect %d wantedY %d (full.cy %d)\n",
			mImageSize.cy, yOfRect, wantedY, fullScreenSize.cy);
	}

	if (mbKeepScreenSizeConstant)
	{
		this->OnSizeExtended(SIZE_MAXIMIZED, mScreenSize.cx, mScreenSize.cy, true);
	}
	else
	{
		if (mpfLog)
		{
			fprintf (mpfLog, "<OnExtraNewRectangleAdjustSize> MoveWindow\n");
			fprintf (mpfLog, "(left %d top %d cx %c cy %d)\n\n",
				WndRect.left, WndRect.top, wantedX, wantedY + EXTRA_VERTICAL_MARGINS);
		}
		mpImageView->MoveWindow(WndRect.left, WndRect.top, wantedX, wantedY + EXTRA_VERTICAL_MARGINS);
	}

	PrintWindowRect("After Change", upfLog);

	if (upfLog)
	{
		fprintf (upfLog, "<OnExtraNewRectangleAdjustSize> End, mImageSize.cx %d\n\n", mImageSize.cx);
		fflush(upfLog);
	}
}

CDisplayRectangle * CScreens::GetDisplayRectangle(int i)
{
    POSITION pos = mRectanglesList.FindIndex(i);
    if (!pos)
        return NULL;
    return mRectanglesList.GetAt(pos);
}
CMetaData * CScreens::GetData(int iDR)
{
	CDisplayRectangle *pDR = GetDisplayRectangle(iDR);
	if (!pDR)
		return NULL;
	return pDR->GetCurrentData();
}
CDisplayRectangle * CScreens::GetRectangleWithData(const char *zDataName)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *pData = pDR->GetCurrentData();
        if (pData->PermanentName() == zDataName)
            return pDR;
    }
    return NULL;
}
CMetaData * CScreens::GetCurrentData(void)
{
    if (!mpCurrentDR)
        return NULL;
    return mpCurrentDR->GetCurrentData();
}
CVolumetric * CScreens::GetCurrentVolumetricData(void)
{
	CMetaData *pData = GetCurrentData();
	if (pData->IsVolumetric())
	{
		CVolumetric *pVolumetric = dynamic_cast<CVolumetric *>(pData);
		return pVolumetric;
	}
	return NULL;
}
COblique * CScreens::GetCurrentObliqueData(void)
{
	CMetaData *pData = GetCurrentData();
	if (pData->IsOblique())
		return dynamic_cast<COblique *>(pData);
	return NULL;
}
CDiffData * CScreens::GetCurrentDiffData(void)
{
	CMetaData *pData = GetCurrentData();
	if (pData->IsDiff())
		return dynamic_cast<CDiffData *>(pData);
	return NULL;
}
CMetaData * CScreens::GetCurrentDataForWindowing(void)
{
    if (!mpCurrentDR)
        return NULL;
    return mpCurrentDR->GetCurrentDataForWindowing();
}
CMetaData * CScreens::GetCurrentDataForScroll(void)
{
	if (!mbAllowPaging)
		return NULL;
    if (!mpCurrentDR)
        return NULL;
    return mpCurrentDR->GetCurrentData();
}
CDisplayRectangle * CScreens::GetCurrentDisplayRectangle(void)
{
    if (mRectanglesList.IsEmpty())
        return NULL;
    return mpCurrentDR;
}
void CScreens::ListData(CList<class CMetaData*,class CMetaData*> &dataList)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        CList<class CMetaData*,class CMetaData*> &list = pRect->GetDataList();
        POSITION posData = list.GetHeadPosition();
        while (posData)
        {
            dataList.AddTail(list.GetNext(posData));
        }
    }
}
void CScreens::UpdateDataNotice(CMetaData *pData)
{
    if (!mnDatasUpdated++)
    {
		CMyTracer myTracer ("CScreens::UpdateDataNotice", mnDatasUpdated);
		if (mMainThreadId == GetCurrentThreadId())
		{
			CString sUpdating;
			sUpdating.LoadString(IDS_UPDATING);
	        CMyWindows::PrintStatus(sUpdating);
		}
        static int count = 0;
        char zBuf[128];
        sprintf_s (zBuf, 128, "\n<UpdateData> Start ======== %d ======>>>\n", ++count);
        if (umpfUpdatesLog)
        {
            fprintf (umpfUpdatesLog, zBuf);
            fprintf (umpfUpdatesLog, "%s\n", (const char *)pData->msName);
            fflush (umpfUpdatesLog);
        }
        if (mbDebugData && mpfLog)
            fprintf (mpfLog, zBuf);
    }
    if (mbDebugData && mpfLog)
    {
        fprintf (mpfLog, "%s\n", (const char *)pData->msName);
        fflush (mpfLog);
    }
}
void CScreens::SetTitle(const char *zTitle)
{
	if (zTitle)
	{
		umsScreenTitle = zTitle;
		UpdateDataSetTitle();
	}

	// Set Application Name in Help Menu
	if (!mpImageView || !mpImageView->m_hWnd)
		return;
    //CMenu *pMenu = mpImageView->GetMenu();
	//if (!pMenu)
	//	return;

	CString sMenuText(umsScreenTitle);
	sMenuText += " ";
	sMenuText += umsVersion;

	CMyMenu::ModifySubMenuItem(mpImageView->GetMenu(), "&Help", ID_HELP_ABOUTV4, sMenuText);

	/*
    int pos=CMyMenu::FindMenuItem(pMenu, "&Help");
    if (pos != -1) 
	{    
		CMenu *pSubMenu=pMenu->GetSubMenu(pos);
		if (pSubMenu)
			pSubMenu->ModifyMenu(1, 
				MF_BYPOSITION | MF_STRING, ID_HELP_ABOUTV4, sMenuText);
    } */
}
void CScreens::UpdateDataSetTitle(void)
{
    CString sTitle(umsScreenTitle);
	if (umbDisplayDisclaimer)
	{
		sTitle += " - ";
		sTitle += umsDisclaimer;
	}
	if (mRectanglesList.IsEmpty())
	{
		sTitle += " - ";
		sTitle += umsVersion;
		SetTitle();
	}
	else
	{
        static int uiTitle = 0;
        char zTitle [32];
        sprintf_s (zTitle, 32, "(%1d) ", uiTitle++ % 10);
        sTitle += zTitle;

		CMetaData *pPrevData = NULL;
		POSITION pos = mRectanglesList.GetHeadPosition();
		while (pos)
		{
			CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
			CMetaData *pData = pRect->GetCurrentData();
			if (!pData)
				break;
	  
			if (pPrevData)
			{
				sTitle += " | ";
				sTitle += pData->GetExtraName(pPrevData->z_name());
			}
			else
				sTitle += pData->GetTitleName();

			if (!pData->IsDiff())
				pPrevData = pData;
		}
	}

    if (mpImageView->m_hWnd)
        mpImageView->SetWindowText(sTitle);

	CString sReady;
	sReady.LoadString(IDS_READY);
    CMyWindows::PrintStatus(sReady);
}
bool CScreens::UpdateDataCheck(void)
{
	static int ucount = 0;
	CMyTracer myTracer0("CScreens::UpdateDataCheck", ++ucount);
    if (mpfLog)
        fprintf (mpfLog, "\n<CScreens::UpdateDataCheck>\n");

	bool bChange = false;
	{
		CMyCriticalSpan span(mCritical, "UpdateDataCheck");
		if (mpImageView && mpImageView->WantNewDisplayFiles())
			mpImageView->SetDisplayFiles();

		static int count = 0;
		CMyTracer myTracer("CScreens::UpdateDataCheck-critical", ++count);
		CExactTimer startTime (true);

		mnDatasUpdated = 0;
		POSITION pos = mRectanglesList.GetHeadPosition();
		while (pos)
		{
			CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
			pRect->UpdateDataCheck();
		}

		if (mbDataSwitched || (mnDatasUpdated > 0))
		{
			bChange = true;
			mbDataSwitched = false;
			UpdateDataSetTitle();

			if (mbMouseOnMe)
				OnMousePos(mLastMousePos);

			CExactTimer endTime (true);
			char zBuf[256];
			sprintf_s (zBuf, 256, "<UpdateData> End ==========(%.3f ms)======<<<\n\n",
				(float)endTime.sec_from (startTime)*1000.0f);
			if (umpfUpdatesLog)
			{
				fprintf (umpfUpdatesLog, zBuf);
				fflush (umpfUpdatesLog);
			}
			if (mbDebugData && mpfLog)
			{
				fprintf (mpfLog, zBuf);
				fflush (mpfLog);
			}
		}
		miPipeMessageHandled = miPipeMessageToHandle;
	}
    return bChange;
}

bool CScreens::OnViewNextfile()
{
    if (!mpCurrentDR)
        return false;

    if (mpCurrentDR->OnViewNextfile())
    {
        if (mRectanglesList.GetSize() > 1)
            AddRefToNewData();
        mbDataSwitched = true;
        return true;
    }

    if (mRectanglesList.GetSize() > 1)
    {
        int iRect = 0;
        POSITION pos = mRectanglesList.GetHeadPosition();
        while (pos)
        {
            CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
            if (pRect->OnViewNextfile())
            {
                SetCurrentDR(pRect);
                AddRefToNewData();
                mbDataSwitched = true;
                return true;
            }
            iRect++;
        }
    }
    return false;
}
bool CScreens::OnViewPrevfile()
{
    if (!mpCurrentDR)
        return false;

    if (mpCurrentDR->OnViewPrevfile())
    {
        if (mRectanglesList.GetSize() > 1)
            AddRefToNewData();
        mbDataSwitched = true;
        return true;
    }

    if (mRectanglesList.GetSize() > 1)
    {
        int iRect = 0;
        POSITION pos = mRectanglesList.GetHeadPosition();
        while (pos)
        {
            CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
            if (pRect->OnViewPrevfile())
            {
                SetCurrentDR(pRect);
                AddRefToNewData();
                mbDataSwitched = true;
                return true;
            }
            iRect++;
        }
    }
    return false;
}
bool CScreens::UpdateZoomByMouse(CPoint point)
{
    CMetaData *pData = GetCurrentData();
    if (!pData || !pData->GetZoomParams())
        return false;

    CZoomPanParams  &zoom = *pData->GetZoomParams();
    if (zoom.IsDisabled())
        return false;

    CSize size = point - mPrevPoint;
    float ratio = (float)size.cy / MOUSE_ZOOM_FACTOR;
	zoom.MultiplyMmPerPixel(1.0f + ratio, pData);
    return true;
}
bool CScreens::UpdatePanByMouse(CPoint point)
{
    CDisplayRectangle *pRect = GetCurrentDisplayRectangle();
    if (!pRect)
        return false;
    CMetaData *pData = pRect->GetCurrentData();
    if (!pData || !pData->GetZoomParams())
        return false;

    CSize size = point - mPrevPoint;
    CZoomPanParams  &pan = *pData->GetZoomParams();
    return pan.UpdateShift(size, pData);
}
void CScreens::OnNewMultiPos(CMetaData *pCurrentData)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
		CMetaData *pData = pRect->GetCurrentData();
		if (pData != pCurrentData)
			pData->PropagateMultiPos(pCurrentData);
	}
}
bool CScreens::MultiDimScroll(int iDim, bool bForward)
{
    CMetaData *pCurrentData = GetCurrentDataForScroll();
    if (!pCurrentData)
		return false;

	int nDim = pCurrentData->GetNDimensions();
	if (iDim >= nDim)
	{
		if (iDim == 2)
			iDim = 0;
		if (iDim == 3)
		{
			if (nDim > 1)
				iDim = 1;
			else
				iDim = 0;
		}
	}

	bool bPropagate = false;
	bool bChange = pCurrentData->MultiDimScroll(iDim, bForward, &bPropagate);
	if (bChange && bPropagate)
	{
		if (mpCurrentDR->GetNData() > 1)
			mpCurrentDR->OnMultiDimScroll(iDim);
        POSITION pos = mRectanglesList.GetHeadPosition();
        while (pos)
        {
            CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
			CMetaData *pData = pRect->GetCurrentData();
			if (pData != pCurrentData)
				pData->PropagateLeaf(pCurrentData, iDim);
		}
	}

	if (bChange) 
		mpImageView->InvalidateRect(NULL,false);
	return bChange;
}
bool CScreens::ScrollInImagesSeries(CPoint point, int zDelta)
{
	if (!mbAllowPaging)
		return false;

	bool bChange = false;
    if (!SetCurrentDR(point, bChange))
        return bChange;

	bChange = MultiDimScroll (0, zDelta < 0);
    return bChange;
}
bool CScreens::ScrollByMouse(CPoint point)
{
	int delta = point.y - mPrevPoint.y;
	return ScrollInImagesSeries(point,-delta);
}
bool CScreens::UpdateCurrentWindowByMouse(CPoint point)
{
	if (!mpCurrentDR)
		return false;
    CMetaData *pData = mpCurrentDR->GetCurrentDataForWindowing();
    if (!pData)
        return false;

    CWindowParams *pWindowParams = pData->GetWindowParams();
	pWindowParams->UpdateByMouseMove(point.x - mPrevPoint.x, point.y - mPrevPoint.y, pData);

	/*
	bool bCT = pData->IsCtImage();
	float range = pData->Range();
	pWindowParams->mWidth += pWindowParams->ScaleDelta (point.x - mPrevPoint.x, range, bCT);
	pWindowParams->mCenter -= pWindowParams->ScaleDelta (point.y - mPrevPoint.y, range, bCT);

	pWindowParams->SetInRange(bCT, pData->Min(), pData->Max());
	*/
    return true;
}
void CScreens::SetScreenDivision(void)
{
    if (mnRectsInRow < 1 || mnRectsRows < 1)
        return;

    int xOfRect = mImageSize.cx / mnRectsInRow;
    int yOfRect = mImageSize.cy / mnRectsRows;
    int x = 0;
    int y = 0;

    int iInRow = 0;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        if (!pRect)
            break;
        pRect->SetShape(x, y, x+xOfRect-1, y+yOfRect-1);
        pRect->SetColorsRaster (mpColorsRaster + y * mImageSize.cx + x, mImageSize.cx);
        if (mpfLog)
        {
            fprintf (mpfLog, "<SetColorsRaster> x %d y %d\n", x, y);
            fprintf (mpfLog, "<SetColorsRaster> mpColorsRaster + y * mImageSize.cx + x, mImageSize.cx %p %d\n",
                mpColorsRaster + y * mImageSize.cx + x, mImageSize.cx);
        }

        x += xOfRect;
        if (++iInRow == mnRectsInRow)
        {
            x = 0;
            y += yOfRect;
            iInRow = 0;
        }
    }
    /*
    if (nRectsInRow < 1)
        return;

    if (mpfLog)
        fprintf (mpfLog, "<SetScreenDivision> x %d y %d nInRow %d\n", xOfRect, yOfRect, nRectsInRow);

    int nRows = ((int)mRectanglesList.GetCount() + nRectsInRow - 1) / nRectsInRow;
    mImageSize.SetSize (xOfRect * nRectsInRow, yOfRect * nRows);
    int len = mImageSize.cx * mImageSize.cy;
    if (mpfLog)
        fprintf (mpfLog, "<SetScreenDivision> cx %d cy %d len %d\n", mImageSize.cx, mImageSize.cy, len);
    if (len > mnAllocated)
    {
        if (mpColorsRaster)
            delete mpColorsRaster;
        mpColorsRaster = new COLORREF [len];
        memset (mpColorsRaster, 0, len * sizeof (mpColorsRaster[0]));
        mnAllocated = len;
        if (mpfLog)
            fprintf (mpfLog, "<SetScreenDivision> mnAllocated %d mpColorsRaster %p\n", mnAllocated, mpColorsRaster );
    }

    int x = 0;
    int y = 0;

    int iInRow = 0;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        if (!pRect)
            break;
        pRect->SetShape(x, y, x+xOfRect, y+yOfRect);
        pRect->SetColorsRaster (mpColorsRaster + y * mImageSize.cx + x, mImageSize.cx);
        if (mpfLog)
        {
            fprintf (mpfLog, "<SetColorsRaster> x %d y %d\n", x, y);
            fprintf (mpfLog, "<SetColorsRaster> mpColorsRaster + y * mImageSize.cx + x, mImageSize.cx %p\n",
                mpColorsRaster + y * mImageSize.cx + x, mImageSize.cx);
        }

        x += xOfRect;
        if (++iInRow == nRectsInRow)
        {
            x = 0;
            y += yOfRect;
            iInRow = 0;
        }
    }
    if (mpfLog)
    {
        fprintf (mpfLog, "\n");
        fflush (mpfLog);
    }
    */
}

void CScreens::DrawGraphics(CDC &dc)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        pRect->DrawGraphics(dc);
    }
}

bool CScreens::SetCurrentDR(CPoint point, bool &bChange)
{
    bool bFound = false;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        bChange |= pRect->SetDRCurrent(point);
        if (pRect->IsCurrent ())
        {
            SetCurrentDRAndSetMenu(pRect);
            bFound = true;
        }
    }
    return bFound;
}
bool CScreens::SetCurrentDR(CDisplayRectangle * pCurrentDR)
{
    if (mpCurrentDR == pCurrentDR)
        return false;

    bool bFound = false;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        //bChange |= pRect->SetCurrent(point);
        if (pRect == pCurrentDR)
        {
            pRect->mbIsCurrent = true;
            SetCurrentDRAndSetMenu(pRect);
            bFound = true;
        }
        else
            pRect->mbIsCurrent = false;
    }
    return bFound;
}
void CScreens::SetCurrentDRAndSetMenu(CDisplayRectangle * pCurrentDR)
{
	if (!mpImageView)
		return;
	mpCurrentDR = pCurrentDR;
	if (!mpCurrentDR)
		return;

	bool bSpecialWindow = false;
	CMetaData *pData = mpCurrentDR->GetCurrentData();
	if (pData)
		bSpecialWindow = pData->UseSpecialWindow();
	mpImageView->CheckMenuItem("Window", ID_SETTINGS_SETSECONDWINDOW, bSpecialWindow);
}
bool CScreens::SetCurrentDR(int iDR)
{
	int i = 0;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
		if (i == iDR)
			return SetCurrentDR(pRect);
		i++;
	}
	return false;
}
bool CScreens::OnMButtonDown(UINT nFlags, CPoint point)
{
    bool bChange = false;
    CDependant::OnMousePrint("OnMButtonDown", nFlags, point);
    SetCurrentDR(point, bChange);
    //if (SetCurrentDR(point, bChange))
    //    bChange |= mpCurrentDR->OnMButtonDown (nFlags,point);
    return bChange;
}
bool CScreens::OnLButtonDown(UINT nFlags, CPoint point)
{
    bool bChange = false;
    CDependant::OnMousePrint("OnLButtonDown", nFlags, point);
    if (!SetCurrentDR(point, bChange))
        return bChange;

    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        if (pRect->PtInRect (point))
        {
            bChange |= pRect->OnLButtonDown (nFlags,point);
            if (bChange && (mpWaitKillFocusDR != pRect))
                SetWaitKillFocus(pRect);
            return bChange;
        }   
    }
    return false;
}
bool CScreens::OnRButtonDown(UINT nFlags, CPoint point)
{
    bool bChange = false;
    CDependant::OnMousePrint("OnRButtonDown", nFlags, point);
    if (!SetCurrentDR(point, bChange))
        return bChange;

    if ((nFlags & MK_SHIFT) || (nFlags & MK_MBUTTON))
		return false;
		
	mbOpenContextMenuOnRButtonUp = true;
	mRButtonDownPoint = point;
    return false;
}
bool CScreens::OnRButtonUp(UINT nFlags, CPoint point)
{
    CDependant::OnMousePrint("OnRButtonUp", nFlags, point);
		
	if (mbOpenContextMenuOnRButtonUp)
	{
		OpenContextMenu(point);
		mbOpenContextMenuOnRButtonUp = false;
	}
    return false;
}
bool CScreens::OnLButtonUp(UINT nFlags, CPoint point)
{
	bool bChange = false;
	mbLeftMouseDoingGraphics = false;
    StopWaitingForKillFocus();
    CDependant::OnMousePrint("OnLButtonUp", nFlags, point);
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        if (pRect->PtInRect (point))
		{
            bChange = pRect->OnLButtonUp (nFlags,point);
			break;
		}
    }
	if (mImageRLeftMouseUpMsg)
		CMyWindows::PostMessageToClient(mImageRLeftMouseUpMsg);

    return bChange;
}
bool CScreens::OnRButtonDblClk(UINT nFlags, CPoint point)
{
    CDependant::OnMousePrint("OnRButtonDblClk", nFlags, point);
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        if (pRect->PtInRect (point))
            return pRect->OnRButtonDblClk (nFlags,point);
    }
    return false;
}

void CScreens::OnSize(UINT , int cx, int cy)
{
	OnSizeExtended(0, cx, cy, false);
}

void CScreens::OnSizeExtended(UINT , int cx, int cy, bool force)
{
    if ((!force) && (mScreenSize.cx == cx && mScreenSize.cy == cy))
        return;

#ifdef _DEBUG
	OpenLogFile();
#endif
    static int count = 0;
	count++;
    if (mpfLog)
        fprintf (mpfLog, "<CScreens::OnSize> %d cx %d cy %d\n", count, cx, cy);
    if (umpfUpdatesLog)
        fprintf (umpfUpdatesLog, "\n<CScreens::OnSize> %d cx %d cy %d\n\n", count, cx, cy);

    mScreenSize.cx = cx;
    mScreenSize.cy = cy;

    CWnd *pWnd = mpImageView->GetDlgItem(IDC_VIEWAREA);
    if (pWnd)
	{
		RECT viewAreaClientRect;
		pWnd->GetClientRect(&viewAreaClientRect);
		mImageSize.cx = viewAreaClientRect.right - viewAreaClientRect.left + 1;
		mImageSize.cy = viewAreaClientRect.bottom - viewAreaClientRect.top + 1;
		int wantedCX = mScreenSize.cx - HORIZONTAL_MARGINS + 1;
		int wantedCY = mScreenSize.cy - VERTICAL_MARGINS - VERTICAL_MARGINS_FOR_STATUS_BAR + 1;
		if (mpfLog)
		{
			fprintf (mpfLog, "<OnSize> Original mImageSize cx %d cy %d\n", mImageSize.cx, mImageSize.cy);
			fprintf (mpfLog, "<OnSize> MoveWindow     Size cx %d cy %d\n", wantedCX, wantedCY);
		}

        pWnd->MoveWindow(0, 0, wantedCX, wantedCY);

		// Pay attention: viewAreaClientRect is smaller than viewAreaSize
		pWnd->GetClientRect(&viewAreaClientRect);
		mImageSize.cx = viewAreaClientRect.right - viewAreaClientRect.left + 1;
		mImageSize.cy = viewAreaClientRect.bottom - viewAreaClientRect.top + 1;
		if (mpfLog)
			fprintf (mpfLog, "<OnSize> Moved    mImageSize cx %d cy %d\n", mImageSize.cx, mImageSize.cy);
	}

    int len = mImageSize.cx * mImageSize.cy;
    if (mpfLog)
        fprintf (mpfLog, "<OnSize> Image cx %d cy %d len %d\n", mImageSize.cx, mImageSize.cy, len);
    if (len > mnAllocated)
    {
        if (mpColorsRaster)
            delete mpColorsRaster;
        mpColorsRaster = new COLORREF [len];
        mnAllocated = len;
		ZeroColorRaster();
        if (mpfLog)
            fprintf (mpfLog, "<OnSize> mnAllocated %d mpColorsRaster %p\n", mnAllocated, mpColorsRaster );
    }

    SetScreenDivision();
    if (mpStatusBar)
        OnSizeSetStatusBar(cx, cy);
    if (mpfLog)
        fprintf (mpfLog, "\n");
}
void CScreens::SetFirstImageSize(void)
{
    if (mpfLog)
        fprintf (mpfLog, "<SetFirstImageSize>\n");
    if ((mpImageView->m_hWnd == NULL) || !mpCurrentDR)
        return;

    CMetaData *pData = mpCurrentDR->GetCurrentData();
    if (!pData)
        return;

	if (pData->IsRaw())
	{
		if (!umsRawScreenTitle.IsEmpty())
			SetTitle(umsRawScreenTitle);
	}
	else if (pData->IsCtImage())
	{
		if (!umsImageScreenTitle.IsEmpty())
			SetTitle(umsImageScreenTitle);
	}

    CSize firstImageSize = pData->DefaultImageSize();
	if (!mbMaxRectanglesInRowSetByUser)
	{
		if (firstImageSize.cx >= 1.5f * firstImageSize.cy)
			SetMaxRectanglesInRow(1,false);
		else
			SetMaxRectanglesInRow(0,false);
	}

	float zoomX = 1;
	float zoomY = 1;
	if (mWantedSize.cx)
	{
		if (firstImageSize.cx > 0)
			zoomX = (float)mWantedSize.cx / (float)firstImageSize.cx;
		firstImageSize.cx = mWantedSize.cx;
	}
	if (mWantedSize.cy)
	{
		if (firstImageSize.cy > 0)
			zoomY = (float)mWantedSize.cy / (float)firstImageSize.cy;
		firstImageSize.cy = mWantedSize.cy;
	}
	float minZoom = min(zoomX,zoomY);
	CZoomPanParams::SetInitialScreenZoomFactor(minZoom);

    if (mImageSize.cx == firstImageSize.cx && mImageSize.cy == firstImageSize.cy)
        return;
    RECT WndRect;
    mpImageView->GetWindowRect(&WndRect);

    if (mpfLog)
        fprintf (mpfLog, "<SetFirstImageSize> firstImage [%d %d]\n", firstImageSize.cx, firstImageSize.cy);

	if (mbKeepScreenSizeConstant)
	{
		this->OnSizeExtended(SIZE_MAXIMIZED, mScreenSize.cx, mScreenSize.cy, true);
	}
	else
	{
		int horizontalMargins = HORIZONTAL_MARGINS + EXTRA_HORIZONTAL_MARGINS;
		int verticalMargins = VERTICAL_MARGINS + EXTRA_VERTICAL_MARGINS + VERTICAL_MARGINS_FOR_STATUS_BAR;
		if (mpfLog)
		{
			fprintf (mpfLog, "<SetFirstImageSize> firstImageSize cx %d cy %d\n", 
				firstImageSize.cx, firstImageSize.cy);
			fprintf (mpfLog, "<SetFirstImageSize> MoveWindow cx %d cy %d (margins h %d v %d)\n", 
				firstImageSize.cx + horizontalMargins, 
				firstImageSize.cy + verticalMargins, horizontalMargins, verticalMargins);
		}
		mpImageView->MoveWindow(WndRect.left, WndRect.top, 
			firstImageSize.cx + horizontalMargins, 
			firstImageSize.cy + verticalMargins);
	}
	if (mpfLog)
	{
		fprintf (mpfLog, "<SetFirstImageSize> END\n\n");
	}
}
void CScreens::OnSizeSetStatusBar(int cx, int cy)
{
    CMetaData *pData = GetCurrentData();

    // Set Status Bar Partition
    int nPage = 90;
    if (pData)
	{
		CPosition *pPos = pData->GetPosition();
		if (pPos)
	        nPage = pPos->GetTextSize();
	}

    int nSpec = 55;
    if (pData)
        nSpec = pData->GetSpecTextSize();

    int nData = 60;
    if (pData)
        nData = pData->GetDataTextSize();

    int startCenter = cx - 65;
    int startWidth = startCenter - 50;
    int startPos = startWidth - 80;
    int startData = startPos - nData;
    int startSpec = startData - nSpec;
    int startStat = startSpec - 170;
    int startPage = startStat - nPage;

    int aParts[8] = { startPage, startStat, startSpec, startData, startPos, 
        startWidth, startCenter, -1 };
		
    if (mpStatusBar->m_hWnd)	
    {
	    mpStatusBar->SetParts(8, aParts);
		mpStatusBar->MoveWindow(0, cy-VERTICAL_MARGINS_FOR_STATUS_BAR, 
								cx, VERTICAL_MARGINS_FOR_STATUS_BAR);
		if (mpfLog)
		{
			fprintf(mpfLog,"<OnSizeSetStatusBar> cx %d cy %d\n", cx, cy);
			fprintf(mpfLog,"aParts %d %d %d data %d %d width %d %d\n", startPage, startStat, startSpec, startData, startPos, 
							startWidth, startCenter);
		}
	}
}

bool CScreens::OnMousePos(CPoint point)
{
    if (mbDebugMouse && umpfUpdatesLog)
        fprintf (umpfUpdatesLog, "\n<CScreens::OnMousePos> x %d y %d\n\n", 
                                                        point.x, point.y);
    bool bChanged = false;
    CDisplayRectangle *pDRWithMouse = NULL;

    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        if (pRect->PtInRect (point))
        {
            bChanged = pRect->OnMousePos(point);
            pDRWithMouse = pRect;
        }
		else if (pRect->mbShouldBroadcastIfLoosingMouse)
			pRect->OnLoosingMouse();
    }
    // Update all other rectangles
    if (pDRWithMouse)
    {
        pos = mRectanglesList.GetHeadPosition();
        while (pos)
        {
            CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
            if (pRect != pDRWithMouse)
            {
                if (pRect == mpWaitKillFocusDR)
                    bChanged |= OnWaitedKillFocus();
            }
        }
    }
    else
    {
        if (mpWaitKillFocusDR)
            bChanged |= OnWaitedKillFocus();

        pos = mGlobalRefList.GetHeadPosition();
        while (pos)
        {
            CGraphicElement *pElement = mGlobalRefList.GetNext(pos);
            bChanged |= pElement->OnMousePosNowhere ();
        }
    }
    return bChanged;
}

bool CScreens::OnMouseMove(UINT nFlags, CPoint point)
{
	if (mbOpenContextMenuOnRButtonUp)
	{
		if (point.x != mRButtonDownPoint.x || point.y != mRButtonDownPoint.y)
			mbOpenContextMenuOnRButtonUp = false;
	}
    mNewMousePos = point;

    if (mbDebugMouse)
        CDependant::OnMousePrint("OnMouseMove", nFlags, point);
    if (nFlags != mPrevMouseFlags)
    {
        mPrevMouseFlags = nFlags;
        mPrevPoint = point;
        return OnMousePos(point);
    }

    bool bChanged = false;

    if (nFlags & MK_LBUTTON) // Left Pressed
    {
        if ((nFlags & MK_SHIFT) || (nFlags & MK_MBUTTON))
            bChanged = UpdatePanByMouse(point);
        else if (nFlags & MK_CONTROL)
            bChanged = UpdateZoomByMouse(point);
        else
		{
			if (mbLeftMouseDoingGraphics)
	            bChanged = UpdateGraphicsOnMouseMove(point);
			else if (mpCurrentDR)
			{
				CPoint delta;
				delta.x = point.x - mPrevPoint.x;
				delta.y = point.y - mPrevPoint.y;
				if (!mpCurrentDR->SpecialMouseScroll(delta, bChanged))
					bChanged = ScrollByMouse (point);
			}
		}
    }
    else if (nFlags & MK_RBUTTON) // Right Pressed
    {
        if (nFlags & MK_MBUTTON)
            bChanged = UpdateZoomByMouse(point);
        else
            bChanged = UpdateGraphicsOnMouseMove(point);
    }
    else if (nFlags & MK_MBUTTON) // Middle Pressed
    {
        bChanged = UpdateCurrentWindowByMouse (point);
    }

    // Update Status Bar & references
    //POSITION pos = mRectanglesList.GetHeadPosition();
    //while (pos)
    //{
    //    CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
    //    if (pRect->PtInRect (point))
    //        bChanged |= pRect->OnMouseMove(nFlags, point);
    //}

    bChanged |= OnMousePos(point);

    mLastMousePos = point;
    mbMouseOnMe = true;
    mPrevPoint = point;
    return bChanged;
}
bool CScreens::UpdateGraphicsOnMouseMove(CPoint point)
{
    bool bChange = false;
    CSize drag = point - mPrevPoint;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        if (pRect->PtInRect (point))
        {
            bChange |= pRect->DragGraphics (drag);
        }
    }
	OnChange();
    return bChange;
}

void CScreens::Paint()
{
	static int umnInPaint = 0;
	umnInPaint++;
	bool bSkippedUpdate = false;

	static int ucount = 0;
	CMyTracer myTracer("CScreens::Paint", ++ucount);
	if (umnInPaint == 1)
		UpdateDataCheck();
	else
		bSkippedUpdate = true;

	if (mbDisablePaint)
	{
		umnInPaint--;
		return;
	}
	{
		CMyCriticalSpan span (mCritical, "Paint");
		CPaintDC dc(mpImageView);

		static bool ubFirst = true;
		if (ubFirst)
		{
			ubFirst = false;
			CZoomPanParams::OnStartFirstPaint();
		}

		CWnd *pWnd=mpImageView->GetDlgItem(IDC_VIEWAREA);
		CPaintDC ViewDC(pWnd);

		RECT updateRect;
		pWnd->GetClientRect(&updateRect);

		static int uccount = 0;
		CMyTracer myTraceCrit("CScreens::Paint-critical", ++uccount);
		static bool bOnce = false;
		if (!bOnce)
		{
			CExactTimer startTime (true);
			Sleep(10);
			CExactTimer endTime (true);
			if (umpfUpdatesLog)
				fprintf (umpfUpdatesLog, "\nTiming of Sleep(10) is %f sec\n\n", endTime.sec_from (startTime));
			bOnce = true;
		}
		CExactTimer startTime (true);

		if (umpfUpdatesLog)
		{
			static int uStartCount = 0;
			fprintf (umpfUpdatesLog, "\nPaint Start ========= %d =======>>>\n", ++uStartCount);
			fflush (umpfUpdatesLog);
		}

		// Zero rest of screen
		if (mnRectsRows > 1)
		{
			int nRects = (int)mRectanglesList.GetSize();
			if (nRects % mnRectsInRow)
				ZeroColorRaster();
		}
		POSITION pos = mRectanglesList.GetHeadPosition();
		while (pos)
		{
			CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
			pRect->Paint();
		}
		if (mpCurrentDR)
		{
			if (mbUpdateStatusBarOnNextPaint)
			{
				mpCurrentDR->PrintStatusBarWithLastCoo();
				mbUpdateStatusBarOnNextPaint = false;
			}
			else
				DisplayWindowStatus();
		}

		// Draw raster (bitmap) graphics and text into memory device.
		// Then BitBlt from memory device into screen device.
		CBitmap BitMap;
		BitMap.CreateCompatibleBitmap(&ViewDC, mImageSize.cx, mImageSize.cy);

		BitMap.SetBitmapBits(mImageSize.cx*mImageSize.cy*sizeof(COLORREF), mpColorsRaster);

		CDC MemDC;
		MemDC.CreateCompatibleDC(&ViewDC);
		MemDC.SelectObject(&BitMap);


		DrawGraphics (MemDC);

		pos = mRectanglesList.GetHeadPosition();
		while (pos)
		{
			CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
			pRect->DisplayText(&MemDC);
		}


		ViewDC.BitBlt(updateRect.left, updateRect.top, mImageSize.cx, mImageSize.cy, &MemDC,
						0, 0, SRCCOPY);

		if (umpfUpdatesLog)
		{
			CExactTimer endTime (true);
			fprintf (umpfUpdatesLog, "Paint End ==========(%.3f ms)======<<<\n\n",
				(float)endTime.sec_from (startTime)*1000.0f);
			fflush(umpfUpdatesLog);
		}

		if (mbSaveOnDataChange && mnDatasUpdated)
			SaveStateForClient();
		mnDatasUpdated = 0;
		CGraphicElement::SaveAllOnChange();
	}
    if (mpfLog)
        fflush(mpfLog);
	OnChange();
	umnInPaint--;

	if (mpImageView && mpImageView->WantNewDisplayFiles())
	{
		if (bSkippedUpdate)
			UpdateDataCheck();
		Refresh();
	}
}
void CScreens::PrintStatusInBar(ESatusBarFields eField, const char *zText)
{
    mpStatusBar->SetText(zText, eField, 0);
}
void CScreens::Refresh(void)
{
    if (mpImageView->m_hWnd)
        mpImageView->InvalidateRect(NULL,false);
}
void CScreens::StaticRefresh(void *)
{
    gScreens.Refresh();
}
void CScreens::ScrollLineLeft(void)
{
	if (!mbAllowPaging)
		return;

	MultiDimScroll(1,false);

    if (mpCurrentDR)
        if (mpCurrentDR->ScrollLineLeft())
            Refresh();
}
void CScreens::ScrollLineRight(void)
{
	if (!mbAllowPaging)
		return;

	MultiDimScroll(1,true);

    if (mpCurrentDR)
        if (mpCurrentDR->ScrollLineRight())
            Refresh();
}
void CScreens::OnViewGotopage(void)
{
	if (!mbAllowPaging)
		return;

    if (mpCurrentDR)
        mpCurrentDR->OnViewGotopage();
}
bool CScreens::OpenContextMenu(CPoint point, int iSubMenu)
{
    mPopupMenuPoint = point;
    mPopupMenuDataCoo = mpCurrentDR->SetSelectedPoint(point);

	CRect WndRect;
	mpImageView->GetWindowRect( WndRect );

	CMenu menu;
	CMenu *pPopup = NULL;

	if (iSubMenu == CContextMenu::USE_SUB_MENU)
		pPopup = gContextMenu.GetMenu();
	else
	{
		menu.LoadMenu(IDR_MENU_POP);
		pPopup = menu.GetSubMenu(iSubMenu);
	}
	if (!pPopup)
		return false;

	CMetaData * pData = GetCurrentData();
	if (pData)
		pData->EditContextMenu(pPopup);

	if (iSubMenu == MIP_MENU_ID)
	{
		if (mpCurrentDR->GetCurrentData()->IsVolumetric())
		{
			pPopup->ModifyMenu(1, 
				MF_BYPOSITION | MF_STRING, ID_MIP_SET_VOI, "Set VOI");
			pPopup->AppendMenu(MF_STRING, 
				ID_MIP_TOPO_VOI, "Topological VOI");
		}
		else
			pPopup->ModifyMenu(1, 
				MF_BYPOSITION | MF_STRING, ID_MIP_CLIENTMESSAGE, "Focus");
	}
	else if (mpGEWithContextMenu)
		mpGEWithContextMenu->EditContextMenu(pPopup);
	/*else if (pGE && pGE->Name() == "VOI")
	{
		CString sText;
		sText.LoadString(IDS_STRING_ROI_2);
		pPopup->ModifyMenu(1, 
			MF_BYPOSITION | MF_STRING, ID_GRAPHIC_DISPLAYONALLIMAGES, sText);
	} */

	//Display the menu
	mpImageView->OnInitMenuPopup( pPopup );
	pPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY,
							WndRect.left+point.x, WndRect.top+point.y, mpImageView);	
    return true;
}

bool CScreens::OpenContextMenu(CPoint point)
{
    if (!mpCurrentDR)
        return false;

    CDRCoordinates drCoo (*mpCurrentDR, CScreenCoordinates(point));
    CGraphicElementOnDR *pGEonDR = mpCurrentDR->FindGraphicNear(drCoo);
    if (pGEonDR)
    {
        CGraphicElement *pGE = pGEonDR->GetGE();
        int iMenu = pGE->WantedContextMenu();
        if (iMenu)
		{
			mpGEWithContextMenu = pGE;
            return OpenContextMenu (point, iMenu);
		}
    }

    CMetaData *pData = mpCurrentDR->GetCurrentDataForWindowing();
	int iMenu = 0;
	if (!pData->WantContextMenu(iMenu))
		return false;

	if (iMenu)
        return OpenContextMenu (point, iMenu);

    if (pData->IsRaw())
        return OpenContextMenu (point, RAW_MENU_ID);

    if (pData->IsReadingsFile())
        return OpenContextMenu (point, RAW_MENU_ID);

    if (pData->IsOblique())
        return OpenContextMenu (point, OBLIQUE_MENU_ID);

    if (pData->IsMip())
        return OpenContextMenu (point, MIP_MENU_ID);

    if (pData->IsMultiDataFile())
        return OpenContextMenu (point, MULTI_DATA_MENU_ID);

    if (pData->IsDiff())
        return OpenContextMenu (point, DIFF_MENU_ID);

	if (mpCurrentDR->HasDataColoring())
        return OpenContextMenu (point, COLORING_MENU_ID);

    return false;
}

void CScreens::AddRefToNewData(void)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
        if (pRect != mpCurrentDR)
        {
            // Add Ref on old DR to new DR and vise versa
            pRect->AddRefBetween(mpCurrentDR);
        }
    }
}
CString CScreens::SaveState(const char *zName, const char *zMessage)
{
	CMyCriticalSpan(mCritical,"SaveState");
    CXMLDump xmlDump(zName,"ImageR");
    CString sfName = xmlDump.msName;

	if (zMessage)
        xmlDump.Write("message",zMessage);

	{
		CXMLDumpScope graph("Screen", xmlDump);
		{
			CXMLDumpScope graph("ScreenSize", xmlDump);
			xmlDump.Write("cx", (int)mScreenSize.cx);
			xmlDump.Write("cy", (int)mScreenSize.cy);
		}
		xmlDump.Write("max_columns", mnMaxRectanglesInRow);
    }

    xmlDump.Write("n_rectangles",(int)mRectanglesList.GetSize());

    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        pDR->SaveState(xmlDump);
    }

    if (mGlobalColorersList.GetSize() > 0)
    {
        CXMLDumpScope graph("GlobalColorersList",xmlDump);
        pos = mGlobalColorersList.GetHeadPosition();
        while (pos)
        {
            CMetaData *pData = mGlobalColorersList.GetNext(pos);
            pData->SaveState(xmlDump);
        }
    }
    return sfName;
}
void CScreens::SaveStateForUser(void)
{
	// This "Save State is called from ImageR main menu
	// Let the user select file name for the saved state
    CMetaData *pData = GetCurrentData();
    if (!pData)
        return;

    CString sfName (GetDataPath() + "MyParams");
    if (!CMyWindows::VerifyDirectory(sfName))
    {
		sfName = CMyWindows::GetLogDir("MyParams");
        if (!CMyWindows::VerifyDirectory(sfName))
            return;
    }

    CFileDialog saveDlg(FALSE,// Save As
        "ImageR.xml",
        "temp",
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        NULL
        );

    sfName += "\\";
    saveDlg.m_ofn.lpstrInitialDir = (const char *)sfName;

	if (saveDlg.DoModal() != IDOK)
        return;

    sfName = saveDlg.m_ofn.lpstrFile;
    sfName = SaveState(sfName);
    gRecentFiles.update(sfName);
    CMyWindows::CreateProcess("Notepad", sfName);
}
void CScreens::LoadSavedState(const char *zfName)
{
#ifdef _DEBUG
	OpenLogFile();
#endif
    if (mpfLog)
        fprintf (mpfLog, "\n<LoadSavedState> %s\n", zfName);
    if (mpXmlParse)
        delete mpXmlParse;

    mpXmlParse = new CXMLParse(zfName);
    CXMLParseNode *pRoot = mpXmlParse->GetRoot();
    if (!pRoot)
        return;

	CXMLParseNode* pScreen = pRoot->GetFirst("Screen");
	if (pScreen)
	{
		if (pScreen->GetValue("max_columns", mnMaxRectanglesInRow))
			mbMaxRectanglesInRowSetByUser = true;
	}

    int i = 1;
    CXMLParseNode *pDRXml = pRoot->GetFirst("DisplayRectangle");
    while (pDRXml)
    {
        if (mpfLog)
            fprintf (mpfLog, "Load Rectangle %d\n", i);
        CXMLParseNode *pDisable = pDRXml->GetFirst("bDisableStateRestore");
        if (!pDisable)
        {
            CDisplayRectangle *pRect = new CDisplayRectangle(mpfLog, pDRXml);
            mRectanglesList.AddTail(pRect);
            OnNewRect();
            if (mRectanglesList.GetSize() > 1)
                AddRefToNewData();
        }
        else
            if (mpfLog)
                fprintf (mpfLog, "Rectangle Disabled (not restored)...\n");
        pDRXml = pRoot->GetNext("DisplayRectangle", pDRXml);
        i++;
    }

    gRecentFiles.update(zfName);
    Refresh();
    if (mpfLog)
    {
        fprintf (mpfLog, "End <LoadSavedState> %s\n\n", zfName);
        fflush(mpfLog);
    }
}
CMetaData * CScreens::GetFirstData(void)
{
    if (mRectanglesList.IsEmpty())
        return NULL;

	CDisplayRectangle *pDR = mRectanglesList.GetHead();
    if (!pDR)
        return NULL;
    return pDR->GetCurrentData();
}
COblique * CScreens::GetOblique(void)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *pData = pDR->GetCurrentData();
        if (pData && pData->IsOblique())
            return dynamic_cast<COblique *>(pData);
    }
    return NULL;
}
CDisplayRectangle * CScreens::GetMipDR(void)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *pData = pDR->GetCurrentData();
        if (pData && pData->IsMip())
            return pDR;
    }
    return NULL;
}
void CScreens::StopWaitingForKillFocus(void)
{
    if (mbCaptureSet)
    {
        ReleaseCapture();
        mbCaptureSet = false;
    }
    mpWaitKillFocusDR = NULL;
}

bool CScreens::OnWaitedKillFocus(void)
{
    bool bChange = mpWaitKillFocusDR->OnKillFocus();
    StopWaitingForKillFocus();
    return bChange;
}
bool CScreens::RemoveGEFromAllOtherDR(const char *zName, CDisplayRectangle *pDRDefend)
{
    bool bChange = false;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        if (pDR != pDRDefend)
            if (pDR->DoGraphicCommand(GCOM_REMOVE, zName))
                bChange = true;
    }
    return bChange;
}
bool CScreens::DisplayOnAllRectangles(CGraphicElement * pGE)
{
    bool bChange = false;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        if (pDR->AddGraphicElement(pGE))
            bChange = true;
    }
	mGlobalGraphicsList.AddTail(pGE);
    return bChange;
}
bool CScreens::AddGlobalRef(CGraphicElement *pGE)
{
    if (mpfLog)
    {
        fprintf (mpfLog, "<CScreens::AddGlobalRef> %s\n", (const char*)pGE->msName);
        fflush (mpfLog);
    }
    mGlobalRefList.AddTail(pGE);
    return true;
}
void CScreens::SetWaitKillFocus(CDisplayRectangle * pDR)
{
    if (mpWaitKillFocusDR == pDR)
        return;

    mpWaitKillFocusDR = pDR;
    if (!mbCaptureSet)
    {
        mpImageView->SetCapture();
        mbCaptureSet = true;
    }
}
void CScreens::SaveStateForClient(const char *zMessage)
{
    CString sSaveDir (CMyWindows::GetLogDir("ImageR"));

    static int count = 0;
    if (!mpCurrentDR)
	{
		if (mpfLog)
			fprintf (mpfLog, "<SaveStateForClient> !mpCurrentDR\n");
        return;
	}

    count++;
    if (count == 1)
    {
        CMyWindows::DeleteFiles(sSaveDir + "\\ImageRSave.*");
        CMyWindows::VerifyDirectory(sSaveDir);
    }

    char zName[128];
    sprintf_s (zName, 128, "%s\\ImageRSave.%d.xml", (const char*)sSaveDir, count);
    if (mpfLog)
        fprintf (mpfLog, "<SaveStateForClient> %s\n", zName);

    SaveState (zName, zMessage);
    BroadcastSaveDone(0); // No object ID - All state saved
    if (mpfLog)
    {
        fprintf (mpfLog, "<SaveStateForClient> End %s\n", zName);
        fflush (mpfLog);
    }
}
void CScreens::BroadcastSaveDone(int objectId)
{
    WPARAM wParam = objectId;
	//PostMessage(HWND_BROADCAST, mSaveDoneMsg, wParam, NULL);
	//PostMessageToClient(mSaveDoneMsg, wParam, NULL);
	CMyWindows::PostMessageToClient(mSaveDoneMsg, wParam, NULL);
	if (mpfLog)
		fprintf (mpfLog, "<BroadcastSaveDone> msg %d object %d\n", mSaveDoneMsg, objectId);
}
void CScreens::AddGlobalColorer(CMetaData * pColorer)
{
    mGlobalColorersList.AddTail(pColorer);
}

bool CScreens::SelectMultiFile(void)
{
    if (!mpCurrentDR)
        return false;

    CMetaData *pData = mpCurrentDR->GetCurrentData();
    if (pData && pData->IsMultiFile())
        return true;

    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *CurpData = pDR->GetCurrentData();
        if (CurpData && CurpData->IsMultiFile())
        {
            SetCurrentDR(pDR);
            return true;
        }
    }
    return false;
}
CString CScreens::GetDataPath(void)
{
    if (mpCurrentDR)
    {
        CMetaData *pData = mpCurrentDR->GetCurrentData();
        if (pData && pData->IsFile())
            return pData->GetPath();
    }

    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *pData = pDR->GetCurrentData();
        if (pData && pData->IsFile())
            return pData->GetPath();
    }

    return CString();
}

void CScreens::SetServerMode(bool bReplace)
{
    mbServer = true;
	mbReplace = bReplace;
}
void CScreens::DoGraphicCommand(EGraphicCommand eGCom, const char * zName)
{
    bool bChange = false;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        bChange |= pDR->DoGraphicCommand(eGCom, zName);
    }
    if (bChange)
        Refresh();
}
CMetaData * CScreens::FindDataByNameOrIndex(const char * zName, CDisplayRectangle **oppDR)
{
	if (!zName)
		return NULL;
	CMetaData * pData = NULL;
	if (isdigit(zName[0]))
	{
		int iRect = atoi(zName);
		CDisplayRectangle *pDR = GetDisplayRectangle(iRect - 1);
		if (pDR)
		{
			pData = pDR->GetCurrentData();
			if (pData)
			{
				if (oppDR)
					*oppDR = pDR;
				return pData;
			}
		}
		return NULL;
	}
		
	return FindDataByName(zName);
}
CMetaData * CScreens::FindDataByName(const char * zName, CDisplayRectangle **oppDR)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *pData = pDR->GetCurrentData();
        if (pData && pData->Is(zName))
        {
            if (oppDR)
                *oppDR = pDR;
            return pData;
        }
    }
    return NULL;
}
bool CScreens::SetExternalFrame(const char * zDataName)
{
    bool bFound = false;
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *pData = pDR->GetCurrentData();
        if (pData && pData->Is(zDataName))
        {
            pDR->mbExternalFrame = true;
            bFound = true;
        }
        else
            pDR->mbExternalFrame = false;
    }
    return bFound;
}
void CScreens::SetPosition(const char * zName, int iImage)
{
	CMetaData * pData = FindDataByNameOrIndex(zName);

    if (pData && pData->GetPosition())
	{
        pData->GetPosition()->SetCurrent(iImage);
		if (GetNDR() > 1)
			OnNewMultiPos(pData);
		pData->ShouldUpdatePage();
		Refresh();
	}
}
void CScreens::SetRelativePosition(const char * zName, int iImage)
{
	if (!zName)
		return;
	CMetaData * pData = NULL;
	if (isdigit(zName[0]))
	{
		int iRect = atoi(zName);
		CDisplayRectangle *pDR = GetDisplayRectangle(iRect - 1);
		if (pDR)
			pData = pDR->GetCurrentData();
	}
	else
		pData = FindDataByName(zName);

    if (pData && pData->SetRelativePosition(iImage))
	{
		OnNewMultiPos(pData);
		Refresh();
	}
}
void CScreens::SetPosition(const char * zName, int iDir, int iImage)
{
    CMetaData * pData = FindDataByName(zName);
    if (pData && pData->GetPosition())
	{
        pData->GetPosition()->SetCurrent(iImage);
		if (pData->IsMultiFile())
		{
			CMultiFile *pMultiFile = (CMultiFile *)pData;
			pMultiFile->SetDir(iDir);
		}
		else
		{
			if (pData->GetPosition(1))
				pData->GetPosition(1)->SetCurrent(iDir);
		}
		Refresh();
	}
}
void CScreens::SetDisplayCenter(const char * zName, CFloatCoordinates &centerCoo)
{
	CMyCriticalSpan(mCritical,"SetDisplayCenter");
    CMetaData * pData = FindDataByName(zName);
    if (pData)
	{
        pData->SetDisplayCenter(centerCoo);
		Refresh();
	}
}
void CScreens::SetAutoWindow(const char * zDataName, class CFloatCoordinates &dataCenterCoo)
{
	CMyCriticalSpan(mCritical,"SetAutoWindow");
	{
		static int count = 0;
		CMyTracer myTracer("CScreens::SetAutoWindow", ++count);
		CDisplayRectangle *pDR = NULL;
		CMetaData * pData = FindDataByName(zDataName, &pDR);
		if (pData)
		{
			pData->UpdatePageData();
			CWindowParams *pWindowParams = pData->GetWindowParams();
			CDataCoordinates dataCoo(pData->CurPage(), dataCenterCoo.fx, dataCenterCoo.fy);
			CDRCoordinates drCoo (*pDR, pData->GetZoomParams(), dataCoo);
			if (pData->FillZoomedRaster (*pDR))
			{
				pWindowParams->SetLocalFocus(pData->GetFloatZoomedRaster(), drCoo);
			}
		}
	}
	Refresh();
}
void CScreens::ToggleSpecialWindow(const char * zDataName)
{
	CMyCriticalSpan(mCritical,"ToggleSpecialWindow");
	{
		static int count = 0;
		CMyTracer myTracer("CScreens::ToggleSpecialWindow", ++count);
		CDisplayRectangle *pDR = NULL;
		CMetaData * pData = FindDataByNameOrIndex(zDataName, &pDR);
		if (pData)
		{
			pData->UpdatePageData();
			pData->ToggleSpecialWindow();
			CWindowParams *pWindowParams = pData->GetWindowParams();
			if (pData->FillZoomedRaster (*pDR))
				pWindowParams->SetAutoFocus(pData);
		}
	}
	Refresh();
}
void CScreens::SetZoom(float zoomFactor)
{
	if (!mpCurrentDR)
		return;
	CZoomPanParams *pZoomParams = mpCurrentDR->GetZoomParams();
	if (!pZoomParams)
		return;
	CMetaData *pData = mpCurrentDR->GetCurrentData();
	pZoomParams->SetZoomFactor(pData, zoomFactor);
	Refresh();
}
void CScreens::SetWindowFromGui(int center, int width, int iRectangle)
{
	CDisplayRectangle *pDR = NULL;
	if (iRectangle < 0)
		pDR = mpCurrentDR;
	else
		pDR = GetDisplayRectangle(iRectangle);
	if (!pDR)
		return;

	CMetaData * pData = pDR->GetCurrentDataForWindowing();
	if (!pData)
		return;

	if (width >= 0)
		pData->SetWindowParamsFromGui((float)center, (float)width);
	else
	{
        CWindowParams *pWindowParams = pData->GetWindowParams();
        pWindowParams->SetAutoFocus(pData);
	}
	Refresh();
}
void CScreens::SetWindowRangeFromGui(int minVal, int maxVal, int iRectangle)
{
	CDisplayRectangle *pDR = NULL;
	if (iRectangle < 0)
		pDR = mpCurrentDR;
	else
		pDR = GetDisplayRectangle(iRectangle);
	if (!pDR)
		return;

	CMetaData * pData = pDR->GetCurrentDataForWindowing();
	if (!pData)
		return;

	pData->SetWindowRangeFromGui((float)minVal, (float)maxVal);
}
void CScreens::BroadcastWindow(int iRectangle)
{
	CDisplayRectangle *pDR = mpCurrentDR;
	if (iRectangle >= 0)
		pDR = GetDisplayRectangle(iRectangle);
	if (!pDR)
		return;

	CMetaData * pData = pDR->GetCurrentDataForWindowing();
	if (pData)
	{
		CWindowParams *pWindowParams = pData->GetWindowParams();
		int center = (int)(pWindowParams->mCenter + 0.5f);
		int width = (int)(pWindowParams->mWidth + 0.5f);
		CMyWindows::PostMessageToClient(mWindowMsg, center, width);
	}
}
void CScreens::SetCursorBroadcast(int iRectangle)
{
	CDisplayRectangle *pDR = mpCurrentDR;
	if (iRectangle >= 0)
		pDR = GetDisplayRectangle(iRectangle);
	if (pDR)
	{
		pDR->SetBroadcastCursorLocation(!pDR->mbBroadcastCursorLocation);
		if (mbMouseOnMe && (pDR == mpCurrentDR))
			OnMousePos(mLastMousePos);
	}
}
void CScreens::ForceDataUpdate(bool bOnlyShared)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *pData = pDR->GetCurrentData();
		if (pData)
		{
			if (bOnlyShared)
			{
				if (pData->IsShared())
					pData->ForceUpdate();
			}
			else
		        pData->ForceUpdate();
		}
    }
    Refresh();
}
void CScreens::AddColorerOnData(CMetaData * pData, CMetaData * pColorer)
{
    POSITION pos = mRectanglesList.GetHeadPosition();
    while (pos)
    {
        CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
        CMetaData *pDRData = pDR->GetCurrentData();
        if (pDRData == pData)
			pDR->AddColorer(pColorer);
    }
    Refresh();
}
void CScreens::AfterFileDelete(CDisplayRectangle *pRect)
{
	if ((pRect->GetNData() < 1) && (mRectanglesList.GetSize() > 1))
	{
		POSITION pos = mRectanglesList.GetHeadPosition();
		while (pos)
		{
			POSITION lastPos = pos;
			CDisplayRectangle *pCurRec = mRectanglesList.GetNext(pos);
			if (pRect == pCurRec)
			{
				if (pRect == mpCurrentDR)
				{
					if (pos)
						SetCurrentDRAndSetMenu(mRectanglesList.GetNext(pos));
					else
						SetCurrentDRAndSetMenu(mRectanglesList.GetHead());
				}
				mRectanglesList.RemoveAt(lastPos);
				delete pRect;

				OnExtraNewRectangleAdjustSize(-1);
				break;
			}
		}
	}
    Refresh();
}
void CScreens::OnFileDelete(void)
{
	if (!mpCurrentDR)
		return;

	CMyCriticalSpan span(mCritical,"DeleteFile");
	mpCurrentDR->DeleteCurrentData();
	AfterFileDelete(mpCurrentDR);
}
void CScreens::DeleteData(const char * zName)
{
	CMyCriticalSpan span(mCritical,"DeleteData");
	POSITION pos = mRectanglesList.GetHeadPosition();
	while (pos)
	{
		CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
		if (pRect->DeleteData(zName))
			AfterFileDelete(pRect);
	}
}
void CScreens::SetWantedSize(int matrix)
{
	mWantedSize.cx = matrix;
	mWantedSize.cy = matrix;
}
void CScreens::DisplayGlobalGraphicsOnNewRect(CDisplayRectangle * pNewRect)
{
	POSITION pos = mGlobalGraphicsList.GetHeadPosition();
	while (pos)
	{
		CGraphicElement *pGE = mGlobalGraphicsList.GetNext(pos);
		if (pGE)
		{
			pNewRect->AddGraphicElement(pGE);
		}
	}
}
bool CScreens::DisplayGraphic(class CGraphicElement *pGE)
{
	if (!mpCurrentDR)
		return false;

	CString sName;
	{
		CXMLDump xmlDump(CMyWindows::GetLogDir(pGE->Name()),"Graphic");
		pGE->SaveState(xmlDump);
		sName = xmlDump.GetName();
	}
	CXMLParse xmlParse(sName);
	CXMLParseNode *pRoot = xmlParse.GetRoot();
	if (!pRoot)
		return false;

	mpCurrentDR->AddGraphicElementFromXml(pRoot);
	return true;
}
void CScreens::OnDicomCompare(void)
{
	CDisplayRectangle *paDRs[2] = {NULL};
	CMetaData *paDatas[2] = {NULL};
	CMyDicom *paDicoms[2] = {NULL};
	int nDicoms = 0;
	CMetaData *pCurrentData = mpCurrentDR->GetCurrentData();
	if (pCurrentData)
	{
		paDicoms[0] = pCurrentData->GetDicom();
		if (paDicoms[0])
		{
			paDatas[0] = pCurrentData;
			paDRs[0] = mpCurrentDR;
			nDicoms++;
		}
	}

	POSITION pos = mRectanglesList.GetHeadPosition();
	while (pos && (nDicoms < 2))
	{
		CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
		if (pDR != mpCurrentDR)
		{
			CMetaData *pData = pDR->GetCurrentData();
			if (pData)
			{
				CMyDicom *pDicom = pData->GetDicom();
				if (pDicom)
				{
					if (nDicoms == 0) // First dicom found
					{
						paDicoms[0] = pDicom;
						paDatas[0] = pData;
						paDRs[0] = pDR;
					}
					else // secon dicom found
					{
						if (pDR->mDRId < paDRs[0]->mDRId) // Second should come first
						{
							paDRs[1] = paDRs[0];
							paDatas[1] = paDatas[0];
							paDicoms[1] = paDicoms[0];
							paDicoms[0] = pDicom;
							paDatas[0] = pData;
							paDRs[0] = pDR;
						}
						else
						{
							paDicoms[1] = pDicom;
							paDatas[1] = pData;
							paDRs[1] = pDR;
						}
					}
					nDicoms++;
				}
			}
		}
	}
	if (nDicoms < 2)
	{
		CMyWindows::MessBox("Missing Data: Failed to find 2 dicom files","No Dicom Data to Compare");
		return;
	}

	CDicomCompare dicomCompare(paDicoms[0], paDicoms[1], paDatas[0], paDatas[1]);
	dicomCompare.Compare();
}
void CScreens::DeleteAllGraphics(bool bRefresh)
{
	POSITION pos = mRectanglesList.GetHeadPosition();
	while (pos)
	{
		CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
		pRect->DeleteGraphics();
	}
	if (bRefresh)
		Refresh();
}
void CScreens::DeleteAllTexts(bool bRefresh)
{
	POSITION pos = mRectanglesList.GetHeadPosition();
	while (pos)
	{
		CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
		pRect->DeleteTexts();
	}
	if (bRefresh)
		Refresh();
}
void CScreens::DeleteAllData(void)
{
	{
		CMyCriticalSpan span (mCritical, "DeleteAll");
		DeleteAllTexts(false);
		DeleteAllGraphics(false);
		POSITION pos = mRectanglesList.GetHeadPosition();
		while (pos)
		{
			CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
			pRect->DeleteCurrentData();
		}
	}
	Refresh();
}
void CScreens::ZeroColorRaster(void)
{
	memset (mpColorsRaster, 0, mnAllocated * sizeof (mpColorsRaster[0]));
}
void CScreens::SetEbwMode(void)
{
	mbEbwMode = true;
	KeepScreenSizeConstant();
	SetMaxRectanglesInRow(2);
	CArchivesFile::umbNeedDataText = false;
}
int CScreens::GetIFreeOrNewDR(void)
{
	int i = 0;
	POSITION pos = mRectanglesList.GetHeadPosition();
	while (pos)
	{
		CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
		CMetaData *pData = pRect->GetCurrentData();
		if (pData == NULL)
			return i;
		i++;
	}
	return i;
}
void CScreens::GetListOfDataUsingPosition(CPosition *pPosition, CList <CMetaData *, CMetaData *> &list)
{
	POSITION pos = mRectanglesList.GetHeadPosition();
	while (pos)
	{
		CDisplayRectangle *pRect = mRectanglesList.GetNext(pos);
		CMetaData *pData = pRect->GetCurrentData();
		if (pData->GetPosition() == pPosition)
			list.AddTail(pData);
	}
}
void CScreens::SetStatusText(const char * zText, int iField)
{
    mpStatusBar->SetText(zText, iField, 0);
}
void CScreens::AllowPaging(bool flag)
{
	mbAllowPaging = flag;
}
void CScreens::SetColorMap(bool bFlag, int iDR)
{
	if (iDR >= 0)
	{
		CDisplayRectangle * pDR = GetDisplayRectangle(iDR);
		pDR->mbUseColorMap = bFlag;
	}
	else
	{
		mpCurrentDR->mbUseColorMap = bFlag;
	}
	Refresh();
}
bool CScreens::ReadColorMap(const char * zfName, int iDR)
{
	FILE *pfColMap = MyFOpen(zfName,"rb");
	if (!pfColMap)
		return false;

	CLogFile log;
	if (umbDebugColorMap)
	{
		CFileName fName(zfName);
		CString sfLogFileName(fName.Private());
		sfLogFileName += ".log";
		log.Open(sfLogFileName);
	}

    for (int iCol = 0; iCol < MAX_COLORS; iCol++)
	{
		float cols[3];
		size_t nRead = fread(cols, sizeof(cols[0]), 3, pfColMap);
		if (nRead < 3)
		{
			CMyWindows::MessBox("Color Map too short", "Data Error");
			fclose(pfColMap);
			return false;
		}
		int red = (int)(cols[0] * (MAX_COLORS - 1));
		int green = (int)(cols[1] * (MAX_COLORS - 1));
		int blue = (int)(cols[2] * (MAX_COLORS - 1));
        maColorsRGBTable[iCol] = RGB(blue, green, red);
		if (log)
			fprintf(log.mpf, "%d\t%f\t%f\t%f\n", iCol, cols[0], cols[1], cols[2]);
	}
			
	fclose(pfColMap);
	SetColorMap(true, iDR);
	return true;
}
void CScreens::CheckForRepeatedDataOrAdjustPosition(CMetaData *pData)
{
	bool bRepeated = false;

	if (pData->IsFile() && mbSetNewPositionOnSameSeries)
	{
		CString sCurName = pData->PermanentName();

		POSITION pos = mRectanglesList.GetHeadPosition();
		while (pos)
		{
			CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
			CMetaData *pRecData = pDR->GetCurrentData();
			if (pRecData != pData)
			{
				if (pRecData->PermanentName() == sCurName)
				{
					bRepeated = true;
					break;
				}
			}
		}
	}

	if (bRepeated)
		pData->SetNewPosition();
	else
	{
		CMetaData *pCurrentData = mpCurrentDR->GetCurrentData();
		if (pCurrentData)
			pData->AdjustPosition(pCurrentData);
	}
}
void CScreens::SetMaxRectanglesInRow(int maxRIR, bool bSetByUser)
{
	mnMaxRectanglesInRow = maxRIR;
	mbMaxRectanglesInRowSetByUser = bSetByUser;

	bool bVertical = (mnMaxRectanglesInRow == 1);
	mpImageView->CheckMenuItem("&Settings", ID_SETTINGS_VERTICAL, bVertical);

}
void CScreens::SetMaxRectanglesInRowFlipVertical(void)
{
	if (mnMaxRectanglesInRow == 1)
		SetMaxRectanglesInRow(0);
	else
		SetMaxRectanglesInRow(1);
}
bool CScreens::VerifyScreenMode(void)
{
	CPaintDC dc(mpImageView);

	CWnd *pWnd=mpImageView->GetDlgItem(IDC_VIEWAREA);
	CPaintDC ViewDC(pWnd);

	CDC MemDC;
	MemDC.CreateCompatibleDC(&ViewDC);

	mnBitsPerPixel = GetDeviceCaps(MemDC.m_hDC, BITSPIXEL);
	int colorPlanes = GetDeviceCaps(MemDC.m_hDC, PLANES);
	CLogFile log;
	if (umDebug)
		log.Open("Screen_Paint");
	if (log)
		fprintf(log.mpf,"bitPixels %d, colorPlanes %d\n", mnBitsPerPixel, colorPlanes);
	if (mnBitsPerPixel < 32)
	{
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf),"Color has only %d-bits. Temporarily adjust settings?", 
			mnBitsPerPixel);

		CDeleteDlg dlg;
		dlg.SetAlternativeText(zBuf);
		if (dlg.DoModal() == IDOK)
		{
			DEVMODE devMode;
			devMode.dmBitsPerPel = 32;
			devMode.dmFields = DM_BITSPERPEL;
			LONG rc = 0;
			try
			{
				rc = ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
				//if (rc != DISP_CHANGE_SUCCESSFUL)
				//	CMyWindows::DisplayLastError("ChangeDisplaySettings");
			}
			catch(...)
			{
				CMyWindows::MessBox("Exception in ChangeDisplaySettings","SW Error");
			}
			mnBitsPerPixel = GetDeviceCaps(MemDC.m_hDC, BITSPIXEL);
			if (log)
			{
				fprintf(log.mpf,"<ChangeDisplaySettings> returned %d 0x%x, new BitsPerPix %d\n", 
										(int)rc, (int)rc, mnBitsPerPixel);
				fprintf(log.mpf,"DISP_CHANGE_SUCCESSFUL %d\n", DISP_CHANGE_SUCCESSFUL);
				fprintf(log.mpf,"DISP_CHANGE_BADDUALVIEW %d\n", DISP_CHANGE_BADDUALVIEW);
				fprintf(log.mpf,"DISP_CHANGE_BADFLAGS %d\n", DISP_CHANGE_BADFLAGS);
				fprintf(log.mpf,"DISP_CHANGE_BADMODE %d\n", DISP_CHANGE_BADMODE);
				fprintf(log.mpf,"DISP_CHANGE_BADPARAM %d\n", DISP_CHANGE_BADPARAM);
				fprintf(log.mpf,"DISP_CHANGE_FAILED %d\n", DISP_CHANGE_FAILED);
				fprintf(log.mpf,"DISP_CHANGE_NOTUPDATED %d\n", DISP_CHANGE_NOTUPDATED);
				fprintf(log.mpf,"DISP_CHANGE_RESTART %d\n", DISP_CHANGE_RESTART);
			}

			if (mnBitsPerPixel < 32)
			{
				if (rc == DISP_CHANGE_BADMODE)
					sprintf_s(zBuf,sizeof(zBuf),"Setting failed: 32-bit mode not supported. Color has only %d-bits.", 
						mnBitsPerPixel);
				else
					sprintf_s(zBuf,sizeof(zBuf),"Setting failed. Color has only %d-bits.", 
						mnBitsPerPixel);
				CMyWindows::MessBox(zBuf, "System Error");
			}
		}
		return mnBitsPerPixel == 32;
	}
	return true;
}
void CScreens::ReadRegistry(CMyRegistry &myReg)
{
	myReg.Get("ScreensDebug", CScreens::umDebug);
	myReg.Get("DebugColor", mDebugColor);
	myReg.Get("DebugMouse", mbDebugMouse);
	if (mbDebugMouse && !CNamedObject::umDebug)
		CNamedObject::umDebug = 1;

	myReg.Get("RawTitle", umsRawScreenTitle);
	myReg.Get("CtImageTitle", umsImageScreenTitle);
	myReg.Get("bDisplayDisclaimer", umbDisplayDisclaimer);
	myReg.Get("bSetNewPositionOnSameSeries", mbSetNewPositionOnSameSeries);
	myReg.Get("bDisplayMinMaxAtWindowStatus", mbDisplayMinMaxAtWindowStatus);
	myReg.Get("bCommonDisplayWidth", mbCommonDisplayWidth);

	bool bDebugColorMap = 0;
	if (myReg.Get("bDebugColorMap", bDebugColorMap))
		umbDebugColorMap = bDebugColorMap;

	bool bDisblePaging;
	if (myReg.Get("bDisablePaging", bDisblePaging))
		AllowPaging(!bDisblePaging);

	CString sMainTitle;
	if (myReg.Get("ImageRMainTitle", sMainTitle) && !sMainTitle.IsEmpty())
		SetTitle(sMainTitle);
}
void CScreens::DisplayWindowStatus(CMetaData *pData, FILE *pfLog)
{
	if (!pData)
	{
		if (!mpCurrentDR)
			return;
		pData = mpCurrentDR->GetCurrentDataForWindowing();
		if (!pData)
			return;
	}

    CWindowParams *pWindowParams = pData->GetWindowParams();
    if (!pWindowParams || !pWindowParams->mbInitialized)
	{
		if (pfLog)
			fprintf (pfLog, "<DisplayWindowStatus> pWindowParams %p %s\n", pWindowParams,
			pWindowParams ? "Not Initialized" : "NULL POINTER");
        return;
	}

    char zBuf[64];
    bool bCt = pData->IsRealCtImage();
    CString sCenter("");
    CString sWidth("");

	if (mbDisplayMinMaxAtWindowStatus)
	{
	    sWidth += pWindowParams->GetLowText();
		sCenter += pWindowParams->GetHighText();
	}
	else
	{
	    sWidth += pWindowParams->GetWidthText();
		sCenter += pWindowParams->GetCenterText();
	}

	// Add Text Title
	if (mbDisplayMinMaxAtWindowStatus)
	{
		sWidth += "WL";
		sCenter += "WH";
		bCt = false;
	}
	else
	{
		sWidth += "W";
		sCenter += "C";
	}

    if (pWindowParams->mIndex)
    {
        sprintf_s (zBuf, 64, "%d", pWindowParams->mIndex);
        sCenter += zBuf;
        sWidth += zBuf;
    }
    else if (bCt)
        sCenter += "t";


    PrintStatusInBar(CScreens::SBAR_WINDOW_WIDTH, sWidth);
    PrintStatusInBar(CScreens::SBAR_WINDOW_CENTER, sCenter);
    if (pfLog)
        fprintf (pfLog, "<DisplayWindowStatus> %s %s\n", (const char *)sWidth, (const char*)sCenter);
}
bool CScreens::SetDisplayWidth(int n)
{
	if (mbCommonDisplayWidth)
	{
		if (n == mnCommonDisplayWidth)
			return false;

		mnCommonDisplayWidth = n;
		bool bChanged = false;
		POSITION pos = mRectanglesList.GetHeadPosition();
		while (pos)
		{
			CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
			bChanged |= pDR->SetDisplayWidthToAllData(n);
		}
		return bChanged;
	}

	CMetaData *pData = GetCurrentData();
	if (!pData)
		return false;
	return pData->SetDisplayWidth(n);
}
void CScreens::OnGEDeleted(CGraphicElement *pGE)
{
	if (mpGEWithContextMenu == pGE)
		mpGEWithContextMenu = NULL;
}
void CScreens::ExportGraphicDataToExcel(void)
{
	static unsigned int uListId = 0;
	uListId++;
	CGraphicsToExcel g2e;

	int n = 0;
	POSITION pos = mRectanglesList.GetHeadPosition();
	while (pos)
	{
		CDisplayRectangle *pDR = mRectanglesList.GetNext(pos);
		n += pDR->AddGESToList(g2e,uListId);
	}
	if (n < 1)
	{
		CMyWindows::MessBox("<CScreens::ExportGraphicDataToExcel> No Rois Found - nothing to save","Warning");
		return;
	}

	g2e.Print();
}
