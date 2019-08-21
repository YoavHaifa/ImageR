// ImageRApp.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CImageRApp:
// See ImageRApp.cpp for the implementation of this class
//

class CImageRApp : public CWinApp
{
public:
	CImageRApp();
	~CImageRApp();

// Overrides
	public:
	virtual BOOL InitInstance();
	virtual HINSTANCE LoadAppLangResourceDLL();
	void ParseCommandLine(class CImageView &dlg);
	int GetNextIntFromCommandLine(void);

	CString msCommandLine;
	int miStart;
	CString msSeparators;
	bool mbCommandLineFinished;

	CString msPipeName;


	//CString GetParamsValue(const char * &zBuffer);
	//int GetIntParamsValue(const char * &zBuffer);
// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CImageRApp theApp;