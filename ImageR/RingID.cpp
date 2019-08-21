#include "StdAfx.h"
#include "RingID.h"
#include "DisplayRectangle.h"
#include "MetaData.h"
#include "Screens.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\ImageRLib\RingLevelInfo.h"

CList<CRingID::SStatData *,CRingID::SStatData *> CRingID::mStatList;
CString CRingID::umsMissingDataMessage(_T(""));
int CRingID::umMissingDataStartSizeMm(120);

CRingID::CRingID(CDisplayRectangle *pDispRect, CDataCoordinates &reconCenter, bool bCenterKnown)
: CGEllipse(pDispRect, "ScanCenter", 0x00ff00, bCenterKnown ? 30 : umMissingDataStartSizeMm)
, mpRli(NULL)
, miPos(-1)
, mbCenterKnown(bCenterKnown)
{
    InitEllipse(reconCenter, bCenterKnown ? 30.0f : (float)umMissingDataStartSizeMm);
    SetDisplayOnAll(true);
    SetCircle();
	if (bCenterKnown)
	    SetFixedCenter();
	SetTextPoint(1);
	ProtectFromDataEclipse();

	CMetaData *pData = pDispRect->GetCurrentData();
	if (pData)
		mpRli = pData->GetRli();
	if (mpRli)
	{
		AddDependant(pData);
		OnChange();
	}
}
CRingID::~CRingID(void)
{
	EmptyStatList();
}
bool CRingID::GetScanCenterInfo(CMetaData *pData, CString &sValue, bool bCustomerSupport)
{
	CMyDicom *pDicom = pData->GetDicom();
	if (!pDicom)
		return false;

	float mmPerPixel = pData->MmPerPixelWidth();
	if (mmPerPixel < 0.001)
		mmPerPixel = 1.0f; // Just to avoid crush on garbage

	float diameter = mapDataPoints[2]->fx - mapDataPoints[0]->fx;
	float radiusMm = (diameter / 2) * mmPerPixel;
	if (bCustomerSupport)
		sValue = "";
	else
	{
		char zBuf[256];
		sprintf_s(zBuf,256,"R=%.2f ", radiusMm);
		sValue = zBuf;
	}
	CString sText;
	return true;
}
bool CRingID::GetStatisticsString(CMetaData *pData, CString &sValue, class CGraphicElementOnDR *)
{
	return GetScanCenterInfo(pData, sValue);
}
void CRingID::OnChange (void)
{
	UpdateText();
    CGEllipse::OnChange();
}
void CRingID::EmptyStatList(void)
{
	while (!mStatList.IsEmpty())
	{
		delete mStatList.GetTail();
		mStatList.RemoveTail();
	}
}
bool CRingID::UpdateText(void)
{
	if (mpRli && mpRli->IsValid())
	{
		int radius = (int)((mapDataPoints[2]->fx - mapDataPoints[0]->fx) / 2.0f +0.5f);
		CString sText = mpRli->GetTextInRange(radius - 1, radius + 1);
		SetText(sText);

		// Update special data

		return true;
	}
	return false;
}
bool CRingID::GetDataDependantText(CMetaData *pData, CString &s)
{
	CRingLevelInfo *pRli = pData->GetRli();
	if (!pRli)
	{
		return GetScanCenterInfo(pData, s, true/*bCustomerSupport*/);
	}
	if (!pRli->IsValid())
		return false;

	int radius = (int)((mapDataPoints[2]->fx - mapDataPoints[0]->fx) / 2.0f +0.5f);
	s = pRli->GetTextInRange(radius - 1, radius + 1);
	s += GetDeltaStat(pData, pRli, radius);
	return true;
}
CString CRingID::GetDeltaStat(CMetaData *pData, CRingLevelInfo * pRli, int radius)
{
	static unsigned long iScreenUpdate = 0;
	if (gScreens.miUpdate != iScreenUpdate)
	{
		EmptyStatList();
		iScreenUpdate = gScreens.miUpdate;
	}
    CTImage<bool> mask("Roi");
    SetDataMask(mask);

    CMetaData::SStatistics stat;
    if (!pData->ComputeStatistics(mask, stat))
		return CString("");
	int area = stat.mn;
	float sum = stat.mSum;
	int ctBias = (int)pData->GetCtBias();

	POSITION pos = mStatList.GetHeadPosition();
	while (pos)
	{
		SStatData *pStat = mStatList.GetNext(pos);
		if (pStat->mpRli == pRli)
		{
			int deltaArea = area - pStat->mArea;
			float deltaSum = sum - pStat->mSum;
			if (deltaArea < 0)
			{
				deltaArea = - deltaArea;
				deltaSum = - deltaSum;
			}
			if (deltaArea > 0)
			{
				char zBuf[128];
				int avg = (int)(deltaSum/deltaArea) + ctBias;
				sprintf_s(zBuf, sizeof(zBuf), " Delta %d avg %d", deltaArea, avg);
				return CString(zBuf);
			}
		}
	}

	mStatList.AddTail(new SStatData(pRli,radius, area, sum));
	return CString("");
}
CRingID *CRingID::CreateRingID(class CDisplayRectangle *pDispRect, CMetaData *pData)
{
	if (!pData)
		return NULL;

	CMyDicom *pDicom = pData->GetDicom();
    if (!pDicom)
	{
		CMyWindows::MessBox("Image is not Dicom", "Missing Data Note");
        return NULL;
	}
	bool bCenterKnown = false;
	float x, y;

		if (umsMissingDataMessage.IsEmpty())
		{
			umsMissingDataMessage = "The DICOM image doesn't contain all the required information.\n";
			umsMissingDataMessage += "The position of scan's rotation center can't be identified.\n";
			umsMissingDataMessage += "You can try to locate the ROI manually in the center of the seen rings.";
		}
		CMyWindows::MessBoxThread(umsMissingDataMessage, "Missing Data Note");
		x = (float)pData->GetNCols() / 2.0f;
		y = (float)pData->GetNLinesInPage() / 2.0f;
	CDataCoordinates reconCenter(x, y);
    CRingID *pRingID = new CRingID(pDispRect, reconCenter, bCenterKnown);
	return pRingID;
}
void CRingID::SetMissingDataText(class CXMLParseNode *pNode)
{
	int iLine = 0;
	CXMLParseNode *pLineXml = pNode->GetFirst("line");
	while (pLineXml)
	{
		CString sLine;
		pLineXml->GetValue(sLine);
		if (!iLine++)
			umsMissingDataMessage = sLine;
		else
		{
			umsMissingDataMessage += "\n";
			umsMissingDataMessage += sLine;
		}

		pLineXml = pNode->GetNextDFS("line", &pLineXml, true/*bShalow*/);
	}
}
bool CRingID::FixCenter(void)
{
	if (mbFixedCenter)
		mbFixedCenter = false;
	else
		SetFixedCenter();
	return true;
}
