#pragma once
#include "..\..\ImageRLib\MetaDataBase.h"
#include "..\..\yUtils\MyCriticalSection.h"
#include "ViewParams.h"
#include "DisplayCoordinates.h"
#include "WindowParams.h"
#include "..\..\ImageRLib\ZoomPanParams.h"

//---------------------------------------------------------------------------------

#define MAX_META_DATA_NAME_LEN 256
#define META_DATA_ID_VERSION 6
#define META_DATA_N_SPARE 253

//---------------------------------------------------------------------------------

class CMetaData : public CMetaDataBase
{
public:
    typedef struct SStatistics
    {
        int mn;
        float mSum;
        float mAverage;
        float mStandardDeviation;
    } SStatistics;

    CMetaData(const char *zName, class CXMLParseNode *pXmlNode = NULL);
    virtual ~CMetaData(void);
    void PostConstruct(class CXMLParseNode *pXmlNode);
    virtual void ParsePosition(class CXMLParseNode *pPosXmlNode);
    virtual void InitPosition(void);
    bool SetNewPosition(const char * zName, int iFirst, int iLast, CXMLParseNode *pMetaDataXml = NULL);

    bool ComputeStatistics(CTImage<bool> &mask, SStatistics &oStat);
    bool IsRealCtImage (void) {return IsCtImage() && (mRescaleSlope == 1);}
    bool UseIntegerWindowStatus (void);

    static int umDebug;
	static CMetaData *umpDebugData;
	virtual CMetaData *GetDataWithText(void){return this;}
	class CDRText * AddDataText(const char *zText, bool bDataSpecific = true);
	virtual int NeedDataTexts(void){return 0;}
	virtual bool WantContextMenu(int &oiMenu);

	virtual void TrapBug(const char *zText1, const char *zText2 = NULL)
	{
		if (umpDebugData)
			umpDebugData->TrapBug(zText1, zText2);
	}

    virtual bool HasSpecialParameters (void) {return false;}
    bool IsOfShape(int nLines, int nCols){return ((mnLinesInPage == nLines) && (mnCols == nCols));}

    virtual int i(void){return 1;}
    virtual void close(void){};
    virtual bool open(void){return true;}

    //virtual bool read_image (int , class MyImage &){return false;}
	virtual bool set_i (int ){return true;}
    virtual CString GetPageText (void) {return CString("P");}
    virtual int GetSpecTextSize(void){return IsCtImage() ? 55 : 0;}
    virtual int GetDataTextSize(void){return 60;}

    virtual void PrintReadingHeaders(void){};

    virtual bool IsMx8000Prep (void) {return false;}
    virtual bool IsHybrid (void) {return false;}
    virtual bool IsDicom (void) {return false;}
    virtual bool IsDicomMatrix (void) {return false;}
    virtual bool IsAirTable() {return false;}
    virtual bool IsChannelTable() {return false;}
    virtual bool IsRaw() {return false;}
    virtual bool IsReadingsFile() {return false;}
    virtual bool IsFile() {return false;}
    virtual bool IsOblique() {return false;}
    virtual bool IsVolumetric(void){return false;}
    virtual bool IsMip(void){return false;}
	virtual class CMipVolume *PMipVolume(void){return NULL;}
	virtual class CMip *PMip(void){return NULL;}
    virtual bool ReadyForWindowFocus(void){return true;}
    virtual bool AllowColors(void){return true;}
	virtual bool IsOnTop(void){return false;}
    virtual CString GetWindowName(void);

    virtual class CMetaData *GetMetaData(void) {return this;}
    virtual void SetDisplayCenter (CFloatCoordinates dataCoo);

    // Colorer Interface
    virtual bool IsColorer(void){return false;}
    //virtual bool IsAlsoColorer(void){return false;}
    virtual bool UpdateColorer(class CDisplayRectangle &dr);
    virtual void AdjustColors(class CDisplayRectangle &dr);
    void AdjustColorsByDataColoring(class CDisplayRectangle &dr);
	bool IsDataForColoring(void){return mbIsDataForColoring;}

	// Data coloring static params
	static bool umbDisplayImageOnZeroColor;
	static bool umbColorTransparency;
	static float umTransparencyFraction;

    virtual bool OnRawDefinedetectorasbad(void){return false;}
    virtual bool OnRawDefinedetectorasgood(void){return false;}
    virtual bool OnRawUpdatebaddetectorsinmdu(void){return false;}
    virtual void OnRawPrintbaddetectorstable(void){}
    virtual void OnSave(void){}
    virtual void OnLoad(void){}
    virtual void OnPrint(void){}
    virtual void OnCompute(void){}
    virtual bool OnNext(void){return false;}
    //virtual bool OnAdd(void){return false;}
    //virtual bool OnDelete(void){return false;}

    virtual bool OnMousePos(CDataCoordinates &){return false;}
    virtual bool OnMousePosOnRef(CDataCoordinates &){return false;}
    virtual bool OnMousePosNotOnRef(void){return false;}
    

    virtual void OnAssignDR (void){};
    void AssignDisplayRectangle (class CDisplayRectangle &dr);

    virtual bool ContainsFile(const char *zName){return msName == zName;}

    virtual int GetMatrix(void) {return 0;}
    virtual bool IsOfType(const char*) {return false;}
    virtual int GetFanHeaderLen(void) {return 0;}
    virtual int GetSliceLen(void) {return 0;}
    virtual int GetNSlices(void) {return 0;}
    virtual int GetNFFS(void) {return 0;}
    virtual ULONGLONG GetOffsetBytes(void) {return 0;}
	virtual void ReReadDicom(void){}
    virtual ULONGLONG GetLength(void) {return 0;}
    virtual int GetSliceLenWithoutFooter(void) {return 0;}
    virtual void set_file_offset(ULONGLONG ) {};
	virtual HANDLE h(void) {return INVALID_HANDLE_VALUE;}
    virtual CMetaData *GetSourceData(void){return NULL;}
    virtual void AddRefBetween(CMetaData*){};
    bool VolumeIsShared(void) {return mbVolumeIsShared;}
    void AttachToSharedVolume(class CFileMapping * pMapping);
	virtual bool OnRefDataUpdate(void){return false;}

    virtual bool DisableZoom(void){return false;}
    virtual CSize DefaultImageSize(void);
    CFloatCoordinates GetCenter();

    virtual void OnStartUpdate (void){};

    virtual CString GetStatisticsStringPerCoo(class CDataCoordinates coo);
    virtual CString GetSpecialString(class CDataCoordinates coo);
    virtual CString GetZoomName(void){return CString("ZoomParams");}
	virtual CString GetPageDataText(int /*i*/){return CString("PageText");}

    const char * z_name(){return (const char *)msName;}

    virtual bool IsValid(void){return mbValid;}
    bool SetSize(int nLines, int nCols);

    float NonZeroAverage (void);
    float Average (void);
    float Min();
    float Max();
    float Range();
    SRoi &GetLoadRoi(void) {return mLoadRoi;}
	virtual bool IsShared () {return false;}
	virtual int n_diff (void) {return 0;}

	virtual CString GetExtraName (const char *z_name);
	virtual CString GetTitleName (void);
	CString PrivateName(void);
    CString FileType();
    bool HasFileType();
    void SetLogFile (FILE *pf){mpfLog = pf;}
    void SetDefaultLogFile (FILE *pf){if (!mpfLog) mpfLog = pf;}

    void PrintScreenToExcel (void);

    // Window Manaqement
    CWindowParams *GetWindowParams(void);
	CWindowParams *GetWindowParamsTyped(void);

	void SetWindowParamsFromGui(float center, float width);
	void SetWindowRangeFromGui(float minVal, float maxVal);
    bool ToggleSpecialWindow(void);
    void DisconnectFromWindow(void);
    void DisconnectFromSpecialWindow(void);
    void DisconnectFromZoom(void);

    void SetSpecialWindow(CWindowParams * pWindowParams = NULL);
    void SetWideWindow(void);
    CDisplayRectangle *GetDR(){return mpDR;}
    bool IsOn(CDisplayRectangle *pDR){return mpDR == pDR;}
    void SetXmlParseNode (class CXMLParseNode *pXmlNode){mpXmlNode = pXmlNode;}

    COLORREF mColor;
	bool UseSpecialWindow(void) {return mbUseSpecialWindow;}
	bool UseSpecialZoom(void) {return mbUseSpecialZoom;}
	static bool umbMultiTypeWindows;
	CMyImage *GetZoomedRaster(void){return mpZoomedRaster;}
	friend class CMultiPageLoader;
	CList<class CDRText*,class CDRText*> *GetTextList(void){return &mDataTextList;}

protected:
    CList<class CDRText*,class CDRText*> mDataTextList;
	class CXMLParseNode* mpXmlNode;  // In case this data was parsed from saved state
	class CXMLParseNode* mpXmlMetaDataNode;  // In case this data was parsed from saved state
	class CXMLParseNode* mpXmlMetaDataBaseNode;  // In case this data was parsed from saved state

    CWindowParams *mpCommonWindowParams;
    CWindowParams *mpSpecialWindowParams;

	static int umiFirstType;
	static int umiLastType;

	bool mbUseSpecialWindow;
    void SetCommonWindow(CWindowParams * pWindowParams);

    class CDisplayRectangle *mpDR;

    FILE *mpfLog;
    bool mbValid;
    bool mbTransparent;
	bool mbDisplayStatistics;

    // Read statistics
    class CTypedData *mpTypedData;
	bool mbTypedDataCreatedByMe;

    SRoi mLoadRoi; // Current ROI of file in Load Buffer

    virtual bool DoUpdatePageData(void);
    void OnFillZoomedRasterError(const char *zText);
	void DeleteAllText(void);

	bool mbChangeEndian;

private:
    CZoomPanParams *mpCommonZoomPanParams;
    CZoomPanParams *mpSpecialZoomPanParams;
	bool mbUseSpecialZoom;

	CTImage<float> *mpZoomedRasterFloat;
	CTImage<S_RGB> *mpZoomedRasterRgb;
	CMyImage *mpZoomedRaster;

	CRect mZoomedRect;
	static CMyCriticalSection mCriticalUpdate;
	CFloatCoordinates *mpWantedCenterCoo;
	//static float umInitialZoomFromCommandLine;

protected:
    CMyImage *mpPageData;
	bool mbPageDataCreatedByMe;

    bool CreatePageData (void);

	void UpdateTexts(void);
	bool GetDumpName(CString &sName, const char *zExtension);
	void DumpAllPageData(const char *zUserName);
	void DumpAllPageDataCoronals(const char *zUserName);
    void DumpAllPageDataSagittals(const char *zUserName);
    void DumpAllPageDataCoronalsMultiFile(const char* zUserName);
    void DumpAllPageDataSagittalsMultiFile(const char* zUserName);
    bool GetInitialZoom(float &oZoomFactor, EZoomMode * poeMode = NULL);

	bool mbMultiPageDisplay;
	class CMultiPageLoader *mpMultiPageLoader;
	bool mbIsDataForColoring;

public:
	class CTypedData *GetTypedData(void){return mpTypedData;}

	const CRect &GetZoomedRect(void){return mZoomedRect;}
    void SetTypedDataHandler(EMyImageDataType eDataType);
    unsigned int GetUnitSize (void);
    // Zoom functionality - should be done on Meta Data level
    void ToggleSpecialZoom (void);
    void SetNewCommonZoom (void);
	virtual void SetNewPosition(void);
	virtual void SetSecondLeafingIndex(void){}
    CZoomPanParams *GetZoomParams (void);
    CZoomPanParams *GetCommonZoomParams (void);
    bool HasZoomPanParams (void) {return mpCommonZoomPanParams != NULL;}
    void SetCommonZoom (CZoomPanParams *pZoomPanParams);
    void SetSpecialZoom ();

    class CMyImage *PageData(void);
    class CMyImage &CurPage(void) {return *mpPageData;}

	class CVolume* GetVolume(void) { return mpVolume; }
	bool HasVolume(void) { return mpVolume != NULL; }
	bool HasPageData();
    virtual void PrintId(FILE * pf, const char * zText);
    bool ForceUpdatePageData(void);
    virtual bool UpdatePageData(void);
    virtual void * GetPageData(void);
    virtual void * GetPageDataLine(int iLine);
    //void * GetDataForDisplay(int iLineInPage, int iColInPage);
    CString GetValueString(class CDataCoordinates coo);
    float GetFloatValue(class CDataCoordinates coo);
    bool FillZoomedRaster(class CDisplayRectangle &displayRect);
    CTImage<float> *GetFloatZoomedRaster(void);
    float * GetFloatZoomedRasterData(void);
    S_RGB * GetRgbZoomedRasterData(void);
    //CTImage<float> &GetZoomedRaster(void){return mZoomedRaster;}
    //float * GetZoomedRasterData(void){return mZoomedRaster.GetData();}

    virtual void SaveState(class CXMLDump & xmlDump);
    void AddDualModeToName(void);
    CString GetShortName(void);

    void SetMmPerPixel(float width, float height, bool bOnlyOnce = false);
    void SetMmPerPixel(CMetaData & other);
    int GetNVolumePages(void);
	bool mbMmPerPixelSet;
	void ForceUpdate(void) {mbPageUpdated = false;}

	virtual bool SaveToRecentFiles(void){return IsFile();}
	virtual CString NameToRecentFiles(void){return Name();}

	bool TryToGetImageSizeFromName(void);
	void DumpSlice(bool bAll = false, bool bUncompressDicom = false);
	void DumpSlicesSubSet();
    void DumpCoronals(void);
    void DumpSagittals(void);
    void PrintStatistics(void);
	void PrintStatisticsAvi(void);
	void PrintStatisticsDiff(CMetaData *pOther);
	bool ZoomedRasterIsReady(void);

	void SaveZoomedRaster(void);
	virtual void EditContextMenu(CMenu * /*pPopup*/){}
	virtual void OnNewGE(CGraphicElement * /*pGE*/){}
	virtual bool DesplayXmlHeader(void){return false;}
	virtual void PrintFromAllDicom(void);
	virtual bool SpecialMouseScroll(CPoint /*point*/, bool & /*obChanged*/){return false;}
	int GetDisplayWidth(void);
	bool SetDisplayWidth(int n);
	CString GetAveragingText(void);
	void SetDataForColoring(void);
	static void ReadRegistry(class CMyRegistry &myReg);
	static void ToggleColorsBaseOnZero(void);
	static void ToggleTransparency(void);
	static bool GuiTransparencyFraction(void);
	static void SetTransparencyPC(int percent);
	bool UserSelectDataType(void);
	CString msUserDefinedTitle;
	CString GetDRTitle(void);
	bool GetDataRangeText(CString &osText);
	virtual bool AddToTitleText(CString &){return false;}
	void OpenWindowingGUI(void);
	void ByeFromWindowDialog(void);
	class CWindowDlg *mpWindParamsDlg;
	void SetVerticalZoom(void);
	void SetVerticalZoom(float factor);
	bool PrintDicom(void);
	virtual void PrintValue2Excel(FILE *pf, int iLine, int iCol);
	virtual void PrintValue2Csv(class CCsvFile &csv, int iLine, int iCol);
	virtual void PrintExcelHeader(FILE *pf);
	virtual void PrintCsvHeader(class CCsvFile &csv);
	bool ScalePageData(class CCompVector &scaled);
	bool SaveToDicom(const char* zDicomFileName);
	bool SaveToMinimalDicom();
	void ChangeEndian();
	void ChangeEndianInPage();
    void SetSyncPosition();

	// Control loaded volume
public:
    bool LoadVolume(void);
    bool StartLoadVolume(void);
    void DeleteVolume();
	static void ToggleEnableFastLoadFromVolume();
	static bool umbEnableFastLoadFromVolume;
    bool IsLoadingVolume() { return mbLoadingVolume; }
    void ExportHistogram();
    void ToggleCTMode();

protected:
    static DWORD WINAPI StaticLoadVolume(void* p);
    virtual bool DoLoadVolume();
    static DWORD WINAPI StaticExportHistogram(void* p);
    void DoExportHistogram();

	class CVolume* mpVolume;
	bool mbVolumeIsShared;
	bool mbFastLoadFromVolume;
	bool mbLoadingVolume;
    bool mbUpdateForLoadingVolume;
    int mnPageLoads;
};
