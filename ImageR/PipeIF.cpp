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
    mLogger.Log("<GetFirstFileName>", (const char*)sName);
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
                mLogger.Printf("Found: %d %s", miNextImage, (const char*)msNextImageName);
 
                while (miNextImage > 1 && !pPosition)
                {
                    Sleep (20);
                    if (!pData)
                        pData = gScreens.GetCurrentData();
                    if (pData && !pPosition)
                        pPosition = pData->GetPosition();
                    if (pData)
                        mLogger.Log ("pData: ", (const char*)pData->msName);
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
                        mLogger.Log("Set Display");
                        mpViewer->InitiateSetDisplayFiles();
                    }
                }                 
                msNextImageName = "";
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
            char zBuf[256];
            sprintf_s (zBuf, 256, msImagePattern, miNextImage);
            msNextImageName = zBuf;
            mLogger.Printf("%3d: Waiting next image %s", miNextImage, (const char *)msNextImageName);
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
	mLogger.Log("<ProcessMessage>", mzRequest);

    /*
    if (_strnicmp(mzRequest, "-cirs ", 6) == 0)
    {
        mnMaxToDisplay = 1;

        msImagePattern = &mzRequest[6];
        msImagePattern += "_rcn01";

        CFileName fOutName (&mzRequest[6]);
        CString sPrivate (fOutName.Private());
        msImagePattern += "-img%03d";

        msImagePattern += ".dat";
        mLogger.Log("msImagePattern", (const char *)msImagePattern);

        msNextImageName = "";
        miNextImage = 0;

        if (!mbWaitInThread)
        {
            mbWaitInThread = true;
            CMyWindows::CreateThread (StaticWaitImageThread, this);
        }
    }*/
    if (_stricmp(mzRequest, "exit") == 0)
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
        CPosition::SetBroadcast();
    }
    else if (_stricmp(mzRequest, "broadcast_position_to_file") == 0)
    {
        CPosition::SetBroadcastToFile();
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
    else if (_stricmp(mzRequest, "set_replace_root") == 0)
    {
        CString sOrigRoot, sNewRoot;
        bool bOk = GetFirstFileName(sOrigRoot);
        bOk |= GetFirstFileName(sNewRoot);
        if (bOk)
        {
            RemovePrefix(sOrigRoot);
			CFileFactory::SetReplaceRoot(sOrigRoot, sNewRoot);
        }
   }
    else if (_stricmp(mzRequest, "load_state_and_replace") == 0)
    {
        CString sfName;
        bool bOk = GetFirstFileName(sfName);
        if (bOk)
        {
            RemovePrefix(sfName);
            CFileFactory::SetReplaceFileName(sfName);
            OnDisplayImage(true);
        }
    }
    else if (_stricmp(mzRequest, "load_state_and_replace2") == 0)
    {
        CString sfName1;
        CString sfName2;
        bool bOk = GetFirstFileName(sfName1);
        bOk &= GetFirstFileName(sfName2);
        if (bOk)
        {
            RemovePrefix(sfName1);
            RemovePrefix(sfName2);
            CFileFactory::SetReplaceFileName(sfName1);
            CFileFactory::SetReplaceFileName2(sfName2);
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
        mLogger.Log("Handling <save_state>");
        CString sName;
        bool bOk = GetFirstSpecialName(sName);
        if (bOk)
            gScreens.SaveStateToClientFile(sName);
		else
            mLogger.Log("Handling <save_state> GetFirstFileName failed");
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
void CPipeIF::UpdateObjects(void)
{
    CString sName;
    while (GetFirstFileName(sName))
    {
		mLogger.Log ("<UpdateObjects> sName %s", (const char*)sName);
        RemovePrefix(sName);
        if (isdigit(sName[0]))
        {
            int id = atoi(sName);
            CNamedObject *pObject = CNamedObject::GetById(id);
            if (pObject)
			{
                mLogger.Log("<UpdateObjects> object %s", (const char*)pObject->Name());
                pObject->UpdateYourself();
			}
        }
    }
}
bool CPipeIF::GetFirstSpecialName(CString & sName)
{
    if (!GetFirstFileName(sName))
        return false;

    RemovePrefix(sName);
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
        RemovePrefix(sName);
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
		mLogger.Log(sMsg);
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
void CPipeIF::RemovePrefix(CString& s)
{
    if (s.Left(2) = "::")
        s = s.Right(s.GetLength() - 2);
}
