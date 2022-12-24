// ImageView.cpp : implementation file
//

#include "stdafx.h"
#include <math.h>
#include <process.h>    /* _beginthread, _endthread */
#include "resource.h"
#include "PrintExcel.h"
#include ".\imageview.h"
#include "..\..\yUtils\DynamicTimer.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyDicomPrint.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyTracer.h"
#include "..\..\yUtils\CodeFile.h"
#include "..\..\yUtils\NameGetDialog.h"
#include "..\..\yUtils\PipeWriter.h"
#include "..\..\yUtils\MyFileDialog.h"
#include "..\..\yUtils\MyMenu.h"

#include "PipeIf.h"
#include "DiffData.h"
#include "FileFactory.h"
#include "Screens.h"
#include "DisplayRectangle.h"
#include "GRoi.h"
#include "GEllipse.h"
#include "..\..\yUtils\ParametersControl.h"
#include "..\..\yUtils\ParamsDlg.h"
#include "..\..\yUtils\DicomFieldDialog.h"
#include "RecentFiles.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\ImageDiffScorer.h"
#include "..\..\ImageRLib\Zoomer.h"
#include "..\..\ImageRLib\WindowAutoFocus.h"
#include "..\..\ImageRLib\Rle1Read.h"
#include "..\..\ImageRLib\ScreenImage.h"

#include "..\..\yUtils\MyRegistry.h"
#include "..\..\yUtils\XMLDump.h"
#include "ArchivesFile.h"
#include "Mip.h"
#include "MultiDataFile.h"
#include "HybridFile.h"
#include "MenuController.h"
#include "RingID.h"
#include "ContextMenu.h"
#include "WindowDlg.h"
#include "Oblique.h"
#include "ProcImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RIGHT_MARGIN 8

#define LOW_MARGIN 82

const	int cMultiFileMagic = 0xabcd4323;
//------------------------------------------------------------------------

FILE * CImageView::GetPFLog(void)
{
    return mpfLog;
}

/////////////////////////////////////////////////////////////////////////////
// CImageView dialog

CImageView::CImageView(int dialogResource, CWnd* pParent /*= NULL*/)
	: CDialog(dialogResource, pParent)
	, mpPipeIF(NULL)
	, mUpperLeftPoint(0,0)
    , mbLongMenus(false)
    , mbDicomMaster(false)
    , mbDicomDateMaster(false)
	, mbChangeDicomTables(false)
    , msFilesFile(_T(""))
    //, msStartFile(_T(""))
	, mbEnableAddSinogram(false)
	, mbUseMenuController(true)
	, mpfLog(NULL)
	, mbWantNewDisplayFiles(false)
	, mbDCP(false)
	, msCommandLinePassword(_T(""))
	, mbStartDiff(false)
	, mbDebugTiming(false)
	//, msCommandLinePassword(_T("2010")) // Special Hard Coded password for DCP (Customer Support) 
{
    init_on_construction();
}

//------------------------------------------------------------------

void CImageView::init_on_construction()
{
    gScreens.SetImageView(this);

	mCompareReadyMsg = RegisterWindowMessage("CompRdy");

    mpStatusBar = new CStatusBarCtrl;
	mnColsToDisplay = 0;
	mnRowsRead = 0;
	mnPages = 0;
    mbLastMoveHorizontal = false;

    miFileSelectingWindow = -1;

    mPixMaxWidth=0;
    mPixMaxHeight=0;

    mLeftCol=0;
    mRightCol=0;

    mnCurrentDisplayRows = 0;
    miFirstRowInDisplayPage = 0;

    mPrevPoint.x=mPrevPoint.y=0;
    mbPrevpoint = false;

    //mpDataView=NULL;
    mbDataViewOn=false;

    mpZoomWeights=NULL;

    mPixMinWidth=128 + RIGHT_MARGIN;
    mPixMinHeight=128 + LOW_MARGIN;

    mpImageViewerOn=NULL;

    mbViewCardiac = false;
    mpDv2EcgvPipe = NULL;

	parentWindowHandle = NULL;
	//clientWindowHandle = NULL;
	CMyWindows::SetClientWindowHandle(NULL);

    mLoadMsg = RegisterWindowMessage("ImageRLoad");
    mSaveMsg = RegisterWindowMessage("ImageRSave");
    mFreezeMsg = RegisterWindowMessage("ImageRFreeze");
    mUnFreezeMsg = RegisterWindowMessage("ImageRUnFreeze");
    mSaveBroadPosMsg = RegisterWindowMessage("ImageRBroadcastPosition");
    mUpdateMsg = RegisterWindowMessage("ImageRUpdate");
    mInitializationMsg = RegisterWindowMessage("ImageRInitilization");
}
//--------------------------------------------------------------------------------------
void CImageView::InitiateSetDisplayFiles(void)
{
	mbWantNewDisplayFiles = true;
	gScreens.Refresh();
	Sleep(100);
}
//--------------------------------------------------------------------------------------
bool CImageView::SetDisplayFiles(void)
{
	mbWantNewDisplayFiles = false;
	if (!mpPipeIF)
		return false;

	int iAtSubRect = 0;
	int nFilesFound = 0;
	{
		CMyCriticalSpan(gScreens.mCritical, "CImageView::SetDisplayFiles");
		static int count = 0;
		CMyTracer myTracer("CScreens::SetDisplayFiles-critical", ++count);
		CString sName;
		while (mpPipeIF->GetFirstFileName(sName))
		{
			if (mpfLog)
			{
				fprintf (mpfLog, "<SetDisplayFiles> %s\n", (const char*)sName);
				fflush(mpfLog);
			}
			bool bOK = false;
			if (sName.Left(2) == "::")
			{
				sName = sName.Right(sName.GetLength() - 2);
				if (sName == "Mip")
					OnAddMip();
				else if (sName == "Diff")
				{
					if (gScreens.GetNDR() > 1)
						OnAddDiff();
				}
				else if (sName.Left(2) == "at")
				{
					sName = sName.Right(sName.GetLength() - 2);
					iAtSubRect = atoi(sName);
				}
			}
			else
			{
				if (iAtSubRect)
				{
					gScreens.SetCurrentDR(iAtSubRect-1);
					bOK = AddFile(sName, false);
					iAtSubRect = 0;
				}
				else
				{
					bOK = AddFile(sName);
					if (!bOK)
					{
						Sleep(500);
						bOK = AddFile(sName);
					}
				}
				if (bOK)
					nFilesFound++;
			}
		}
	}

	if (!nFilesFound)
		return false;

    gScreens.Refresh();
    return true;
}
//---------------------------------------------------------------------
CImageView::~CImageView()
{
	if (mpPipeIF)
		delete mpPipeIF;

    if (mpStatusBar)
        delete mpStatusBar;

    if (mpZoomWeights)
        delete mpZoomWeights;

	if (mbDebugTiming)
	{
		CFileName fName("timing.log");
		FILE *pf = CMyWindows::FOpenLogFile (fName, true/*bAppend*/);
		if (pf)
		{
			CEventTracer::report_all(pf);
			fclose(pf);
		}
	}

	if (mpfLog)
		fclose(mpfLog);
}
void CImageView::StartPipeIF(const char *zPipeName)
{
	mpPipeIF = new CPipeIF (zPipeName, this);
}
//-----------------------------------------------------------------------

void CImageView::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImageView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CImageView, CDialog)
	//{{AFX_MSG_MAP(CImageView)
	ON_WM_SIZE()
	ON_COMMAND(ID_PARAMETERS_WINDOW, OnParametersWindow)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
    ON_WM_MOUSEWHEEL()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_COMMAND(ID_VIEW_STATISTICS, OnViewStatistics)
	ON_WM_DESTROY()
	ON_COMMAND(ID_EDIT_GOTO, OnViewGotopage)
	ON_COMMAND(ID_VIEW_ZOOM, OnViewZoom)
	ON_COMMAND(ID_VIEW_RESIZETOPAGESIZE, OnViewResizetopagesize)

	ON_COMMAND(ID_FILE_RECENTFILES0, OnFileRecentfiles0)
	ON_COMMAND(ID_FILE_RECENTFILES1, OnFileRecentfiles1)
	ON_COMMAND(ID_FILE_RECENTFILES2, OnFileRecentfiles2)
	ON_COMMAND(ID_FILE_RECENTFILES3, OnFileRecentfiles3)
	ON_COMMAND(ID_FILE_RECENTFILES4, OnFileRecentfiles4)
	ON_COMMAND(ID_FILE_RECENTFILES5, OnFileRecentfiles5)
	ON_COMMAND(ID_FILE_RECENTFILES6, OnFileRecentfiles6)

	ON_WM_CLOSE()
	ON_COMMAND(ID_FILE_EXIT1, OnFileExit1)
	ON_COMMAND(ID_ADD_FILE, &CImageView::OnAddFile)
	ON_COMMAND(ID_SETTINGS_SETFULLROI, OnSettingsSetfullroi)
	//ON_COMMAND(ID_SETTINGS_SETCURRENTROI, OnSettingsSetcurrentroi)
	ON_COMMAND(ID_SETTINGS_SETSECONDWINDOW, OnWindowSpecialWindow)
	ON_COMMAND(ID_PRINT_EXCEL, OnPrintExcel)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_PRINT_SCREENTOEXCEL, OnExportScreentoexcel)
    ON_COMMAND(ID_PRINT_INFO, OnPrintInfo)
    ON_COMMAND(ID_WINDOW_AUTO, OnWindowAutoWide)
    ON_COMMAND(ID_WINDOW_AUTOFOCUS, OnWindowAutofocus)
    ON_COMMAND(ID_WINDOW_SELECTWINDOW, OnWindowSelectwindow)
    ON_COMMAND(ID_PRINT_DUMPSLICE, OnExportDumpslice)
    ON_COMMAND(ID_WINDOW_LOCALFOCUS, OnWindowLocalfocus)
    ON_COMMAND(ID_FILE_OPENAIR, OnFileOpenair)
    ON_COMMAND(ID_VIEW_CARDIACDATA, OnViewCardiacdata)
    ON_COMMAND(ID_SETTINGS_SELECTIMAGESBYPARAMETERS, OnSettingsSelectimagesbyparameters)
    ON_COMMAND(ID_VIEW_PREV, OnViewPrevfile)
    ON_COMMAND(ID_VIEW_NEXT, OnViewNextfile)
    ON_COMMAND(ID_ADD_CORONAL, OnFileAddcoronal)
    ON_COMMAND(ID_VIEW_TOGGLECOLORS, OnViewTogglecolors)
    ON_COMMAND(ID_OBLIQUE_SETCORONAL, OnObliqueSetcoronal)
    ON_COMMAND(ID_OBLIQUE_SETSAGITTAL, OnObliqueSetsagittal)
	ON_COMMAND(ID_EIMAGE_NEXTSERIES, OnViewNextfile)
	ON_COMMAND(ID_EIMAGE_LOCALFOCUS, OnWindowLocalfocus)
	ON_COMMAND(ID_EIMAGE_TOGGLECOLORS, OnViewTogglecolors)
    ON_COMMAND(ID_FILE_SAVE_STATE, OnFileSaveState)
    ON_COMMAND(ID_GRAPHIC_DELETE, OnGraphicDelete)
    ON_COMMAND(ID_ADD_SINOGRAM, OnAddSinogram)
    ON_COMMAND(ID_ADD_SMOOTH, OnAddSmooth)
    ON_COMMAND(ID_DICOM_DATE, OnDicomDate)
    ON_COMMAND(ID_SETTINGS_INTERACTIVETUNING, OnSettingsInteractivetuning)
    ON_COMMAND(ID_GRAPHIC_ADDRECTANGLE, OnGraphicAddrectangle)
    ON_COMMAND(ID_GRAPHIC_ADDELLIPSE, OnGraphicAddellipse)
    ON_COMMAND(ID_HELP_OPENMANUAL, OnHelpOpenmanual)
    ON_COMMAND(ID_GRAPHIC_DISPLAYONALLIMAGES, OnGraphicDisplayonallimages)
    ON_COMMAND(ID_VIEW_NEWZOOM, OnViewSpecialZoom)
    ON_COMMAND(ID_GRAPHIC_ADDCURSOR, OnGraphicAddcursor)
    ON_COMMAND(ID_FILE_OPENDIRECTORY, OnFileOpendirectory)
    ON_COMMAND(ID_DICOM_PRINT, OnDicomPrint)
	ON_COMMAND(ID_ADD_DIFF, &CImageView::OnAddDiff)
	ON_COMMAND(ID_ADD_MIP, &CImageView::OnAddMip)
    //ON_COMMAND(ID_ADD_RATIO, OnAddRatio)
    ON_COMMAND(ID_GRAPHIC_SETCOLOR, OnGraphicSetcolor)
    ON_COMMAND(ID_MIP_SELECTSLICE, OnMipSelectslice)
    ON_COMMAND(ID_GRAPHIC_ADDTEXT, OnGraphicAddtext)
    ON_COMMAND(ID_DICOM_EDITTEXTFIELDS, OnDicomEdittextfields)
    ON_COMMAND(ID_MULTIDATA_SETSIZE, OnMultidataSetsize)
    ON_COMMAND(ID_DIFF_COMPARE, OnDiffCompare)
    ON_COMMAND(ID_DIFF_RED, OnDiffRed)
    ON_COMMAND(ID_DIFF_GREEN, OnDiffGreen)
    ON_COMMAND(ID_DIFF_BLUE, OnDiffBlue)
    ON_COMMAND(ID_GRAPHIC_ADDCIRCLR, OnGraphicAddcircle)
    ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_AIR_PRINTALLTOEXCEL, OnAirPrintalltoexcel)
	
	////////////////////////////////////////////////////////////
	ON_COMMAND(ID_GRAPHIC_ADDLINE, &CImageView::OnGraphicAddline)
	ON_COMMAND(ID_DICOM_SAVEUNCOMPRERSSED, &CImageView::OnDicomSaveuncomprerssed)
	ON_COMMAND(ID_DICOM_COMPARE, &CImageView::OnDicomCompare)
	ON_COMMAND(ID_PRINT_DUMPALLSLICES, &CImageView::OnExportDumpallslices)
	ON_COMMAND(ID_FILE_DELETE, &CImageView::OnFileDelete)
	ON_COMMAND(ID_GRAPHIC_RENAME, &CImageView::OnGraphicRename)
	ON_COMMAND(ID_GRAPHIC_SETTEXT, &CImageView::OnGraphicSettext)
	ON_COMMAND(ID_MIP_CLIENTMESSAGE, &CImageView::OnMipClientmessage)
	ON_COMMAND(ID_GRAPHIC_ADDCONTOUR, &CImageView::OnGraphicAddcontour)
	ON_COMMAND(ID_MIP_SET_VOI, &CImageView::OnMipSetvoi)
	ON_COMMAND(ID_MIP_TOPO_VOI, &CImageView::OnMipTopovoi)
	ON_COMMAND(ID_DICOM_AMENDALLBYXML, &CImageView::OnDicomAmendallbyxml)
	ON_COMMAND(ID_DICOM_SHOWSCANCENTER, &CImageView::OnDicomShowscancenter)
	ON_COMMAND(ID_WINDOW_HYBRID_STOP_AUTO, &CImageView::OnHybridStopAuto)
	ON_COMMAND(ID_VIEW_DATACOO, &CImageView::OnViewDatacoo)
	ON_COMMAND(ID_PRINT_SAVEZOOMED, &CImageView::OnExportSavezoomed)
	ON_COMMAND(ID_SETTINGS_NEWPOSITION, &CImageView::OnSettingsNewposition)
	ON_COMMAND(ID_DICOM_CHECKSERIES, &CImageView::OnDicomCheckseries)
	ON_COMMAND(ID_VIEW_TOGGLETEXT, &CImageView::OnViewToggletext)
	ON_COMMAND(ID_GRAPHIC_DELETEALLGRAPHICS, &CImageView::OnGraphicDeleteallgraphics)
	ON_COMMAND(ID_GRAPHIC_DELETEALLTEXTS, &CImageView::OnGraphicDeletealltexts)

	// ON_COMMAND(ID_NEWIMAGE_SAVESERIES, &CImageView::OnNewimageSaveseries)
	ON_COMMAND(ID_DIFF_SELECTPLUS, &CImageView::OnDiffSelectplus)
	ON_COMMAND(ID_DIFF_SELECTMINUS, &CImageView::OnDiffSelectminus)
	ON_COMMAND(ID_VIEW_TOGGLEGRAPHICS, &CImageView::OnViewTogglegraphics)
	ON_COMMAND(ID_DICOM_PRINTFROMALLSLICES, &CImageView::OnDicomPrintfromallslices)
	ON_COMMAND(ID_RINGID_SHOW, &CImageView::OnRingidShow)
	ON_COMMAND(ID_RINGID_DELETE, &CImageView::OnRingidDelete)
	ON_COMMAND(ID_RINGID_FIXCENTER, &CImageView::OnRingidFixcenter)
	ON_COMMAND(ID_GENERIC_SPECIALWINDOW, &CImageView::OnGenericSpecialwindow)
	ON_COMMAND(ID_GENERIC_SPECIALZOOM, &CImageView::OnGenericSpecialzoom)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW0, &CImageView::OnPresetSelected0)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW1, &CImageView::OnPresetSelected1)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW2, &CImageView::OnPresetSelected2)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW3, &CImageView::OnPresetSelected3)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW4, &CImageView::OnPresetSelected4)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW5, &CImageView::OnPresetSelected5)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW6, &CImageView::OnPresetSelected6)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW7, &CImageView::OnPresetSelected7)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW8, &CImageView::OnPresetSelected8)
	ON_COMMAND(ID_GENERIC_PRESET_WINDOW9, &CImageView::OnPresetSelected9)
	ON_COMMAND(ID_DIFF_STATISTICS, &CImageView::OnDiffStatistics)
	ON_COMMAND(ID_PRINT_STATISTICS, &CImageView::OnPrintStatistics)
	ON_COMMAND(ID_ADD_COMBINE, &CImageView::OnAddCombine)
	ON_COMMAND(ID_GRAPHIC_DISPLAYSTATISTICS, &CImageView::OnGraphicDisplaystatistics)
	ON_COMMAND(ID_PRINT_DUMPCORONALS, &CImageView::OnExportDumpcoronals)
	ON_COMMAND(ID_SETTINGS_SECONDLEAFINDEX, &CImageView::OnSettingsSecondleafindex)
	ON_COMMAND(ID_SETTINGS_VERTICAL, &CImageView::OnSettingsVertical)
	ON_COMMAND(ID_SETTINGS_NCOLUMNS, &CImageView::OnSettingsNcolumns)
	ON_COMMAND(ID_SETTINGS_DISPLAYWIDTH, &CImageView::OnSettingsDisplaywidth)
	ON_COMMAND(ID_FILE_OPENCOLORER, &CImageView::OnFileOpencolorer)
	ON_COMMAND(ID_COLORING_BASE_ON_ZERO, &CImageView::OnMenuColorsBaseOnZero)
	ON_COMMAND(ID_COLORING_TOGGLETRANSPARENCY, &CImageView::OnColoringToggletransparency)
	ON_COMMAND(ID_COLORING_TRANSPARENCYFRACTION, &CImageView::OnColoringTransparencyfraction)
	ON_COMMAND(ID_SETTINGS_DATATYPE, &CImageView::OnSettingsDatatype)
	ON_COMMAND(ID_WINDOW_SHOWMINMAX, &CImageView::OnWindowShowminmax)
	ON_COMMAND(ID_EXPORT_DUMPSUB, &CImageView::OnExportDumpsub)
	ON_COMMAND(ID_DIFF_FINDBESTFIT, &CImageView::OnDiffFindbestfit)
	ON_COMMAND(ID_EXPORT_REORDERFILES, &CImageView::OnExportReorderfiles)
	ON_COMMAND(ID_EXPORT_REVERSEFILESORDER, &CImageView::OnExportReversefilesorder)
	ON_COMMAND(ID_EXPORT_SAVEWINDOW, &CImageView::OnExportSavewindow)
	ON_COMMAND(ID_VIEW_TITLES, &CImageView::OnViewTitles)
	ON_COMMAND(ID_SETTINGS_TITLE, &CImageView::OnSettingsTitle)
	ON_COMMAND(ID_SETTINGS_TITLEFORIMAGESET, &CImageView::OnSettingsTitleforimageset)
	ON_COMMAND(ID_MULTIDATA_SHOWCHESSHEADER, &CImageView::OnMultidataShowchessheader)
	ON_COMMAND(ID_GRAPHIC_SAVETOFILE, &CImageView::OnGraphicSavetofile)
	ON_COMMAND(ID_GRAPHIC_LOADFROMXML, &CImageView::OnGraphicLoadfromxml)
	ON_COMMAND(ID_VIEW_DATARANGE, &CImageView::OnViewDatarange)
	ON_COMMAND(ID_OBLIQUE_SETVERTICALZOOM, &CImageView::OnObliqueSetverticalzoom)
	ON_COMMAND(ID_OBLIQUE_INVERTUP, &CImageView::OnObliqueInvertup)
	ON_COMMAND(ID_SETTINGS_VERTICALZOOMFACTOR, &CImageView::OnSettingsVerticalzoomfactor)
	ON_COMMAND(ID_GRAPHIC_SAVEDATATOEXCEL, &CImageView::OnGraphicSavedatatoexcel)
	ON_COMMAND(ID_EXPORT_GRAPHICDATATOEXCEL, &CImageView::OnExportGraphicdatatoexcel)
		ON_COMMAND(ID_DICOM_EXPORTRASTERTODICOM, &CImageView::OnDicomExportrastertodicom)
		ON_COMMAND(ID_DICOM_EXPORTTOMINIMALDICOM, &CImageView::OnDicomExporttominimaldicom)
		ON_COMMAND(ID_ADD_CHANGEENDIAN, &CImageView::OnAddChangeendian)
		ON_COMMAND(ID_DIFF_FINDBESTTRANSFORMATION, &CImageView::OnDiffFindbesttransformation)
		ON_COMMAND(ID_SETTINGS_TOGGLEFASTLOADING, &CImageView::OnSettingsTogglefastloading)
		ON_COMMAND(ID_EXPORT_HISTOGRAM, &CImageView::OnExportHistogram)
		ON_COMMAND(ID_TOGGLECTMODE_ONCURRENTIMAGE, &CImageView::OnTogglectmodeOncurrentimage)
		ON_COMMAND(ID_TOGGLECTMODE_ONALL, &CImageView::OnTogglectmodeOnall)
		ON_COMMAND(ID_EXPORT_DUMPSAGITTALS, &CImageView::OnExportDumpsagittals)
		END_MESSAGE_MAP()

//----------------------------------------------------

int CImageView::n_rows_in_display_rect(int RectRows)
{
	//return (RectRows - 36 + mZoomParams.ZoomFactor - 1) / mZoomParams.ZoomFactor;
	return RectRows - 36;
}

void CImageView::SetUpperLeft(int x, int y)
{
	mUpperLeftPoint.x = x;
	mUpperLeftPoint.y = y;
}
/////////////////////////////////////////////////////////////////////////////
// CImageView message handlers

void CImageView::InitAfterCommandLineIsProcessed(CString &sCommandLine)
{
	ReadRegistry();

	if (!msCommandLinePassword.IsEmpty() && (msCommandLinePassword != "ignore"))
	{
		if (msCommandLinePassword == "2010")
		{
			mbDCP = true;
			CString sDir("c:\\TEMP\\MyLog");
			if (CMyWindows::SetLogDir(sDir))
				CMyWindows::AddDirectoryToRemoveOnExit(sDir);
			gScreens.mbCallGlobalClean = true;
			CMyDicomPrint::umbTerminateNotepadOnexit = true;
		}
		if (sCommandLine.Find(msCommandLinePassword) < 0)
		{
			CString sTitle(gScreens.GetTitle());
			sTitle += " Error";
			CMyWindows::MessBox("You can run this application only within the DCP", 
				sTitle);
			exit(1);
		}
	}

#ifdef _DEBUG
    mpfLog = CMyWindows::FOpenLogFile("ImageR");
    CMyWindows::SetLog(mpfLog);
#endif
}

//BOOL CImageView::CreateDlgIndirect(LPCDLGTEMPLATE lpDialogTemplate,
//	CWnd* pParentWnd, HINSTANCE hInst)
//{
//	//lpDialogTemplate->dwExtendedStyle ^= WS_EX_OVERLAPPEDWINDOW;
//	DLGTEMPLATE t;
//	memcpy(&t, lpDialogTemplate, sizeof(DLGTEMPLATE));
//	
//	return CDialog::CreateDlgIndirect(&t, //lpDialogTemplate,	
//		pParentWnd, hInst);
//}


BOOL CImageView::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// This should be in OnCreate(), but strangly it's not called:
	//DWORD style = this->GetStyle();
	//this->ModifyStyleEx(WS_OVERLAPPEDWINDOW, 0); //WS_EX_STATICEDGE);
	//this->ModifyStyleEx(DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | 
	//	WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_POPUP | WS_VISIBLE | 
	//	WS_CAPTION | WS_SYSMENU |
	//	WS_EX_APPWINDOW, 0);

	if (CMyWindows::GetClientWindowHandle() != NULL)
	{
		//::PostMessage(clientWindowHandle, mInitializationMsg, (WPARAM) m_hWnd, NULL);
		CMyWindows::PostMessageToClient(mInitializationMsg, (WPARAM) m_hWnd, NULL);
	}

	if (parentWindowHandle != NULL)
	{
#if !NoSetParent
		::SetParent(m_hWnd, parentWindowHandle); // avoid MFC logic
		RECT parentWindowRectangle;
		BOOL status = ::GetWindowRect(parentWindowHandle, &parentWindowRectangle);
		if (status)
		{
			CRect myRectangle(&parentWindowRectangle);
			//myRectangle.DeflateRect(0, myRectangle.Height()/4);		
			//myRectangle.MoveToXY(1, myRectangle.Height());
			myRectangle.DeflateRect(2, 2);
			myRectangle.MoveToXY(1,1);


			this->SetWindowPos(NULL,
				myRectangle.left, myRectangle.top, 
				myRectangle.Width(), myRectangle.Height(),
				0); // flags

		}
#else
		if (parentWindowHandle != NULL)
		{
			//CWnd* ownerWindow = CWnd::FromHandle(parentWindowHandle);
			//if (ownerWindow != NULL)
			//{
			//	this->SetOwner(ownerWindow);
			//}
			::SetWindowLongPtr(m_hWnd, GWLP_HWNDPARENT, PtrToLong(parentWindowHandle) );
		}
#endif
	}
	else
	{
		if (mUpperLeftPoint.x || mUpperLeftPoint.y)
		    SetWindowPos(NULL, mUpperLeftPoint.x, mUpperLeftPoint.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	HICON hIcon = NULL;
	if (mbDCP)
		hIcon=AfxGetApp()->LoadIcon(IDI_ICON_DCP);
	else
	    hIcon=AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(hIcon, TRUE);			// Set big icon
	SetIcon(hIcon, FALSE);		// Set small icon

    CStatic *pStatic=(CStatic *)GetDlgItem(IDC_VIEWAREA);
    RECT Rect;
    GetClientRect(&Rect);
    if (pStatic) 
    {
        if (!mDataArea.SubclassWindow(pStatic->m_hWnd))
            return FALSE;
        mDataArea.SetFocus();
        pStatic->MoveWindow(0, 0, Rect.right-16, Rect.bottom-36);
    }

    //CWnd *pWnd=GetDlgItem(IDC_SCROLLBARH);
    //if (pWnd)
    //    pWnd->MoveWindow(0, Rect.bottom-36, Rect.right-16, 16);
    //pWnd=GetDlgItem(IDC_SCROLLBARV);
    //if (pWnd)
    //    pWnd->MoveWindow(Rect.right-16, 0, 16, Rect.bottom-36);

    mpStatusBar->Create(CCS_BOTTOM | WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                      CRect(0, 0, 0, 0), this, IDC_STATUS_BAR1);
    int cx=Rect.right;
    int aParts[8]={ cx-552, cx-480, cx-330, cx-275, cx-215, cx-115, cx-65, -1 };
    mpStatusBar->SetParts(8, aParts);
    gScreens.mpStatusBar = mpStatusBar;
    CMyWindows::SetStatusBar (mpStatusBar);
    CMyWindows::PrintStatus("No File Loaded");
	CheckMenuItem("&View", ID_VIEW_DATACOO, CDisplayRectangle::umbPrintDataCoo);
	CheckMenuItem("&View", ID_VIEW_TITLES, CDisplayRectangle::umbDisplayTitles);
	CheckMenuItem("&View", ID_VIEW_DATARANGE, CDisplayRectangle::umbDisplayDataRange);

    RECT WndRect;
    GetWindowRect(&WndRect);

//  mPixMaxWidth -= (WndRect.right-Rect.right)+16-MAX_ZOOM;
//  mPixMaxHeight -= (WndRect.bottom-Rect.bottom)+36-MAX_ZOOM;

	int nRows = mnRowsRead ? mnRowsRead : mCommonParams.nRowsInFile;
	//mRightCol=(min((int)mnColsPreZoom, Rect.right-16)+mZoomParams.ZoomFactor-1)/mZoomParams.ZoomFactor;
	mRightCol = min((int)mnColsPreZoom, Rect.right-16);
	mnCurrentDisplayRows = min(nRows, n_rows_in_display_rect(Rect.bottom));

	EditMenu();

	gScreens.UpdateDataSetTitle();
    gScreens.Refresh();

    if (!msFilesFile.IsEmpty())
    {
		CFileNamePattern::DisableMultiLevelFilesArray();
        CFileFactory factory(mpfLog);
        factory.CreateFiles(msFilesFile);
    }
    else if (!msStartFiles.IsEmpty())
    {
		//CFileFactory::SetReplaceFileName("F:\\ImageQualityReview\\Data\\First initial results\\DLNR\\I00002.dcm");

		POSITION pos = msStartFiles.GetHeadPosition();
		while (pos)
		{
			CString *psfName = msStartFiles.GetNext(pos);
	        AddFile(*psfName);
		}

		if (mbStartDiff && msStartFiles.GetSize() > 1)
		{
			OnAddDiff();
		}
    }

	gScreens.VerifyScreenMode();
	CWindowDlg::SetImageView(this);

	// Trying to hide the window:
	//this->ShowWindow(SW_MINIMIZE);// SW_HIDE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
CMenu *CImageView::MyGetSubMenu(const char *zSubject)
{
    CMenu *pMenu=GetMenu();
	if (!pMenu)
		return NULL;

    int pos=CMyMenu::FindMenuItem(pMenu, zSubject);
    if (pos != -1)   
		return pMenu->GetSubMenu(pos);

	CString sAccelerated("&");
	sAccelerated += zSubject;
    pos=CMyMenu::FindMenuItem(pMenu, sAccelerated);
    if (pos != -1)   
		return pMenu->GetSubMenu(pos);

	return NULL;
}
void CImageView::EnableMenuItem(const char *zSubject, int id)
{
	CMenu *pSubMenu=MyGetSubMenu(zSubject);
	if (pSubMenu)
		pSubMenu->EnableMenuItem(id, MF_ENABLED);
}
void CImageView::AddMenuItem(const char *zSubject, int id, const char *zText)
{
	CMenu *pSubMenu=MyGetSubMenu(zSubject);
	if (pSubMenu)
		pSubMenu->AppendMenu(MF_STRING, id, zText);
}
void CImageView::RemoveMenuItem(const char *zSubject, int id)
{
	CMenu *pSubMenu=MyGetSubMenu(zSubject);
	if (pSubMenu)
		pSubMenu->RemoveMenu(id, MF_BYCOMMAND);
}
void CImageView::EditMenu(void)
{
    CMenu *pMenu=GetMenu();
	if (!pMenu)
		return;

	int prevState = pMenu->RemoveMenu(8, MF_BYPOSITION);

    if (!mbLongMenus)
    {
        // File options
        prevState = pMenu->RemoveMenu(ID_FILE_OPENAIR, MF_BYCOMMAND);
        // View
        prevState = pMenu->RemoveMenu(ID_VIEW_STATISTICS, MF_BYCOMMAND);
        prevState = pMenu->RemoveMenu(ID_VIEW_RESIZETOPAGESIZE, MF_BYCOMMAND);
        prevState = pMenu->RemoveMenu(ID_VIEW_CARDIACDATA, MF_BYCOMMAND);
        // Settings
        prevState = pMenu->RemoveMenu(ID_SETTINGS_SETFULLROI, MF_BYCOMMAND);
        prevState = pMenu->RemoveMenu(ID_SETTINGS_SETDISPLAYCENTER, MF_BYCOMMAND);
        prevState = pMenu->RemoveMenu(ID_SETTINGS_SELECTIMAGESBYPARAMETERS, MF_BYCOMMAND);
        // Window
        prevState = pMenu->RemoveMenu(ID_WINDOW_SELECTWINDOW, MF_BYCOMMAND);
        // Print
        //prevState = pMenu->RemoveMenu(ID_PRINT_DUMPSLICE, MF_BYCOMMAND);
        //prevState = pMenu->RemoveMenu(ID_PRINT_DICOM, MF_BYCOMMAND);
        // Dicom
		prevState = pMenu->RemoveMenu(ID_ADD_MIP, MF_BYCOMMAND);
		prevState = pMenu->RemoveMenu(ID_ADD_COMBINE, MF_BYCOMMAND);
		prevState = pMenu->RemoveMenu(ID_ADD_RATIO, MF_BYCOMMAND);
		//prevState = pMenu->RemoveMenu(ID_ADD_SMOOTH, MF_BYCOMMAND);
	}

	if (mbUseMenuController)	// "false" = don't run , "true" = run
	{
		CMenuController cMenu;
		cMenu.RunXML();
	}
	/*else 
	{
		CMenu *pMenu=GetMenu();
		if (pMenu)
		{
			int pos=CMyMenu::FindMenuItem(pMenu, "&File");
			if (pos != -1) 
			{
				CMenu *pSubMenu=pMenu->GetSubMenu(pos);
				pSubMenu->EnableMenuItem(0, MF_BYPOSITION|MF_GRAYED);
				pSubMenu->EnableMenuItem(1, MF_BYPOSITION|MF_GRAYED);
			}
			pos=CMyMenu::FindMenuItem(pMenu, "&Settings");
			if (pos != -1) 
			{
				CMenu *pSubMenu=pMenu->GetSubMenu(pos);
				pSubMenu->EnableMenuItem(1, MF_BYPOSITION|MF_GRAYED);
			}
		}
	} */

    if (!mbDicomMaster)
    {
        // Remove special DICOM options
        prevState = pMenu->RemoveMenu(ID_DICOM_CHANGETABLE, MF_BYCOMMAND);
        prevState = pMenu->RemoveMenu(ID_DICOM_EDITTEXTFIELDS, MF_BYCOMMAND);
        prevState = pMenu->RemoveMenu(ID_DICOM_AMENDALLBYXML, MF_BYCOMMAND);
    }
    if (!mbDicomDateMaster)
		pMenu->RemoveMenu(ID_DICOM_DATE, MF_BYCOMMAND);
    // Remove anyway!
    pMenu->RemoveMenu(ID_SETTINGS_SETFULLROI, MF_BYCOMMAND);
    pMenu->RemoveMenu(ID_SETTINGS_SETDISPLAYCENTER, MF_BYCOMMAND);
    pMenu->RemoveMenu(ID_SETTINGS_SELECTIMAGESBYPARAMETERS, MF_BYCOMMAND);
    pMenu->RemoveMenu(ID_SETTINGS_INTERACTIVETUNING, MF_BYCOMMAND);

    CMenu *pSubMenuFile=MyGetSubMenu("File");
    if (pSubMenuFile) 
    {
        int pos=CMyMenu::FindMenuItem(pSubMenuFile, "&Recent Files");
        if (pos != -1) 
        {
            CMenu *pSubMenuRecent=pSubMenuFile->GetSubMenu(pos);
            gRecentFiles.init(pSubMenuRecent);        
        }

        pos=CMyMenu::FindMenuItem(pSubMenuFile, "&Add");
        if (pos != -1) 
        {
            CMenu *pSubMenuAdd=pSubMenuFile->GetSubMenu(pos);

			if (mbEnableAddSinogram)
				pSubMenuAdd->AppendMenu(MF_STRING, ID_ADD_SINOGRAM, "Sinogram");

        }
    }
}

//---------------------------------------------------------------------------------------

void CImageView::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED)
        return;

    gScreens.OnSize (nType, cx, cy);

    //set_scrolls(cx-16, cy-36);

    InvalidateRect(NULL,false);
}


//---------------------------------------------------------------------------------------

LRESULT CImageView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	static bool ubFreeze = false;
	static int count = 0;
	FILE *pf = NULL; // MyFOpen("d:\\MyLog\\CImageView__WindowProc.log",count++?"a":"w");
	//if (!pf)
	//	pf = CMyWindows::FOpenLogFile ("ImageR_WindowProc");
	if (pf)
	{
		int command=LOWORD(wParam);
        static int ucount = 0;
        ucount++;
    
		if (message ==  WM_LBUTTONDOWN )
            fprintf (pf, "%3d:  %4d WM_LBUTTONDOWN  %d\n", ucount, message, command);
		else if (message ==  WM_LBUTTONUP )
            fprintf (pf, "%3d:  %4d WM_LBUTTONUP  %d\n", ucount, message, command);
		else if (message == mSaveMsg)
            fprintf (pf, "%3d:  Save State %d\n", ucount, message);
		else if (message == mFreezeMsg)
            fprintf (pf, "%3d:  Freeze %d\n", ucount, message);
		else if (message == mUnFreezeMsg)
            fprintf (pf, "%3d:  UnFreeze %d\n", ucount, message);
		else
            fprintf (pf, "%3d:  %4d  %d\n", 
                ucount, message, command);
		//fflush(pf);
		fclose(pf);
	}

	if (message == mUnFreezeMsg)
	{
		ubFreeze = false;
		return 0;
	}
	if (ubFreeze)
		return 0;

	if (message == mFreezeMsg)
	{
		ubFreeze = true;
		return 0;
	}
	if (message == mSaveMsg)
	{
		OnSaveStateForClient();
		return 0;
	}
	if (message == mSaveBroadPosMsg)
	{
		OnBroadPos();
		return 0;
	}
	if (message == mLoadMsg)
	{
		OnLoadToCurrentDR();
		return 0;
	}
	if (message == mUpdateMsg)
	{
        gScreens.Refresh();
		return 0;
	}   
    switch (message) 
    {
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *pMinMax=(MINMAXINFO *)lParam;
            pMinMax->ptMinTrackSize.x=mPixMinWidth;
            pMinMax->ptMinTrackSize.y=mPixMinHeight;
            if (mPixMaxHeight == 0 || mPixMaxWidth == 0) 
            {
                mPixMaxWidth=pMinMax->ptMaxTrackSize.x;
                mPixMaxHeight=pMinMax->ptMaxTrackSize.y;
            }
            return 0;
        }
        case IVM_UPDATE_WIND_PARAMS:
		{
            gScreens.Refresh();
            return 0;
		}
        //case WM_COPYDATA:
        //    load_data((COPYDATASTRUCT *)lParam);
        //    return TRUE;
    }
	return CDialog::WindowProc(message, wParam, lParam);
}
//--------------------------------------------------------------------

void CImageView::OnParametersWindow() 
{
	CMetaData *pData = gScreens.GetCurrentDataForWindowing();
    if (pData)
		pData->OpenWindowingGUI();
 }
//--------------------------------------------------------------------

void CImageView::OnFileOpen () 
{
    CFileName fName;
    if (SelectDataFile (fName))
		AddFile(fName, false);
}
void CImageView::OnFileOpendirectory()
{
    CString sPath;
	if (CMyWindows::SelectDirectory("Select Data Path", sPath))
		AddFile(sPath, true);
}
void CImageView::OnFileOpencolorer()
{
	CMyFileDialog dlg(CMyFileDialog::FD_OPEN, "Select Colorer");
	if (dlg.DoModal())
	{
		CString sPath(dlg.GetPathName());
		if (CFileName::Exist(sPath))
			OnNewColorer(sPath);
	}
}
void CImageView::OnNewColorer(const char *zfName)
{
	CFileFactory factory(mpfLog);
	CMetaData *pData = factory.CreateFileObject(zfName);
	pData->SetDataForColoring();
	CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
	pDR->AddColorer(pData);
	gScreens.Refresh();
}

//------------------------------------------------------------------------------
void CImageView::OnPaint() 
{
	gScreens.Paint ();
}
//-----------------------------------------------------------------------

float CImageView::get_data_value(unsigned int , unsigned int )
{
    return 0;
}
//-----------------------------------------------------------------------
void CImageView::OnLButtonUp(UINT nFlags, CPoint point) 
{
    bool bChange = gScreens.OnLButtonUp(nFlags, point);
	CDialog::OnLButtonUp(nFlags, point);
    if (bChange)
        InvalidateRect(NULL,false);
}
//-----------------------------------------------------------------------
void CImageView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CDialog::OnLButtonDblClk(nFlags, point);
}
//-----------------------------------------------------------------------
void CImageView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
    bool bChange = gScreens.OnRButtonDblClk(nFlags, point);
	CDialog::OnRButtonDblClk(nFlags, point);
    if (bChange)
        InvalidateRect(NULL,false);
}
//-----------------------------------------------------------------------
void CImageView::OnRButtonDown(UINT nFlags, CPoint point)
{
    bool bChange = gScreens.OnRButtonDown(nFlags, point);
	CDialog::OnRButtonDown(nFlags, point);
    if (bChange)
        InvalidateRect(NULL,false);
}
//-----------------------------------------------------------------------
void CImageView::OnRButtonUp(UINT nFlags, CPoint point)
{
    bool bChange = gScreens.OnRButtonUp(nFlags, point);
	CDialog::OnRButtonUp(nFlags, point);
    if (bChange)
        InvalidateRect(NULL,false);
}
//-----------------------------------------------------------------------
void CImageView::OnMButtonDown(UINT nFlags, CPoint point)
{
    bool bChange = gScreens.OnMButtonDown (nFlags, point);

	CDialog::OnMButtonDown(nFlags, point);
    if (bChange)
        InvalidateRect(NULL,false);
}
//-----------------------------------------------------------------------
void CImageView::OnLButtonDown(UINT nFlags, CPoint point)
{
    bool bChange = gScreens.OnLButtonDown (nFlags, point);

	CDialog::OnLButtonDown(nFlags, point);
    if (bChange)
        InvalidateRect(NULL,false);
}
//-----------------------------------------------------------------------
void CImageView::OnMouseMove(UINT nFlags, CPoint point) 
{
    bool bChange = gScreens.OnMouseMove (nFlags, point);

	CDialog::OnMouseMove(nFlags, point);
    if (bChange)
        InvalidateRect (NULL, false);
}
BOOL CImageView::OnMouseWheel(UINT , short zDelta, CPoint point) 
{
    RECT WndRect;
    GetWindowRect(&WndRect);
	point.x -= WndRect.left;
	point.y -= WndRect.top;
	return gScreens.ScrollInImagesSeries(point, zDelta);
}
void CImageView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	int Delta=0;
	UINT Mask=SIF_POS | SIF_TRACKPOS;
	SCROLLINFO ScrInfo= { sizeof(SCROLLINFO), Mask,
							0, 0, 0, 0, 0 };

	//if (!pScrollBar)
	//	pScrollBar=(CScrollBar *)GetDlgItem(IDC_SCROLLBARH);

	if (pScrollBar)
		pScrollBar->GetScrollInfo(&ScrInfo, Mask);
  switch (nSBCode) {
    case SB_LINELEFT:
        gScreens.ScrollLineLeft();
        break;
    case SB_LINERIGHT:
        gScreens.ScrollLineRight();
        break;
    case SB_PAGELEFT:
      Delta=-(int)min(IMAGE_PAGE_SIZE, mLeftCol);
      break;
    case SB_PAGERIGHT:
      Delta=min(IMAGE_PAGE_SIZE, (int)mnColsPreZoom-(int)mRightCol);
      break;
    case SB_THUMBTRACK:
      Delta=ScrInfo.nTrackPos-ScrInfo.nPos;
      break;
    case SB_BOTTOM:
      Delta = mnColsPreZoom - mRightCol;
      break;
    case SB_TOP:
      Delta = -(int)mLeftCol;
      break;
    default:
      Delta=0;
      break;
  }

  if (Delta) {
    ScrInfo.fMask=SIF_POS;
    ScrInfo.nPos=mLeftCol+Delta;
    mLeftCol += Delta;
    mRightCol += Delta;
    pScrollBar->SetScrollInfo(&ScrInfo);
    InvalidateRect(NULL,false);
  }

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

//---------------------------------------------------------------------------

void CImageView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
   
    UINT Mask=SIF_POS | SIF_TRACKPOS;
    SCROLLINFO ScrInfo= { sizeof(SCROLLINFO), Mask, 0, 0, 0, 0, 0 };

    //if (!pScrollBar)
    //    pScrollBar=(CScrollBar *)GetDlgItem(IDC_SCROLLBARV);

	if (pScrollBar)
		pScrollBar->GetScrollInfo(&ScrInfo, Mask);

    CMetaData *pData = gScreens.GetCurrentDataForScroll();
    if (pData)
    {
        CPosition *pPosition = pData->GetPosition();
        if (pPosition)
        {
            bool bChange = false;

            switch (nSBCode) 
            {
            case SB_LINEUP:
				gScreens.MultiDimScroll(2,false);
                break;
            case SB_LINEDOWN:
				gScreens.MultiDimScroll(2,true);
                break;
            case SB_PAGEUP:
				gScreens.MultiDimScroll(3,false);
                break;
            case SB_PAGEDOWN:
				gScreens.MultiDimScroll(3,true);
                break;
            case SB_THUMBTRACK:
                break;
            case SB_BOTTOM:
                bChange = pPosition->Last();
                break;
            case SB_TOP:
                bChange = pPosition->First();
                break;
            default:
                break;
            }
            if (bChange)
            {
                gScreens.Refresh();
            }
        }
        else
        {
            if (mpfLog)
                fprintf (mpfLog, "No Position\n");
        }
    }
    else
    {
        if (mpfLog)
            fprintf (mpfLog, "No Data\n");
    }
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}
void CImageView::OnViewStatistics() 
{
  //if (!mpDataBuf)
  //  return;
  /*

  mpStatistics=new CStatistics(this, &mCommonParams, this);

  mpStatistics->DoModal();
  */
}

void CImageView::OnDestroy() 
{
	mDataArea.UnsubclassWindow();
	CDialog::OnDestroy();
}
//-------------------------------------------------------------------------------------

void CImageView::OnViewGotopage() 
{
    gScreens.OnViewGotopage();
}

//-------------------------------------------------------------------------------------

void CImageView::OnViewZoom() 
{
    CDisplayRectangle * pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnViewZoom();
}

//---------------------------------------------------------------------------------------

void CImageView::resize_window_to_page_size (RECT &Rect)
{
	CWnd* parentWindow = this->GetParent();
	if (parentWindow != NULL)
	{
		return;
	}

  RECT WndRect;
  GetWindowRect(&WndRect);

  Rect.bottom = mnZoomedDisplayedRowsInMultiPage + 36;
  if (Rect.bottom > mPixMaxHeight)
    Rect.bottom = mPixMaxHeight;
  else if (Rect.bottom < mPixMinHeight-LOW_MARGIN)
    Rect.bottom=mPixMinHeight-LOW_MARGIN;

  if (WndRect.top + Rect.bottom > mPixMaxHeight)
    WndRect.top = mPixMaxHeight-Rect.bottom;

  Rect.right = mnColsToDisplay + 16;
  if (Rect.right > mPixMaxWidth)
    Rect.right = mPixMaxWidth;
  else if (Rect.right < mPixMinWidth - RIGHT_MARGIN)
    Rect.right = mPixMinWidth - RIGHT_MARGIN;
  if (WndRect.left + Rect.right > mPixMaxWidth)
    WndRect.left = mPixMaxWidth-Rect.right;

  MoveWindow (WndRect.left, WndRect.top, Rect.right+8, Rect.bottom+48);
}

//---------------------------------------------------------------------------------------

void CImageView::OnViewResizetopagesize() 
{
  RECT Rect;
  GetClientRect(&Rect);
  resize_window_to_page_size(Rect);

  //set_scrolls(Rect.right-16, Rect.bottom-36, SCROLL_CUR);
  InvalidateRect(NULL,false);
}

void CImageView::OnFileRecentfiles0() 
{
  open_recent_file(0);
}
void CImageView::OnFileRecentfiles1() 
{
  open_recent_file(1);
}
void CImageView::OnFileRecentfiles2() 
{
  open_recent_file(2);
}
void CImageView::OnFileRecentfiles3() 
{
  open_recent_file(3);
}
void CImageView::OnFileRecentfiles4()
{
  open_recent_file(4);
}
void CImageView::OnFileRecentfiles5()
{
  open_recent_file(5);
}
void CImageView::OnFileRecentfiles6()
{
  open_recent_file(6);
}
//---------------------------------------------------------------------------------------
void CImageView::open_recent_file(int index)
{
    const char *pName=gRecentFiles.MenuFileExist(index);
    if (pName) 
	{
		if (CMyWindows::IsDirectory(pName))
			AddFile(pName, true);
		else
			AddFile(pName, false);
	}
}
//---------------------------------------------------------------------------------------

void CImageView::OnClose() 
{
  if (mpImageViewerOn) {
    *mpImageViewerOn=false;
  }
	CDialog::OnClose();
}
//---------------------------------------------------------------------------------------

void CImageView::OnFileExit1() 
{
	exit (0);
}
/*
void CImageView::OnFileAdd() 
{
    CFileName fName;
    if (SelectDataFile (fName))
        AddFile (fName);
} */
/*
void CImageView::OnAddDiff() 
{
    OnAddDiffOrRatio("Diff");
} 
void CImageView::OnAddRatio()
{
    OnAddDiffOrRatio("Ratio");
} */
void CImageView::OnAddCombine()
{
    OnAddDiffOrRatio("Combine");
}
void CImageView::OnAddDiffOrRatio(const char *zName)
{
	if (gScreens.GetNDR() < 2)
	{
		CMetaData *pData = gScreens.GetCurrentData();
		if (pData && pData->IsHybrid())
			OnFileOpendirectory();
		else
			OnAddFile();;
	}

	int iDR = gScreens.GetIFreeOrNewDR();
	if (iDR < 2)
		return;

	bool bNewDisplay = (iDR == gScreens.GetNDR());

	CDisplayRectangle *pLast = gScreens.GetDisplayRectangle(iDR - 1);
    CDisplayRectangle *pPrev = gScreens.GetDisplayRectangle(iDR - 2);
    if (!pLast || !pPrev)
        return;

	if (bNewDisplay)
	    gScreens.AddData(new CDiffData (pPrev, pLast, zName));
	else
	{
		gScreens.SetCurrentDR(iDR);
	    gScreens.AddData(new CDiffData (pPrev, pLast, zName), false);
	}

    gScreens.Refresh();
}
void CImageView::OnFileAddcoronal()
{
    CFileFactory factory(mpfLog);
    factory.AddComputedData(CFileFactory::COMP_DATA_OBLIQUE);
}
void CImageView::OnAddMip()
{
    CFileFactory factory(mpfLog);
    factory.AddComputedData(CFileFactory::COMP_DATA_MIP);
}
void CImageView::OnAddSinogram()
{
    CFileFactory factory(mpfLog);
    factory.AddComputedData(CFileFactory::COMP_DATA_BEAM_HARDENING);
}
//----------------------------------------------------

void CImageView::OnSettingsSetfullroi() 
{
	//mExtraParams.bFullRoi = true;
	gScreens.Refresh();
}
//--------------------------------------------------------------------------------------

bool CImageView::SelectDataFile (CFileName &fName)
{
    bool bUseCurrentPath = false;
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
    {
        msLastOpenPath = pData->GetPath();
		bUseCurrentPath = true;
    }

    CMyFileDialog dlg(CMyFileDialog::FD_OPEN, "Open File to View", msLastOpenPath, bUseCurrentPath);

	if (dlg.DoModal())
    {
        fName.Set(dlg.m_ofn.lpstrFile);
        msLastOpenPath = fName.Path();
        return true;
    }
    return false;
}

//------------------------------------------------------------------

void CImageView::MyErrExit(const char *)
{
	exit (1);
}
void CImageView::ReadRegistry()
{
    CMyRegistry myReg("SOFTWARE\\ImageR", KEY_READ);
    if (!myReg)
        return;
    myReg.Get("bLongMenus", mbLongMenus);
    myReg.Get("bDicomMaster", mbDicomMaster);
    myReg.Get("bDicomDateMaster", mbDicomDateMaster);
	myReg.Get("bChangeDicomTables", mbChangeDicomTables);
	myReg.Get("bEnableAddSinogram", mbEnableAddSinogram);
	myReg.Get("bAutoFocus", CHybridFile::umbAutoFocusOnNewFile);
	myReg.Get("UpdateDebug", CNamedObject::umDebug);
	myReg.Get("MaxZoomFactor", CZoomPanParams::MAX_ZOOM_FACTOR);
	if (myReg.Get("bDebugTiming", mbDebugTiming))
	{
		if (mbDebugTiming)
			CEventTracer::umbEnable = true;
	}

	myReg.Get("DebugWindowDlg", CWindowDlg::umDebug);

	myReg.Get("bLoadVolume", CMetaData::umbEnableFastLoadFromVolume);

	int tmp = 0;
	if (myReg.Get("DebugScorers", tmp))
		CImageDiffScorer::SetDebug(tmp);

	bool bShowMeasurementsInMm = true;
	if (myReg.Get("bShowMeasurementsInMm", bShowMeasurementsInMm))
		CGraphicElement::SetMeasurementsDisplayMode(bShowMeasurementsInMm);

	int dicomDebug = 0;
	if (myReg.Get("DicomDebug", dicomDebug))
		CMyDicom::SetDebugLevel(dicomDebug);

	int nImageInMatrixLine = 0;
	if (myReg.Get("nImageInMatrixLine", nImageInMatrixLine))
		CFileNamePattern::SetNImagesInMatrixLine(nImageInMatrixLine);

	if (mbDicomMaster || mbChangeDicomTables)
		CDicomFieldDialog::AllowTablesChange();

	DWORD color;
	if (myReg.Get("DefaultGraphicColor", color))
	{
		if (color != 0)
			CGraphicElement::SetDefColor(color);
	}

	int debugCodeFile = 0;
	if (myReg.Get("DebugCodeFile", debugCodeFile))
		CCodeFile::SetDebug(debugCodeFile);

	int bDebugWindowAutoFocus = 0;
	if (myReg.Get("bDebugWindowAutoFocus", bDebugWindowAutoFocus) && bDebugWindowAutoFocus != 0)
		CWindowAutoFocus::SetDebug();

	int debugUncompress = 0;
	if (myReg.Get("DebugUncompress", debugUncompress) && debugUncompress != 0)
		CRle1Read::SetDebug(debugUncompress);

	myReg.Get("CommandLinePass", msCommandLinePassword);

	CXMLParseNode *pNodeSize = myReg.GetXmlNode("ring_id_missing_data_size_mm");
	if (pNodeSize)
		pNodeSize->GetValue(CRingID::umMissingDataStartSizeMm);

	CXMLParseNode *pNode = myReg.GetXmlNode("ring_id_missing_data_text");
	if (pNode)
		CRingID::SetMissingDataText(pNode);

	 CCtImages::ReadRegistry(myReg);
	CGraphicElement::ReadRegistry(myReg);
	CZoomer::ReadRegistry(myReg);
	CMetaData::ReadRegistry(myReg);
	gScreens.ReadRegistry(myReg);
	CDisplayRectangle::ReadRegistry(myReg);
	CWindowParams::ReadRegistry(myReg);
	CDiffData::ReadRegistry(myReg);
}
bool CImageView::AddFile (const char *zName, bool bNewDisplay)
{
    if (mpfLog)
    {
        fprintf (mpfLog, "<CImageView::AddFile> Create file <%s> bNewDisplay %s\n",
            zName, bNewDisplay ? "YES" : "NO");
        fflush (mpfLog);
    }
    CFileFactory factory(mpfLog);
    return factory.CreateAndDisplayFileObject (zName, bNewDisplay);
}

afx_msg void CImageView::OnGetMinMaxInfo(MINMAXINFO* )
{
    static int count = 0;
    count++;
}

//-----------------------------------------------------------------------
void CImageView::OnPrintExcel() 
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
	    return;

    fprintf (mpfLog, "\n<OnPrintExcel> %s\n", pData->z_name());

    CPrintExcel printExcel;
    printExcel.m_Till = mImViewParams.nRowsInPage - 1;

    if (printExcel.DoModal() == IDOK)
        printExcel.PrintFromFile (*pData, mCommonParams.nColsInFile, mCommonParams.eDataType, mpfLog);
}
void CImageView::OnExportScreentoexcel()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
	    return;

    pData->PrintScreenToExcel ();
}
void CImageView::OnPrintInfo()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
	    return;
    CGraphicElement *pGE = gScreens.mpGEWithContextMenu;
    if (!pGE)
	    return;

    pGE->PrintInfo (pData);
}
void CImageView::OnWindowAutoWide()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
    {
        pData->SetWideWindow();
        gScreens.Refresh();
    }
}

void CImageView::OnWindowAutofocus()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
    {
        CWindowParams *pWindowParams = pData->GetWindowParams();
        pWindowParams->SetAutoFocus(pData);
        gScreens.Refresh();
    }
}
void CImageView::OnWindowLocalfocus()
{
    CMetaData *pData = gScreens.GetCurrentData();
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();

    if (pData && pDR && pDR->mpSelectedPoint)
    {
        CWindowParams *pWindowParams = pData->GetWindowParams();
        pWindowParams->SetLocalFocus(pData->GetFloatZoomedRaster(), *pDR->mpSelectedPoint);
        gScreens.Refresh();
    }
}
void CImageView::OnWindowSpecialWindow() 
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
	{
        bool bSpecial = pData->ToggleSpecialWindow();
		CheckMenuItem("Window", ID_SETTINGS_SETSECONDWINDOW, bSpecial);
	}
}
void CImageView::OnViewSpecialZoom()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
        pData->ToggleSpecialZoom();
}

void CImageView::OnWindowSelectwindow()
{
}
void CImageView::OnFileOpenair()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
	    return;

    if (!pData->IsRaw())
        return;
    CString msName (pData->GetDicom()->GetName());
    AddFile(msName, false);
}
void CImageView::OnViewCardiacdata()
{
/*    if (mnFiles < 1)
        return;

    mbViewCardiac = !mbViewCardiac;
    if (mbViewCardiac)
    {
        if (!mpDv2EcgvPipe)
        {
            mpDv2EcgvPipe = new CPipeWriter("\\\\.\\pipe\\Dv2EcgvPipe");
            if (mpDv2EcgvPipe->VerifyConnection())
            {
                char zBuf[256] = "con ";
                strcat (zBuf, mapData[0]->GetPath());
                mpDv2EcgvPipe->WriteText (zBuf);
                sprintf (zBuf, "img %d", miDisplayPage-1);
                mpDv2EcgvPipe->WriteText (zBuf);
            }
        }
    }
    else
    {
        if (mpDv2EcgvPipe)
        {
            mpDv2EcgvPipe->WriteText ("dis"); // Disconnect
            delete mpDv2EcgvPipe;
            mpDv2EcgvPipe = NULL;
        }
    } */
}


void CImageView::OnSettingsSelectimagesbyparameters()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
	    return;

    CMyWindows::CreateThread (CMultiFile::StaticSelectByParameters, pData);
}
void CImageView::OnViewPrevfile()
{
    if (gScreens.OnViewPrevfile())
        gScreens.Refresh();
}
void CImageView::OnViewNextfile()
{
    if (gScreens.OnViewNextfile())
        gScreens.Refresh();
}
void CImageView::OnViewTogglecolors()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->ToggleColors();
}
void CImageView::OnViewToggletext()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->ToggleText();
}
void CImageView::OnViewTogglegraphics()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->ToggleGraphics();
}
void CImageView::OnInitMenuPopup(CMenu *pPopup)
{
    CDialog::OnInitMenuPopup( pPopup, 0, false );
}
void CImageView::OnAirPrintalltoexcel()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
        pData->OnPrint();
}
void CImageView::OnRawDefinedetectorasbad()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
        pData->OnRawDefinedetectorasbad();
}

void CImageView::OnRawDefinedetectorasgood()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
        pData->OnRawDefinedetectorasgood();
}
void CImageView::OnRawUpdatebaddetectorsinmdu()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
        pData->OnRawUpdatebaddetectorsinmdu();
}
void CImageView::OnRawPrintbaddetectorstable()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
        pData->OnRawPrintbaddetectorstable();
}
void CImageView::OnObliqueSetcoronal()
{
	COblique * pOblique = gScreens.GetCurrentObliqueData();
    if (pOblique)
        pOblique->SetCoronal();
}
void CImageView::OnObliqueSetsagittal()
{
	COblique * pOblique = gScreens.GetCurrentObliqueData();
    if (pOblique)
        pOblique->SetSagittal();
}
void CImageView::OnObliqueSetverticalzoom()
{
	OnSettingsVerticalzoomfactor();
}
void CImageView::OnObliqueInvertup()
{
	CVolumetric * pVolumetric = gScreens.GetCurrentVolumetricData();
    if (pVolumetric)
	{
        pVolumetric->InvertUpDown();
		gScreens.Refresh();
	}
}
void CImageView::OnDicomPrint()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
		return;
	if (!pData->PrintDicom())
	{
		if (pData->DesplayXmlHeader())
			;
		else
		{
			char zBuf[512];
			sprintf_s(zBuf,512,"No DICOM data found for <%s>", (const char*)pData->Name());
			CMyWindows::MessBox(zBuf,"Notice");
		}
	}
}
void CImageView::OnDicomPrintfromallslices()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
		return;
	if (pData->IsDicom())
        pData->PrintFromAllDicom();
}

void CImageView::OnFileSaveState()
{
	gScreens.SaveStateForUser();
}


void CImageView::OnGraphicDelete()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnGraphicDelete();
}
void CImageView::OnGraphicSetcolor()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnGraphicSetcolor();
}
void CImageView::OnGraphicSettext()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnGraphicSetText();
}
void CImageView::OnGraphicDisplaystatistics()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnGraphicDisplayStatistics();
}
void CImageView::OnGraphicRename()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnGraphicRename();
}
void CImageView::OnAddSmooth()
{
	float smoothFactor = 1;
	if (CNameGetDialog::GetFloatValue("Get Smooth Factor", smoothFactor))
		CProcImage::SetSmoothFactor(smoothFactor);

    CFileFactory factory(mpfLog);
    factory.AddComputedData(CFileFactory::COMP_DATA_SMOOTH);
}
void CImageView::OnSettingsInteractivetuning()
{
    if (gParametersControl.GetNParams() < 1)
    {
        CFileDialog dlg(TRUE);
		CString sInitDir(CMyWindows::GetLogDir("Params"));
        dlg.m_ofn.lpstrInitialDir = sInitDir;

	    if (dlg.DoModal() == IDOK)
        {
            gParametersControl.Init(dlg.m_ofn.lpstrFile);
        }
    }
    if (gParametersControl.GetNParams() < 1)
        return;

    CParamsDlg *pDlg = new CParamsDlg;
    pDlg->SetUpdateFunc(CScreens::StaticRefresh,&gScreens);
    pDlg->CreateThreadDoModal();
}
void CImageView::OnGraphicAddcursor()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
    {
        pDR->AddCursor("Cursor");
        gScreens.Refresh();
    }
}

void CImageView::OnGraphicAddline()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
    {
        pDR->AddLine("Line");
        gScreens.Refresh();
    }
}
void CImageView::OnGraphicAddrectangle()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
    {
        pDR->AddRoi("Rectangle");
        gScreens.Refresh();
    }
}
void CImageView::OnGraphicAddellipse()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
    {
        pDR->AddEllipse("Ellipse");
        gScreens.Refresh();
    }
}
void CImageView::OnGraphicAddcircle()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
    {
		pDR->AddEllipse("Circle", CGraphicElement::GetDefColor(), true);
        gScreens.Refresh();
    }
}
void CImageView::OnGraphicAddcontour()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
    {
        pDR->AddContour("Contour");
        gScreens.Refresh();
    }
}
void CImageView::OnGraphicAddtext()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->AddUserText();
}
void CImageView::OnHelpOpenmanual()
{
    CString sfName ("\"");
    sfName += CMyWindows::GetApplicationPath();
    sfName += "Docs\\ImageR Users Manual.htm\"";
	if (CFileName::Exist(sfName))
		CMyWindows::CreateProcess ("C:\\Program Files\\Internet Explorer\\Iexplore.exe", sfName);
}

void CImageView::OnGraphicDisplayonallimages()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnGraphicDisplayonallimages();
}
void CImageView::OnDicomAmendallbyxml()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData || !pData->IsDicom())
        return;
    if (!pData->IsMultiFile())
        return;
    CMultiFile *pMultiFile = (CMultiFile *)pData;
	pMultiFile->AmendAllByXml();
}
void CImageView::OnDicomCheckseries()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData || !pData->IsDicom())
        return;
    if (!pData->IsMultiFile())
        return;
    CMultiFile *pMultiFile = (CMultiFile *)pData;
	pMultiFile->CheckDicomSeries();
}
void CImageView::OnDicomShowscancenter()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
    {
        if (pDR->AddScanCenter())
	        gScreens.Refresh();
    }
}
void CImageView::CheckMenuItem(const char *zSection, int id, bool bCheck)
{
	CMenu *pSubMenu=MyGetSubMenu(zSection);
	if (pSubMenu)
	{
		if (bCheck)
			pSubMenu->CheckMenuItem(id, MF_BYCOMMAND | MF_CHECKED);
		else
			pSubMenu->CheckMenuItem(id, MF_BYCOMMAND | MF_UNCHECKED);
	}
}
void CImageView::OnHybridStopAuto()
{
	CHybridFile::umbAutoFocusOnNewFile = !CHybridFile::umbAutoFocusOnNewFile;
	CheckMenuItem("Window", ID_WINDOW_HYBRID_STOP_AUTO, !CHybridFile::umbAutoFocusOnNewFile);
}
void CImageView::OnDicomDate()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData || !pData->IsDicom())
        return;
	if (pData->IsHybrid())
	{
		CString sDate = pData->GetDicom()->EditDate();
		if (sDate.GetLength() != 8)
		{
			CMyWindows::MessBox("Date String Should be 8 characters","Error");
			return;
		}
		CHybridFile *pHybrid = (CHybridFile *)pData;
		pHybrid->SetNewDate(sDate);
	}
    if (!pData->IsMultiFile())
        return;
    CMultiFile *pMultiFile = (CMultiFile *)pData;

    CString sDate = pData->GetDicom()->EditDate();
    //CString sDate = pData->GetDicom()->GetTextField(STUDY_DATE_GROUP, STUDY_DATE_NUM);
    if (sDate.GetLength() == 8)
    {
        char zBuf[512];
        sprintf_s (zBuf, 512, "Image: %s\nNew Date %s\nAre you sure you want to changed data for %d images?",
            (const char*)pData->msName, (const char*)sDate, pData->GetNPages());
        int rc = MessageBox(zBuf, "Change Dicom Date Confirmation", MB_OKCANCEL);
        if (rc == IDOK)
        {
            pMultiFile->ChangeDicomDateToAllFiles(sDate);
        }
    }
}
void CImageView::OnSaveStateForClient(void)
{
    gScreens.SaveStateForClient();
	OnBroadPos();
}
void CImageView::OnBroadPos(void)
{
	CPosition::SetBroadcast();
}
void CImageView::OnLoadToCurrentDR(void)
{
    static int count = 0;
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (!pDR)
        return;

    count++;
    char zName[128];
	sprintf_s (zName, 128, "%s\\take.%d.Graphic.xml", 
		(const char*)CMyWindows::GetLogDir("ImageRInput"), count);
    AddFile(zName);
}

void CImageView::OnMipSelectslice()
{
    CMetaData *pData = gScreens.GetCurrentData();
	CMip *pMip = pData->PMip();
    if (pMip)
        pMip->SelectSlice();
}
void CImageView::OnMipClientmessage()
{
    CMetaData *pData = gScreens.GetCurrentData();
	CMip *pMip = pData->PMip();
    if (pMip)
	{
		gScreens.SaveStateForClient("MipFocus");
	}
}
void CImageView::OnMipSetvoi()
{
    CMetaData *pData = gScreens.GetCurrentData();
	CMip *pMip = pData->PMip();
    if (pMip)
	{
		pMip->SetVoi();
	}
}
void CImageView::OnMipTopovoi()
{
    CMetaData *pData = gScreens.GetCurrentData();
	CMip *pMip = pData->PMip();
    if (pMip)
	{
		pMip->SetTopoVoi();
	}
}

void CImageView::OnDicomEdittextfields()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
        return;

    CMyDicom *pDicom = pData->GetDicom();
    if (!pDicom)
        return;

    pDicom->EditTextFields();
}

void CImageView::OnMultidataSetsize()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData->IsMultiDataFile())
        return;

    CMyWindows::CreateThread(CMultiDataFile::StaticSetImageSize, pData);
}
void CImageView::OnMultidataShowchessheader()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData->IsMultiDataFile())
        return;

	CMultiDataFile *pMultiDataFile = (CMultiDataFile *)pData;
    pMultiDataFile->ShowHeader();
}
void CImageView::OnDiffRed()
{
    CDiffData *pDiff = gScreens.GetCurrentDiffData();
    if (!pDiff)
        return;
    pDiff->SetColor(DIFF_COL_RED);
}
void CImageView::OnDiffGreen()
{
    CDiffData *pDiff = gScreens.GetCurrentDiffData();
    if (!pDiff)
        return;
    pDiff->SetColor(DIFF_COL_GREEN);
}
void CImageView::OnDiffBlue()
{
    CDiffData *pDiff = gScreens.GetCurrentDiffData();
    if (!pDiff)
        return;
    pDiff->SetColor(DIFF_COL_BLUE);
}
void CImageView::OnDiffCompare()
{
    CDiffData *pDiff = gScreens.GetCurrentDiffData();
    if (!pDiff)
        return;
    pDiff->StartCompare();
}
void CImageView::OnDiffStatistics()
{
    CDiffData *pDiff = gScreens.GetCurrentDiffData();
    if (!pDiff)
        return;
    pDiff->PrintStatistics();
}
void CImageView::OnDiffFindbestfit()
{
    CDiffData *pDiff = gScreens.GetCurrentDiffData();
    if (!pDiff)
        return;
    if (pDiff->FindBestFit())
		gScreens.Refresh();
}
void CImageView::OnDiffFindbesttransformation()
{
	CDiffData* pDiff = gScreens.GetCurrentDiffData();
	if (!pDiff)
		return;
	if (pDiff->FindBestTransformation(true))
		gScreens.Refresh();
}
void CImageView::OnViewRefresh()
{
	ReadRegistry();
    gScreens.ForceDataUpdate();
}
void CImageView::OnDicomCompare()
{
	gScreens.OnDicomCompare();
}
void CImageView::OnDicomSaveuncomprerssed()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
		return;
	CMyDicom *pDicom = pData->GetDicom();
	if (!pDicom)
	{
		CMyWindows::MessBox("Dicom Data not found", "Warning");
		return;
	}
	if (!pDicom->IsCompressed())
	{
		CMyWindows::MessBox("Dicom Data is not compressed", "Warning");
		return;
	}
	pData->DumpSlice(true /*bAll*/, true /*bUncompressDicom*/);
}
void CImageView::OnExportDumpslice()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
		pData->DumpSlice();
}
void CImageView::OnExportDumpallslices()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData) 
		pData->DumpSlice(true /*bAll*/);
}
void CImageView::OnExportDumpsub()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData) 
		pData->DumpSlicesSubSet();
}
void CImageView::OnExportReorderfiles()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
	{
		if (pData->IsMultiFile())
		{
			CMultiFile *pMultiFile = (CMultiFile *)pData;
			pMultiFile->ReorderFiles();
		}
	}
}
void CImageView::OnExportReversefilesorder()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData)
	{
		if (pData->IsMultiFile())
		{
			CMultiFile *pMultiFile = (CMultiFile *)pData;
			pMultiFile->ReverseFilesOrder();
		}
	}
}
void CImageView::OnExportSavewindow()
{
	Sleep(300);
	CScreenImageIF screenImageIF;
	if (screenImageIF.CaptureWindow(m_hWnd))
		screenImageIF.Save();
}
void CImageView::OnExportDumpcoronals()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData) 
		pData->DumpCoronals();
}

void CImageView::OnExportDumpsagittals()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData) 
		pData->DumpSagittals();
}
void CImageView::OnPrintStatistics()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (pData) 
		pData->PrintStatistics();
}
void CImageView::OnFileDelete()
{
	gScreens.OnFileDelete();
}
void CImageView::SetTopMost(void)
{
	int rc = SetWindowPos(&CWnd::wndTopMost, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	if (rc)
		MessageBox("Set Window Pos failed", "SW Error", MB_OK);
}
void CImageView::OnViewDatacoo()
{
	CDisplayRectangle::umbPrintDataCoo = !CDisplayRectangle::umbPrintDataCoo;
	CheckMenuItem("&View", ID_VIEW_DATACOO, CDisplayRectangle::umbPrintDataCoo);
}
void CImageView::OnViewTitles()
{
	CDisplayRectangle::umbDisplayTitles = !CDisplayRectangle::umbDisplayTitles;
	CheckMenuItem("&View", ID_VIEW_TITLES, CDisplayRectangle::umbDisplayTitles);
	gScreens.Refresh();
}
void CImageView::OnViewDatarange()
{
	CDisplayRectangle::umbDisplayDataRange = !CDisplayRectangle::umbDisplayDataRange;
	CheckMenuItem("&View", ID_VIEW_DATARANGE, CDisplayRectangle::umbDisplayDataRange);
	gScreens.Refresh();
}
void CImageView::OnExportSavezoomed()
{
	CDisplayRectangle::umbSaveZoomedRaster = !CDisplayRectangle::umbSaveZoomedRaster;
	CheckMenuItem("&Print", ID_PRINT_SAVEZOOMED, CDisplayRectangle::umbSaveZoomedRaster);
	if (CDisplayRectangle::umbSaveZoomedRaster)
		gScreens.Refresh();
}
void CImageView::OnSettingsNewposition()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
	    return;

	pData->SetNewPosition();
}
void CImageView::OnSettingsSecondleafindex()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
	    return;

	pData->SetSecondLeafingIndex();
}
void CImageView::OnSettingsVertical()
{
	gScreens.SetMaxRectanglesInRowFlipVertical();
}
void CImageView::OnSettingsNcolumns()
{
	CNameGetDialog dlg;
	int n = 1;
	if (dlg.GetIntValue("Select number of image columns", n))
		gScreens.SetMaxRectanglesInRow(n);
}
void CImageView::OnSettingsDisplaywidth()
{
    CMetaData *pData = gScreens.GetCurrentData();
    if (!pData)
	    return;

	CNameGetDialog dlg;
	int n = pData->GetDisplayWidth();
	if (dlg.GetIntValue("Select number of images to average", n))
	{
		if (gScreens.SetDisplayWidth(n))
			gScreens.Refresh();
	}
}
void CImageView::OnGraphicDeleteallgraphics()
{
	gScreens.DeleteAllGraphics();
}
void CImageView::OnGraphicDeletealltexts()
{
	gScreens.DeleteAllTexts();
}
void CImageView::OnGraphicSavetofile()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnGraphicSavetofile();
}
void CImageView::OnGraphicSavedatatoexcel()
{
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (pDR)
        pDR->OnGraphicSaveData2Excel();
}
void CImageView::OnGraphicLoadfromxml()
{
	CString sDefPath(CMyWindows::GetLogDir("Graphic"));
	CMyFileDialog dlg(CMyFileDialog::FD_OPEN, "Select Graph Definition XML File", sDefPath);
	if (dlg.DoModal())
	{
		CString sPath(dlg.GetPathName());
		if (CFileName::Exist(sPath))
		{
			AddFile(sPath,false);
			gScreens.Refresh();
		}
	}

}
void CImageView::SetParentWindowHandle(HWND parentHandle)
{
	this->parentWindowHandle = parentHandle;
}
HWND CImageView::GetParentWindowHandle()
{
	return parentWindowHandle;
}
void CImageView::OnNewimageSaveseries()
{
    static UINT umSaveSeries = 0;
	if (!umSaveSeries)
		umSaveSeries = RegisterWindowMessage("SaveImagesSeries");

	WPARAM wParam = 0;
	CMetaData *pData = gScreens.GetCurrentData();
	if (pData)
	{
		CMyImage::SSharedMemHeader *pHeader = pData->GetSharedHeader();
		if (pHeader)
			wParam = pHeader->mId;
	}
	CMyWindows::PostMessageToClient(umSaveSeries, wParam, NULL);
}
void CImageView::OnDiffSelectplus()
{
    CDiffData *pDiff = gScreens.GetCurrentDiffData();
    if (!pDiff)
        return;
	pDiff->ChangePlus();
}
void CImageView::OnDiffSelectminus()
{
    CDiffData *pDiff = gScreens.GetCurrentDiffData();
    if (!pDiff)
        return;
	pDiff->ChangeMinus();
}
void CImageView::OnRingidShow()
{
	OnDicomShowscancenter();
}
void CImageView::OnRingidDelete()
{
        gScreens.DoGraphicCommand(GCOM_REMOVE, "ScanCenter");
}
void CImageView::OnRingidFixcenter()
{
	gScreens.DoGraphicCommand(GCOM_FIX_CENTER, "ScanCenter");
}

void CImageView::OnGenericSpecialwindow()
{
	OnWindowSpecialWindow();
}

void CImageView::OnGenericSpecialzoom()
{
	OnViewSpecialZoom();
}
void CImageView::OnPresetSelected0()
{
	gContextMenu.OnPresetSelected(0);
}
void CImageView::OnPresetSelected1()
{
	gContextMenu.OnPresetSelected(1);
}
void CImageView::OnPresetSelected2()
{
	gContextMenu.OnPresetSelected(2);
}
void CImageView::OnPresetSelected3()
{
	gContextMenu.OnPresetSelected(3);
}
void CImageView::OnPresetSelected4()
{
	gContextMenu.OnPresetSelected(4);
}
void CImageView::OnPresetSelected5()
{
	gContextMenu.OnPresetSelected(5);
}
void CImageView::OnPresetSelected6()
{
	gContextMenu.OnPresetSelected(6);
}
void CImageView::OnPresetSelected7()
{
	gContextMenu.OnPresetSelected(7);
}
void CImageView::OnPresetSelected8()
{
	gContextMenu.OnPresetSelected(8);
}
void CImageView::OnPresetSelected9()
{
	gContextMenu.OnPresetSelected(9);
}
void CImageView::OnMenuColorsBaseOnZero()
{
	CMetaData::ToggleColorsBaseOnZero();
	gScreens.Refresh();
}
void CImageView::OnColoringToggletransparency()
{
	CMetaData::ToggleTransparency();
	gScreens.Refresh();
}
void CImageView::OnColoringTransparencyfraction()
{
	CMetaData::GuiTransparencyFraction();
	gScreens.Refresh();
}
void CImageView::OnSettingsDatatype()
{
	CMetaData *pData = gScreens.GetCurrentData();
	if (pData->UserSelectDataType())
		gScreens.Refresh();
}
void CImageView::OnWindowShowminmax()
{
	gScreens.mbDisplayMinMaxAtWindowStatus = !gScreens.mbDisplayMinMaxAtWindowStatus;
	gScreens.DisplayWindowStatus();
}
void CImageView::OnSettingsTitle()
{
	CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
	if (pDR)
	{
		pDR->SetTitleByUser(false /*!bToData*/);
	}
}
void CImageView::OnSettingsTitleforimageset()
{
	CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
	if (pDR)
	{
		pDR->SetTitleByUser(true /*bToData*/);
	}
}
void CImageView::OnSettingsVerticalzoomfactor()
{
	CMetaData * pData = gScreens.GetCurrentData();
    if (pData)
        pData->SetVerticalZoom();
}
void CImageView::OnExportGraphicdatatoexcel()
{
	gScreens.ExportGraphicDataToExcel();
}
void CImageView::OnAddFile()
{
	CFileName fName;
	if (SelectDataFile(fName))
		AddFile(fName);
}
void CImageView::OnAddDiff()
{
	OnAddDiffOrRatio("Diff");
}
void CImageView::OnAddCoronal33994()
{
	CFileFactory factory(mpfLog);
	factory.AddComputedData(CFileFactory::COMP_DATA_OBLIQUE);
}
void CImageView::OnAddCombine33997()
{
	OnAddDiffOrRatio("Combine");
}
void CImageView::OnAddRatio33998()
{
	OnAddDiffOrRatio("Ratio");
}
void CImageView::OnDicomExportrastertodicom()
{
	CMyFileDialog dlg(CMyFileDialog::FD_OPEN, "Select Template Dicom File");
	if (dlg.DoModal())
	{
		CString sPath(dlg.GetPathName());
		if (CFileName::Exist(sPath))
		{
			CMetaData* pData = gScreens.GetCurrentData();
			if (pData)
				pData->SaveToDicom(sPath);

		}
	}
}
void CImageView::OnDicomExporttominimaldicom()
{
	CMetaData* pData = gScreens.GetCurrentData();
	if (pData)
		pData->SaveToMinimalDicom();
}
void CImageView::OnAddChangeendian()
{
	CMetaData* pData = gScreens.GetCurrentData();
	if (pData)
	{
		pData->ChangeEndian();
		gScreens.Refresh();
	}
}

void CImageView::OnSettingsTogglefastloading()
{
	CMetaData::ToggleEnableFastLoadFromVolume();
}

void CImageView::OnExportHistogram()
{
	CMetaData* pData = gScreens.GetCurrentData();
	if (pData)
	{
		pData->ExportHistogram();
	}
}

void CImageView::OnTogglectmodeOncurrentimage()
{
	CMetaData* pData = gScreens.GetCurrentData();
	if (pData)
	{
		pData->ToggleCTMode();
		gScreens.Refresh();
	}
}

void CImageView::OnTogglectmodeOnall()
{
	CMetaData* pData = gScreens.GetCurrentData();
	if (pData)
	{
		CMetaDataBase::ToggleCTGlobal();
		gScreens.ForceDataUpdate();
	}
}

