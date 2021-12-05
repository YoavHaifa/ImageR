#include "StdAfx.h"
#include "Oblique.h"

#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"

#include "..\..\ImageRLib\TVolume.h"

#include "ObliquePosition.h"
#include "ObliqueRef.h"
#include "Screens.h"
#include "MultiFile.h"
#include "DisplayRectangle.h"

COblique::COblique(CMultiFile *pData, CObliquePosition *pOblPos)
: CMetaData("Oblique")
, CVolumetric ("Oblique", pData)
, mpOblPos(pOblPos)
{
    InitOnConstruction();
}
COblique::COblique(CXMLParseNode *pXmlNode)
: CMetaData(pXmlNode->GetFirstDeepString("name", "Oblique"))
, CVolumetric (pXmlNode->GetFirstDeepString("name", "Oblique"), pXmlNode)
, mpOblPos(NULL)
{
    CXMLParseNode *pXmlPositionNode = pXmlNode->GetFirstDeep("position");
    mpOblPos = dynamic_cast<CObliquePosition *>(CNamedObject::GetParsed(pXmlPositionNode));
    InitOnConstruction(pXmlPositionNode);
}
COblique::~COblique(void)
{
	if (mpOblRefOnIm)
	{
		CDisplayRectangle* pDataDR = mpVolumeData->GetDR();
		pDataDR->RemoveGraphicElement(mpOblRefOnIm);
	}
}
COblique::EOblType COblique::GetType (const char *zType)
{
    if (_stricmp (zType, "Coronal") == 0)
        return OBL_CORONAL;
    if (_stricmp (zType, "Sagittal") == 0)
        return OBL_SAGITTAL;
    return OBL_CORONAL;
}
CString COblique::GetCurrentImageName()
{
	EOblType eType = GetPos().GetType();
	char zBuf[128];
	switch (eType)
	{
	case OBL_CORONAL:
		sprintf_s(zBuf, sizeof(zBuf), "Coronal line %d", mpOblPos->miCurrent);
		return CString(zBuf);
	case OBL_SAGITTAL:
		sprintf_s(zBuf, sizeof(zBuf), "Sagittal col %d", mpOblPos->miCurrent);
		return CString(zBuf);
	default:
		break;
	}
	return CString("Oblique");
}
CString COblique::GetTypeName (EOblType eType)
{
	//char zBuf[128];
    switch (eType)
    {
    case OBL_CORONAL:
		/*if (mpOblPos)
		{
			sprintf_s(zBuf, sizeof(zBuf), "Coronal line %d", mpOblPos->miCurrent);
			return CString(zBuf);
		} */
        return CString("Coronal");
    case OBL_SAGITTAL:
		/* if (mpOblPos)
		{
			sprintf_s(zBuf, sizeof(zBuf), "Sagittal col %d", mpOblPos->miCurrent);
			return CString(zBuf);
		} */
        return CString("Sagittal");
	default:
        break;
    }
    return CString("Oblique");
}
CString COblique::GetZoomName(void)
{
    EOblType eType = GetPos().GetType();
    return GetTypeName(eType) + "Zoom";
}

void COblique::InitOnConstruction(CXMLParseNode *pXmlPositionNode)
{
	if (mDataType)
	    SetTypedDataHandler(mDataType);
	else
	    SetTypedDataHandler(DATA_TYPE_USHORT);

    if (!HasPosition())
    {
        if (mpOblPos)
            SetPosition(mpOblPos);
        else
        {
            mpOblPos = new CObliquePosition(pXmlPositionNode);
            SetPosition(mpOblPos);
            SetLeafRange(0, mpVolumeData->GetNLinesInPage() - 1);
            if (mpOblPos->GetType() == OBL_UNDEF)
            {
                mpOblPos->miCurrent = mpOblPos->miLast / 2;
                SetCoronal();
            }
        }
    }
	mMultiPosition.msType = "Oblique";
}

bool COblique::DoUpdatePageData()
{
    if (!mpfLog)
        mpfLog = CMyWindows::FOpenLogFile (CNamedObject::GetUniqueName(msName));

    CVolume *pVolume = mpVolumeData->GetVolume();
    if (!pVolume)
        return false;
    VerifyMmPerPixelPage();

    unsigned char *pPage = (unsigned char *)mpPageData->GetDataStart();
    unsigned int nBytesInLine = mnCols * mDataUnitSize;
    unsigned int pageOffset = 0;

	int iCurrentPos = GetPosition()->miCurrent;
    EOblType eType = GetPos().GetType();
    switch (eType)
    {
    case OBL_CORONAL:
        pageOffset = iCurrentPos * nBytesInLine;
        break;
    case OBL_SAGITTAL:
        pageOffset = iCurrentPos * mDataUnitSize;
        break;
    }
    msName = GetCurrentImageName();
    AddDualModeToName();

    int nLinesToCompute = pVolume->mnLoaded;
    int iFirstImage = mpVolumeData->GetVolume()->First();
    int iLastImage = mpVolumeData->GetVolume()->Last();
    /*if (mpfLog)
    {
        fprintf (mpfLog, "<COblique::DoUpdatePageData> iFirstImage %d nLinesToCompute %d\n",
            iFirstImage, nLinesToCompute);
        fprintf (mpfLog, "<COblique::DoUpdatePageData> miCurrent %d pageOffset %d\n", 
            mpPosition->miCurrent, pageOffset);
        fprintf (mpfLog, "pVolHeader %p\n", pVolume->GetHeader());
    } */
    for (int i = 0; i < nLinesToCompute; i++)
    {
        unsigned char *pVolPage = NULL;
		if (mbInvertUpDown)
			pVolPage = (unsigned char *) pVolume->GetPageVP (iLastImage - i);
		else
			pVolPage = (unsigned char *) pVolume->GetPageVP (iFirstImage + i);

        pVolPage += pageOffset;

        switch (eType)
        {
        case OBL_CORONAL:
            memcpy (pPage, pVolPage, nBytesInLine);
            break;
        case OBL_SAGITTAL:
			{
				int iPage = 0;
	            for (int iLine = 0; iLine < mnCols; iLine++)
				{
					int iGet = iLine * nBytesInLine;
					for (unsigned int iChar = 0; iChar < mDataUnitSize; iChar++)
						pPage[iPage++] = pVolPage[iGet++];
				}
			}
            break;
        }
        pPage += nBytesInLine;
    }

    if (nLinesToCompute < mnLinesInPage)
        memset (pPage, 0, (mnLinesInPage - nLinesToCompute) * nBytesInLine);
    
    OnChange();
    if (mpfLog)
    {
        fprintf (mpfLog, "\n");
        fflush (mpfLog);
    }
    return true;
}
void COblique::SetCoronal(void)
{
    if (!GetPos().SetType(OBL_CORONAL, mpVolumeData->GetNLinesInPage() - 1))
		return;

	if (!mpVolumeData->IsSquare())
	{
		SetLeafRange(0, mpVolumeData->GetNLinesInPage() - 1);
		mnCols = mpVolumeData->GetNCols();
	}
    gScreens.Refresh();
}
void COblique::SetSagittal(void)
{
    if (!GetPos().SetType(OBL_SAGITTAL, mpVolumeData->GetNCols() - 1))
		return;

 	if (!mpVolumeData->IsSquare())
	{
		SetLeafRange(0, mpVolumeData->GetNCols() - 1);
		mnCols = mpVolumeData->GetNLinesInPage();
	}
   gScreens.Refresh();
}
void COblique::OnAssignDR (void)
{
	// Create reference to image on the oblique
	CGLine::umbCreatOnly2Points = true;
	CObliqueRef *pImRefOnObl = new CObliqueRef(mpDR, false, this, mpVolumeData);
    CDataCoordinates *pFirst =  pImRefOnObl->GetFirstDataCoo();
    CDataCoordinates *pSecond =  pImRefOnObl->GetSecondDataCoo();
	pFirst->fx = 5;
	pSecond->fx = (float)mnCols - 5;
	pFirst->fy = (float)mnLinesInPage / 2;
	pSecond->fy = (float)mnLinesInPage / 2;
	mpDR->AddGraphicElement(pImRefOnObl);

	// Create reference to the oblique on the image
	CDisplayRectangle *pDataDR = mpVolumeData->GetDR();
	CGLine::umbCreatOnly2Points = true;
	mpOblRefOnIm = new CObliqueRef(pDataDR, true, this, mpVolumeData);
    pFirst =  mpOblRefOnIm->GetFirstDataCoo();
    pSecond = mpOblRefOnIm->GetSecondDataCoo();
	pFirst->fx = 5;
	pSecond->fx = (float)mpVolumeData->GetNCols() - 5;
	pFirst->fy = (float)mpVolumeData->GetNLinesInPage() / 2;
	pSecond->fy = (float)mpVolumeData->GetNLinesInPage() / 2;
	pDataDR->AddGraphicElement(mpOblRefOnIm);
}
