#include "StdAfx.h"
#include "Mip.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\ImageRLib\TVolume.h"
#include "..\..\ImageRLib\DataVolume.h"
#include "GCursor.h"
#include "GContour.h"
#include "DisplayRectangle.h"
#include "Screens.h"

FILE *CMip::umpfLog = NULL;
CGCursor *CMip::umpRefToImage = NULL;

CMip::CMip(void)
: CMetaData("Mip")
, mpRefToMip(NULL)
, mpVoi(NULL)
, mpVoiBounds(NULL)
, mpTopoVoi(NULL)
, mpTopoContour(NULL)
{
}
CMip::~CMip(void)
{
	if (mpRefToMip)
		delete mpRefToMip;
	if (mpTopoVoi)
		delete mpTopoVoi;
	if (mpTopoContour)
		delete mpTopoContour;
}
void CMip::OnAssignDR (void)
{
    if (!umpRefToImage)
    {
        umpRefToImage = new CGCursor(this, mpDR, "MipRef2Im", 0x6080ff);
        umpRefToImage->ProtectFromDataEclipse();
		umpRefToImage->GetLButtonUp(this);
		//umpRefToImage->SetSelectable(false);
    }
    mpDR->AddGraphicElement (umpRefToImage);
    CDataCoordinates * pDataCoo = umpRefToImage->GetFirstDataCoo();

	pDataCoo->fx = GetNCols() / 2.0f;
    pDataCoo->fy = GetNLinesInPage() / 2.0f;

	CMetaData *pRefData = NULL; //(CMetaData *)GetPosition()->GetMultiFileData();
	if (pRefData)
	{
		CDisplayRectangle *pImageDR = pRefData->GetDR();
		pImageDR->AddRef(this);
		if (pRefData->VolumeIsShared())
			GetDR()->mbDisableStateRestore = true;
		if (!mpRefToMip)
		{
			mpRefToMip = new CGCursor(pRefData, pImageDR, "ImRef2Mip", 0x8060ff);
			mpRefToMip->ProtectFromDataEclipse();
			mpRefToMip->SetSelectable(false);
			//mpRefToMip->mbDebug = true;
		}
		pImageDR->AddGraphicElement (mpRefToMip);
	}
	UpdateRefToImagePos();
    if (umpfLog && pRefData)
    {
        fprintf (umpfLog, "<OnAssignDR> this %p name %s mpVolumeData %s\n", this, (const char*)msName, (const char*)pRefData->msName);
        fprintf (umpfLog, "<OnAssignDR> pDataCoo %p\n", pDataCoo);
        fprintf (umpfLog, "<OnAssignDR> mbDisableStateRestore %d\n", (int)mpDR->mbDisableStateRestore);
        fprintf (umpfLog, "<OnAssignDR> Volume Data Class %s\n", (const char*)pRefData->GetClass());
		if (pRefData->GetVolume())
			fprintf (umpfLog, "<OnAssignDR> Volume Class %s\n", (const char*)pRefData->GetVolume()->GetClass());
        fprintf (umpfLog, "<OnAssignDR> umpRefToImage %p\n", umpRefToImage);
        fprintf (umpfLog, "<OnAssignDR> umpRefToImage name %s\n", (const char*)umpRefToImage->msName);
        fprintf (umpfLog, "\n");
        fflush (umpfLog);
    }
}
bool CMip::VerifyPointer(void * p, const char *zText)
{
	if (p < (void *)0x1000)
	{
		static int nErr = 0;
		if (!nErr++)
		{
			char zBuf[256];
			sprintf_s(zBuf, 256, "Bad Pointer in <%s>", zText);
			CMyWindows::MessBox (zBuf , "SW Error");
		}
		return false;
	}
	return true;
}
bool CMip::OnMousePos(CDataCoordinates &dataCoo)
{
	bool bChanged = false;
	if (umpRefToImage)
	{
		CDataCoordinates * pDataCoo = umpRefToImage->GetFirstDataCoo();
		if (!pDataCoo || !VerifyPointer(pDataCoo, "CMip::OnMousePosOnRef"))
			return false;
		if (pDataCoo->fx != dataCoo.fx)
		{
			pDataCoo->fx = dataCoo.fx;
			bChanged = true;
			UpdateImageRefToMip();
		}
	}
	return bChanged;
}
bool CMip::OnMousePosOnRef(CDataCoordinates &dataCoo)
{
    if (!umpRefToImage)
        return false;

    CDataCoordinates * pDataCoo = umpRefToImage->GetFirstDataCoo();
    if (!pDataCoo || !VerifyPointer(pDataCoo, "CMip::OnMousePosOnRef"))
        return false;
    pDataCoo->fx = dataCoo.fx;
	UpdateImageRefToMip();
    return true;
}
bool CMip::OnMousePosNotOnRef(void)
{
    if (!umpRefToImage)
        return false;
    CDataCoordinates * pDataCoo = umpRefToImage->GetFirstDataCoo();
    if (!pDataCoo || !VerifyPointer(pDataCoo, "CMip::OnMousePosNotOnRef"))
        return false;
        
    pDataCoo->fx = mnCols / 2.0f;
	UpdateImageRefToMip();
    return true;
}
void CMip::OnPositionChange(void)
{
	if (!umpRefToImage)
		return;
    UpdateRefToImagePos();
}
void CMip::UpdateRefToImagePos()
{
	if (!umpRefToImage || !HasPosition())
		return;

    CDataCoordinates * pDataCoo = umpRefToImage->GetFirstDataCoo();
    if (!pDataCoo || !VerifyPointer(pDataCoo, "CMip::UpdateRefToImagePos"))
        return;

	CPosition *pPos = GetPosition();
    int first = pPos->miFirst;
	int step = max(1, pPos->mStep);

    pDataCoo->fy = (float)(pPos->miCurrent - first) / step;

	UpdateImageRefToMip();
}
bool CMip::OnLButtonUp(class CGraphicElement *pGE)
{
	if (pGE != umpRefToImage)
		return false;
		
    CDataCoordinates * pDataCoo = umpRefToImage->GetFirstDataCoo();
    if (!pDataCoo || !VerifyPointer(pDataCoo, "CMip::OnLButtonUp"))
        return false;

	CPosition *pPos = GetPosition();
    int first = pPos->miFirst;
	int step = max(1, pPos->mStep);

	int newPosRel = (int)(pDataCoo->fy + 0.5f);
	int newPos;
	if (newPosRel < 0)
		newPos = first;
	else
		newPos = first + newPosRel * step;
	if (newPos > pPos->miLast)
		newPos = pPos->miLast;

	return pPos->SetCurrent(newPos);
}
bool CMip::UpdateImageRefToMip(void)
{
	if (!mpRefToMip || !umpRefToImage || !mpPageData)
		return false;
    CDataCoordinates * pMipDataCoo = umpRefToImage->GetFirstDataCoo();
    CDataCoordinates * pImageDataCoo = mpRefToMip->GetFirstDataCoo();
	pImageDataCoo->fx = pMipDataCoo->fx;

	CMetaData *pRefData = NULL; // (CMetaData *)GetPosition()->GetMultiFileData();
	if (!pRefData)
		return false;

	int mipX = (int)(pMipDataCoo->fx + 0.5f);
	int mipY = (int)(pMipDataCoo->fy + 0.5f);
	short mipValue = (short)mpPageData->GetFloatValue(mipY, mipX);
	CMyImage *pImagePage = pRefData->PageData();
	if (mpVoiBounds)
		pImageDataCoo->fy = (float)pImagePage->GetYOfValueAtX(mipX, mipValue, mpVoiBounds);
	else
		pImageDataCoo->fy = (float)pImagePage->GetYOfValueAtX(mipX, mipValue);
	//int yOfMax = pImagePage->GetYOfMaxAtX(mipX);

	char zBuf[128];
	sprintf_s(zBuf, 128, "[X%d Y%d] mip %d imY %d", 
		mipX, mipY, mipValue, (int)pImageDataCoo->fy);
	CMyWindows::PrintStatus(zBuf);
	return true;
}
void CMip::SelectSlice(void)
{
    CDataCoordinates dataCoo (mpDR->GetSelectedDataCoo());

	CPosition *pPos = GetPosition();
	int step = max(1, pPos->mStep);

    pPos->ClipAndSetCurrent((int)(dataCoo.fy * step + 0.5) + pPos->miFirst);
    gScreens.Refresh();
}
bool CMip::OnRefDataUpdate(void)
{
	return UpdateImageRefToMip();
}
