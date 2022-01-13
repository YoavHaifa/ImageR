#include "StdAfx.h"
#include "MipVolume.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\DynamicTimer.h"
#include "..\..\ImageRLib\LinesTable.h"
#include "..\..\ImageRLib\Mipper.h"
#include "..\..\ImageRLib\TVolume.h"
#include "..\..\ImageRLib\DataVolume.h"
#include "Screens.h"
#include "VolTissue.h"
#include "GCursor.h"
#include "GContour.h"
#include "GEllipse.h"
#include "Screens.h"
#include "DisplayRectangle.h"

int CMipVolume::umnMipVois = 0;

CMipVolume::CMipVolume(CDisplayRectangle* pSrcDR)
: CMetaData("Mip")
, CVolumetric ("Mip", pSrcDR)
, mnImageLines(0)
, mnImagePixels(0)
, mpVOIMask(NULL)
, mpVOILinesTable(NULL)
{
    InitOnConstruction();
}
CMipVolume::CMipVolume(CXMLParseNode *pXmlNode)
: CMetaData(pXmlNode->GetFirstDeepString("name", "Mip"))
, CVolumetric (pXmlNode->GetFirstDeepString("name", "Mip"), pXmlNode)
, mnImageLines(0)
, mnImagePixels(0)
, mpVOIMask(NULL)
, mpVOILinesTable(NULL)
{
    InitOnConstruction();
}
CMipVolume::~CMipVolume(void)
{
    if (mpVOILinesTable)
        delete mpVOILinesTable;
}
void CMipVolume::InitOnConstruction(void)
{
    if (!umpfLog)
        umpfLog = CMyWindows::FOpenLogFile("Mip");
    gScreens.mVolumeRefreshInterval = 10;

    SetTypedDataHandler(DATA_TYPE_USHORT);
    SetNewPosition("mip", 1,1);
    CMetaData* pVolumeData = GetSourceData();
    if (pVolumeData)
    {
        mnImageLines = pVolumeData->GetNLinesInPage();
        mnImagePixels = pVolumeData->GetNCols();
    }
}
bool CMipVolume::ShouldUpdate()
{
    int idRemove = FindTissueToRemove();
    if (idRemove != mRemovedTissuesId)
        return true;
    return CVolumetric::ShouldUpdate();
}
int CMipVolume::FindTissueToRemove(void)
{
	mTissuesToRemove.RemoveAll();

	int i = 2;
	int id = 1;
    POSITION pos = mTissuesList.GetHeadPosition();
    while (pos)
    {
        CVolTissue *pTissue = mTissuesList.GetNext(pos);
        if (pTissue->IsCommand("Remove"))
		{
			mTissuesToRemove.AddTail(pTissue);
			id *= i;
		}
		i++;
    }
	return id;
}
bool CMipVolume::DoUpdatePageData()
{
	static CEventTracer EvTracer ("CMip::DoUpdatePageData");
	CDynamicTimer DynTimer (&EvTracer);

    if (!umpfLog)
        umpfLog = CMyWindows::FOpenLogFile (CNamedObject::GetUniqueName(msName));

    CMetaData* pVolumeData = GetSourceData();
    CVolume *pVolume = GetSourceVolume();
    if (!pVolume)
        return false;
    if (VerifyMmPerPixelPage())
    {
        if (umpRefToImage)
            umpRefToImage->SetOn(mpPageData);
    }

    mRemovedTissuesId = FindTissueToRemove();

    if (mpVOIMask)
    {
        if (!mpVOILinesTable)
            mpVOILinesTable = new CLinesTable;
        mpVOILinesTable->Init(*mpVOIMask);
    }

	CMipper mipper(mnImageLines, mnImagePixels);
    mpPageData->Zero();
    unsigned short *pMipLine = (unsigned short *)mpPageData->GetDataStart();
   

    int nLinesToCompute = pVolume->mnLoaded;
    int iFirstImage = pVolume->First();
    if (umpfLog)
    {
        fprintf (umpfLog, "<CMip::DoUpdatePageData> iFirstImage %d nLinesToCompute %d\n",
            iFirstImage, nLinesToCompute);
        //fprintf (umpfLog, "<CMip::DoUpdatePageData> miCurrent %d pageOffset %d\n", 
        //    mpPosition->miCurrent, pageOffset);
        fprintf (umpfLog, "pVolHeader %p\n", pVolume->GetHeader());
		if (mTissuesToRemove.GetSize() > 0)
			fprintf (umpfLog, "Remove: %s\n", (const char*)mTissuesToRemove.GetHead()->msName);
    }
    for (int iMipLine = 0; iMipLine < nLinesToCompute; iMipLine++)
    {
        int iImage = iFirstImage + iMipLine;
        unsigned short *pVolPage = (unsigned short *) pVolume->GetPageVP (iImage);
        if (mTissuesToRemove.GetSize() > 0)
        {
			bool *pPrevMask = NULL;
			POSITION pos = mTissuesToRemove.GetHeadPosition();
			while (pos)
			{
				CVolTissue *pTissueToRemove = mTissuesToRemove.GetNext(pos);
	            bool *pMask = pTissueToRemove->GetImageMask(iImage);
				if (pos)
					pPrevMask = pMask;
				else if (pPrevMask)
			        ProjectImageRemove2Tissues(pVolPage, pPrevMask, pMask, pMipLine);
				else
			        ProjectImageRemoveTissue(pVolPage, pMask, pMipLine);
			}
        }
        else
        {
            if (mpVOIMask)
                ProjectImageByMask(pVolPage, pMipLine);
			else if (mpVoi)
			{
				if (!mpVoiBounds)
				{
					mpVoiBounds = new CDataRoiLineBounds;
					mpVoiBounds->AddCheck(mpVoi);
				}
				mpVoiBounds->Init(pVolumeData->GetNLinesInPage(), pVolumeData->GetNCols(),*mpVoi);
                mipper.ProjectImageInRoi(pVolPage, pMipLine, mpVoiBounds);
				CWindowParams *pWindowParams = GetWindowParams();
				if (pWindowParams)
					pWindowParams->SetAutoFocus(this);
			}
			else
                mipper.ProjectImage(pVolPage, pMipLine);
        }
        pMipLine += mnCols;
    }
    
    OnChange();
    if (umpfLog)
    {
        fprintf (umpfLog, "----------------\n\n");
        fflush (umpfLog);
    }
    return true;
}
void CMipVolume::AddTissue(class CVolTissue * pTissue)
{
    mTissuesList.AddTail(pTissue);
    //AddCheck(pTissue);
}
void CMipVolume::ProjectImageByMask(unsigned short * pImage, unsigned short * pMipLine)
{
    mpVOILinesTable->StartLoop();
    int nPixels, iPixel, iLine;
    while (mpVOILinesTable->GetNextValidLine(nPixels, &iPixel, &iLine))
    {
        unsigned short * pSeg = pImage + iLine * mnImagePixels + iPixel;
        bool *bMask = mpVOILinesTable->GetCurLineSeg(*mpVOIMask);

        while (nPixels--)
        {
            if (*bMask++)
            {
                unsigned short value = *pSeg;
                if (value > pMipLine[iPixel])
                    pMipLine[iPixel] = value;
            }
            pSeg++;
            iPixel++;
        }
    }
}
void CMipVolume::ProjectImageRemoveTissue(unsigned short * pImage, bool *pTissue, unsigned short * pMipLine)
{
    for (int iImLine = 0; iImLine < mnImageLines; iImLine++)
    {
        for (int iPix = 0; iPix < mnImagePixels; iPix++)
        {
            bool bTissue = *pTissue++;
            if (!bTissue)
            {
                unsigned short value = *pImage;
                if (value > pMipLine[iPix])
                    pMipLine[iPix] = value;
            }
            pImage++;
        }
    }
}
void CMipVolume::ProjectImageRemove2Tissues(unsigned short * pImage, bool *pTissue1, bool *pTissue2, unsigned short * pMipLine)
{
    for (int iImLine = 0; iImLine < mnImageLines; iImLine++)
    {
        for (int iPix = 0; iPix < mnImagePixels; iPix++)
        {
            bool bTissue1 = *pTissue1++;
            if (!bTissue1)
            {
				bool bTissue2 = *pTissue2;
				if (!bTissue2)
				{
	                unsigned short value = *pImage;
		            if (value > pMipLine[iPix])
			            pMipLine[iPix] = value;
				}
            }
			pTissue2++;
            pImage++;
        }
    }
}
void CMipVolume::SetTopoVoi(void)
{
	CDisplayRectangle *pImageDR = NULL;

	CMetaData *pRefData = NULL; // (CMetaData *)GetPosition()->GetMultiFileData();
	if (pRefData)
		pImageDR = pRefData->GetDR();

	if (!mpTopoVoi && pImageDR)
	{
		mpTopoVoi = new CDataVolume(pRefData, "MipTopoVoi");
		mpTopoVoi->Init(mpRefToMip);
		mpTopoContour = new CGContour (pImageDR, mpTopoVoi, 0xffc080);
		mpTopoContour->ProtectFromDataEclipse();
		pImageDR->AddGraphicElement (mpTopoContour);

		AddCheck(mpTopoVoi);
		mpTopoVoi->AddDependant(this);
		gScreens.Refresh();
	}
}
void CMipVolume::SetVoi(void)
{
	CDisplayRectangle *pImageDR = NULL;
	CMetaData *pRefData = NULL; // (CMetaData *)GetPosition()->GetMultiFileData();
	if (pRefData)
		pImageDR = pRefData->GetDR();

	if (!mpVoi && pImageDR)
	{
		umnMipVois++;
		mpVoi = new CGEllipse(pImageDR, "MipVoi", 
			(umnMipVois & 1) ? 0xc000ff : 0x00c0ff, 128);
		mpVoi->ProtectFromDataEclipse();
		pImageDR->AddGraphicElement (mpVoi);

		AddCheck(mpVoi);
		mpVoi->AddDependant(this);
		gScreens.Refresh();
	}
}
void CMipVolume::Bye(CDependant *pObject)
{
	if (pObject == mpVoi)
	{
		RemoveCheck(mpVoi);
		mpVoi = NULL;
		if (mpVoiBounds)
		{
			delete mpVoiBounds;
			mpVoiBounds = NULL;
		}
		mbPageUpdated = false;
		CWindowParams *pParams = GetWindowParams();
		if (pParams)
			pParams->mbInitialized = false;
		umnMipVois--;
	}
}
