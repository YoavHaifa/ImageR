#pragma once
// ImageView.h : header file
//
#include "resource.h"
#include "ViewParams.h"
#include "DataArea.h"
#include "MetaData.h"

/////////////////////////////////////////////////////////////////////////////
// CImageView dialog

#define SCROLL_TOP    0
#define SCROLL_BOTTOM -1
#define SCROLL_CUR    -2

#define MAX_VIEWER_FILES 20

class CImageView : public CDialog
{
// Construction
public:
	bool AddFile (const char *zName, bool bNewDisplay = true);
	void ReadRegistry(void);
	void MyErrExit (const char *zText);
    bool SetDisplayFiles(void);

	bool set_data_size (unsigned int ByteLength);

    FILE *GetPFLog(void);
	CImageView(int dialogResource, CWnd* pParent = NULL); 
    virtual ~CImageView();

    void ImageViewerSetOn(int *pOn) { mpImageViewerOn=pOn; }
    void post_load_data(int FirstRow);

    float get_data_value (unsigned int iRow, unsigned int iCol);

    // Dialog Data
    //{{AFX_DATA(CImageView)
    enum DialogResource { IDD = IDD_IMAGEVIEW_DIALOG, IDD1 = IDD_IMAGEVIEW_DIALOG1 };
    //}}AFX_DATA


	void StartPipeIF(const char *zPipeName);
	void InitiateSetDisplayFiles(void);
	bool WantNewDisplayFiles() {return mbWantNewDisplayFiles;}

	void SetParentWindowHandle(HWND parentHandle);
	HWND GetParentWindowHandle();
	void InitAfterCommandLineIsProcessed(CString &sCommandLine);

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CImageView)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

    // Implementation
	int mbDCP;
    void PaintZoomInterpolate(RECT &UpdRect);
    void PaintZoomDuplicate(RECT &UpdRect);

	bool open_files();
	bool SelectDataFile (class CFileName &fName);

	bool mbWantNewDisplayFiles;
	CPoint mUpperLeftPoint;
	CPoint mPrevPoint;
	bool mbPrevpoint;
    CString msLastOpenPath;

	int n_rows_in_display_rect (int RectRows);

    UINT mLoadMsg;
    UINT mSaveMsg;
    UINT mFreezeMsg;
    UINT mUnFreezeMsg;
	UINT mSaveBroadPosMsg;
    UINT mUpdateMsg;
	UINT mInitializationMsg;

	//virtual BOOL CreateDlgIndirect(LPCDLGTEMPLATE lpDialogTemplate,
	//CWnd* pParentWnd, HINSTANCE hInst);


	// Generated message map functions
	//{{AFX_MSG(CImageView)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnParametersWindow();
	afx_msg void OnFileOpen();
	afx_msg void OnPaint();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnViewStatistics();
	afx_msg void OnDestroy();
	afx_msg void OnViewGotopage();
    afx_msg void OnViewPrevfile();
    afx_msg void OnViewNextfile();
	afx_msg void OnViewZoom();
	afx_msg void OnViewResizetopagesize();

	afx_msg void OnFileRecentfiles0();
	afx_msg void OnFileRecentfiles1();
	afx_msg void OnFileRecentfiles2();
	afx_msg void OnFileRecentfiles3();
	afx_msg void OnFileRecentfiles4();
	afx_msg void OnFileRecentfiles5();
	afx_msg void OnFileRecentfiles6();

	afx_msg void OnClose();
	//afx_msg void OnFileAdd();
	afx_msg void OnSettingsSetfullroi();
	afx_msg void OnWindowSpecialWindow();
	//afx_msg void OnAddDiff();
	afx_msg void OnPrintExcel();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    CStatusBarCtrl *mpStatusBar;
    //CDataView      *mpDataView;
    int             mbDataViewOn;

    SCommonParams    mCommonParams;
    SImageViewParams mImViewParams;
    SRoi             mRoi; // Current ROI - Full or Partial

    int              mPixMaxWidth;
    int              mPixMaxHeight;
    int              mPixMinWidth;
    int              mPixMinHeight;

    unsigned int     mnPages;
    unsigned int     mLeftCol;
    unsigned int     mRightCol;
    unsigned int     miFirstRowInDisplayPage;
    unsigned int     mnCurrentDisplayRows;
    unsigned int     mnDisplayRowsInPagesRow;
    unsigned int     mnDisplayRowsInMultiPage;

    SZoomParams mZoomParams;
    float *mpZoomWeights;
	class CPipeIF   *mpPipeIF;

    CDataArea        mDataArea;

    int  *mpImageViewerOn;
	FILE *mpfLog;

    unsigned int mnColsPreZoom;
    unsigned int mnColsToDisplay;
    unsigned int mnZoomedDisplayRowsInPage;
    unsigned int mnZoomedDisplayedRowsInMultiPage;
    unsigned int mnRowsRead;

    UINT mCompareReadyMsg;

    int miFileSelectingWindow; // file that needs new window selections

    void set_scrolls(int ViewX, int ViewY, int FirstRow=SCROLL_CUR);
    void resize_window_to_page_size(RECT &Rect);
    void WriteParamsToFile(const char *zfName);

    void open_recent_file(int index);

    void init_on_construction();
    bool mbLastMoveHorizontal;
    bool mbMouseMoved;
	int mbDebugTiming;

    class CPipeWriter *mpDv2EcgvPipe;
	HWND parentWindowHandle;
	//HWND clientWindowHandle;
public:
	afx_msg void OnFileExit1();
    afx_msg void OnExportScreentoexcel();
    afx_msg void OnPrintInfo();
    afx_msg void OnWindowAutoWide();
    afx_msg void OnWindowAutofocus();
    afx_msg void OnWindowSelectwindow();
    afx_msg void OnExportDumpslice();
    afx_msg void OnWindowLocalfocus();
    afx_msg void OnFileOpenair();
    afx_msg void OnViewCardiacdata();
    bool mbViewCardiac;
    afx_msg void OnSettingsSelectimagesbyparameters();
    afx_msg void OnFileAddcoronal();
    afx_msg void OnViewTogglecolors();
    void OnInitMenuPopup(CMenu *pPopup);
    afx_msg void OnRawDefinedetectorasbad();
    afx_msg void OnRawUpdatebaddetectorsinmdu();
    afx_msg void OnRawDefinedetectorasgood();
    afx_msg void OnRawPrintbaddetectorstable();
    afx_msg void OnObliqueSetcoronal();
    afx_msg void OnObliqueSetsagittal();
    afx_msg void OnFileSaveState();
    afx_msg void OnGraphicDelete();
    afx_msg void OnAddSinogram();
    afx_msg void OnAddSmooth();
    afx_msg void OnSettingsInteractivetuning();
    void EditMenu(void);
    bool mbLongMenus;
    bool mbDicomMaster;
    bool mbDicomDateMaster;
    bool mbChangeDicomTables;
    afx_msg void OnGraphicAddrectangle();
    afx_msg void OnGraphicAddellipse();
    afx_msg void OnHelpOpenmanual();
    afx_msg void OnGraphicDisplayonallimages();
    afx_msg void OnViewSpecialZoom();
    afx_msg void OnGraphicAddcursor();
    afx_msg void OnDicomDate();
    afx_msg void OnFileOpendirectory();
    void OnSaveStateForClient(void);
	void OnBroadPos(void);
    void OnLoadToCurrentDR(void);
    afx_msg void OnDicomPrint();
    CString msFilesFile;
    //CString msStartFile;
	CList<CString*,CString*> msStartFiles;
	bool mbStartDiff;
    afx_msg void OnAddMip();
    //afx_msg void OnAddRatio();
    void OnAddDiffOrRatio(const char *zName);
    afx_msg void OnGraphicSetcolor();
    afx_msg void OnMipSelectslice();
    afx_msg void OnGraphicAddtext();
    afx_msg void OnDicomEdittextfields();
    afx_msg void OnMultidataSetsize();
    afx_msg void OnDiffCompare();
    afx_msg void OnDiffRed();
    afx_msg void OnDiffGreen();
    afx_msg void OnDiffBlue();
    afx_msg void OnGraphicAddcircle();
    afx_msg void OnViewRefresh();
	afx_msg void OnAirPrintalltoexcel();
	afx_msg void OnGraphicAddline();
	afx_msg void OnDicomSaveuncomprerssed();
	afx_msg void OnDicomCompare();
	afx_msg void OnExportDumpallslices();
public:
	afx_msg void OnFileDelete();
public:
	void SetTopMost(void);
public:
	afx_msg void OnGraphicRename();
public:
	afx_msg void OnGraphicSettext();
public:
	afx_msg void OnMipClientmessage();
	afx_msg void OnMipSetvoi();
	afx_msg void OnMipTopovoi();
public:
	afx_msg void OnGraphicAddcontour();
	afx_msg void OnDicomAmendallbyxml();
	afx_msg void OnDicomShowscancenter();
	afx_msg void OnHybridStopAuto();

	void AddMenuItem(const char *zSubject, int id, const char *zText);
	void RemoveMenuItem(const char *zSubject, int id);
	void CheckMenuItem(const char *zSection, int id, bool bCheck);
	void EnableMenuItem(const char *zSubject, int id);
	CMenu *MyGetSubMenu(const char *zSubject);

	afx_msg void OnViewDatacoo();
	void SetUpperLeft(int x, int y);
	afx_msg void OnExportSavezoomed();
	afx_msg void OnSettingsNewposition();
	bool mbEnableAddSinogram;
	afx_msg void OnDicomCheckseries();
	afx_msg void OnViewToggletext();
	afx_msg void OnGraphicDeleteallgraphics();
	afx_msg void OnGraphicDeletealltexts();
	bool mbUseMenuController;
	afx_msg void OnNewimageSaveseries();
	afx_msg void OnDiffSelectplus();
	afx_msg void OnDiffSelectminus();
	afx_msg void OnViewTogglegraphics();
	CString msCommandLinePassword;
	afx_msg void OnDicomPrintfromallslices();
	afx_msg void OnRingidShow();
	afx_msg void OnRingidDelete();
	afx_msg void OnRingidFixcenter();
	afx_msg void OnGenericSpecialwindow();
	afx_msg void OnGenericSpecialzoom();
	afx_msg void OnPresetSelected0();
	afx_msg void OnPresetSelected1();
	afx_msg void OnPresetSelected2();
	afx_msg void OnPresetSelected3();
	afx_msg void OnPresetSelected4();
	afx_msg void OnPresetSelected5();
	afx_msg void OnPresetSelected6();
	afx_msg void OnPresetSelected7();
	afx_msg void OnPresetSelected8();
	afx_msg void OnPresetSelected9();
	afx_msg void OnDiffStatistics();
	afx_msg void OnPrintStatistics();
	afx_msg void OnAddCombine();
	afx_msg void OnGraphicDisplaystatistics();
	afx_msg void OnPrintDumpcoronals();
	afx_msg void OnSettingsSecondleafindex();
	afx_msg void OnSettingsVertical();
	afx_msg void OnSettingsNcolumns();
	afx_msg void OnSettingsDisplaywidth();
	afx_msg void OnFileOpencolorer();
	afx_msg void OnMenuColorsBaseOnZero();
	afx_msg void OnColoringToggletransparency();
	afx_msg void OnColoringTransparencyfraction();
	void OnNewColorer(const char *zfName);
	afx_msg void OnSettingsDatatype();
	afx_msg void OnWindowShowminmax();
	afx_msg void OnExportDumpsub();
	afx_msg void OnDiffFindbestfit();
	afx_msg void OnExportReorderfiles();
	afx_msg void OnExportReversefilesorder();
	afx_msg void OnExportSavewindow();
	afx_msg void OnViewTitles();
	afx_msg void OnSettingsTitle();
	afx_msg void OnSettingsTitleforimageset();
	afx_msg void OnMultidataShowchessheader();
	afx_msg void OnGraphicSavetofile();
	afx_msg void OnGraphicLoadfromxml();
	afx_msg void OnViewDatarange();
	afx_msg void OnObliqueSetverticalzoom();
	afx_msg void OnObliqueInvertup();
	afx_msg void OnSettingsVerticalzoomfactor();
	afx_msg void OnGraphicSavedatatoexcel();
	afx_msg void OnExportGraphicdatatoexcel();
	afx_msg void OnAddFile33992();
	afx_msg void OnAddDiff33993();
	afx_msg void OnAddCoronal33994();
	afx_msg void OnAddMip33995();
	afx_msg void OnAddSmooth33996();
	afx_msg void OnAddCombine33997();
	afx_msg void OnAddRatio33998();
	afx_msg void OnDicomExportrastertodicom();
};

