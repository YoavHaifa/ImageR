#include "StdAfx.h"
#include "PipeIf.h"
#include "ImageView.h"
#include "Screens.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyTracer.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\FileName.h"
#include "MetaData.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\FileNamePattern.h"
#include "FileFactory.h"

CPipeIF::CPipeIF(const char *zName, CImageView *pViewer)
: CPipeReader (zName)
, mpViewer (pViewer)
, msNextImageName(_T(""))
, msImagePattern(_T(""))
, miNextImage(0)
, mbWaitInThread(false)
, mnMaxToDisplay(10)
, mbWaitPrep(false)
, mNextFileUseSpecialPos(0)
{
    if (mhPipe == INVALID_HANDLE_VALUE)
        mpViewer->MyErrExit("CreatePipe");
	SetExitOnError();
}

CPipeIF::~CPipeIF(void)
{
}
bool CPipeIF::GetFirstFileName (CString &sName)
{
    if (mFilesList.IsEmpty())
        return false;

    sName = *(mFilesList.GetHead());
    mFilesList.RemoveHead();
    if (mpfLog)
    {
        fprintf (mpfLog, "<GetFirstFileName> %s\n", (const char*)sName);
        fflush (mpfLog);
    }
    return true;
}

DWORD __stdcall CPipeIF::StaticWaitImageThread (void *p)
{
    CPipeIF *pIF = (CPipeIF *)p;
    return pIF->WaitImageThread();
}

DWORD __stdcall CPipeIF::WaitImageThread (void)
{
    CMetaData *pData = NULL;
    CPosition *pPosition = NULL;
    while (1)
    {
        if (msNextImageName != "")
        {
            if (CFileName::Exist(msNextImageName))
            {
                if (mpfLog)
                {
                    fprintf (mpfLog, "Found: %d %s\n", miNextImage, (const char*)msNextImageName);
                    fflush (mpfLog);
                }
                if (mbWaitPrep)
                    WaitPrep();
 
                while (miNextImage > 1 && !pPosition)
                {
                    Sleep (20);
                    if (!pData)
                        pData = gScreens.GetCurrentData();
                    if (pData && !pPosition)
                        pPosition = pData->GetPosition();
                    if (mpfLog && pData)
                        fprintf (mpfLog, "pData: %s\n\n", (const char*)pData->msName);
                }

                if (pPosition)
				{
					pPosition->SetNotLast();
                    pPosition->SetCurrent(miNextImage);
				}
                else
                {
                    mFilesList.AddTail(new CString(msNextImageName));
                    if (mFilesList.GetSize() >= mnMaxToDisplay)
                    {
                        //Sleep(100);
                        if (mpfLog)
                            fprintf (mpfLog, "Set Display\n");
                        mpViewer->InitiateSetDisplayFiles();
                    }
                }                 
                msNextImageName = "";
                if (mpfLog)
                    fflush (mpfLog);
				gScreens.Refresh();
            }
            else
            {
                Sleep (20);
            }
        }
        else if (msImagePattern != "")
        {
            // Start looking for next image
            miNextImage++;
            if (mbWaitPrep)
            {
                if (miNextImage == 1)
                    msNextImageName = msImagePattern;
                else
                {
                    msNextImageName = "";
                    msImagePattern = "";
                }
            }
            else
            {
                char zBuf[256];
                sprintf_s (zBuf, 256, msImagePattern, miNextImage);
                msNextImageName = zBuf;
            }
            if (mpfLog)
            {
                if (mbWaitPrep)
                    fprintf (mpfLog, "%3d: Waiting Prep %s\n", miNextImage, (const char *)msNextImageName);
                else
                    fprintf (mpfLog, "%3d: Waiting next image %s\n", miNextImage, (const char *)msNextImageName);
                fflush (mpfLog);
            }
        }
        else
            Sleep (20);
    }
}

//------------------------------------------------------------------
bool CPipeIF::ShouldWait(void)
{
	return gScreens.miPipeMessageToHandle > gScreens.miPipeMessageHandled;
}

//------------------------------------------------------------------
void CPipeIF::ProcessMessage (void)
{
	if (mpfLog)
		fprintf(mpfLog, "<ProcessMessage> %s\n", mzRequest);

    if (_strnicmp(mzRequest, "-cirs ", 6) == 0)
    {
        mnMaxToDisplay = 1;

        msImagePattern = &mzRequest[6];
        msImagePattern += "_rcn01";

        CFileName fOutName (&mzRequest[6]);
        CString sPrivate (fOutName.Private());
        mbWaitPrep = (sPrivate.Left(4) == "prep");
        if (!mbWaitPrep)
            msImagePattern += "-img%03d";

        msImagePattern += ".dat";
        if (mpfLog)
            fprintf (mpfLog, "msImagePattern %s\n\n", (const char *)msImagePattern);

        msNextImageName = "";
        miNextImage = 0;

        if (!mbWaitInThread)
        {
            mbWaitInThread = true;
            CMyWindows::CreateThread (StaticWaitImageThread, this);
        }
    }
    else if (_stricmp(mzRequest, "exit") == 0)
    {
        mpViewer->OnFileExit1();
    }
    else if (_stricmp(mzRequest, "topmost") == 0)
    {
		mpViewer->SetTopMost();
    }
    else if (_stricmp(mzRequest, "update") == 0)
    {
        UpdateObjects();
        gScreens.Refresh();
    }
    else if (_stricmp(mzRequest, "update_all") == 0)
    {
		gScreens.ForceDataUpdate();
    }
    else if (_stricmp(mzRequest, "update_shared") == 0)
    {
		gScreens.ForceDataUpdate(true /*bOnlyShared*/);
    }
    else if (_stricmp(mzRequest, "disable_paging") == 0)
    {
		gScreens.AllowPaging(false);
    }
    else if (_stricmp(mzRequest, "enable_paging") == 0)
    {
		gScreens.AllowPaging(true);
    }
    else if (_stricmp(mzRequest, "broadcast_position") == 0)
    {
		CPosition::SetBroadcats();
    }
    else if (_stricmp(mzRequest, "broadcast_left_mouse_up") == 0)
    {
		gScreens.InitLeftMouseUpMsg();
	}
    else if (_stricmp(mzRequest, "display") == 0)
    {
		OnDisplayImage();
    }
	else if (_stricmp(mzRequest, "replace") == 0)
	{
		OnDisplayImage(true);
	}
	else if (_stricmp(mzRequest, "load_state_and_replace") == 0)
	{
		CString sfName;
		bool bOk = GetFirstFileName(sfName);
		if (bOk)
		{
			CFileFactory::SetReplaceFileName(sfName);
			OnDisplayImage(true);
		}
	}
	else if (_stricmp(mzRequest, "display_colorer") == 0)
    {
		CString sfName;
		bool bOk = GetFirstFileName(sfName);
		if (bOk)
			mpViewer->OnNewColorer(sfName);
    }
    else if (_stricmp(mzRequest, "remove_graphics") == 0)
		SendGraphicCommand(GCOM_REMOVE);
    else if (_stricmp(mzRequest, "disable_graphics") == 0)
		SendGraphicCommand(GCOM_DISABLE);
    else if (_stricmp(mzRequest, "enable_graphics") == 0)
		SendGraphicCommand(GCOM_ENABLE);
    else if (_stricmp(mzRequest, "set_title") == 0)
		OnSetTitle();
    else if (_stricmp(mzRequest, "set_zoom") == 0)
		OnSetZoom();
    else if (_stricmp(mzRequest, "set_status") == 0)
		OnSetStatus();
    else if (_stricmp(mzRequest, "set_window") == 0)
		OnSetWindow();
    else if (_stricmp(mzRequest, "set_window_range") == 0)
		OnSetWindowRange();
    else if (_stricmp(mzRequest, "get_window") == 0)
		OnGetWindow();
    else if (_stricmp(mzRequest, "set_cursor_broadcast") == 0)
		OnSetCursorBroadcast();
    else if (_stricmp(mzRequest, "set_position") == 0)
		OnSetPosition();
    else if (_stricmp(mzRequest, "set_relative_position") == 0)
		OnSetRelativePosition();
    else if (_stricmp(mzRequest, "set_position_2d") == 0)
		OnSetPosition2d();
    else if (_stricmp(mzRequest, "vertical") == 0)
		gScreens.SetMaxRectanglesInRow(1);
	else if (_stricmp(mzRequest, "set_same_pos_on_same_series") == 0)
		gScreens.SetNewPositionOnSameSeries(false);
    else if (_stricmp(mzRequest, "colormap") == 0)
		OnDisplayColorMap();
    else if (_stricmp(mzRequest, "delete") == 0)
	{
        CString sName;
        bool bOk = GetFirstSpecialName(sName);
        if (bOk)
            gScreens.DeleteData(sName);
	}
    else if (_stricmp(mzRequest, "n_columns") == 0)
    {
		int nCols;
		bool bOk = GetFirstInt(nCols);
		if (bOk)
			gScreens.SetMaxRectanglesInRow(nCols);
	}
    else if (_stricmp(mzRequest, "next_file_use_special_pos") == 0)
    {
		int iPos;
		bool bOk = GetFirstInt(iPos);
		if (bOk)
			mNextFileUseSpecialPos = iPos;
	}
    else if (_stricmp(mzRequest, "set_display_center") == 0)
    {
        CString sName;
        int x, y;
        bool bOk = GetFirstSpecialName(sName);
        bOk |= GetFirstInt(x);
        bOk |= GetFirstInt(y);
        CFloatCoordinates coo((float)x, (float)y);
        if (bOk)
        {
            gScreens.SetDisplayCenter(sName, coo);
        }
    }
    else if (_stricmp(mzRequest, "set_auto_window") == 0)
    {
        CString sName;
        int x, y;
        bool bOk = GetFirstSpecialName(sName);
        bOk |= GetFirstInt(x);
        bOk |= GetFirstInt(y);
        CFloatCoordinates coo((float)x, (float)y);
        if (bOk)
        {
            gScreens.SetAutoWindow(sName, coo);
        }
    }
    else if (_stricmp(mzRequest, "set_new_window") == 0)
    {
        CString sName;
        bool bOk = GetFirstSpecialName(sName);
        if (bOk)
        {
            gScreens.ToggleSpecialWindow(sName);
        }
    }
    else if (_stricmp(mzRequest, "disable_paint") == 0)
    {
		gScreens.DisablePaint();
    }
    else if (_stricmp(mzRequest, "enable_paint") == 0)
    {
		gScreens.EnablePaint();
    }
    else if (_stricmp(mzRequest, "delete_all") == 0)
    {
		gScreens.DeleteAllData();
    }
    else if (_stricmp(mzRequest, "set_frame") == 0)
    {
        CString sName;
        bool bOk = GetFirstSpecialName(sName);
        if (bOk)
            gScreens.SetExternalFrame(sName);
    }
	else if (_stricmp(mzRequest, "save_state") == 0)
	{
		if (mpfLog)
			fprintf(mpfLog, "Handling <save_state>\n");
        CString sName;
        bool bOk = GetFirstSpecialName(sName);
        if (bOk)
            gScreens.SaveStateToClientFile(sName);
		else
		{
			if (mpfLog)
				fprintf(mpfLog, "Handling <save_state> GetFirstFileName failed\n");
		}
	}
    else
    {
        if (CFileName::Exist(mzRequest))
            mFilesList.AddTail(new CString (mzRequest));
        else if (_strnicmp ("Mem:", mzRequest, 4) == 0)
		{
            mFilesList.AddTail(new CString(mzRequest));
		}
        else if (_strnicmp ("::", mzRequest, 2) == 0)
            mFilesList.AddTail(new CString(mzRequest));
    }
}


bool CPipeIF::WaitPrep(void)
{
    int nSleep = 0;
    ULONG64 len = CMyWindows::GetFileLength(msNextImageName);
    while ((len < 10000000) && (nSleep < 90000))
    {
        Sleep(100);
        nSleep += 100;
        len = CMyWindows::GetFileLength(msNextImageName);
    }
    if (mpfLog)
    {
        fprintf (mpfLog, "10M arrived? %I64d\n", len);
        fflush (mpfLog);
    }
    FILE *pf = MyFOpen (msNextImageName, "rb");
    while (!pf)
    {
        static int count = 0;
        if (!count++ && mpfLog)
        {
            fprintf (mpfLog, "Wait for fopen...\n");
            fflush (mpfLog);
        }
        pf = MyFOpen (msNextImageName, "rb");
    }
    fclose (pf);
    if (mpfLog)
    {
        fprintf (mpfLog, "fopen succeeded\n");
        fflush (mpfLog);
    }
    return true;
}

void CPipeIF::UpdateObjects(void)
{
    CString sName;
    while (GetFirstFileName(sName))
    {
		if (mpfLog)
			fprintf (mpfLog, "<UpdateObjects> sName %s\n", (const char*)sName);
        if (sName.Left(2) == "::")
            sName = sName.Right(sName.GetLength() - 2);
        if (isdigit(sName[0]))
        {
            int id = atoi(sName);
            CNamedObject *pObject = CNamedObject::GetById(id);
            if (pObject)
			{
				if (mpfLog)
					fprintf (mpfLog, "<UpdateObjects> object %s\n", (const char*)pObject->Name());
                pObject->UpdateYourself();
			}
        }
    }
}
bool CPipeIF::GetFirstSpecialName(CString & sName)
{
    if (!GetFirstFileName(sName))
        return false;

    if (sName.Left(2) == "::")
        sName = sName.Right(sName.GetLength() - 2);
    return true;
}
bool CPipeIF::GetFirstFloat(float & floatValue)
{
    CString sText;
    if (!GetFirstSpecialName(sText))
        return false;

    floatValue = (float)atof((const char *)sText);
    return true;
}
bool CPipeIF::GetFirstInt(int & value)
{
    CString sText;
    if (!GetFirstSpecialName(sText))
        return false;

    value = atoi((const char *)sText);
    return true;
}
void CPipeIF::SendGraphicCommand(EGraphicCommand eGCom)
{
    CString sName;
    while (GetFirstFileName(sName))
    {
        if (sName.Left(2) == "::")
            sName = sName.Right(sName.GetLength() - 2);
        gScreens.DoGraphicCommand(eGCom, sName);
    }
}
void CPipeIF::OnSetTitle(void)
{
    CString sTitle;
    bool bOk = GetFirstSpecialName(sTitle);
	if (bOk)
		gScreens.SetTitle(sTitle);
}
void CPipeIF::OnSetPosition(void)
{
    CString sName;
    int iImage;
    bool bOk = GetFirstSpecialName(sName);
    bOk |= GetFirstInt(iImage);
    if (bOk)
    {
        gScreens.SetPosition(sName, iImage);
    }
}
void CPipeIF::OnSetRelativePosition(void)
{
    CString sName;
    int iImage;
    bool bOk = GetFirstSpecialName(sName);
    bOk |= GetFirstInt(iImage);
    if (bOk)
    {
        gScreens.SetRelativePosition(sName, iImage);
    }
}
void CPipeIF::OnSetPosition2d(void)
{
    CString sName;
    int iDir;
    int iImage;
    bool bOk = GetFirstSpecialName(sName);
    bOk |= GetFirstInt(iDir);
    bOk |= GetFirstInt(iImage);
    if (bOk)
    {
        gScreens.SetPosition(sName, iDir, iImage);
    }
}
void CPipeIF::OnSetZoom(void)
{
	float zoomFactor;
    bool bOk = GetFirstFloat(zoomFactor);
    if (bOk)
    {
		gScreens.SetZoom(zoomFactor);
    }
}
void CPipeIF::OnSetStatus(void)
{
    CString sText;
    bool bOK = GetFirstSpecialName(sText);
	if (bOK)
	{
		int iField;
		bOK = GetFirstInt(iField);
	    if (bOK)
		{
			gScreens.SetStatusText(sText, iField);
		}
	}
}
void CPipeIF::OnSetCursorBroadcast(void)
{
	int iRectangle;
    bool bOk = GetFirstInt(iRectangle);
    if (bOk)
    {
		gScreens.SetCursorBroadcast(iRectangle);
    }
}
void CPipeIF::OnGetWindow(void)
{
	int iRectangle;
    bool bOk = GetFirstInt(iRectangle);
    if (bOk)
    {
		gScreens.BroadcastWindow(iRectangle);
    }
}
void CPipeIF::OnSetWindow(void)
{
    int center;
    int width;
	int iRectangle;
    bool bOk = GetFirstInt(center);
    bOk |= GetFirstInt(width);
    bOk |= GetFirstInt(iRectangle);
    if (bOk)
    {
        gScreens.SetWindowFromGui(center, width, iRectangle);
    }
}
void CPipeIF::OnSetWindowRange(void)
{
    int minVal;
    int maxVal;
	int iRectangle;
    bool bOk = GetFirstInt(minVal);
    bOk |= GetFirstInt(maxVal);
    bOk |= GetFirstInt(iRectangle);
    if (bOk)
    {
        gScreens.SetWindowRangeFromGui(minVal, maxVal, iRectangle);
    }
}
void CPipeIF::OnDisplayColorMap(void)
{
	CString sfName;
	int iRectangle;
    bool bOk = GetFirstFileName(sfName);
    bOk |= GetFirstInt(iRectangle);
    if (bOk)
	{
		if (sfName == "::On")
			gScreens.SetColorMap(true, iRectangle);
		else if (sfName == "::Off")
			gScreens.SetColorMap(false, iRectangle);
		else
			gScreens.ReadColorMap(sfName, iRectangle);
	}
}
void CPipeIF::OnDisplayImage(bool bReplace)
{
	static int count = 0;
	CString sMsg(_T("CPipeIF::ProcessMessage - "));
	sMsg += mzRequest;
	CMyTracer myTracer(sMsg, ++count);
    gScreens.SetServerMode(bReplace);
    if (!mFilesList.IsEmpty())
	{
		if (mNextFileUseSpecialPos)
			CFileNamePattern::SetNextFileUseSpecialPos(mNextFileUseSpecialPos);
		if (mpfLog)
		{
			fprintf (mpfLog, "%s\n", (const char*)sMsg);
			fflush (mpfLog);
		}
		gScreens.miPipeMessageToHandle = miMessage;
        mpViewer->InitiateSetDisplayFiles();
		/*if (mpfLog)
		{
			fprintf (mpfLog, "CPipeIF::ProcessMessage - after calling InitiateSetDisplayFiles()\n");
			fflush (mpfLog);
		} */
		CFileNamePattern::SetNextFileUseSpecialPos(0);
		mNextFileUseSpecialPos = 0;
	}
}
