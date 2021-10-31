#pragma once
#include "afxtempl.h"
#include "ViewParams.h"
#include "DisplayCoordinates.h"
#include "..\..\ImageRLib\Dependant.h"
#include "..\..\ImageRLib\ZoomPanParams.h"
#include "..\..\ImageRLib\TImage.h"
#include "..\..\ImageRLib\GraphicElement.h"

class CDisplayRectangle : public CRect, public CDependant
{
public:
    CDisplayRectangle(FILE *pfLog, class CXMLParseNode *pDRXml = NULL);
    ~CDisplayRectangle(void);

private:
    CList<class CMetaData*,class CMetaData*> mDataList;
    CList<class CMetaData*,class CMetaData*> mColorersList;
    CList<class CGraphicElementOnDR*,class CGraphicElementOnDR*> mGraphicList;
    CList<class CMetaData*,class CMetaData*> mRefList;
    CList<class CDRText*,class CDRText*> mTextList;

    CList<class CDRText*,class CDRText*> mCurrentImageTextList;
	CString msNameOfCurrentImageWithText;

    CGraphicElementOnDR *mpCurrentGE;
    bool mbEnableColorers;
	bool mbEnableText;
	bool mbEnableGraphics;

    class CXMLParseNode *mpDRXml;

    class CMetaData *mpCurrentData;
    FILE *mpfLog;

    CTImage<unsigned char> mGrayRaster;
    CTImage<unsigned char> mGrayRasterForColorer;
    CTImage<COLORREF> mColorRaster;
    bool mbColorRasterUpdated;
    COLORREF *mpColors;

    int mColorsLineLen;
    CDataCoordinates mMouseDataCoo;

	static DWORD umDebug;
	static DWORD umDebugText;
	static bool umbWrapOnNextAndPrev;
	static int umId;

public:
	int mDRId;
	static bool umbPrintDataCoo;
	static bool umbSaveZoomedRaster;
	static bool umbSaveZoomedRasterAsShort;
	static bool umbDisplayTitles;
	static bool umbDisplayDataRange;

    void DeleteGE(class CGraphicElement *pGE);
    virtual void Bye(CDependant* );

    CDataCoordinates GetSelectedDataCoo(void);
    CDRCoordinates *mpSelectedPoint;

    CList<class CMetaData*,class CMetaData*> &GetDataList(void){return mDataList;}
	int GetNData(void){return (int)mDataList.GetSize();}

    class CXMLParseNode *GetXMLParseNode(void) {return mpDRXml;}
    CGraphicElementOnDR *FindGraphicNear(CDRCoordinates &drCoo);
    CDataCoordinates GetMouseDataPos(void){return mMouseDataCoo;}
    void ToggleColors(void);
    void ToggleText(void);
	void ToggleGraphics(void);

    //CDRCoordinates mCenter;
    void AddData(class CMetaData * pData);
    CMetaData * GetCurrentData(void);
    CMetaData * GetCurrentDataForWindowing(void);
    bool OnViewPrevfile(void);
    bool OnViewNextfile(void);

    void DrawGraphics(CDC &dc);
    bool AddRef(CMetaData *pData);

    bool AddGraphicElementFromXml(class CXMLParseNode *pNode);
    bool AddGraphicElement(CGraphicElement *pElement);
    bool AddGraphicElement(CGraphicElementOnDR * pElement);
	bool AddGraphicElementFromSharedMemImage(class CSharedMemFile *pSharedMemFile);

    bool RemoveGraphicElement(CGraphicElementOnDR * pElement);

    bool OnLButtonDown(UINT nFlags, CScreenCoordinates screenCoo);
    bool OnLButtonUp(UINT nFlags, CScreenCoordinates screenCoo);
	bool OnRButtonDblClk(UINT nFlags, CScreenCoordinates screenCoo);
    bool OnMousePos(CScreenCoordinates screenCoo);
	void OnLoosingMouse(void);
    void DisplayWindowStatus(void);
	bool UpdateMousePos(CScreenCoordinates screenCoo);
	bool UpdateMousePosToNewScreenCoo(void);

    void SetShape(int left, int top, int right, int bottom);
    bool DragGraphics(CSize size);
    void Paint(void);
    void SetColorsRaster(COLORREF * pStart, int lineLen);
    void ApplyColorers(void);
	void CopyZoomedRgbToScreen(void);
    void ApplyColorer(CMetaData *pColorer, FILE *pfLog);
    void PrintStatusStatistics(CDataCoordinates &dataCoo);
    void PrintStatusBar(CDRCoordinates drCoo);
    void PrintStatusBarWithLastCoo(void);
	CDRCoordinates mLastDrCoo;

    CDRCenteredCoordinates GetStartCoo(void);
    CDRCenteredCoordinates GetEndCoo(void);
    CDataCoordinates GetDataStartCoo(void);
    CDataCoordinates GetDataEndCoo(void);

    CRect GetDisplayRect(void);
    int GetRasterLen(void);
    void DrawLine(CDC &dc, CDRCoordinates p1, CDRCoordinates p2, COLORREF color);
    void DrawLineOnScreen(CDC &dc, CScreenCoordinates p1, CScreenCoordinates p2, COLORREF color);

    bool ClipCoo(LONG & x, LONG & y, const LONG x1, const LONG y1, const LONG maxX);
    void OnViewGotopage(void);
    bool ScrollLineLeft(void);
    bool ScrollLineRight(void);
    CDataCoordinates SetSelectedPoint(CPoint point);
    void SetSelectedPoint(CDRCoordinates &drCoo);
    void OnViewZoom(void);
    void SaveState(class CXMLDump & xmlDump, bool bCurrent);
	class CGCursor * AddCursor(const char * zName, COLORREF color = CGraphicElement::GetDefColor());
    void AddContour(const char * zName, COLORREF color = CGraphicElement::GetDefColor());
    class CGLine * AddLine(const char * zName, COLORREF color = CGraphicElement::GetDefColor());
	static const int ELLIPSE_DEFAULT_SIZE_MM = 10;

    class CGRoi * AddRoi(const char * zName, COLORREF color = CGraphicElement::GetDefColor(), 
		int sizeMm = ELLIPSE_DEFAULT_SIZE_MM);
    class CGEllipse * AddEllipse(const char * zName, COLORREF color = CGraphicElement::GetDefColor(), 
        bool bCircle = false, int sizeMm = ELLIPSE_DEFAULT_SIZE_MM);
	void AddSinus(class CRawFile *pRaw);
    class CRingID * AddScanCenter(class CXMLParseNode *pGraphicsXml = NULL);
    void OnGraphicDelete(void);
    bool UpdateColorRaster(void);
    void GrayToScreenColors(void);
    void ColorRasterToScreenColors(void);
	void ClearScreen(void);

    void AddRefBetween(CDisplayRectangle * pOtherDR);
    void OnDataEclipse(void);
	void DeleteGraphics(bool bOnDataEclipse = false);
    void InitFromXml(CXMLParseNode * pDRXml);
    void InitDataFromXml(CXMLParseNode *pDataXml);
    bool InitGraphicsFromXml(CXMLParseNode *pGraphicsXml,
            CList<class CGraphicElement*,class CGraphicElement*> *pList = NULL);
    void InitTextsFromXml(CXMLParseNode *pTextsXml);
	void CheckXmlForDisabledGE(CGraphicElement *pGE, CXMLParseNode *pGraphicsXml);

    float GetMinScreenSizeMm(void);
    bool OnKillFocus(void);
    void OnGraphicDisplayonallimages(void);
    CZoomPanParams * GetZoomParams(void);
    CZoomPanParams * GetCommonZoomParams(void);
    class CPosition * GetPosition(void);
    void AddColorer(CMetaData * pColorer);
    bool mbDisableStateRestore;
    bool mbBroadcastCursorLocation;
    void SetBroadcastCursorLocation (bool flag) {mbBroadcastCursorLocation = flag;}
    void AddText(class CDRText * pText);
	bool DisplayTextOfCurrentImage(CDC * pDC);
    bool DisplayText(CDC * pDC);
	void DisplayTextItem(class CDRText *pText, CDC * pDC);
	void DisplayTitle(CDC * pDC);
	void DisplayDataRange(CDC * pDC);
	void DisplayTitleText(CDC * pDC, const char *zText);

    void OnGraphicSetcolor(void);
    class CMyImage * GetCurrentDataPage(void);
    bool DeleteText(int id);
    bool DeleteText(const char * zPrefix);
    void AddUserText(void);
    bool mbIsCurrent;
    bool SetDRCurrent(CPoint point);
    bool IsCurrent(void){return mbIsCurrent;}
    void DrawFrame(CDC &dc, int delta, COLORREF color);
    int mbExternalFrame;
	bool DoGraphicCommand(EGraphicCommand eGCom, const char * zName);
	bool DeleteCurrentData(void);
	CGraphicElementOnDR *FindGraphic(const char * zName);

	bool DeleteData(const char * zName);
	void UpdateDataCheck(void);
	void OnGraphicRename(void);
	void OnGraphicSetText(void);
	void DeleteTexts(void);
	bool mbShouldBroadcastIfLoosingMouse;
	bool SpecialMouseScroll(CPoint point, bool & obChanged);
	bool mbUseColorMap;
	void OnGraphicDisplayStatistics(void);
	void OnMultiDimScroll(int iDim);
	bool FillGrayRaster(CMetaData *pData, CTImage<unsigned char> &grayRaster, bool bColorer = false);
	CTImage<unsigned char> &GetGrayRasterForColorer(void);
	CTImage<COLORREF> &GetColorRaster(void){return mColorRaster;}
	bool HasDataColoring(void);
	static void ReadRegistry(class CMyRegistry &myReg);
	int mnPaint;

	bool SetDisplayWidthToAllData(int n);
	CString msUserDefinedTitle;
	static const int umTextHeight = 20;
	static const int umTextCharWidth = 20;
	static float umCharFracPerTextWidth;
	//int mId; - deleted - use mDRId
	void SetTitleByUser(bool bToData);
	bool OnGraphicSavetofile(void);
	bool OnGraphicSaveData2Excel(void);
	int mnTitleLinesDisplayed;
	int AddGESToList(class CGraphicsToExcel &g2e, unsigned int listId);

	static bool umbReverseRGBOrder;
};
