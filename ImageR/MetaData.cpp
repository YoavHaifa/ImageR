#include "StdAfx.h"
#include "MetaData.h"
#include "TypedData.h"

#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyTracer.h"
#include "..\..\yUtils\CsvFile.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyProgress.h"
#include "..\..\yUtils\MyRegistry.h"
#include "..\..\yUtils\NameGetDialog.h"
#include "..\..\yUtils\MyMath.h"
#include "..\..\yUtils\MyDicomWriter.h"
#include "..\..\yUtils\Histogram.h"

#include "..\..\ImageRLib\Zoomer.h"
#include "..\..\ImageRLib\ImageStatistics.h"
#include "..\..\ImageRLib\TVolume.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\TImage.h"

#include "DisplayRectangle.h"
#include ".\Screens.h"
#include "DRText.h"
#include "TextController.h"
#include <Math.h>
#include "ContextMenu.h"
#include "PrintStat.h"
#include "MultiPageLoader.h"
#include "DataTypeDialog.h"
#include "WindowDlg.h"
#include "FileFactory.h"

int CMetaData::umDebug = 0;
static bool ubDebugZoom = false;
CMetaData *CMetaData::umpDebugData = NULL;
CMyCriticalSection CMetaData::mCriticalUpdate("MetaDataUpdate");
//float CMetaData::umInitialZoomFromCommandLine = 0;
bool CMetaData::umbMultiTypeWindows = true;
int CMetaData::umiFirstType = -1;
int CMetaData::umiLastType = -2;

// Data coloring static params
bool CMetaData::umbDisplayImageOnZeroColor = false;
bool CMetaData::umbColorTransparency = true;
float CMetaData::umTransparencyFraction = 0.5f;
bool CMetaData::umbEnableFastLoadFromVolume = false;

void CMetaData::ToggleEnableFastLoadFromVolume()
{
	umbEnableFastLoadFromVolume = !umbEnableFastLoadFromVolume;
}

CMetaData::CMetaData(const char *zName, CXMLParseNode *pXmlNode)
: CMetaDataBase(zName)
, mpTypedData(NULL)
, mpfLog(NULL)
, mpPageData(NULL)
, mpVolume(NULL)
, mpZoomedRasterFloat(NULL)
, mpZoomedRasterRgb(NULL)
, mpZoomedRaster(NULL)
, mpCommonWindowParams(NULL)
, mpSpecialWindowParams(NULL)
, mbUseSpecialWindow(false)
, mpCommonZoomPanParams(NULL)
, mpSpecialZoomPanParams(NULL)
, mbUseSpecialZoom(false)
, mpDR(NULL)
, mpXmlNode(pXmlNode)
, mpXmlMetaDataNode(NULL)
, mpXmlMetaDataBaseNode(NULL)
, mbValid(true)
, mColor(0)
, mbTransparent(false)
, mbMmPerPixelSet(false)
, mbVolumeIsShared(false)
, mbDisplayStatistics(true)
, mZoomedRect(0,0,0,0)
, mbTypedDataCreatedByMe(false)
, mbPageDataCreatedByMe(false)
, mpWantedCenterCoo(NULL)
, mbMultiPageDisplay(false)
, mpMultiPageLoader(NULL)
, mbIsDataForColoring(false)
, msUserDefinedTitle(_T(""))
, mpWindParamsDlg(NULL)
, mbChangeEndian(false)
, mbFastLoadFromVolume(false)
, mbLoadingVolume(false)
, mbUpdateForLoadingVolume(false)
, mnPageLoads(0)
{
	SetCtImage(true);
    CFileName fName (zName);
	TryToGetImageSizeFromName();

    if (pXmlNode)
    {
		mpXmlMetaDataNode = mpXmlNode->GetFirstDeep("MetaData");
		mpXmlMetaDataBaseNode = mpXmlMetaDataNode->GetFirstDeep("MetaDataBase");
        
		CXMLParseNode *pWindowXml = mpXmlNode->GetFirstDeep("WindowParams");
        if (pWindowXml)
        {
			static CWindowParams* upWindowSave = NULL;
			static bool ubOnce = false;

			if (upWindowSave && CFileFactory::CreatingData2())
			{
				SetCommonWindow(upWindowSave);
				upWindowSave = NULL;
			}
			else
			{
				CNamedObject *pObject = CNamedObject::GetParsed(pWindowXml);
				if (pObject)
					SetCommonWindow((CWindowParams *)pObject);
				else
					SetCommonWindow (new CWindowParams(pWindowXml));
			}

			CXMLParseNode *pSpecialWindowXml = mpXmlNode->GetFirstDeep("SpecialWindow");
			if (pSpecialWindowXml)
			{
				CXMLParseNode *pSpecialWindowDetails = pSpecialWindowXml->GetFirstDeep("WindowParams");
				SetSpecialWindow(new CWindowParams(pSpecialWindowDetails));
				pSpecialWindowXml->GetValue("b_use_special_window",mbUseSpecialWindow);
			}
			if (!ubOnce)
			{
				upWindowSave = mpCommonWindowParams;
				ubOnce = true;
			}
		}

        CXMLParseNode *pZoomXml = mpXmlNode->GetFirstDeep("zoom_params");
        if (pZoomXml)
        {
			static CZoomPanParams* upZoomSave = NULL;
			static bool ubOnce = false;
			CZoomPanParams* pZoom = NULL;
			CDisplayRectangle* pDR = gScreens.GetCurrentDR();
			if (pDR && pDR->mbMultiVariant)
			{
				CMetaData* pPrev = pDR->GetCurrentData();
				if (pPrev)
					pZoom = pPrev->GetZoomParams();
			}
			else if (upZoomSave && CFileFactory::CreatingData2())
			{
				pZoom = upZoomSave;
				upZoomSave = NULL;
			}
			if (!pZoom)
				pZoom = (CZoomPanParams *)CNamedObject::GetParsed(pZoomXml);
            if (!pZoom)
                pZoom = new CZoomPanParams(pZoomXml);
            SetCommonZoom (pZoom);
			if (!ubOnce)
			{
				upZoomSave = pZoom;
				ubOnce = true;
			}
        }
    }
}
CMetaData::~CMetaData(void)
{
	DeleteVolume();

    if (mpWindParamsDlg)
        delete mpWindParamsDlg;

	if (mpMultiPageLoader)
		delete mpMultiPageLoader;

	if (mpZoomedRaster)
		delete mpZoomedRaster;

    if (mpTypedData && mbTypedDataCreatedByMe)
        delete mpTypedData;
	if (mpPageData && mbPageDataCreatedByMe)
        delete mpPageData;
    if (mpfLog)
        fflush (mpfLog);
	if (mpWantedCenterCoo)
		delete mpWantedCenterCoo;

	if (mpSpecialZoomPanParams)
		delete mpSpecialZoomPanParams;

	if (mpSpecialWindowParams)
		delete mpSpecialWindowParams;

	DeleteAllText();

}
void CMetaData::DeleteAllText(void)
{
	while (!mDataTextList.IsEmpty())
	{
		delete mDataTextList.GetTail();
		mDataTextList.RemoveTail();
	}
}
void CMetaData::PostConstruct(class CXMLParseNode *pXmlNode)
{
    if (!mpXmlNode)
        mpXmlNode = pXmlNode;



    if (mpXmlNode)
    {
        CXMLParseNode *pPositionXml = mpXmlNode->GetFirstDeep("multi_position");
        if (pPositionXml)
        {
            CPosition *pPos = (CPosition *)CNamedObject::GetParsed(pPositionXml);
            if (pPos)
                SetPosition(pPos);
            else
                ParsePosition(pPositionXml);
        }
    }

    if (!HasPosition())
        InitPosition();
}
void CMetaData::ParsePosition(class CXMLParseNode *pPosXmlNode)
{
    if (HasPosition())
		mMultiPosition.RestoreState(pPosXmlNode);
	else
		SetPosition(new CPosition(pPosXmlNode));
}
void CMetaData::InitPosition(void)
{
    if (!HasPosition())
        SetNewPosition("Pos", 0, 0);
}
bool CMetaData::SetNewPosition(const char * zName, int iFirst, int iLast, CXMLParseNode *pMetaDataXml)
{
	CXMLParseNode *pPositionXml = NULL;
    if (pMetaDataXml)
        pPositionXml = pMetaDataXml->GetDFS("position");

    if (pPositionXml)
        SetPosition(new CPosition(pPositionXml));
    else
        SetPosition(new CPosition(zName));

    SetLeafRange(iFirst, iLast);
    return true;
}

void CMetaData::SetTypedDataHandler(EMyImageDataType eDataType)
{
    if (mpTypedData)
    {
        if (mDataType == eDataType)
            return;
        delete mpTypedData;
    }
	SetDataType(eDataType);

    if ((mDataType != DATA_TYPE_SHORT) && (mDataType != DATA_TYPE_USHORT))
        SetCtImage(false);

	if (mDataType == DATA_TYPE_RGB)
		mbDisplayStatistics = false;

    mpTypedData = CTypedData::GetNew(eDataType);
	mbTypedDataCreatedByMe = true;
    if (mpPageData)
    {
        delete mpPageData;
        mpPageData = NULL;
    }
    mbPageUpdated = false;
}

void CMetaData::PrintId(FILE * pf, const char * zText)
{
    if (zText)
        fprintf (pf, "%s ", zText);
	fprintf (pf, "File <%s>", z_name());
    if (HasFileType())
	    fprintf (pf, " Type <%s>", (const char*)FileType());
	fprintf (pf, "\n");
}
float CMetaData::Min ()
{
    if (!mpTypedData)
        return 0.0f;
    return mpTypedData->Min();
}
float CMetaData::Max ()
{
    if (!mpTypedData)
        return MAX_CT_VALUE;
    return mpTypedData->Max();
}
float CMetaData::Range ()
{
    if (!mpTypedData)
        return MAX_CT_VALUE;
    return mpTypedData->Width();
}
float CMetaData::Average ()
{
    if (!mpTypedData)
        return 0.0f;
    return mpTypedData->Average ();
}
float CMetaData::NonZeroAverage ()
{
    if (!mpTypedData)
        return 0.0f;
    return mpTypedData->NonZeroAverage ();
}
//--------------------------------------------------------------------------------------
CString CMetaData::GetExtraName(const char *zPrevName)
{
    CString myName (msName);
    CString sOther (zPrevName);
    myName.MakeLower();
    sOther.MakeLower();
    const char *zMy = myName;
    const char *zOther = sOther;
    int myNameLength = msName.GetLength();

	for (int i = 0; i < myNameLength; i++)
	{
		if (!zMy[i])
			return msName;

		if (zMy[i] != zOther[i])
        {
            if (i <= 2)
                return msName;
            return msName.Right(myNameLength - i + 2);
        }
	}

	// All Same
	return CString(_T("(Same)"));
}

CString CMetaData::PrivateName(void) 
{
    CFileName name (msName);
    return CString (name.Private());
}
void CMetaData::PrintScreenToExcel ()
{
    CString sfName = mpTypedData->PrintScreenToExcel (mpPageData->GetDataStart(), PrivateName(),
        mnLinesInPage, mnCols, mnCols, GetFileNameText());

    if (!sfName.IsEmpty())
        CMyWindows::OpenExcel(sfName);
}
bool CMetaData::HasFileType() 
{
    CFileName name (msName);
    return (strlen(name.Type()) > 0);
}
CString CMetaData::FileType() 
{
    CFileName name (msName);
    return CString(name.Type());
}
CMyImage *CMetaData::PageData(void)
{
    if (!mpPageData)
        CreatePageData();
    return mpPageData;
}
bool CMetaData::SetSize(int nLines, int nCols)
{
    if (nLines == mnLinesInPage && nCols == mnCols && mpPageData)
        return true;


    mbPageUpdated = false;
    mnLinesInPage = nLines;
    mnCols = nCols;
    if (!mpPageData)
        if (!CreatePageData())
            return false;

    return mpPageData->SetSize(mnLinesInPage, mnCols);
}
bool CMetaData::HasPageData()
{
    if (!mpPageData)
        return false;
    if (mpPageData->mnPixels == 0)
        return false;
    if (mpPageData->mbUpdateFailed)
        return false;
    return true;
}
bool CMetaData::CreatePageData ()
{
    if (mnLinesInPage <= 0 || mnCols <= 0)
        return false;
    
    CFileName fName(msName);
    CString sPageName (fName.Private());
    sPageName += "_page";

    mpPageData = CreatePage(sPageName);

    if (!mpPageData)
		return false;

    mbPageDataCreatedByMe = true;
    mpPageData->mMmPerPixelHeight = mMmPerPixelHeight;
    mpPageData->mMmPerPixelWidth = mMmPerPixelWidth;
    return true;
}
bool CMetaData::ForceUpdatePageData(void)
{
	mbPageUpdated = false;
	return UpdatePageData();
}
bool CMetaData::UpdatePageData()
{
	if (mbLoadingVolume && !mbUpdateForLoadingVolume)
		return false;

	CMyCriticalSpan span(mCriticalUpdate,"UpdatePageData");

	static int count = 0;
	CMyTracer myTracer ("CMetaData::UpdatePageData", ++count);
    if (mbDebug && (umDebug & 0x80))
    {
		if (!mpfLog)
			mpfLog = CMyWindows::FOpenLogFile(Name());
    }

	// Make sure page size is well defined
    if (!mnLinesInPage || !mnCols)
	{
		static int unWarnings = 0;
		if (!unWarnings++)
		{
			char zBuf[256];
			sprintf_s(zBuf,sizeof(zBuf),"nLines %d nCols %d - %s", mnLinesInPage, mnCols, (const char*)Name());
			CMyWindows::MessBox(zBuf,"Warning: No Data to Display");
		}
		return false; 
	}

	CMyTracer::Print("Check mnCols");
    if (mnCols < 0 || mnCols > 10000)
    {
        char zBuf[256];
        sprintf_s (zBuf, 256, "%s\n mnCols %d", (const char*)msName, mnCols);
        MessageBox (NULL, zBuf, "Error", MB_OK);
        return false;
    }

	// Make sure you have a target page
    if (!mpPageData)
        if (!CreatePageData ())
            return false;

    if (mpPageData)
    {
        if (mpPageData->SetSize(mnLinesInPage, mnCols))
            mpPageData->mbUpdateFailed = false;
    }
	CMyTracer::Print("Check HasPageData");
    if (!HasPageData())
        return false;

    if (mbPageUpdated && !ShouldUpdate())
        return true;

    if (mpTypedData)
        mpTypedData->Init();

	CMyTracer::Print("Check mpPosition");
	if (HasPosition())
	{
		CFileNamePattern::SetAutoCorrect(true);
		bool bSet = set_i(GetPosition()->miCurrent);
		CFileNamePattern::SetAutoCorrect(false);
        if (!bSet)
	    {
		    if (mpfLog)
			    fprintf (mpfLog, "Failed to set page: Name <%s>, iCurrent %d\n\n", z_name(), GetPosition()->miCurrent);
		    return false;
	    }
	}

    if ((umDebug & 0x2) && mpfLog)
    {
        fprintf (mpfLog, "<CMetaData::UpdatePageData> %s %s\n", 
            (const char *)GetClass(), (const char *)msName);
        fflush (mpfLog);
    }

	CMyTracer::Print("Check UpdateDataNotice");
    gScreens.UpdateDataNotice(this);

	CMyTracer::Print("Check OnStartUpdate");
    OnStartUpdate();
	bool bUpdated = false;
	if (mbMultiPageDisplay)
	{
		CMyTracer::Print("Check mpMultiPageLoader->UpdatePage");
		bUpdated = mpMultiPageLoader->UpdatePage();
	}
	else
	{
		CMyTracer::Print("Check DoUpdatePageData");
		mpPageData->mnRealPixels = 0; // Means undefined - unless it will be defied by the loader
		bUpdated = DoUpdatePageData ();
	}
    if (!bUpdated)
    {
		UpdateTexts();
        if (mpfLog)
        {
            fprintf (mpfLog, "<CMetaData::UpdatePageData> FAILED %s\n", (const char *)msName);
            fflush (mpfLog);
        }
        if (umpfUpdatesLog)
        {
            fprintf (umpfUpdatesLog, "<CMetaData::UpdatePageData> FAILED %s\n", (const char *)msName);
            fflush (umpfUpdatesLog);
        }
        return false;
    }
	else
	{
		if (mbChangeEndian)
			ChangeEndianInPage();
	}

    mbPageUpdated = true;
	CMyTracer::Print("Check umDebug");
    if ((umDebug & 0x2) && mpfLog)
    {
        fprintf (mpfLog, "<CMetaData::UpdatePageData> OK %s\n", (const char *)msName);
        fflush (mpfLog);
    }
	if (mbDebug && (umDebug & 0x100))
		mpPageData->Dump();

	CMyTracer::Print("Check mpTypedData");
    if (mpTypedData)
    {
        static int typedCount = 0;
        
		if (mnLastColsToIgnore)
			mpTypedData->OnReadLines(mpPageData->GetDataStart(), mnLinesInPage, mnCols, mnLastColsToIgnore);
		else
	        mpTypedData->OnReadBytes(mpPageData->GetDataStart(), mpPageData->GetRealByteSize());

        if ((mpTypedData->Min() < MIN_CT_VALUE) || (mpTypedData->Max() > MAX_CT_VALUE))
			SetCtImage(false);
		if (!gScreens.mbEbwMode && !typedCount++)
        {
            if (mpTypedData->mnSummed > 0 && mpTypedData->mSum > -1.0f && mpTypedData->mSum < 1.0f)
            {
                if (!IsChannelTable())
                {
                    double sum = mpTypedData->mSum;
					double avg = sum / mpTypedData->mnSummed;
					if ((avg > -0.00001) && (avg < 0.00001))
					{
						char zBuf[256];
						if (sum > 0.001f || sum < -0.001f)
							sprintf_s (zBuf, sizeof(zBuf), "%s\n%d pixels read, sum %f", (const char*)msName, 
								mpTypedData->mnSummed, sum);
						else
							sprintf_s (zBuf, sizeof(zBuf), "%s\n%d pixels read, sum %.12f", (const char*)msName, 
								mpTypedData->mnSummed, sum);
						CString sWarning(zBuf);
						if (mpTypedData->mnNan > 0)
						{
							sprintf_s (zBuf, sizeof(zBuf), ", %d NAN", mpTypedData->mnNan);
							sWarning += zBuf;
						}
						CString sEmpty;
						sEmpty.LoadString(IDS_EMPTY_DATA);
						CMyWindows::MessBox (zBuf, sEmpty);
					}
                }
                if (mDataType == DATA_TYPE_FLOAT || mDataType == DATA_TYPE_DOUBLE)
                    mpTypedData->SetFormat("%.12f");
            }
        }
        if (/*(umDebug & 0x2) && */ mpfLog)
        {
            fprintf (mpfLog, "<CMetaData::UpdatePageData> %s\n", 
                (const char *)msName);
            fprintf (mpfLog, "%s\n", (const char*)mpTypedData->GetRangeAndAverageText());
            fprintf (mpfLog, "\n");
            fflush (mpfLog);
        }
    }
    if (umpfUpdatesLog)
        fprintf (umpfUpdatesLog, "<CMetaData::UpdatePageData> ");

	CMyTracer::Print("Check UpdateTexts");
	UpdateTexts();
    OnChange();
    if (umpfUpdatesLog)
        fflush (umpfUpdatesLog);
	mnPageLoads++;
    return true;
}
void CMetaData::UpdateTexts(void)
{
	int iText = 0;
	POSITION pos = mDataTextList.GetHeadPosition();
	while (pos)
	{
		mDataTextList.GetNext(pos)->Update(iText++);
	}
}
bool CMetaData::DoUpdatePageData()
{
	static int unWarning = 0;
	if (!unWarning++)
	{
		char zBuf[256];
		sprintf_s(zBuf,sizeof(zBuf),"Class <%s> should implement \"DoUpdatePageData()\" to see data", (const char*)GetClass());
		CMyWindows::MessBox(zBuf, "SW Warning");
	}
    if (mpPageData)
        mpPageData->mbUpdateFailed = true;
    return false;
}
bool CMetaData::DoLoadVolume()
{
	static int unWarning = 0;
	if (!unWarning++)
	{
		char zBuf[256];
		sprintf_s(zBuf,sizeof(zBuf),"Class <%s> should implement \"DoLoadVolume()\" to see volumetric data", (const char*)GetClass());
		CMyWindows::MessBox(zBuf, "SW Warning");
	}
    if (mpPageData)
        mpPageData->mbUpdateFailed = true;
	return false;
}
bool CMetaData::LoadVolume(void)
{
	StartLoadVolume();
	Sleep(1000);
	while (mbLoadingVolume)
		Sleep(100);
	return true;
}
bool CMetaData::StartLoadVolume ()
{
    if (!mpVolume)
    {
        //int nPages = GetNPages();
        mpVolume = CVolume::GetNew(msName + "Vol", mDataType, mnLinesInPage, mnCols,
            GetPosition());
        if (!mpVolume)
            return false;
        return CMyWindows::CreateThread (StaticLoadVolume, this);
    }
    return true;
}
DWORD WINAPI CMetaData::StaticLoadVolume(void *p)
{
    CMetaData *pData = (CMetaData *)p;
	while (!gScreens.FirstPaintFinished() || pData->mnPageLoads == 0)
		Sleep(10);
	pData->mbLoadingVolume = true;
	pData->DoLoadVolume();
	pData->mbLoadingVolume = false;
	return 0;
}
void * CMetaData::GetPageData(void)
{
    if (!HasPageData())
        return NULL;
    return mpPageData->GetDataStart();
}
void * CMetaData::GetPageDataLine(int iLine)
{
    if (!HasPageData())
        return NULL;
    return mpPageData->GetLineDataStart(iLine);
}

//void * CMetaData::GetDataForDisplay(int iLineInPage, int iColInPage)
//{
//    if (!HasPageData())
//        return NULL;
//    return mpPageData->GetDataForDisplay (iLineInPage, iColInPage);
//}

CString CMetaData::GetValueString(CDataCoordinates coo)
{
    if (!coo.IsOnRaster())
        return CString();
    if (!HasPageData())
        return CString();
    return mpPageData->GetValueString (coo);  
}

float CMetaData::GetFloatValue(class CDataCoordinates coo)
{
    if (!coo.IsOnRaster())
        return 0.0f;
    if (!HasPageData())
        return 0.0f;
    return mpPageData->GetFloatValue (coo.Y(), coo.X());  
}
CFloatCoordinates CMetaData::GetCenter()
{
    return CFloatCoordinates (GetNCols()/2.0f, GetNLinesInPage() / 2.0f);
}
CSize CMetaData::DefaultImageSize(void)
{
    if (mnCols > 0 && mnLinesInPage > 0)
	{
		float zoomFromNameOrCommandLine = 1;
		if (GetInitialZoom(zoomFromNameOrCommandLine))
	        return CSize((int)(mnCols*zoomFromNameOrCommandLine + 0.5f), (int)(mnLinesInPage*zoomFromNameOrCommandLine + 0.5f));
	       
		int nLines = max(32,mnLinesInPage);
		if (mVerticalZoomFactor > 0)
			nLines = max(32,(int)(mnLinesInPage * mVerticalZoomFactor+ 1));
		int nCols = max(mnCols,256);
		return CSize(nCols,nLines);
	}
    return CSize(512,512);
}
CString CMetaData::GetStatisticsStringPerCoo(CDataCoordinates coo)
{
	if (!mbDisplayStatistics)
        return CString();
    if (!coo.IsOnRaster())
        return CString();
    if (!HasPageData())
        return CString();
    return mpPageData->GetStatisticsStringPerCoo (coo, this);  
}
CString CMetaData::GetSpecialString(CDataCoordinates coo)
{
    if (!coo.IsOnRaster())
        return CString();
	if (IsCtImage() || (mRescaleIntercept != 0) || (mRescaleSlope != 1.0f))
    {
        char zBuf[32];
        //unsigned short *pDataBuf = (unsigned short *)GetDataForDisplay (coo.Y(), coo.X());
        float value = mpPageData->GetFloatValue(coo.Y(), coo.X());
		value = Rescale(value);
		if (mRescaleIntercept == 1)
		{
			short CtVal = (short)(value + 0.5f);
			sprintf_s(zBuf, 32, "CT: %d", CtVal);
		}
		else
		{
			int iVal = (int)value;
			if ((float)iVal == value)
				sprintf_s(zBuf, 32, "Sc: %d", iVal);
			else
				sprintf_s(zBuf, 32, "Sc: %.1f", value);
		}
        return CString(zBuf);
    }
    return CString();
}


void CMetaData::OnFillZoomedRasterError(const char *zText)
{
    static int nErrors = 0;
    if (!nErrors++)
    {
        char zBuf[256];
        sprintf_s (zBuf, 256, "Fill Zoomed Raster Error: %s\n%s", zText, (const char*)msName);
        MessageBox(NULL, zBuf, msName, MB_OK);
    }
}
bool CMetaData::FillZoomedRaster(CDisplayRectangle &displayRect)
{
	float fillValue = 0.0f;
	if (mDataType == DATA_TYPE_SHORT)
		fillValue = -32767.0f;

    CLogFile log;
	ubDebugZoom = ((umDebug & 0x1000) != 0);
    if (ubDebugZoom)
    {
        CFileName fName(msName);
        CString sPrivate("Zoom\\");
        sPrivate += fName.Private();
        log.Open (sPrivate + "_FillRaster");
    }

	if (!mpZoomedRaster)
	{
		CString sName(CFileName::GetPrivate(msName) + "_Zoomed");
		if (mDataType == DATA_TYPE_RGB)
		{
			mpZoomedRasterRgb = new CTImage<S_RGB>(sName);
			mpZoomedRaster = mpZoomedRasterRgb;
		}
		else
		{
			mpZoomedRasterFloat = new CTImage<float>(sName);
			mpZoomedRaster = mpZoomedRasterFloat;
		}
		if (mpCommonZoomPanParams)
			mpZoomedRaster->AddCheck(mpCommonZoomPanParams);
	}
    // Set Zoom Raster Dependencies
    if (!mpZoomedRaster->miUpdate)
    {
        mpZoomedRaster->AddCheck(this);
        mpZoomedRaster->AddCheck(&displayRect);
    }

    // Set raster's shape to current display rectangle
	mpZoomedRaster->SetSize(displayRect.bottom-displayRect.top+1, displayRect.right-displayRect.left+1);
    if (log)
        fprintf (log.mpf, "zoomedRect x %d - %d y %d - %d\n", displayRect.left, displayRect.right,
            displayRect.top, displayRect.bottom);

    if (!HasPageData())
    {
        mpZoomedRaster->Zero();
		if (!gScreens.mbIgnoreServerTimingProblems)
	        OnFillZoomedRasterError("No Page Data");
        return false;
    }
    if (!mpZoomedRaster->ShouldUpdate())
        return true;
    if (mMmPerPixelHeight != mpPageData->MmPerPixelHeight())
    {
        OnFillZoomedRasterError("mMmPerPixelHeight != mpPageData->MmPerPixelHeight()");
        return false;
    }

    // Find wanted area corners
    GetZoomParams();

	EZoomMode eMode = GetZoomParams()->GetMode() ;

	if ((mDataType == DATA_TYPE_BOOL) 
		|| (mDataType == DATA_TYPE_COLORREF) 
		|| (mDataType == DATA_TYPE_RGB))
		eMode = ZOOM_DUP;
	
	bool bOK = true;
	if (mpZoomedRasterFloat)
	{
		bOK = CZoomer::Zoom(*mpZoomedRasterFloat, mpPageData, GetZoomParams(),
				eMode, fillValue, &mZoomedRect);
	}
	else
	{
		bOK = CZoomer::Zoom(*mpZoomedRasterRgb, mpPageData, GetZoomParams(),
				eMode, S_RGB(), &mZoomedRect);
	}

	if (bOK)
		mpZoomedRaster->OnChange();
    return bOK;
}
bool CMetaData::ZoomedRasterIsReady(void)
{
	return mpZoomedRaster->miUpdate > miUpdate;
}
void CMetaData::ToggleSpecialZoom ()
{
	if (mbUseSpecialZoom)
	{
		mbUseSpecialZoom = false;
		mpCommonZoomPanParams->OnChange();
	}
	else
	{
		if (!mpSpecialZoomPanParams)
		{
			SetSpecialZoom();
		}
		mbUseSpecialZoom = true;
		mpSpecialZoomPanParams->OnChange();
	}
	gScreens.Refresh();
}
void CMetaData::SetNewCommonZoom ()
{
    if (mpCommonZoomPanParams)
        return;

	CSize wantedSize = gScreens.GetWantedSize();
    SetCommonZoom (new CZoomPanParams(GetZoomName()));
	CZoomPanParams::SetCommonZoom(mpCommonZoomPanParams);

    mpCommonZoomPanParams->SetMmPerPixel(MmPerPixelWidth());
	float zoomFromNameOrCommandLine = 1;
	EZoomMode eMode = ZOOM_BILINEAR;
	if (GetInitialZoom(zoomFromNameOrCommandLine, &eMode))
	{

		mpCommonZoomPanParams->MultiplyZoomFactor(zoomFromNameOrCommandLine);
		mpCommonZoomPanParams->SetMode(eMode);
	}
	/* Zoom due to wanted size will be handled later
	if (wantedSize.cx)
	{
		float zoomFactor = (float)wantedSize.cx / mnCols;
		mpCommonZoomPanParams->MultiplyZoomFactor(zoomFactor);
	} */
    if (DisableZoom())
        mpCommonZoomPanParams->Disable();
}
bool CMetaData::GetInitialZoom(float &oZoomFactor, EZoomMode * poeMode)
{
	bool bSet = false;
	float zoomFactor = 0;
	if (CZoomPanParams::GetInitialZoom(&zoomFactor) > 0)
	{
		; //zoomFactor = umInitialZoomFromCommandLine;
	}
	else if (CMyWindows::GetValueFromString(msName,"_zoom",zoomFactor))
		;
	else if (CMyWindows::GetValueFromString(msName,"_dzoom",zoomFactor))
	{
		if (poeMode)
			*poeMode = ZOOM_DUP;
	}
	else if (CMyWindows::GetValueFromString(msName,"_czoom",zoomFactor))
	{
		if (poeMode)
			*poeMode = ZOOM_BICUBIC;
	}

	if (zoomFactor >= 0.001f)
	{
		CZoomPanParams::SetInitialZoom(zoomFactor);
		oZoomFactor = zoomFactor;
		bSet = true;
	}

	float verticalZoomFactor = 0;
	if (CMyWindows::GetValueFromString(msName,"_vzoom",verticalZoomFactor))
		SetVerticalZoom(verticalZoomFactor);

	return bSet;
}
void CMetaData::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("MetaData",xmlDump);
    CMetaDataBase::SaveState(xmlDump);
    if (mDumpCount)
        return;

	xmlDump.Write("data_type", CMyImage::GetDataTypeString(EMyImageDataType(mDataType)));
	xmlDump.Write("data_unit_size", mDataUnitSize);

    if (mpCommonWindowParams)
        mpCommonWindowParams->SaveState(xmlDump);

    if (mpSpecialWindowParams)
	{
	    CXMLDumpScope graph1("SpecialWindow",xmlDump);

		xmlDump.Write("b_use_special_window", mbUseSpecialWindow);
        mpSpecialWindowParams->SaveState(xmlDump);
	}

    if (mpCommonZoomPanParams)
        mpCommonZoomPanParams->SaveState(xmlDump);
	if (mpSpecialZoomPanParams)
	{
		CXMLDumpScope graph2("SpecialZoom",xmlDump);
		xmlDump.Write("b_use_special_zoom", mbUseSpecialZoom);
		mpSpecialZoomPanParams->SaveState(xmlDump);
	}

    xmlDump.Write("rows",mnLinesInPage);
    xmlDump.Write("columns",mnCols);

    if (mMmPerPixelWidth == mMmPerPixelHeight)
        xmlDump.Write("mm_per_pix",mMmPerPixelWidth);
    else
    {
        xmlDump.Write("mm_per_pix_width",mMmPerPixelWidth);
        xmlDump.Write("mm_per_pix_height",mMmPerPixelHeight);
    }
	if (IsCtImage() || mbBiasSet)
	{
		float bias = GetBias();
        xmlDump.Write("data_bias", bias);
	}
    if (mpVolume)
        mpVolume->SaveState(xmlDump);

	if (mpTypedData)
	{
		CXMLDumpScope graph3("LastUpdate",xmlDump);
		xmlDump.Write("i_update", (unsigned int)miUpdate);
		xmlDump.Write("min", mpTypedData->Min());
		xmlDump.Write("max", mpTypedData->Max());
		xmlDump.Write("n", mpTypedData->mnChecked);
		xmlDump.Write("average", mpTypedData->Average());
		if (mpTypedData->mnNan > 0)
			xmlDump.Write("nNAN", mpTypedData->mnNan);
	}
	if (mZoomedRect.left > 0 || mZoomedRect.top > 0)
	{
		char zBuf[128];
		sprintf_s(zBuf,128, "x [%d - %d] y [%d - %d]", mZoomedRect.left, mZoomedRect.right, mZoomedRect.top, mZoomedRect.bottom);
		xmlDump.Write("zoomed_rect", zBuf);
	}
	if (!mDataTextList.IsEmpty())
	{
		CXMLDumpScope graph4("Texts",xmlDump);
		POSITION pos = mDataTextList.GetHeadPosition();
		while (pos)
			mDataTextList.GetNext(pos)->SaveState(xmlDump);
	}
	if (mbMultiPageDisplay)
		mpMultiPageLoader->SaveState(xmlDump);
}
CWindowParams *CMetaData::GetWindowParamsTyped(void)
{
	static int count = 0;
	count++;

	if (umiFirstType < 0)
	{
		umiFirstType = mpTypesPosition->miFirst;
		umiLastType = mpTypesPosition->miLast;
		// int n = umiLastType - umiFirstType + 1;

	}
	int iCur = mpTypesPosition->miCurrent;
	if (iCur < umiFirstType || iCur > umiLastType)
		return NULL;


    if (mpCommonWindowParams)
    {
        if (!mpCommonWindowParams->mbInitialized)
            mpCommonWindowParams->SetAutoFocus(this);
        mpCommonWindowParams->mpCurrentData = this;
    }
	return mpCommonWindowParams;
}
CWindowParams *CMetaData::GetWindowParams (void) 
{
	if (mpTypesPosition && umbMultiTypeWindows)
	{
		CWindowParams *pWind = GetWindowParamsTyped();
		if (pWind)
			return pWind;
	}

    // No Ready Window - See what may be done
    if (!mpCommonWindowParams)
    {
        CString sWantedWindName = GetWindowName();
        CWindowParams *pWind = (CWindowParams *)CNamedObject::GetNamed(sWantedWindName);
        if (pWind)
            SetCommonWindow(pWind);
        else
			SetCommonWindow(new CWindowParams(this));
    }

	CWindowParams *pParams = mbUseSpecialWindow ? mpSpecialWindowParams : mpCommonWindowParams;

    if (pParams)
    {
        if (!pParams->mbInitialized)
            pParams->SetAutoFocus(this);
        pParams->mpCurrentData = this;
    }

    return pParams;
}
void CMetaData::SetWindowParamsFromGui(float center, float width)
{
	CWindowParams *pParams = GetWindowParams();
	if (!pParams)
		return;

	pParams->SetCenterFromGui(center);
	pParams->SetWidthFromGui(width);
}
void CMetaData::SetWindowRangeFromGui(float minVal, float maxVal)
{
	CWindowParams *pParams = GetWindowParams();
	if (!pParams)
		return;

	pParams->SetRangeFromGui(minVal, maxVal);
}
CZoomPanParams *CMetaData::GetCommonZoomParams (void)
{
    // If no ready Zoom - See what may be done
    if (!mpCommonZoomPanParams)
    {
        CString sWantedZoomName = GetZoomName();
        CZoomPanParams *pZoom = (CZoomPanParams *)CNamedObject::GetNamed(sWantedZoomName);
        if (pZoom)
            SetCommonZoom(pZoom);
        else
            SetNewCommonZoom();
    }
	return mpCommonZoomPanParams;
}
CZoomPanParams *CMetaData::GetZoomParams (void)
{
    // If no ready Zoom - See what may be done
    if (!mpCommonZoomPanParams)
		GetCommonZoomParams();

	if (mbUseSpecialZoom && mpSpecialZoomPanParams)
		return mpSpecialZoomPanParams;
    return mpCommonZoomPanParams;
}
void CMetaData::DisconnectFromZoom(void)
{
    if (!mpCommonZoomPanParams)
        return;

    mpZoomedRaster->RemoveCheck(mpCommonZoomPanParams);

    int n = mpCommonZoomPanParams->RemoveDependant(this);
    if (n < 1)
        delete mpCommonZoomPanParams;
    mpCommonZoomPanParams = NULL;
}
void CMetaData::DisconnectFromWindow(void)
{
    if (!mpCommonWindowParams)
        return;

    int n = mpCommonWindowParams->RemoveDependant(this);
    if (n < 1)
        delete mpCommonWindowParams;
    mpCommonWindowParams = NULL;
}
void CMetaData::DisconnectFromSpecialWindow(void)
{
    if (!mpSpecialWindowParams)
        return;

    int n = mpSpecialWindowParams->RemoveDependant(this);
    if (n < 1)
        delete mpSpecialWindowParams;
    mpSpecialWindowParams = NULL;
}
bool CMetaData::ToggleSpecialWindow(void)
{
	if (mbUseSpecialWindow)
		mbUseSpecialWindow = false;
	else
	    SetSpecialWindow();
	gScreens.Refresh();
	return mbUseSpecialWindow;
}
void CMetaData::SetCommonWindow(CWindowParams * pWindowParams)
{
    if (mpCommonWindowParams == pWindowParams)
        return;

    if (mpCommonWindowParams)
        DisconnectFromWindow();

    mpCommonWindowParams = pWindowParams;
    mpCommonWindowParams->AddDependant(this);
	mbUseSpecialWindow = false;
}
void CMetaData::SetSpecialWindow(CWindowParams * pWindowParams)
{
	mbUseSpecialWindow = true ;
    if (mpSpecialWindowParams)
		return;

	if (pWindowParams)
	{
		mpSpecialWindowParams = pWindowParams;
		mpSpecialWindowParams->OnChange();
	}
	else
	{
		mpSpecialWindowParams = new CWindowParams (this);
		CString sName = "Special";
		sName += mpSpecialWindowParams->Name();
		mpSpecialWindowParams->msName = sName;
	}
    mpSpecialWindowParams->AddDependant(this);
}
void CMetaData::SetCommonZoom (CZoomPanParams *pZoomPanParams) 
{
    if (mpCommonZoomPanParams == pZoomPanParams)
        return;

    if (mpCommonZoomPanParams)
        DisconnectFromZoom();

    mpCommonZoomPanParams = pZoomPanParams;
    mpCommonZoomPanParams->AddDependant(this);

	if (mpZoomedRaster)
		mpZoomedRaster->AddCheck(mpCommonZoomPanParams);

	if (mpWantedCenterCoo)
	{
		SetDisplayCenter(*mpWantedCenterCoo);
		delete mpWantedCenterCoo;
		mpWantedCenterCoo = NULL;
	}
}
void CMetaData::SetSpecialZoom (void) 
{
	if (mpSpecialZoomPanParams)
		return;

	mpSpecialZoomPanParams = new CZoomPanParams(*mpCommonZoomPanParams);
    mpSpecialZoomPanParams->AddDependant(this);

    mpZoomedRaster->AddCheck(mpSpecialZoomPanParams);
}
void CMetaData::SetWideWindow(void)
{
	CWindowParams *pParams = GetWindowParams ();

    pParams->mCenter = Average();
    pParams->mWidth = Range();
    if (pParams->mWidth < 0.001f)
        pParams->mWidth = 100.0f;
}
void CMetaData::AssignDisplayRectangle (class CDisplayRectangle &dr)
{
    if (mpDR != &dr)
    {
		if (!mpDR)
		{
			gTexts.SetDataTexts(this);
			UpdateTexts();
		}
        mpDR = &dr;
        OnAssignDR();
    }
}
void CMetaData::AddDualModeToName(void)
{
    CString sLast = msName.Right(2);
    if (sLast == "L0" || sLast == "L1")
        msName = msName.Left(msName.GetLength() - 2);

}
CString CMetaData::GetShortName(void)
{
    if (IsFile())
    {
        CString s = CFileName::GetLastDirName(msName);
        if (!s.IsEmpty())
            return s;
    }
    return msName;
}

void CMetaData::SetMmPerPixel(float width, float height, bool bOnlyOnce)
{
	CZoomPanParams *pParams = GetZoomParams();
	if (pParams)
	{
		if (!bOnlyOnce || !pParams->IsInitialized())
			pParams->SetMmPerPixel(width, bOnlyOnce);
		//pParams->SetMmPerPixel(width / mMmPerPixelWidth);
	}

    mMmPerPixelWidth = width;
    mMmPerPixelHeight = height;
    mbMmPerPixelSet = true;
    if (mpPageData)
    {
        mpPageData->mMmPerPixelWidth = width;
        mpPageData->mMmPerPixelHeight = height;
    }
}

void CMetaData::SetMmPerPixel(CMetaData & other)
{
    SetMmPerPixel(other.mMmPerPixelWidth, other.mMmPerPixelHeight, true);
}
int CMetaData::GetNVolumePages(void)
{
    if (!mpVolume)
        return 0;
    return mpVolume->GetNPages();
}
void CMetaData::AdjustColorsByDataColoring(class CDisplayRectangle &dr)
{
	CTImage<COLORREF> &colorRaster = dr.GetColorRaster();
	CTImage<unsigned char> &grayRaster = dr.GetGrayRasterForColorer();
	unsigned char *pGray = grayRaster.GetData();
    COLORREF *pRGBTable = gScreens.GetColorsRgbTable();
    int n = colorRaster.GetNPixels();
    COLORREF *pColors = colorRaster.GetData();

	if (umbColorTransparency)
	{
		while (n--)
		{
			COLORREF color = pRGBTable[*pGray++];
			if (color != 0)
			{
				COLORREF baseColor = *pColors;
				COLORREF result = 0;

				unsigned char *pBase = (unsigned char *)&baseColor;
				unsigned char *pCol = (unsigned char *)&color;
				unsigned char *pRes = (unsigned char *)&result;
				for (int iCol = 0; iCol < 3; iCol++)
					pRes[iCol] = (unsigned char)(pBase[iCol] * umTransparencyFraction) +
						(unsigned char)(pCol[iCol] * (1.0f - umTransparencyFraction));

				*pColors = result;
			}
			pColors++;
		}
	}
	else
	{
		if (umbDisplayImageOnZeroColor)
		{
			while (n--)
			{
				COLORREF color = pRGBTable[*pGray++];
				if (color != 0)
					*pColors = color;
				pColors++;
			}
		}
		else
		{
			while (n--)
			{
				*pColors++ = pRGBTable[*pGray++];
			}
		}
	}
}
void CMetaData::AdjustColors(CDisplayRectangle &dr)
{
	CTImage<COLORREF> &colorRaster = dr.GetColorRaster();
	if (!mpZoomedRasterFloat)
		return;

    if (!mpZoomedRaster->IsOfShape(colorRaster.mnLines, colorRaster.mnCols))
        return;

	bool bDump = (gScreens.mDebugColor & 0x100) || (mbDebug && (umDebug & 0x200));
	if (bDump)
	{
		colorRaster.Dump();
		mpZoomedRaster->Dump();
	}

    int n = colorRaster.GetNPixels();
    COLORREF *pColors = colorRaster.GetData();
	if (mpfLog)
		fprintf (mpfLog, "<CMetaData::AdjustColors> %s type %d color %x %s\n", 
		(const char*)Name(), mDataType, mColor, mbTransparent ? "Transparent" : "");

    float *pColorerData = mpZoomedRasterFloat->GetData();
	if (mDataType == DATA_TYPE_RGB)
	{
        while (n--)
        {
			float color = *pColorerData++;
            if (color > 0)
                *pColors = (COLORREF)color;
            pColors++;
        }
	}
	else if (mDataType == DATA_TYPE_COLORREF)
	{
        while (n--)
        {
			float color = *pColorerData++;
            if (color > 0)
                *pColors = (COLORREF)color;
            pColors++;
        }
	}
	else if (mbIsDataForColoring)
		AdjustColorsByDataColoring(dr);
    else if (mbTransparent)
    {
        while (n--)
        {
            if (*pColorerData++ > 0)
                *pColors &= mColor;
            pColors++;
        }
    }
    else
    {
        while (n--)
        {
            if (*pColorerData++ > 0)
                *pColors = mColor;
            pColors++;
        }
    }
	if (bDump)
		colorRaster.Dump();
	if (mpfLog)
		fprintf (mpfLog, "<CMetaData::AdjustColors> End %s type %d color %x %s\n", 
		(const char*)Name(), mDataType, mColor, mbTransparent ? "Transparent" : "");
}
bool CMetaData::ComputeStatistics(CTImage<bool> &mask, SStatistics &oStat)
{
    if (!HasPageData())
        return false;

    oStat.mn = 0;
    oStat.mSum = 0.0f;
    oStat.mAverage = 0.0f;
    oStat.mStandardDeviation = 0.0f;

    // Count selected pixels
    bool *pCheck = mask.GetData();
    int count = mask.GetNPixels();
    while (count--)
    {
        if (*pCheck++)
            oStat.mn++;
    }
    if (oStat.mn < 1)
        return false;

    float *pBuffer = new float [oStat.mn];
	memset(pBuffer, 0, oStat.mn*sizeof(float));

    bool *pMask = mask.GetData();

    // Get Values and Sum
    int n = 0;
    for (int iLine = 0; iLine < mnLinesInPage; iLine++)
    {
        for (int iCol = 0; iCol < mnCols; iCol++)
        {
            if (*pMask++)
            {
                float value = mpPageData->GetFloatValue(iLine, iCol);
                pBuffer[n++] = value;
                oStat.mSum += value;
            }
        }
    }
	if (oStat.mn > 0)
	    oStat.mAverage = oStat.mSum / oStat.mn;
	else
	    oStat.mAverage = 0;

    // Compute STD
    float sumVariance = 0.0f;
    for (int i = 0; i < oStat.mn; i++)
    {
        float diff = pBuffer[i] - oStat.mAverage;
        sumVariance += diff * diff;
    }
    delete[] pBuffer;
    //if (n > 1)
    //    oStat.mStandardDeviation = (float)sqrt(sumVariance / (n-1));
    if (n > 0)
        oStat.mStandardDeviation = (float)sqrt(sumVariance / n);
    return true;
}

unsigned int CMetaData::GetUnitSize (void)
{
	if (mDataUnitSize > 0)
		return mDataUnitSize;
    if (mpTypedData)
        return mpTypedData->GetUnitSize();
    return 0;
}
void CMetaData::SetDisplayCenter (CFloatCoordinates dataCoo)
{
	CZoomPanParams *pParams = GetZoomParams();
	if (!pParams)
	{
		if (mpWantedCenterCoo)
			*mpWantedCenterCoo = dataCoo;
		else
			mpWantedCenterCoo = new CFloatCoordinates(dataCoo);
		return;
	}

    float shiftXmm = (dataCoo.fx - mnCols / 2.0f) * mMmPerPixelWidth;
    float shiftYmm = (dataCoo.fy - mnLinesInPage / 2.0f) * mMmPerPixelHeight;

    pParams->SetShift(shiftXmm, shiftYmm);
}
bool CMetaData::TryToGetImageSizeFromName(void)
{
    CFileName fName(msName);
    CString sPrivate(fName.Private());

	if (CMyWindows::GetValueFromString(sPrivate, "_matrix", mnCols))
	{
		mnLinesInPage = mnCols;
	}
	else if (CMyWindows::GetValueFromString(sPrivate, "_mat", mnCols))
	{
		mnLinesInPage = mnCols;
	}
	else
	{
	    CMyWindows::GetValueFromString(sPrivate, "_width", mnCols);
		CMyWindows::GetValueFromString(sPrivate, "_height", mnLinesInPage);
	}
	CMyWindows::GetValueFromString(sPrivate, "_tail", mnLastColsToIgnore);
	CMyWindows::GetValueFromString(sPrivate, "_header", mPageHeaderLength);

    if (fName.IsOfMetaType("char"))
        SetTypedDataHandler(DATA_TYPE_CHAR);
    else if (fName.IsOfMetaType("bool"))
        SetTypedDataHandler(DATA_TYPE_BOOL);
    else if (fName.IsOfMetaType("float"))
        SetTypedDataHandler(DATA_TYPE_FLOAT);
    else if (fName.IsOfMetaType("double"))
        SetTypedDataHandler(DATA_TYPE_DOUBLE);
    else if (fName.IsOfMetaType("unsigned int"))
        SetTypedDataHandler(DATA_TYPE_UINT);
    else if (fName.IsOfMetaType("uint"))
        SetTypedDataHandler(DATA_TYPE_UINT);
    else if (fName.IsOfMetaType("int"))
        SetTypedDataHandler(DATA_TYPE_INT);
    else if (fName.IsOfMetaType("struct S_RGB"))
        SetTypedDataHandler(DATA_TYPE_RGB);
    else if (fName.IsOfMetaType("COLORREF"))
        SetTypedDataHandler(DATA_TYPE_COLORREF);
    else if (fName.IsOfMetaType("unsigned long"))
        SetTypedDataHandler(DATA_TYPE_COLORREF);
    else if (fName.IsOfMetaType("unsigned short"))
        SetTypedDataHandler(DATA_TYPE_USHORT);
    else if (fName.IsOfMetaType("ushort"))
        SetTypedDataHandler(DATA_TYPE_USHORT);
    else if (fName.IsOfMetaType("short"))
        SetTypedDataHandler(DATA_TYPE_SHORT);

    if (!mnCols && !mnLinesInPage)
		return false;
	return true;
}
void CMetaData::AttachToSharedVolume(CFileMapping * pMapping)
{
    if (mpfLog)
    {
        fprintf (mpfLog, "<CMultiFile::AttachToSharedVolume> %s\n", (const char*)msName);
        fflush (mpfLog);
    }

	DeleteVolume();
    mpVolume = CVolume::GetNew(pMapping, true);
    CMyImage::SSharedMemHeader *pHeader = mpVolume->GetHeader();
    if (pHeader)
    {
        mMmPerPixelPage = abs(pHeader->mSecondBedPos - pHeader->mFirstBedPos);
        if (mMmPerPixelPage > -0.001 && mMmPerPixelPage < 0.001)
        {
            static int nNotice = 0;
            if (!nNotice++)
                CMyWindows::MessBox ("Bed Position is constant", msName);
            mMmPerPixelPage = -1.0f;
        }
    }
    mbVolumeIsShared = true;
}
bool CMetaData::GetDumpName(CString &osName, const char *zExtension)
{
	CFileName fName(msName);
	CString sSavePrivateName(fName.Private());

    CFileDialog saveDlg(FALSE,// Save As
        zExtension,
        sSavePrivateName,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        NULL
        );

	CString sPath(fName.Path());
	if (sPath.IsEmpty())
	{
		CMetaData *pFirstData = gScreens.GetFirstData();
		if (pFirstData)
			sPath = pFirstData->GetPath();
	}
    saveDlg.m_ofn.lpstrInitialDir = (const char *)sPath;

	if (saveDlg.DoModal() != IDOK)
        return false;

    osName = saveDlg.m_ofn.lpstrFile;
    if (osName.IsEmpty())
        return false;
	return true;
}
void CMetaData::DumpAllPageData(const char *zUserName)
{
	CString sName(mpPageData->GetDumpName(zUserName));
	FILE * pf = MyFOpenWithErrorBox (sName, "wb", "file to Dump Slices");
	if (!pf)
		return;

	char zBuf[512];
	int nSaved = 0;
	for (int i = StartLoopI(); i >= 0; i = LoopNext())
	{
		mbPageUpdated = false;
		if (!UpdatePageData())
			break;
		if (!mpPageData->Dump(pf))
			break;
		nSaved++;
		sprintf_s(zBuf, "Saved %3d", nSaved);
		CMyWindows::PrintStatus(zBuf);
	} 
	fclose (pf);
	sprintf_s(zBuf, "%d slices saved to file <%s>", nSaved, (const char*)sName);
	CMyWindows::MessBox(zBuf, "Dump Finished");
}
void CMetaData::DumpAllPageDataCoronals(const char* zUserName)
{
	CPosition* pPos = GetPosition();
	int nOutputLines = pPos->GetNFiles();
	if (nOutputLines > 1024)
	{
		DumpAllPageDataCoronalsMultiFile(zUserName);
		return;
	}

	CString sName(mpPageData->GetDumpName(zUserName, nOutputLines));
	FILE* pf = MyFOpenWithErrorBox(sName, "wb", "file to Dump Slices");
	if (!pf)
		return;

	unsigned int lineSize = mpPageData->GetLineByteSize();
	__int64 outputPageSize = (__int64)lineSize * nOutputLines;

	CMyProgress progress(1, (short)nOutputLines);

	char zBuf[512];
	int nSaved = 0;
	bool bFailed = false;
	for (int i = StartLoopI(); i >= 0; i = LoopNext())
	{
		progress.SetPos(nSaved);

		mbPageUpdated = false;
		if (!UpdatePageData())
			break;

		__int64 offset = nSaved * lineSize;
		char* pData = (char*)mpPageData->GetDataStart();
		for (int iLine = 0; iLine < mnLinesInPage; iLine++)
		{
			_fseeki64(pf, offset, SEEK_SET);
			size_t nWritten = fwrite(pData, 1, lineSize, pf);
			if (nWritten < lineSize)
			{
				CMyWindows::MessBox("Failed to write to disk", "Dump Error");
				bFailed = true;
				break;
			}
			pData += lineSize;
			offset += outputPageSize;
		}
		if (bFailed)
			break;
		nSaved++;
		sprintf_s(zBuf, "Saved %3d", nSaved);
		CMyWindows::PrintStatus(zBuf);
	}
	fclose(pf);
	sprintf_s(zBuf, "%d slices saved to file <%s>", nSaved, (const char*)sName);
	CMyWindows::MessBox(zBuf, "Dump Finished");
}
void CMetaData::DumpAllPageDataSagittals(const char* zUserName)
{
	CPosition* pPos = GetPosition();
	int nOutputLines = pPos->GetNFiles();
	/*
	if (nOutputLines > 1024)
	{
		DumpAllPageDataSagittalsMultiFile(zUserName);
		return;
	}*/

	//CString sName(mpPageData->GetDumpName(zUserName, nOutputLines));

	//unsigned int colSize = mpPageData->GetColByteSize();
	//__int64 outputPageSize = (__int64)colSize * nOutputLines;

	CMyProgress progress(1, (short)nOutputLines);

	char zBuf[512];
	int nSaved = 0;
	//bool bFailed = false;
	CTVolume<float>  vol("Sagittals");
	vol.SetVolumeSize(nOutputLines, mnLinesInPage, 0, mnCols - 1);
	int iInputImage = 0;

	for (int i = StartLoopI(); i >= 0; i = LoopNext())
	{
		progress.SetPos(nSaved);

		mbPageUpdated = false;
		if (!UpdatePageData())
			break;

		for (int iCol = 0; iCol < mnCols; iCol++)
		{
			float* pSagittal = (float *)vol.GetPageRelativeVP(iCol);
			float *pSagittalLine = pSagittal + iInputImage * mnLinesInPage;

			for (int iLine = 0; iLine < mnLinesInPage; iLine++)
			{
				pSagittalLine[iLine] = mpPageData->GetFloatValue(iLine, iCol);
			}
		}
		nSaved++;
		iInputImage++;
		sprintf_s(zBuf, "Saved %3d", nSaved);
		CMyWindows::PrintStatus(zBuf);
	}

	/*
	FILE* pf = MyFOpenWithErrorBox(sName, "wb", "file to Dump Slices");
	if (!pf)
		return;
	fclose(pf);*/
	CString sDumpName;
	CMyWindows::PrintStatus("Saving to file...");
	if (vol.DumpVolume(zUserName, sDumpName))
	{
		sprintf_s(zBuf, "%d sagittals saved to file <%s>", nSaved, (const char*)sDumpName);
		CMyWindows::MessBox(zBuf, "Dump Sagittals Finished");
		CMyWindows::PrintStatus((const char*)sDumpName);
	}
	else
		CMyWindows::MessBox("Dump Sagittals Failed", "SW Error");
}
void CMetaData::DumpAllPageDataCoronalsMultiFile(const char *zUserName)
{
	CPosition *pPos = GetPosition();
	int nOutputLines = pPos->GetNFiles();
	CString sName(mpPageData->GetDumpName(zUserName, 512));
	CFileName fName(sName);

	int nFiles = mnLinesInPage;
	FILE **paFiles = new FILE *[nFiles];

	for (int iFile = 0; iFile < nFiles; iFile++)
	{
		CString sf = fName.GetIndexedName(iFile + 1, true/*bKeepSecondExtension*/);
		paFiles[iFile] = MyFOpenWithErrorBox (sf, "wb", "file to Dump Slices");
		if (!paFiles[iFile])
		{
			for (int i = 0; i < iFile; i++)
				fclose(paFiles[i]);
			delete paFiles;
			return;
		}
	}

	unsigned int lineSize = mpPageData->GetLineByteSize();

	CMyProgress progress(1,(short)nOutputLines);

	char zBuf[512];
	int nSaved = 0;
	bool bFailed = false;
	for (int i = StartLoopI(); i >= 0; i = LoopNext())
	{
		progress.SetPos(nSaved);

		mbPageUpdated = false;
		if (!UpdatePageData())
			break;

		char *pData = (char *)mpPageData->GetDataStart();
		for (int iLine = 0; iLine < mnLinesInPage; iLine++)
		{
			size_t nWritten = fwrite(pData,1,lineSize,paFiles[iLine]);
			if (nWritten < lineSize)
			{
				CMyWindows::MessBox("Failed to write to disk","Dump Error");
				bFailed = true;
				break;
			}
			pData += lineSize;
		}
		if (bFailed)
			break;
		nSaved++;
		sprintf_s(zBuf, "Saved %3d", nSaved);
		CMyWindows::PrintStatus(zBuf);
	} 
	for (int i = 0; i < nFiles; i++)
		fclose(paFiles[i]);
	delete paFiles;
	sprintf_s(zBuf, "%d slices saved to %d files <%s>", nSaved, nFiles, (const char*)sName);
	CMyWindows::MessBox(zBuf, "Dump Finished");
}
void CMetaData::DumpSlice(bool bAll, bool bUncompressDicom)
{
	static int count = 0;
	CMyTracer myTrace("CMetaData::DumpSlice",++count);
	if (!mpPageData)
	{
		CMyWindows::MessBox("Page data not defined", "Dump Error");
		return;
	}

	CString sUserDefinedName;
	if (!GetDumpName(sUserDefinedName, bUncompressDicom ? "" : "TImage"))
		return;

	if (bAll)
	{
		if (bUncompressDicom)
			DumpAllDicom(sUserDefinedName);
		else
			DumpAllPageData(sUserDefinedName);

		mbPageUpdated = false;
	}
	else
	{
		mpPageData->Dump(sUserDefinedName);
	}
}
void CMetaData::DumpSlicesSubSet()
{
	CPosition *pPos = GetPosition();
	int iFirst, iLast, step;
	if (!pPos->SelectSubset(iFirst, iLast, step))
		return;

	CString sUserDefinedName;
	if (!GetDumpName(sUserDefinedName, "TImage"))
		return;

	CString sName(mpPageData->GetDumpName(sUserDefinedName));
	FILE * pf = MyFOpenWithErrorBox (sName, "wb", "file to Dump Slices");
	if (!pf)
		return;

	char zBuf[512];
	int nSaved = 0;
	if (pPos->TrySetCurrent(iFirst))
	{
		bool bFinished = false;
		while (!bFinished)
		{
			mbPageUpdated = false;
			if (!UpdatePageData())
				break;
			if (!mpPageData->Dump(pf))
				break;
			nSaved++;
			sprintf_s(zBuf, "Saved %3d", nSaved);
			CMyWindows::PrintStatus(zBuf);
			if (iLast >= iFirst)
			{
				if (pPos->miCurrent >= iLast)
					break;
				if (pPos->miCurrent + step <= iLast)
				{
					if (!pPos->TrySetCurrent(pPos->miCurrent + step))
						break;
				}
				else
				{
					//if (!pPos->TrySetCurrent(iLast))
					break;
				}
			}
			else // (iLast < iFirst)
			{
				if (pPos->miCurrent <= iLast)
					break;
				if (pPos->miCurrent + step >= iLast)
				{
					if (!pPos->TrySetCurrent(pPos->miCurrent + step))
						break;
				}
				else
				{
					if (!pPos->TrySetCurrent(iLast))
						break;
				}
			}
		} 
	}
	fclose (pf);
	sprintf_s(zBuf, "%d slices saved to file <%s>", nSaved, (const char*)sName);
	CMyWindows::MessBox(zBuf, "Dump Finished");
}
void CMetaData::DumpCoronals(void)
{
	if (!mpPageData)
	{
		CMyWindows::MessBox("Page data not defined", "Dump Error");
		return;
	}

	CString sUserDefinedName;
	if (!GetDumpName(sUserDefinedName, "TImage"))
		return;

	DumpAllPageDataCoronals(sUserDefinedName);
}
void CMetaData::DumpSagittals(void)
{
	if (!mpPageData)
	{
		CMyWindows::MessBox("Page data not defined", "Dump Error");
		return;
	}

	CString sUserDefinedName;
	if (!GetDumpName(sUserDefinedName, "TImage"))
		return;

	DumpAllPageDataSagittals(sUserDefinedName);
}
void CMetaData::PrintStatistics(void)
{
	CMyRegistry reg;
	bool bAvi = false;
	if (reg.Get("avi_statistics",bAvi) && bAvi)
	{
		PrintStatisticsAvi();
		return;
	}

	CImageStatistics::ReadRegistry();
	CImageStatistics pis(Name());
	if (!pis.Opened())
		return;

	if (!UpdatePageData())
		return;
		
	if (!pis.ComputeAndPrint(mpPageData))
		return;

	pis.Display();
}
void CMetaData::PrintStatisticsAvi(void)
{
	CPrintStat ps;
	if (!ps.mLog)
		return;

	char zBuf[128];
	int nPrinted = 0;
	for (int i = StartLoopI(); i >= 0; i = LoopNext())
	{
		mbPageUpdated = false;
		if (!UpdatePageData())
			break;
		if (!ps.ComputeAndPrint(mpPageData))
			break;
		nPrinted++;
		sprintf_s(zBuf, "printed %3d", nPrinted);
		CMyWindows::PrintStatus(zBuf);
	} 
	//sprintf_s(zBuf, "%d slices saved to file <%s>", nSaved, sName);
	//CMyWindows::MessBox(zBuf, "Dump Finished");
}
void CMetaData::PrintStatisticsDiff(CMetaData * /*pOther*/)
{
	CPrintStat ps;
	if (!ps.mLog)
		return;

	char zBuf[128];
	int nPrinted = 0;
	for (int i = StartLoopI(); i >= 0; i = LoopNext())
	{
		mbPageUpdated = false;
		if (!UpdatePageData())
			break;
		if (!ps.ComputeAndPrint(mpPageData))
			break;
		nPrinted++;
		sprintf_s(zBuf, "printed %3d", nPrinted);
		CMyWindows::PrintStatus(zBuf);
	} 
}
CDRText * CMetaData::AddDataText(const char *zText, bool bDataSpecific)
{
	CDRText *pText = new CDRText(zText);
	pText->SetOnData(this);
	pText->DisableStateRestore();
	pText->SetDataSpecific(bDataSpecific);
	pText->SetAtLine((int)mDataTextList.GetSize());
	mDataTextList.AddTail(pText);
	return pText;
}
void CMetaData::SaveZoomedRaster(void)
{
	CFileName fName(Name());
	CString sDir(CMyImage::GetDumpDir() + "\\");
	CString sPrivate(fName.Private());
	int iUS = sPrivate.Find("_");
	if (iUS > 0)
		sPrivate = sPrivate.Left(iUS);
	sDir += sPrivate;
	CMyWindows::VerifyDirectory(sDir);
	CString sName = (sDir + "\\");
	sName += sPrivate;
	char zBuf[128];
	sprintf_s(zBuf, 128, "_%03d", GetRelativePosition());
	sName += zBuf;
	int nLines = mpZoomedRaster->GetNLines();
	int nCols = mpZoomedRaster->GetNCols();
	int nPixels = nLines * nCols;
	sName += CMyImage::GetDumpNameDimensions(nLines, nCols);
	if (mVerticalZoomFactor > 0.01f && mVerticalZoomFactor != 1)
	{
		sprintf_s(zBuf, 128, "_vzoom%.2f", mVerticalZoomFactor);
		sName += zBuf;
	}
	sName += ".";
	if (CDisplayRectangle::umbSaveZoomedRasterAsShort && mpZoomedRasterFloat)
	{
		sName += "short.TImage";
		float *pFloat = mpZoomedRasterFloat->GetData();
		short *pShort = new short [nPixels];
		for (int i = 0; i < nPixels; i++)
			pShort[i] = (short)pFloat[i];

		FILE *pf = MyFOpenWithErrorBox(sName, "wb", "SaveZoomedRaster");
		if (pf)
		{
			fwrite(pShort,sizeof(short),nPixels,pf);
			fclose (pf);
		}
		delete pShort;
	}
	else
	{
		if (mpZoomedRasterFloat)
			sName += "float.TImage";
		else
			sName += "RGB.TImage";

		FILE *pf = MyFOpenWithErrorBox(sName, "wb", "SaveZoomedRaster");
		mpZoomedRaster->Dump(pf);
		fclose(pf);
	}
}
void CMetaData::SetNewPosition(void)
{
	static int iPos = 0;
	char zBuf[1024];
	sprintf_s (zBuf, "NewPos_%d", ++iPos);
	/*
	SetPosition (new CPosition(zBuf, GetPosition())); */
	mMultiPosition.msType = zBuf;
}
CString CMetaData::GetTitleName (void)
{
	CString sName(z_name());
	CPosition *pSecPos = GetPosition(1);
	if (pSecPos)
	{
		char zBuf[128];
		sprintf_s(zBuf,128,"[%d/%d]", 
			pSecPos->GetCurrentRelative()+1, pSecPos->GetNFiles());
		sName += zBuf;
	}
	return sName;
}
void CMetaData::PrintFromAllDicom(void)
{
	CString sName;
	{
		CLogFile fLog("dicom",PrivateName() + ".dicom_tab.xls");
		if (!fLog)
			return;
		sName = fLog.mName;

		CList<CString*,CString*> namesList;
		CList<unsigned long,unsigned long> tagsList;
		// Read Definitions
		{
			CXMLParse parseFile(CMyWindows::GetApplicationPath() + "PrintFromAllDicom.xml");
			CXMLParseNode *pRoot = parseFile.GetRoot();
			if (pRoot)
			{
				CXMLParseNode *pDef = pRoot->GetFirstDeep("field");
				while (pDef)
				{
					CString sFieldName;
					unsigned long tag;
					if (pDef->GetValue("name", sFieldName))
					{
						CXMLParseNode *pTag = pDef->GetFirst("tag");
						if (pTag)
						{
							if (pTag->GetHexaValue(tag))
							{
								CString *ps = new CString((const char *)sFieldName);
								namesList.AddTail(ps);
								tagsList.AddTail(tag);
							}
						}
					}
					pDef = pRoot->GetNextDFS("field", &pDef);
				}
			}
		}


		// Write header
		fprintf(fLog.mpf, "#\t");
		fprintf(fLog.mpf, "Pos\t");
		fprintf(fLog.mpf, "Hour\t");
		fprintf(fLog.mpf, "Min\t");
		fprintf(fLog.mpf, "Sec\t");
		fprintf(fLog.mpf, "rel-T\t");
		if (!namesList.IsEmpty())
		{
			POSITION pos = namesList.GetHeadPosition();
			while (pos)
			{
				CString *ps = namesList.GetNext(pos);
				fprintf(fLog.mpf, "%s\t", (const char *)*ps);
			}
		}
		fprintf(fLog.mpf, "File\n");
		int maxLoopLevel = -1;

		int n = 0;
		for (int i = StartLoopI(); i >= 0; i = LoopNext(maxLoopLevel))
		{
			mbPageUpdated = false;
			UpdatePageData();
			CMyDicom *pDicom = GetDicom();
			if (!pDicom)
				break;

			n++;
			fprintf(fLog.mpf, "%d\t", n);
			double z;
			if (pDicom->GetSliceLocation(z))
				fprintf(fLog.mpf, "%.3f\t", z);

			int h,m;
			float s;
			if (pDicom->ReadTimeField(ACQ_TIME_GROUP, ACQ_TIME_NUM, h, m, s))
			{
				fprintf(fLog.mpf, "%d\t", h);
				fprintf(fLog.mpf, "%d\t", m);
				fprintf(fLog.mpf, "%.2f\t", s);
				float deltaSec = pDicom->GetRelativeAcqTimeSec();
				fprintf(fLog.mpf, "%.2f\t", deltaSec);
			}
			if (!tagsList.IsEmpty())
			{
				POSITION pos = tagsList.GetHeadPosition();
				while (pos)
				{
					unsigned long tag = tagsList.GetNext(pos);
					CString sVal = pDicom->GetTextField(tag);
					if (sVal.IsEmpty())
						sVal = "NULL";
					fprintf(fLog.mpf, "%s\t", (const char*)sVal);
				}
			}

			fprintf(fLog.mpf, "%s", (const char*)Name());
			fprintf(fLog.mpf, "\n");
		}
		while (!namesList.IsEmpty())
		{
			delete namesList.GetTail();
			namesList.RemoveTail();
		}
	}
	CMyWindows::OpenExcel(sName);
}
bool CMetaData::WantContextMenu(int &oiMenu)
{
	if (gContextMenu.SelectMenuForData(this, oiMenu))
	{
		if (oiMenu < 0)
		{
			oiMenu = 0;
			return false;
		}
		return true;
	}
	return true;
}
CTImage<float> *CMetaData::GetFloatZoomedRaster(void)
{
	return mpZoomedRasterFloat;
}
float * CMetaData::GetFloatZoomedRasterData(void)
{
	if (!mpZoomedRasterFloat)
		return NULL;
	return mpZoomedRasterFloat->GetData();
}
S_RGB * CMetaData::GetRgbZoomedRasterData(void)
{
	if (!mpZoomedRasterRgb)
		return NULL;
	return mpZoomedRasterRgb->GetData();
}
int CMetaData::GetDisplayWidth(void)
{
	if (!mbMultiPageDisplay || !mpMultiPageLoader)
		return 1;
	return mpMultiPageLoader->GetNWanted();
}
bool CMetaData::SetDisplayWidth(int n)
{
	if (n < 2) // Disable Multi Display...
	{
		if (!mbMultiPageDisplay)
			return false;
		mbMultiPageDisplay = false;
		mbPageUpdated = false;
		return true;
	}

	if (!mpMultiPageLoader)
		mpMultiPageLoader = new CMultiPageLoader(this);
	else if (mbMultiPageDisplay && (mpMultiPageLoader->GetNWanted() == n))
		return false;

	mpMultiPageLoader->SetNWanted(n);
	mbMultiPageDisplay = true;
	mbPageUpdated = false;
	return true;
}
CString CMetaData::GetAveragingText(void)
{
	if (!mbMultiPageDisplay || !mpMultiPageLoader)
		return CString(_T(""));
	return mpMultiPageLoader->GetText();
}
void CMetaData::SetDataForColoring(void)
{
	mbIsDataForColoring = true;
}
bool CMetaData::UpdateColorer(class CDisplayRectangle &dr)
{
	if (!mbIsDataForColoring)
		return false;

	PropagateLeaf(dr.GetCurrentData(), 0);

	if (!UpdatePageData())
		return false;

	if (!FillZoomedRaster(dr))
		return false;

	if (!dr.FillGrayRaster(this, dr.GetGrayRasterForColorer(), true /*bColorer*/))
		return false;

	return true;
}
CString CMetaData::GetWindowName(void)
{
	if (mbIsDataForColoring)
		return CString(_T("ColorerWindow"));
	return umsDefaultWindowName;
}
void CMetaData::ReadRegistry(CMyRegistry &myReg)
{
	myReg.Get("MetaDataDebug", umDebug);
	myReg.Get("bMultiTypeWindows", umbMultiTypeWindows);
	myReg.Get("bDisplayImageOnZeroColor", umbDisplayImageOnZeroColor);
	myReg.Get("bAllCT", umbAllCt);
	myReg.Get("bAllNotCT", umbAllNotCt);

	int colorTransparencyPC = 0;
	if (myReg.Get("ColorTransparencyPC", colorTransparencyPC))
		SetTransparencyPC(colorTransparencyPC);
	else
		myReg.Get("bColorTransparency", umbColorTransparency);
}
void CMetaData::ToggleColorsBaseOnZero(void)
{
	umbDisplayImageOnZeroColor = !umbDisplayImageOnZeroColor;
}
void CMetaData::ToggleTransparency(void)
{
	umbColorTransparency = !umbColorTransparency;
}
bool CMetaData::GuiTransparencyFraction(void)
{
	CNameGetDialog dlg;
	int transPC = 0;
	if (dlg.GetIntValue("Insert Transparency percent (0 - 100)", transPC))
	{
		SetTransparencyPC(transPC);
		return true;
	}
	return false;
}
void CMetaData::SetTransparencyPC(int percent)
{
	if (percent <= 0)
		umbColorTransparency = false;
	else
	{
		umTransparencyFraction = (float)percent / 100.0f;
		if (umTransparencyFraction > 1.0f)
			umTransparencyFraction = 1.0f;
	}
}
bool CMetaData::UserSelectDataType(void)
{
	CDataTypeDialog dlg(mDataType);
	if (dlg.DoModal())
	{
		EMyImageDataType eWantedDT = dlg.meWantedDataType;
		if (eWantedDT == mDataType)
			return false;

		SetTypedDataHandler(eWantedDT);
		CWindowParams *pParams = GetWindowParams();
		if (pParams)
		{
			pParams->mbInitialized = false;
            //pParams->SetAutoFocus(this);
		}
		OnChange();
		return true;
	}

	return false;
}
bool CMetaData::UseIntegerWindowStatus (void)
{
	if (IsRealCtImage())
		return true;
	if (mDataType == DATA_TYPE_FLOAT || mDataType == DATA_TYPE_DOUBLE)
		return false;
	return true;
}
CString CMetaData::GetDRTitle(void)
{
	if (!msUserDefinedTitle.IsEmpty())
		return msUserDefinedTitle;

	CString sTitle(Name());
	AddToTitleText(sTitle);
	return sTitle;
}
bool CMetaData::GetDataRangeText(CString &osText)
{
	if (!mpTypedData)
		return false;

	osText = mpTypedData->GetRangeAndAverageText();;
	return true;
}
void CMetaData::OpenWindowingGUI(void)
{
	CWindowParams *pWindowParams =GetWindowParams();
    if (IsCtImage())
        pWindowParams->SetRange (0, MAX_CT_VALUE);
    else
        pWindowParams->SetRange (Min(), Max());

    if (mpfLog)
        fprintf (mpfLog, "\n<OnParametersWindow> Window C%s %s W %s\n", 
			IsCtImage() ? "t" : "",
        (const char *)pWindowParams->GetCenterText(), (const char *)pWindowParams->GetWidthText());

	CWindowDlg *pNew = new CWindowDlg(this, pWindowParams);
    mpWindParamsDlg= pNew;
    mpWindParamsDlg->Create(IDD_WINDPARAMS_DIALOG, NULL);
    //mpWindParamsDlg->set_on(pWindowParams);
    mpWindParamsDlg->ShowWindow(SW_SHOW);

	//else
    //    mpWindParamsDlg->SendMessage(WM_SYSCOMMAND, SC_RESTORE, 0);
}
void CMetaData::ByeFromWindowDialog(void)
{
	mpWindParamsDlg = NULL;
}
void CMetaData::SetVerticalZoom(void)
{
	if (mMmPerPixelWidth == 0)
		mMmPerPixelWidth = 1;

	float factor = mMmPerPixelHeight / mMmPerPixelWidth;
	if (!CNameGetDialog::GetFloatValue("Insert Vertical Zoom Factor", factor, 2))
		return;
	SetVerticalZoom(factor);
}
void CMetaData::SetVerticalZoom(float factor)
{
	if (factor < 0.01f)
		return;

	CMyMath::Clip(0.1f, factor, 20.0f);
	mVerticalZoomFactor = factor;
	mMmPerPixelHeight = mMmPerPixelWidth * mVerticalZoomFactor;
	mbPageUpdated = false;
	if (mpPageData)
		mpPageData->mMmPerPixelHeight = mMmPerPixelHeight;
	gScreens.Refresh();
}
bool CMetaData::PrintDicom(void)
{
	if (!IsDicom())
		return false;
	CMyDicom *pDicom = GetDicom();
	if (!pDicom)
		return false;
	if (HasSpecialPrint())
	    pDicom->Print(NULL, this);
	else
	    pDicom->Print();

	return true;
}
void CMetaData::PrintValue2Excel(FILE *pf, int iLine, int iCol)
{
	if (mpTypedData)
	{
		CString s(mpTypedData->Value2s(mpPageData->GetDataStart(), mnCols, iLine, iCol,
			mRescaleSlope, mRescaleIntercept));
		fprintf(pf,"%s\t",(const char*)s);
	}
	else
		fprintf(pf,"=?=\t");
}
void CMetaData::PrintExcelHeader(FILE *pf)
{
	fprintf(pf,"%s\t", (const char*)Name());
}
void CMetaData::PrintCsvHeader(CCsvFile &csv)
{
	csv.PrintCell(Name());
}
void CMetaData::PrintValue2Csv(CCsvFile &csv, int iLine, int iCol)
{
	if (mpTypedData)
	{
		CString s(mpTypedData->Value2s(mpPageData->GetDataStart(), mnCols, iLine, iCol,
			mRescaleSlope, mRescaleIntercept));
		csv.PrintCell(s);
	}
	else
		csv.PrintCell("=?=");
}
bool CMetaData::ScalePageData(class CCompVector &scaled)
{
	if (!mpPageData)
		return false;

	int n = mpPageData->GetNPixels();
	scaled.VerifySize(n);
	int nLines = mpPageData->GetNLines();
	int nCols = mpPageData->GetNCols();
	float *pScaled = scaled.GetData();
	int i = 0;
	for (int iLine = 0; iLine < nLines; iLine++)
	{
		for (int iCol = 0; iCol < nCols; iCol++)
		{
			float value = mpPageData->GetFloatValue(iLine, iCol);
			pScaled[i++] = value * mRescaleSlope + mRescaleIntercept;
		}
	}
	return true;
}
bool CMetaData::SaveToDicom(const char* zDicomFileName)
{
	CMyDicom templateDicom(zDicomFileName);
	if (!templateDicom.IsDicom())
	{
		CMyWindows::MessBox(zDicomFileName, "Template is not Dicom");
		return false;
	}
	if (!templateDicom.HasVR())
	{
		CMyWindows::MessBox(zDicomFileName, "Template must be Dicom with VR");
		return false;
	}
	if (!templateDicom.HasPixels())
	{
		CMyWindows::MessBox(zDicomFileName, "Template has no pixels");
		return false;
	}

	templateDicom.IncrementSopTextField(SOP_INSTANCE_UID_GROUP, SOP_INSTANCE_UID_NUM, 1);
	templateDicom.IncrementSopTextField(SERIES_INSTANCE_UID_GROUP, SERIES_INSTANCE_UID_NUM, 1);

	if (templateDicom.HasTagRef(IMAGE_ROWS_GROUP, IMAGE_ROWS_NUM))
	{
		unsigned short nRows = (unsigned short)mnLinesInPage;
		templateDicom.SetTagData(IMAGE_ROWS_GROUP, IMAGE_ROWS_NUM, &nRows, sizeof(nRows));
	}
	if (templateDicom.HasTagRef(IMAGE_COLS_GROUP, IMAGE_COLS_NUM))
	{
		unsigned short nCols = (unsigned short)mnCols;
		templateDicom.SetTagData(IMAGE_COLS_GROUP, IMAGE_COLS_NUM, &nCols, sizeof(nCols));
	}

	CString sfName(CMyDicomWriter::GetNameForDicomExport(msName, "RDicom"));
	CMyDicomWriter dicomWriter(&templateDicom, sfName, "<CMetaData::SaveToDicom>");
	short* pRaster = (short *)mpPageData->GetDataStart();
	if (!dicomWriter.SaveWithNewRasterInsteadOfCompressed(pRaster, mnCols * mnLinesInPage * sizeof(short)))
		return false;

	CMyWindows::MessBox(sfName, "Raster saved as Dicom");
	return true;
}
bool CMetaData::SaveToMinimalDicom()
{
	// TEMP Debug option for static function "CMyDicomWriter::SaveMinimalDicom" - 
	// BUT the static function only works on certain files - 
	// And this function works on any raster
	bool bDebug = false;
	bool bOK = false;
	CString sResult;
	if (bDebug)
	{
		bOK = CMyDicomWriter::SaveMinimalDicom(msName, &sResult);
	}
	else
	{
		sResult = CMyDicomWriter::GetNameForDicomExport(msName, "MinDicom");
		short* pRaster = (short*)mpPageData->GetDataStart();
		CMyDicomWriter dicomWriter(NULL, sResult, "<CMetaData::SaveToMinDicom>");
		if (!dicomWriter.SaveMinimalDicom((unsigned short)mnLinesInPage, (unsigned short)mnCols, pRaster))
			return false;
		bOK = true;
	}

	if (bOK)
		CMyWindows::MessBox(sResult, "Raster saved as minimal Dicom");
	return bOK;
}
void CMetaData::ChangeEndian()
{
	mbChangeEndian = !mbChangeEndian;
	ForceUpdate();
}
void CMetaData::ChangeEndianInPage()
{
	if (mDataType == DATA_TYPE_USHORT || mDataType == DATA_TYPE_SHORT)
	{
		unsigned short* pShort = (unsigned short*)mpPageData->GetDataStart();
		int nPixels = mnLinesInPage * mnCols;
		for (int i = 0; i < nPixels; i++)
		{
			unsigned char* pChar = (unsigned char*)pShort;
			unsigned char ch0 = pChar[0];
			pChar[0] = pChar[1];
			pChar[1] = ch0;
			pShort++;
		}
	}
}
void CMetaData::DeleteVolume()
{
	if (mpVolume)
	{
		delete mpVolume;
		mpVolume = NULL;
	}
}
void CMetaData::SetSyncPosition()
{
	mMultiPosition.SetSyncPosition();
}
void CMetaData::ExportHistogram()
{
	CMyWindows::CreateThread(StaticExportHistogram, this);
}
DWORD WINAPI CMetaData::StaticExportHistogram(void* p)
{
	CMetaData* pData = (CMetaData*)p;
	pData->DoExportHistogram();
	return 0;
}
void CMetaData::DoExportHistogram()
{
	LoadVolume();

	//if (mpVolume)
	//	CMyWindows::MessBox("Volume Loaded", "Note");

	unsigned short* pVol = (unsigned short*)mpVolume->GetPageRelativeVP(0);
	int nInImage = mnCols * mnLinesInPage;
	int nPages = GetNPages();
	int n = nInImage * nPages;

	//unsigned short minVal = 0;
	unsigned short maxVal = pVol[0];
	for (int i = 0; i < n; i++)
	{
		if (pVol[i] > maxVal)
			maxVal = pVol[i];
	}

	//char zBuf[128];
	//sprintf_s(zBuf, sizeof(zBuf), "Data range %d - %d", minVal, maxVal);
	//CMyWindows::MessBox(zBuf, "Note");

	CString sPath(CFileName::GetPath(msName));
	CString sDir(CFileName::GetPrivate(sPath));
	CString sName(sDir + " full detail");

	// Full detail histogram
	CHistogram hist((const char*)sName, maxVal + 2);
	hist.Compute((const short*)pVol, n);
	CString sfName = hist.PrintCSV();

	// Unite every 10 values
	CString sName10(sDir + " div by 10");
	CHistogram hist10((const char*)sName10, (maxVal + 9) / 10 + 2);
	hist10.ComputeDiv((const short*)pVol, n, 10);
	hist10.PrintCSV();

	CMyWindows::MessBox(sfName, "Histogram Ready");
}
void CMetaData::ToggleCTMode()
{ 
	umbCtIsGlobal = false;
	mbCtImage = !mbCtImage;
	OnChange();
}
