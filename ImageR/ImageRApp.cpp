// ImageRApp.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ImageRApp.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyRegistry.h"
#include "ImageView.h"
#include "PipeIf.h"
#include "..\..\ImageRLib\FileNamePattern.h"
#include "Screens.h"
#include "ArchivesFile.h"
#include "FileTypes.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CImageRApp

BEGIN_MESSAGE_MAP(CImageRApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CImageRApp construction

CImageRApp::CImageRApp()
	: msPipeName(_T(""))
	, msCommandLine(_T(""))
	, miStart(0)
	, msSeparators(_T(" \t\n"))
	, mbCommandLineFinished(false)
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}
CImageRApp::~CImageRApp()
{
}

// The one and only CImageRApp object

CImageRApp theApp;


// CImageRApp initialization

BOOL CImageRApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	msCommandLine = GetCommandLine();

	int dialogResource = CImageView::IDD;
	if ((msCommandLine.Find("-parent_handle") >= 0)
		|| (msCommandLine.Find("-ebw") >= 0))
	{
		dialogResource = CImageView::IDD1; // Use dialog resource template without caption and borders.
		gScreens.SetEbwMode();
	}
	CImageView dlg(dialogResource); 
	ParseCommandLine(dlg);
	
	m_pMainWnd = &dlg;
	CMyWindows::SetMainWindow(m_pMainWnd);

	gFileTypes.Init();

	FILE *pfLog = dlg.GetPFLog();
	if (pfLog)
		fprintf (pfLog, "<CDataViewApp::InitInstance> cmd line %s (len %d)\n", 
		theApp.m_lpCmdLine, (int)strlen(theApp.m_lpCmdLine));

    // Look for pipe name in command line


	/*
    size_t len = strlen (theApp.m_lpCmdLine);
    if (len > 5)
        for (size_t i = 0; i < len - 1; i++)
        {
            if (theApp.m_lpCmdLine[i] == '-')
            {
                const char *pRest = &theApp.m_lpCmdLine[i+1];
                if ((_strnicmp (pRest, "p ", 2) == 0) || (_strnicmp (pRest, "pipe ", 5) == 0))
                {
                    msPipeName = GetParamsValue(pRest);
                }
                else if ((_strnicmp (pRest, "m ", 2) == 0) || (_strnicmp (pRest, "matrix ", 7) == 0))
                {
                    int matrix = GetIntParamsValue(pRest);
                    if (matrix)
						gScreens.SetWantedSize(matrix);
                }
                else if (_strnicmp (pRest, "screen_pos ", 11) == 0)
                {
                    int x = GetIntParamsValue(pRest);
                    int y = GetIntParamsValue(pRest);
					if (x || y)
						dlg.SetUpperLeft(x,y);
                }
                else if (_strnicmp (pRest, "n_images_in_matrix_line ", 19) == 0)
                {
                    int nImagesInLine = GetIntParamsValue(pRest);
                    if (nImagesInLine)
						CFileNamePattern::SetNImagesInMatrixLine(nImagesInLine);
                }
                else if (_strnicmp (pRest, "files ", 6) == 0)
                {
                    dlg.msFilesFile = GetParamsValue(pRest);
                }
                else if (_strnicmp (pRest, "diff ", 4) == 0)
                {
                    dlg.mbStartDiff = true;
                }
                else if (_strnicmp (pRest, "scan_center", 11) == 0)
                {
					gScreens.mbShowScanCenter = true;
                }
				else if (_strnicmp (pRest, "vertical", 8) == 0)
				{
					gScreens.SetMaxRectanglesInRow(1);
				}
				else if (_strnicmp (pRest, "n_in_row", 8) == 0)
				{
					int nInRow = GetIntParamsValue(pRest);
					if (nInRow)
						gScreens.SetMaxRectanglesInRow(nInRow);
				}
				else if (_strnicmp (pRest, "n_columns", 9) == 0)
				{
					int nInRow = GetIntParamsValue(pRest);
					if (nInRow)
						gScreens.SetMaxRectanglesInRow(nInRow);
				}
				else if (_strnicmp (pRest, "parent_handle ", 11) == 0)
				{
					INT64 parentHandle = GetIntParamsValue(pRest);
					dlg.SetParentWindowHandle((HWND) parentHandle);
					//CWnd* parentWindow = CWnd::FromHandle((HWND) parentHandle);
					//dlg.SetParent(parentWindow);

					//::SetParent(dlg.GetSafeHwnd(), (HWND) parentHandle); // avoid MFC logic
					//RECT parentWindowRectangle;
					//BOOL status = ::GetWindowRect(dlg.GetSafeHwnd(), &parentWindowRectangle);
					//if (status)
					//{
					//	//RECT mySize(parentWindowRectangle);
					//	CRect myRectangle(parentWindowRectangle);
					//	myRectangle.DeflateRect(2, 2);
					//	
					//	::SetWindowPos(dlg.GetSafeHwnd(), NULL, 
					//		myRectangle.left, myRectangle.top, //1, 1,
					//		myRectangle.Width(), myRectangle.Height(),//viewerSize.Width-2, ViewerSize.Height-2, );
					//		0); // flags
					//}
				}
				else if (_strnicmp (pRest, "client_handle ", 11) == 0)
				{
					INT64 clientWindowHandle = GetIntParamsValue(pRest);
					CMyWindows::SetClientWindowHandle((HWND) clientWindowHandle);
				}

            }
        }
		*/

	dlg.InitAfterCommandLineIsProcessed(msCommandLine);

	// May run ADPlus here to catch crashes
	bool bRunADPlus = false;
	if (CMyRegistry::GetAppReg("bRunADPlus",bRunADPlus) && bRunADPlus)
	{
		DWORD pid = GetCurrentProcessId();
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf)," -p %d", pid);
		CString sCmd0("CScript.exe");
		CString sCmd ("\"C:\\Program Files\\Debugging Tools for Windows (x86)\\ADPlus.vbs\" ");
		//CString sPar (" -quiet -crash -pn ImageR.exe -o ");
		//CString sPar (" -crash -NoDumpOnFirst -MiniOnSecond -o ");
		CString sPar (" -crash -o ");
		sPar += CMyWindows::GetLogDir();
		sPar += zBuf;
		CMyWindows::CreateProcess(sCmd0,sCmd + sPar);
	}

	if (!msPipeName.IsEmpty())
		dlg.StartPipeIF(msPipeName);

	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CImageRApp::ParseCommandLine(CImageView &dlg)
{
	CMyWindows::ReadFilesNamesFromCommandLine(msCommandLine, dlg.msStartFiles);

    CMyWindows::SetApplicationPath (msCommandLine);

	if (msCommandLine.Find("-shallow") >= 0)
		CFileNamePattern::DisableMultiLevelFilesArray();

	if (msCommandLine.Find("-dup_zoom") >= 0)
		CZoomPanParams::SetDuplicateAsDefault();

	float zoomFactor = 0;
	if (CMyWindows::ReadParameterFromCommandLine(msCommandLine, "-zoom", zoomFactor))
		CZoomPanParams::SetInitialZoom(zoomFactor);

	if (msCommandLine.Find("-disable_2d_leafing") >= 0)
		CCtImages::umbAllow2DLeafing = false;

	if (msCommandLine.Find("-2d_leafing") >= 0)
		CArchivesFile::umbAskFor2DLeafing = false;

	if (msCommandLine.Find("-3d_leafing") >= 0)
		CFileNamePattern::EnableMultiLevelFilesArray(3);

	miStart = 0;
	CString sToken = msCommandLine.Tokenize(msSeparators,miStart);
	while (!sToken.IsEmpty())
	{
		if (sToken == "-pipe" || sToken == "-p")
        {
			msPipeName = msCommandLine.Tokenize(msSeparators,miStart);
        }
		else if (sToken == "-matrix")
        {
            int matrix = GetNextIntFromCommandLine();
            if (matrix > 10)
				gScreens.SetWantedSize(matrix);
        }
		else if (sToken == "-screen_pos")
        {
            int x = GetNextIntFromCommandLine();
            int y = GetNextIntFromCommandLine();
			if (x || y)
				dlg.SetUpperLeft(x,y);
        }
		else if (sToken == "-n_images_in_matrix_line")
        {
            int nImagesInLine = GetNextIntFromCommandLine();
            if (nImagesInLine > 0)
				CFileNamePattern::SetNImagesInMatrixLine(nImagesInLine);
        }
		else if (sToken == "-files")
        {
            dlg.msFilesFile = msCommandLine.Tokenize(msSeparators,miStart);
        }
		else if (sToken == "-diff")
        {
            dlg.mbStartDiff = true;
        }
		else if (sToken == "-scan_center")
        {
			gScreens.mbShowScanCenter = true;
        }
		else if (sToken == "-vertical")
		{
			gScreens.SetMaxRectanglesInRow(1);
		}
		else if (sToken == "-n_in_row" || sToken == "-n_columns")
		{
			int nInRow = GetNextIntFromCommandLine();
			if (nInRow > 0)
				gScreens.SetMaxRectanglesInRow(nInRow);
		}
		else if (sToken == "-parent_handle")
		{
			INT64 parentHandle = GetNextIntFromCommandLine();
			dlg.SetParentWindowHandle((HWND) parentHandle);
		}
		else if (sToken == "-client_handle")
		{
			INT64 clientWindowHandle = GetNextIntFromCommandLine();
			CMyWindows::SetClientWindowHandle((HWND) clientWindowHandle);
		}
		if (mbCommandLineFinished)
			break;
		sToken = msCommandLine.Tokenize(msSeparators,miStart);
	}
}
int CImageRApp::GetNextIntFromCommandLine(void)
{
	if (mbCommandLineFinished)
		return 0;

	CString sInt = msCommandLine.Tokenize(msSeparators,miStart);
	if (sInt.IsEmpty())
	{
		mbCommandLineFinished = true;
		return 0;
	}
	return atoi(sInt);
}

// Finds resource DLL for language.
// Language is found according to thread locale (GetThreadLocale())
// (base works according to GetUser[System]DefaultUILanguage())
HINSTANCE CImageRApp::LoadAppLangResourceDLL()
{
	HINSTANCE hInstance = CMyWindows::LoadAppLangResourceDLL(m_hInstance);
	return hInstance;
}
