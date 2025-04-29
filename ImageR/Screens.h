#pragma once
#include "afxtempl.h"
#include "ViewParams.h"
#include "..\..\ImageRLib\ZoomPanParams.h"
#include "atltypes.h"
#include "DisplayCoordinates.h"
#include "..\..\ImageRLib\ImageRIFBase.h"
#include "..\..\yUtils\MyCriticalSection.h"

class CScreens : public CImageRIFBase
{
public:
    typedef enum ESatusBarFields
    {
        SBAR_GENERAL,
        SBAR_PAGE = 1,
        SBAR_STATISTICS = 2,
        SBAR_TYPED = 3,
        SBAR_DATA = 4,
        SBAR_POS = 5,
        SBAR_WINDOW_WIDTH = 6,
        SBAR_WINDOW_CENTER = 7,
        SBAR_LAST = 8
    } ESatusBarFields;

	CString masSatusBarTexts[SBAR_LAST];

    // Construct/Destruct
    CScreens(void);
    ~CScreens(void);

    void OnNewRect(bool bLoadingSavedState = false);
    void AddData(class CMetaData *pData, bool bAddDisplay = true);
    void OnExtraNewRectangleAdjustSize(int delta);
    class CDisplayRectangle * GetRectangleWithData(const char *zDataName);
    class CDisplayRectangle * GetDisplayRectangle(int i);
    class CDisplayRectangle * GetCurrentDisplayRectangle(void);
    class CMetaData* GetCurrentFileData(bool bOrAny = false);
    class CMetaData* GetCurrentData(void);
    class CVolumetric * GetCurrentVolumetricData(void);
    class COblique * GetCurrentObliqueData(bool bOrAny = false);
    class CDiffData * GetCurrentDiffData(void);
    class CMetaData * GetData(int iDR);
    class CMetaData * GetCurrentDataForWindowing(void);
    class CMetaData * GetCurrentDataForScroll(void);

    bool UpdateDataCheck(void);
    bool OnViewPrevfile(void);
    bool OnViewNextfile(void);
    void SetImageView (class CImageView *pImageView) {mpImageView = pImageView;}
    class CImageView * GetImageView (void) {return mpImageView;}
    COLORREF *GetGrayRgbTable (void){return maGrayRGBTable;}
    COLORREF *GetColorsRgbTable (void){return maColorsRGBTable;}
    CDataCoordinates GetPopupDataCoo(void){return mPopupMenuDataCoo;}
    class CStatusBarCtrl *mpStatusBar;

    void ListData(CList<class CMetaData*,class CMetaData*> &dataList);
    UINT CursorLocationMsg(void) {return mCursorLocationMsg;}
	CSize GetWantedSize(void){return mWantedSize;}
	void SetTitle(const char *zTitle = NULL);
	CString GetTitle(void){return umsScreenTitle;}
	void UpdateStatusBarOnNextPaint(void) {mbUpdateStatusBarOnNextPaint = true;}

	CMyCriticalSection mCritical;
	bool mbCallGlobalClean;

    static CString umsRawScreenTitle;
    static CString umsImageScreenTitle;
	static CString umsDisclaimer;
	static bool umbDisplayDisclaimer;

    class CImageView *mpImageView;
    class CDisplayRectangle* GetCurrentDR() { return mpCurrentDR; }

private:
    static CString umsScreenTitle;
    static CString umsVersion;

    UINT mSaveDoneMsg;
    UINT mCursorLocationMsg;
    UINT mImageRLeftMouseUpMsg;
	UINT mWindowMsg;

    // Manage Window Size
    CSize mScreenSize;
    CSize mImageSize;
    CSize mMaxSize;
    CSize mWantedSize;

    // Manage Display Rectangles
    CList<class CDisplayRectangle *, class CDisplayRectangle *> mRectanglesList;

    class CDisplayRectangle *mpCurrentDR;
    class CDisplayRectangle *mpWaitKillFocusDR;
    bool mbCaptureSet;

    COLORREF         maGrayRGBTable[MAX_COLORS+1];
    COLORREF         maColorsRGBTable[MAX_COLORS+1];

    CPoint mPrevPoint;
    CPoint mPopupMenuPoint;
    CDataCoordinates mPopupMenuDataCoo;
    CPoint mLastMousePos;
    CPoint mNewMousePos;

    UINT mPrevMouseFlags;

    FILE *mpfLog;
    COLORREF *mpColorsRaster;
    int mnAllocated;

    bool mbMouseOnMe;
    int mnRectsInRow;
    int mnRectsRows;

	bool mbUpdateStatusBarOnNextPaint;

    class CXMLParse *mpXmlParse;

    // Constants
    static const int VERTICAL_MARGINS = 0;
    static const int HORIZONTAL_MARGINS = 20;

	static const int EXTRA_VERTICAL_MARGINS = 87;
	static const int VERTICAL_MARGINS_FOR_STATUS_BAR = 20;
	static const int EXTRA_HORIZONTAL_MARGINS = 17;

    static const int MAX_DISPLAY_RECTANGLES = 100;
    static float MOUSE_ZOOM_FACTOR;

    void SetFirstImageSize(bool bLoadingSavedState);
	void PrintWindowRect(const char *zText, FILE *pf = NULL);
    CString SaveState(const char *zName, const char *zMessage = NULL);
	void OnSizeExtended(UINT , int cx, int cy, bool force);

public:
    void UpdateDataSetTitle(void);

	CPoint &GetNewMousePos(void){return mNewMousePos;}

    CList<class CGraphicElement*,class CGraphicElement*> mGlobalRefList;
    CList<class CGraphicElement*,class CGraphicElement*> mGlobalGraphicsList;
    CList<class CMetaData*,class CMetaData*> mGlobalColorersList;

    bool AddGlobalRef(CGraphicElement *pGE);
    void UpdateDataNotice(CMetaData *pData);
    bool ScrollByMouse(CPoint point);
    bool UpdateCurrentWindowByMouse(CPoint point);
    bool UpdateZoomByMouse(CPoint point);
    bool UpdatePanByMouse(CPoint point);
    void SetScreenDivision(void);
    void DrawGraphics(class CDC &dc);

    bool SetCurrentDR(CPoint point, bool &bChange);
    bool SetCurrentDR(class CDisplayRectangle * pCurrentDR);
    void SetCurrentDRAndSetMenu(class CDisplayRectangle * pCurrentDR);
    bool SetCurrentDR(int iDR);

    bool UpdateGraphicsOnMouseMove(CPoint point);
    bool OnMButtonDown(UINT nFlags, CPoint point);
    bool OnLButtonDown(UINT nFlags, CPoint point);
    bool OnMouseMove(UINT nFlags, CPoint point);
    bool OnMousePos(CPoint point);
	bool OnRButtonDown(UINT nFlags, CPoint point);
	bool OnRButtonUp(UINT nFlags, CPoint point);
	bool OnLButtonUp(UINT nFlags, CPoint point);
	bool OnRButtonDblClk(UINT nFlags, CPoint point);

    void OnSize(UINT nType, int cx, int cy) ;
    void OnSizeSetStatusBar(int cx, int cy);

    void Paint(void);
    bool FirstPaintFinished() { return mnPaints > 0; }
    void PrintStatusInBar(ESatusBarFields eField, const char *zText);

    virtual void Refresh(void);
    static void StaticRefresh(void *pScreens);
    void ScrollLineLeft(void);
    void ScrollLineRight(void);
    void OnViewGotopage(void);
    bool OpenContextMenu(CPoint point);
    bool OpenContextMenu(CPoint point, int iSubMenu);
    void AddRefToNewData(void);
    void SaveStateForUser(void);

    void LoadSavedState(const char *zfName);
    CMetaData * GetFirstData(void);
    CDisplayRectangle *GetMipDR(void);
	static int umDebug;
    bool mbDebugMouse;
    bool mbDebugData;
    DWORD mDebugColor;
    DWORD mMainThreadId;
    int mnDatasUpdated;
    void StopWaitingForKillFocus(void);
    bool OnWaitedKillFocus(void);
    bool DisplayOnAllRectangles(class CGraphicElement * pGE);
    bool RemoveGEFromAllOtherDR(const char *zName, CDisplayRectangle *pDR);
    void SetWaitKillFocus(CDisplayRectangle * pDR);
	void SetNewPositionOnSameSeries(bool flag){mbSetNewPositionOnSameSeries = flag;}
	bool mbDisplayMinMaxAtWindowStatus;

private:
    bool mbDataSwitched;
	void AfterFileDelete(CDisplayRectangle *pRect);
	bool mbDisablePaint;
	void OnNewMultiPos(CMetaData *pCurrentData);
	bool mbSetNewPositionOnSameSeries;
    int mnScrollsSinceLastPaint;
    bool mbDebugScroll;
    int mnPaints;

public:
	bool MultiDimScroll(int iDim, bool bForward);
    bool mbSaveOnDataChange;
	void SaveStateForClient(const char* zMessage = NULL);
	void SaveStateToClientFile(const char* zfName);
	void AddGlobalColorer(CMetaData * pColorer);
    bool SelectMultiFile(void);
    void BroadcastSaveDone(int objectId);
    CString GetDataPath(void);
    int mVolumeRefreshInterval;
    void SetServerMode(bool bReplace);
    bool mbServer;
    bool mbReplace;
    class COblique * GetOblique(void);

	void DoGraphicCommand(EGraphicCommand eGCom, const char * zName);

    virtual void SetPosition(const char * zName, int iImage);
    virtual void SetRelativePosition(const char * zName, int iImage);
    virtual void SetPosition(const char * zName, int iDir, int iImage);
    virtual void SetDisplayCenter(const char * zName, CFloatCoordinates &centerCoo);
    virtual void SetAutoWindow(const char * zDataName, class CFloatCoordinates &dataCenterCoo);
    virtual void ToggleSpecialWindow(const char * zDataName);
	void SetWindowFromGui(int center, int width, int iRectangle = -1); // index of 1st rectangle is 0
														// -1 implies using current rectangle
	void SetWindowRangeFromGui(int minVal, int maxVal, int iRectangle = -1); // index of 1st rectangle is 0
														// -1 implies using current rectangle
	void SetZoom(float zoomFactor);

    CMetaData * FindDataByName(const char * zName, CDisplayRectangle **oppDR = NULL);
    CMetaData * FindDataByNameOrIndex(const char * zName, CDisplayRectangle **oppDR = NULL);

    virtual bool DisplayGraphic(class CGraphicElement *);

	// Get number of display rectangles
    int GetNDR(void){return (int)mRectanglesList.GetSize();}

    bool SetExternalFrame(const char * zDataName);
    void ForceDataUpdate(bool bOnlyShared = false);
	void AddColorerOnData(CMetaData * pData, CMetaData * pColorer);
public:
	bool ScrollInImagesSeries(CPoint point, int zDelta);
public:
	void OnFileDelete(void);
public:
public:
	void DeleteData(const char * zName);
	void SetWantedSize(int matrix);
	bool mbShowScanCenter;

	// Display matrix format
	int mnMaxRectanglesInRow;
	bool mbMaxRectanglesInRowSetByUser;
	void SetMaxRectanglesInRow(int maxRIR, bool bSetByUser = true);
	void SetMaxRectanglesInRowFlipVertical(void);

	bool mbIgnoreServerTimingProblems;
	void DisplayGlobalGraphicsOnNewRect(CDisplayRectangle * pNewRect);
	void OnDicomCompare(void);
	void DeleteAllGraphics(bool bRefresh = true);
	void DeleteAllTexts(bool bRefresh = true);
	bool mbKeepScreenSizeConstant;
	bool mbEbwMode;

	void KeepScreenSizeConstant(void){mbKeepScreenSizeConstant = true;}

	void ZeroColorRaster(void);

	CGraphicElement *mpGEWithContextMenu;
	bool mbLeftMouseDoingGraphics;
	void SetEbwMode(void);
	bool mbOpenContextMenuOnRButtonUp;
	void BroadcastWindow(int iRectangle = -1); // Default is current window
	void SetCursorBroadcast(int iRectangle = -1); // Default is current window
	void DisablePaint() { mbDisablePaint = true; }
	void EnablePaint() { mbDisablePaint = false; }
	int miPipeMessageToHandle;
	int miPipeMessageHandled;
	void DeleteAllData();
	int GetIFreeOrNewDR();
	bool HasDiffInput();
	void GetListOfDataUsingPosition(class CPosition *pPosition, 
		CList <CMetaData *, CMetaData *> &list);
	void SetStatusText(const char * zText, int iField);
	void InitLeftMouseUpMsg(void);
	bool mbAllowPaging;
	void AllowPaging(bool flag);
	bool ReadColorMap(const char * zfName, int iDR = -1);
	void SetColorMap(bool bFlag, int iDR = -1);
	static bool umbDebugColorMap;
	CPoint mRButtonDownPoint;
	void CheckForRepeatedDataOrAdjustPosition(CMetaData *pData);
	bool VerifyScreenMode(void);
	int mnBitsPerPixel;
	void ReadRegistry(class CMyRegistry &myReg);
	void DisplayWindowStatus(CMetaData *pData = NULL, FILE *pfLog = NULL);

	bool SetDisplayWidth(int n);
	bool mbCommonDisplayWidth;
	int mnCommonDisplayWidth;
	void OpenLogFile(void);
	void OnGEDeleted(CGraphicElement *pGE);
	void ExportGraphicDataToExcel(void);

    // Adjustable configuration values
    int mFastScrollFactor;
};

extern CScreens gScreens;
