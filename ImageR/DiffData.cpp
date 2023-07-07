#include "StdAfx.h"
#include ".\diffdata.h"
#include "TypedData.h"

#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\TFileCompare.h"
#include "..\..\yUtils\MyRegistry.h"
#include "..\..\yUtils\MyProgress.h"
#include "..\..\yUtils\Histogram.h"

#include "..\..\ImageRLib\ImageStatistics.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\CompareManager.h"

#include "Screens.h"
#include "DisplayRectangle.h"

bool CDiffData::umbAllowDiffOfDifferentSize = true;
bool CDiffData::umbScaleDiff = true;

//--------------------------------------------------------------------

template <typename TIN>
void T_vector_ratio (TIN *pPlus, TIN *pMinus, float *pResult, int n)
{
	for (int i = 0; i < n; i++)
	{
        if (pMinus[i])
		    pResult[i] = (1000.0f * pPlus[i]) / pMinus[i];
        else if (pPlus[i])
            pResult[i] = pPlus[i];
        else
            pResult[i] = 0.0f;
	}
}
template <typename TIN, typename TOUT>
void T_vector_combine (TIN *pPlus, TIN *pMinus, TOUT *pResult, float w1, float w2, int n)
{
	for (int i = 0; i < n; i++)
	{
        pResult[i] = (TOUT)(pPlus[i] * w1 + pMinus[i] * w2);
	}
}
template <typename TINP, typename TINM, typename TOUT>
int T_vector_sub(TINP* pPlus, TINM* pMinus, TOUT* pResult, int n)
{
	int nDiff = 0;
	for (int i = 0; i < n; i++)
	{
		pResult[i] = pPlus[i] - pMinus[i];
		if (pResult[i])
			nDiff++;
	}
	return nDiff;
}
void VectorAbs(float* pResult, int n)
{
	for (int i = 0; i < n; i++)
		pResult[i] = abs(pResult[i]);
}
void VectorAbs(double* pResult, int n)
{
	for (int i = 0; i < n; i++)
		pResult[i] = abs(pResult[i]);
}
void VectorAbs(int* pResult, int n)
{
	for (int i = 0; i < n; i++)
		pResult[i] = abs(pResult[i]);
}
int vector_diff (bool *pPlus, bool *pMinus, bool *pResult, int n)
{
	int nDiff = 0;
	for (int i = 0; i < n; i++)
	{
		pResult[i] = (pPlus[i] != pMinus[i]);
		if (pResult[i])
			nDiff++;
	}
	return nDiff;
}
int vector_diff (char *pPlus, char *pMinus, char *pResult, int n)
{
	int nDiff = 0;
	for (int i = 0; i < n; i++)
	{
		pResult[i] = (pPlus[i] != pMinus[i]);
		if (pResult[i])
			nDiff++;
	}
	return nDiff;
}
int vector_diff (S_RGB *pPlus, S_RGB *pMinus, short *pResult, int n, int iColor, int &onAnyDiff)
{
	if (CDisplayRectangle::umbReverseRGBOrder)
	{
		if (iColor == DIFF_COL_BLUE)
			iColor = DIFF_COL_RED;
		else if (iColor == DIFF_COL_RED)
			iColor = DIFF_COL_BLUE;
	}
	int nDiff = 0;
	onAnyDiff = 0;
	for (int i = 0; i < n; i++)
	{
		pResult[i] = (short)pPlus[i].RGB[iColor] - (short)pMinus[i].RGB[iColor];
		if (pResult[i])
			nDiff++;

		bool bDiff = false;
		for (int ic = 0; ic < 3; ic++)
		{
			if (pPlus[i].RGB[ic] != pMinus[i].RGB[ic])
				bDiff = true;
		}
		if (bDiff)
			onAnyDiff++;
	}
	return nDiff;
}
//--------------------------------------------------------------------

CDiffData::CDiffData(CDisplayRectangle *pDRPlus, CDisplayRectangle *pDRMinus, const char *zName)
: CMetaData (zName ? zName : "DIFF")
, mpDRPlus(pDRPlus)
, mpDRMinus(pDRMinus)
, mnDiff(0)
, mbRatio(false)
, mbCombine(false)
, mWeight1(0.5f)
, mWeight2(0.5f)
, mpCompareManager(NULL)
, mbCompareTried(false)
, mbCompareFailed(false)
, miDiff(0)
, mbRgb(false)
, miRgb(0)
, mnAnyDiffForRgb(0)
, mnLinesToCompare(0)
, mnColsToCompare(0)
, meDataTypePlus(DATA_TYPE_UNDEF)
, meDataTypeMinus(DATA_TYPE_UNDEF)
, mbPrintHistogram(false)
, mbShouldScale(false)
, mScaledPlus("ScaledPlus")
, mScaledMinus("ScaledMinus")
, mbTransform(false)
, mTransFactor(1)
, mTransBias(0)
, mbLookingForBestFit(false)
, mbAbs(false)
{
	if (msName == "Ratio")
		mbRatio = true;
	else if (msName == "Combine")
		mbCombine = true;
	else if (msName == "AbsDiff")
		mbAbs = true;

    InitOnConstruction();
	SetCommonZoom(mpDRPlus->GetCommonZoomParams());
}

CDiffData::CDiffData(CXMLParseNode *pXmlNode)
: CMetaData (pXmlNode->GetFirstDeepString("name", "DIFF"))
, mpDRPlus((CDisplayRectangle *)CNamedObject::GetParsed(pXmlNode, "plus_id"))
, mpDRMinus((CDisplayRectangle *)CNamedObject::GetParsed(pXmlNode, "minus_id"))
, mnDiff(0)
, mbRatio(false)
, mbCombine(false)
, mWeight1(0.5f)
, mWeight2(0.5f)
, mpCompareManager(NULL)
, mbRgb(false)
, miRgb(0)
, mnAnyDiffForRgb(0)
, mnLinesToCompare(0)
, mnColsToCompare(0)
, mbShouldScale(false)
, mScaledPlus("ScaledPlus")
, mScaledMinus("ScaledMinus")
, mbTransform(false)
, mTransFactor(1)
, mTransBias(0)
, mbLookingForBestFit(false)
, mbAbs(false)
{
    if (pXmlNode->GetFirstDeep("b_ratio"))
        mbRatio = true;
    else if (pXmlNode->GetFirstDeep("b_combine"))
        mbCombine = true;
	if (mpDRPlus && mpDRMinus)
	    InitOnConstruction();
	if (mpDRPlus)
		SetCommonZoom(mpDRPlus->GetCommonZoomParams());
}
CDiffData::~CDiffData(void)
{
    if (mpCompareManager && CCompareManager::GetNInstance())
        delete mpCompareManager;
}
void CDiffData::InitOnConstruction(void)
{
	CMetaData *pPlus = mpDRPlus->GetCurrentData();
	CMetaData *pMinus = mpDRMinus->GetCurrentData();
	if (!pPlus)
		return;

	SetCtImage(false, true /*force*/);
	mnLinesInPage = pPlus->GetNLinesInPage();
    mnCols = pPlus->GetNCols();
    SetMmPerPixel(*pPlus);

	// Check whether should scale data
	if (umbScaleDiff)
	{
		float biasP, biasM, slopeP, slopeM;
		pPlus->GetRescale(biasP, slopeP);
		pMinus->GetRescale(biasM, slopeM);
		if (slopeP != 1 || slopeM != 1 || biasP != biasM)
			mbShouldScale = true;
	}

    SetDiffDataType();

    SetPositionLike(pPlus);
	AddCheck(mpDRPlus);
	AddCheck(mpDRMinus);

	miPlus = -1;
	miMinus = -1;
	miDiff = -1; // Not Yest Defined
}
void CDiffData::EditContextMenu(CMenu *pPopup)
{
	mDataList.RemoveAll();
	gScreens.GetListOfDataUsingPosition(GetPosition(),mDataList);
	int n = (int)mDataList.GetSize();

	miPlus = -1;
	miMinus = -1;
	miDiff = -1;

	int i = 0;
	POSITION pos = mDataList.GetHeadPosition();
	while (pos)
	{
		CMetaData *pData = mDataList.GetNext(pos);
		if (pData->IsOn(mpDRPlus))
			miPlus = i;
		if (pData->IsOn(mpDRMinus))
			miMinus = i;
		if (pData == this)
			miDiff = i;
		i++;
	}

	int nMayBePlus = miMinus;
	if (miMinus > miDiff)
		nMayBePlus--;
	if (nMayBePlus < 2)
		pPopup->RemoveMenu(ID_DIFF_SELECTPLUS, MF_BYCOMMAND);

	int nMayBeMinus = n - miPlus - 2; // Minus the Diff itself and the PLUS data
	if (nMayBeMinus < 2)
		pPopup->RemoveMenu(ID_DIFF_SELECTMINUS, MF_BYCOMMAND);
	if (mbRgb)
	{
		pPopup->AppendMenu(MF_STRING, 
					ID_DIFF_RED, "Red");
		pPopup->AppendMenu(MF_STRING, 
					ID_DIFF_GREEN, "Green");
		pPopup->AppendMenu(MF_STRING, 
					ID_DIFF_BLUE, "Blue");
	}
}
void CDiffData::SetDiffDataType(void)
{
	mbRgb = false;
	CMetaData *pPlus = mpDRPlus->GetCurrentData();
	if (!pPlus)
		return;
	CMetaData *pMinus = mpDRMinus->GetCurrentData();
	if (!pMinus)
		return;

    meDataTypePlus = pPlus->GetDataType();
    meDataTypeMinus = pMinus->GetDataType();
	EMyImageDataType eDataType = meDataTypePlus;

    if (mbRatio || mbShouldScale)
        eDataType = DATA_TYPE_FLOAT;
	else if (meDataTypeMinus == DATA_TYPE_DOUBLE || meDataTypePlus == DATA_TYPE_DOUBLE)
		eDataType = DATA_TYPE_DOUBLE;
	else if (meDataTypeMinus == DATA_TYPE_FLOAT || meDataTypePlus == DATA_TYPE_FLOAT)
		eDataType = DATA_TYPE_FLOAT;
    else if (meDataTypePlus == DATA_TYPE_INT || meDataTypeMinus == DATA_TYPE_INT)
        eDataType = DATA_TYPE_INT;
    else if (meDataTypePlus == DATA_TYPE_UINT || meDataTypeMinus == DATA_TYPE_UINT)
        eDataType = DATA_TYPE_INT;
    else if (meDataTypePlus == DATA_TYPE_USHORT || meDataTypeMinus == DATA_TYPE_USHORT)
        eDataType = DATA_TYPE_SHORT;
    else if (meDataTypePlus == DATA_TYPE_RGB)
	{
		mbRgb = true;
        eDataType = DATA_TYPE_SHORT;
	}

    SetTypedDataHandler(eDataType);
}
bool CDiffData::DoUpdatePageData()
{
	mbCompareTried = true;
	mbCompareFailed = false;
	if (mbCombine)
		GetCombineParams();

	mnDiff = 0;
	mnAnyDiffForRgb = 0;
	CMetaData *pPlus = mpDRPlus->GetCurrentData();
	if (!pPlus)
	{
		mbCompareFailed = true;
		return false;
	}
	CMetaData *pMinus = mpDRMinus->GetCurrentData();
	if (!pMinus)
	{
		mbCompareFailed = true;
		return false;
	}

    if (!pPlus->UpdatePageData())
	{
		mbCompareFailed = true;
        return false;
	}
    if (!pMinus->UpdatePageData())
	{
		mbCompareFailed = true;
        return false;
	}

	if (pPlus->IsHybrid())
	{
		SetMmPerPixel(*pPlus);
	}
	mnLinesToCompare = pPlus->GetNLinesInPage();
	mnColsToCompare = pPlus->GetNCols();
	bool bCompareInOneChunck = true;
	if ((pPlus->GetNLinesInPage() != pMinus->GetNLinesInPage())
		|| (pPlus->GetNCols() != pMinus->GetNCols()))
	{
		if (!umbAllowDiffOfDifferentSize)
		{
			static int unErr = 0;
			mbCompareFailed = true;
			if (!unErr++)
				CMyWindows::MessBoxThread("Can't diff images of different size", "Warning");
			return false;
		}
		if (pMinus->GetNLinesInPage() < mnLinesToCompare)
			mnColsToCompare = pMinus->GetNLinesInPage();
		if (pMinus->GetNCols() < mnColsToCompare)
			mnColsToCompare = pMinus->GetNCols();

		if (pPlus->GetNCols() != pMinus->GetNCols())
			bCompareInOneChunck = false;

		static int unWarn = 0;
		if (!unWarn++)
			CMyWindows::MessBoxThread("Comparing images of different size", "Warning");
	}

    void *pPlusData = pPlus->GetPageData();
    void *pMinusData = pMinus->GetPageData();
    if (!pPlusData || !pMinusData)
    {
        mpPageData->mbUpdateFailed = true;
		mbCompareFailed = true;
        return false;
    }

	int n = mnLinesToCompare * mnColsToCompare;
	if (mbShouldScale)
	{
		pPlus->ScalePageData(mScaledPlus);
		pMinus->ScalePageData(mScaledMinus);
		pPlusData = mScaledPlus.GetData();
		pMinusData = mScaledMinus.GetData();
	}

    void *pResult = (unsigned short *)mpPageData->GetDataStart();
    memset (pResult, 0, mpPageData->GetByteSize());
	mpPageData->Zero();
    //int n = mnLinesToCompare * mnColsToCompare;

	if (bCompareInOneChunck)
	{
		if (mbTransform)
		{
			FindBestTransformation();
			CCompVector minusTransformed("vTrans", n);
			minusTransformed.Transform((float*)pMinusData, mTransFactor, mTransBias);
			pMinusData = minusTransformed.GetData();
			CompareTypedData(pPlusData, pMinusData, pResult, n);
		}
		else
			CompareTypedData(pPlusData, pMinusData, pResult, n);
	}
	else
	{
		for (int iLine = 0; iLine < mnLinesToCompare; iLine++)
		{
			void *pResultLine = mpPageData->GetLineDataStart(iLine);
			void *pPlusDataLine = pPlus->GetPageDataLine(iLine);
			void *pMinusDataLine = pMinus->GetPageDataLine(iLine);

			CompareTypedData(pPlusDataLine, pMinusDataLine, pResultLine, mnColsToCompare);
			//if (mbAbs)
			//	AbsTypedData(pResultLine, mnColsToCompare);
		}
	}

    //if (mpTypedData)
    //    mpTypedData->OnRead (pResult, n);
    return true;
}
void CDiffData::ReportTypeError(void)
{
	static int unErr = 0;
	if (unErr++ == 0)
	{
		char zBuf[128];
		if (meDataTypePlus == meDataTypeMinus)
			sprintf_s(zBuf,sizeof(zBuf),"<CDiffData::DoUpdatePageData> type %d %s diff is not supported", 
				meDataTypePlus, (const char*)CMyImage::GetDataTypeString(meDataTypePlus));
		else
			sprintf_s(zBuf,sizeof(zBuf),"<CDiffData::DoUpdatePageData> type %d %s vs %d %s diff is not supported", 
				meDataTypePlus, (const char*)CMyImage::GetDataTypeString(meDataTypePlus),
				meDataTypeMinus, (const char*)CMyImage::GetDataTypeString(meDataTypeMinus)
				);
		CMyWindows::MessBox(zBuf,"Warning");
	}
	mbCompareFailed = true;
}
template <typename TINP>
bool CDiffData::CompareTypedDataShort(TINP *pPlusData, void *pMinusData, void *pResult, int n)
{
    switch (meDataTypeMinus) 
	{
    case DATA_TYPE_SHORT:
		mnDiff += T_vector_sub (pPlusData, (short *)pMinusData, (short *)pResult, n);
        break;
    case DATA_TYPE_USHORT:
		mnDiff += T_vector_sub (pPlusData, (unsigned short *)pMinusData, (short *)pResult, n);
        break;
    case DATA_TYPE_INT:
		mnDiff += T_vector_sub (pPlusData, (int *)pMinusData, (int *)pResult, n);
        break;
    case DATA_TYPE_UINT:
		mnDiff += T_vector_sub (pPlusData, (unsigned int *)pMinusData, (int *)pResult, n);
        break;
    case DATA_TYPE_FLOAT:
		mnDiff += T_vector_sub (pPlusData, (float *)pMinusData, (float *)pResult, n);
        break;
    case DATA_TYPE_DOUBLE:
		mnDiff += T_vector_sub (pPlusData, (double *)pMinusData, (double *)pResult, n);
        break;
    default:
		ReportTypeError();
		return false;
	}
	return true;
}
template <typename TINP>
bool CDiffData::CompareTypedDataInt(TINP *pPlusData, void *pMinusData, void *pResult, int n)
{
    switch (meDataTypeMinus) 
	{
    case DATA_TYPE_SHORT:
		mnDiff += T_vector_sub (pPlusData, (short *)pMinusData, (int *)pResult, n);
        break;
    case DATA_TYPE_USHORT:
		mnDiff += T_vector_sub (pPlusData, (unsigned short *)pMinusData, (int *)pResult, n);
        break;
    case DATA_TYPE_INT:
		mnDiff += T_vector_sub (pPlusData, (int *)pMinusData, (int *)pResult, n);
        break;
    case DATA_TYPE_UINT:
		mnDiff += T_vector_sub (pPlusData, (unsigned int *)pMinusData, (int *)pResult, n);
        break;
    case DATA_TYPE_FLOAT:
		mnDiff += T_vector_sub (pPlusData, (float *)pMinusData, (float *)pResult, n);
        break;
    case DATA_TYPE_DOUBLE:
		mnDiff += T_vector_sub (pPlusData, (double *)pMinusData, (double *)pResult, n);
        break;
    default:
		ReportTypeError();
		return false;
	}
	return true;
}
bool CDiffData::CompareTypedDataFloat(float *pPlusData, void *pMinusData, void *pResult, int n)
{
    switch (meDataTypeMinus) 
	{
    case DATA_TYPE_SHORT:
		mnDiff += T_vector_sub (pPlusData, (short *)pMinusData, (float *)pResult, n);
        break;
    case DATA_TYPE_USHORT:
		mnDiff += T_vector_sub (pPlusData, (unsigned short *)pMinusData, (float *)pResult, n);
        break;
    case DATA_TYPE_INT:
		mnDiff += T_vector_sub (pPlusData, (int *)pMinusData, (float *)pResult, n);
        break;
    case DATA_TYPE_UINT:
		mnDiff += T_vector_sub (pPlusData, (unsigned int *)pMinusData, (float *)pResult, n);
        break;
    case DATA_TYPE_FLOAT:
		mnDiff += T_vector_sub (pPlusData, (float *)pMinusData, (float *)pResult, n);
        break;
    case DATA_TYPE_DOUBLE:
		mnDiff += T_vector_sub (pPlusData, (double *)pMinusData, (double *)pResult, n);
        break;
    default:
		ReportTypeError();
		return false;
	}
	return true;
}
bool CDiffData::CompareTypedDataDouble(double *pPlusData, void *pMinusData, double *pResult, int n)
{
    switch (meDataTypeMinus) 
	{
    case DATA_TYPE_SHORT:
		mnDiff += T_vector_sub (pPlusData, (short *)pMinusData, pResult, n);
        break;
    case DATA_TYPE_USHORT:
		mnDiff += T_vector_sub (pPlusData, (unsigned short *)pMinusData, pResult, n);
        break;
    case DATA_TYPE_INT:
		mnDiff += T_vector_sub (pPlusData, (int *)pMinusData, pResult, n);
        break;
    case DATA_TYPE_UINT:
		mnDiff += T_vector_sub (pPlusData, (unsigned int *)pMinusData, pResult, n);
        break;
    case DATA_TYPE_FLOAT:
		mnDiff += T_vector_sub (pPlusData, (float *)pMinusData, pResult, n);
        break;
    case DATA_TYPE_DOUBLE:
		mnDiff += T_vector_sub (pPlusData, (double *)pMinusData, pResult, n);
        break;
    default:
		ReportTypeError();
		return false;
	}
	return true;
}
bool CDiffData::CompareTypedData(void *pPlusData, void *pMinusData, void *pResult, int n)
{
	EMyImageDataType meCompareType = meDataTypePlus;
	if (mbShouldScale)
		meCompareType = DATA_TYPE_FLOAT;

    switch (meCompareType) 
	{
    case DATA_TYPE_SHORT:
		return CompareTypedDataShort((short *)pPlusData, pMinusData, pResult, n);
        break;
    case DATA_TYPE_USHORT:
        {
            unsigned short *pp = (unsigned short *)pPlusData;
            unsigned short *pm = (unsigned short *)pMinusData;
            if (mbRatio)
				T_vector_ratio (pp, pm, (float *)pResult, n);
            else if (mbCombine)
                T_vector_combine (pp, pm, (short *)pResult, mWeight1, mWeight2, n);
            else
				return CompareTypedDataShort((unsigned short *)pPlusData, pMinusData, pResult, n);
        }
        break;
    case DATA_TYPE_INT:
		return CompareTypedDataInt((int *)pPlusData, pMinusData, pResult, n);
		if (mbAbs)
			VectorAbs((int*)pResult, n);
        break;
    case DATA_TYPE_UINT:
		return CompareTypedDataInt((unsigned int *)pPlusData, pMinusData, pResult, n);
		if (mbAbs)
			VectorAbs((int*)pResult, n);
		break;
    case DATA_TYPE_FLOAT:
		mnDiff += T_vector_sub ((float *)pPlusData, (float *)pMinusData,
				(float *)pResult, n);
 		if (mbAbs)
			VectorAbs((float*)pResult, n);
		break;
    case DATA_TYPE_DOUBLE:
		mnDiff += T_vector_sub ((double *)pPlusData, (double *)pMinusData,
				(double *)pResult, n);
		if (mbAbs)
			VectorAbs((double*)pResult, n);
        break;
    case DATA_TYPE_BOOL:	
        mnDiff += vector_diff ((bool *)pPlusData, (bool *)pMinusData, (bool *)pResult, n);
        break;
    case DATA_TYPE_CHAR:	
        mnDiff += vector_diff ((char *)pPlusData, (char *)pMinusData, (char *)pResult, n);
        break;
    case DATA_TYPE_RGB:	
		mnDiff += vector_diff ((S_RGB *)pPlusData, (S_RGB *)pMinusData, (short *)pResult, n, miRgb, mnAnyDiffForRgb);
        break;
    default:
		ReportTypeError();
		return false;
    }
	return true;
}
/*
template <typename TINP>
bool CDiffData::AbsTypedData(TINP* pResult, int n)
{
	for (int i = 0; i < n; i++)
		pResult[i] = abs(pResult[i]);
}*/
void CDiffData::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("DiffData",xmlDump);
    CMetaData::SaveState(xmlDump);
    xmlDump.Write("plus_id", mpDRPlus->mUId);
    xmlDump.Write("minus_id", mpDRMinus->mUId);
    if (mbRatio)
        xmlDump.Write("b_ratio", mbRatio);
    else if (mbCombine)
        xmlDump.Write("b_combine", mbCombine);
    else if (mbRgb)
		xmlDump.Write("b_RGB", mbRgb);
}
void CDiffData::StartCompare(void)
{
	CMetaData *pPlus = mpDRPlus->GetCurrentData();
	if (!pPlus)
		return;
	CMetaData *pMinus = mpDRMinus->GetCurrentData();
	if (!pMinus)
		return;
    if (!mpCompareManager || (CCompareManager::GetNInstance() == 0))
	{
		if (CCompareManager::ShouldSaveCompareData())
		{
			CCompareManager::umsCompareRootDir = CMyWindows::GetLogDir();
			CCompareManager::umsCompareDataDir = CMyWindows::GetLogDir("Compare");
			CMyWindows::DeleteDirFiles(CCompareManager::umsCompareDataDir);
		}
        mpCompareManager = new CCompareManager(*pPlus, *pMinus, &gScreens, pPlus->PermanentName());
	}
}
bool CDiffData::ChangePlus(void)
{
	miPlus++;
	if (miPlus == miDiff)
		miPlus++;
	if (miPlus >= miMinus)
		miPlus = 0;

	RemoveCheck(mpDRPlus);
	mpDRPlus = GetDRAt(miPlus);
	AddCheck(mpDRPlus);
	ForceUpdate();
	gScreens.Refresh();
	return true;
}
bool CDiffData::ChangeMinus(void)
{
	miMinus++;
	if (miMinus == miDiff)
		miMinus++;
	if (miMinus >= mDataList.GetSize())
	{
		miMinus = miPlus + 1;
		if (miMinus == miDiff)
			miMinus++;
	}

	RemoveCheck(mpDRMinus);
	mpDRMinus = GetDRAt(miMinus);
	AddCheck(mpDRMinus);
	ForceUpdate();
	gScreens.Refresh();
	return true;
}
CDisplayRectangle *CDiffData::GetDRAt(int iWanted)
{
	int i = 0;
	POSITION pos = mDataList.GetHeadPosition();
	while (pos)
	{
		CMetaData *pData = mDataList.GetNext(pos);
		if (i == iWanted)
			return pData->GetDR();
		i++;
	}
	return NULL;
}
CString CDiffData::GetPageDataText(int iText)
{
	if (iText == 0)
	{
		CFileName fNamePlus(mpDRPlus->GetCurrentData()->Name());
		CFileName fNameMinus(mpDRMinus->GetCurrentData()->Name());
		CString sPrivatePlus = fNamePlus.Private();
		CString sPrivateMinus = fNameMinus.Private();
		while (fNamePlus.Length() > 1 && fNameMinus.Length() > 1
			&& fNamePlus != fNameMinus)
		{
			if (sPrivatePlus != sPrivateMinus)
				break;

			fNamePlus.Set(fNamePlus.PathDir());
			fNameMinus.Set(fNameMinus.PathDir());
			sPrivatePlus = fNamePlus.Private();
			sPrivateMinus = fNameMinus.Private();
		}
		CString sText = sPrivatePlus;
		if (mbCombine)
		{
			char zBuf[128];
			sprintf_s(zBuf,sizeof(zBuf)," * %.3f + %.3f * ", mWeight1, mWeight2);
			sText += zBuf;
		}
		else
		{
			sText += " - ";
		}
		sText += sPrivateMinus;
		return sText;
	}

	if (mbCombine)
		return CString(_T(""));

	if (!mbCompareTried)
		return CString(_T("Not Initialized!"));

	if (mbCompareFailed)
		return CString(_T("Diff Failed!"));

	CString sText(_T(""));
	if (mbRgb)
	{
		switch(miRgb)
		{
		case DIFF_COL_BLUE:
			sText += "BLUE ";
			break;
		case DIFF_COL_GREEN:
			sText += "GREEN ";
			break;
		case DIFF_COL_RED:
			sText += "RED ";
			break;
		}
	}
	if (mnDiff < 1)
	{
		sText += "Same!";
		return sText;
	}

	sText += "n differ ";
	char zBuf[128];
	sprintf_s(zBuf,sizeof(zBuf),"%d ", mnDiff);
	sText += zBuf;
	sText += mpTypedData->GetRangeText();
	if (mbRgb)
	{
		sprintf_s(zBuf,sizeof(zBuf)," [%d RGB Diff]", mnAnyDiffForRgb);
		sText += zBuf;
	}
	return sText;
}
void CDiffData::PrintStatistics(void)
{
	CMetaData *pPlus = mpDRPlus->GetCurrentData();
	CMetaData *pMinus = mpDRMinus->GetCurrentData();

	if (pPlus->IsDataOfType(DATA_TYPE_FLOAT))
	{
		CTFileCompare<float> fComp(pPlus->Name(), pMinus->Name(), "Pixel", "Image", 
			pPlus->GetNCols(), pPlus->GetNLinesInPage());
		fComp.compare_files(true /*bPrint*/);
		if (mbPrintHistogram)
			PrintHistogram(fComp);
	}
	else if (pPlus->IsDataOfType(DATA_TYPE_USHORT))
	{
		CTFileCompare<unsigned short> fComp(pPlus->Name(), pMinus->Name(), "Pixel", "Image", 
			pPlus->GetNCols(), pPlus->GetNLinesInPage());
		fComp.compare_files(true /*bPrint*/);
		if (mbPrintHistogram)
			PrintHistogram(fComp);
	}
}
void CTFileCompare<float>::Print (FILE *pf, float value)
{
    if (pf)
	{
		if (value > 100.0f || value < -100.0f)
	        fprintf (pf, " %9.3f", value);
		else if (value > 0.1f || value < -0.1f)
	        fprintf (pf, " %9.6f", value);
		else 
	        fprintf (pf, " %12.9f", value);
	}
}
void CTFileCompare<unsigned short>::Print (FILE *pf, unsigned short value)
{
    if (pf)
        fprintf (pf, " %8u", value);
}
void CDiffData::GetCombineParams(void)
{
	static const int UNDEF = -100000;
	int w1pc = UNDEF;
	int w2pc = UNDEF;
	CMyRegistry reg;
	reg.Get("CombineW1PC",w1pc);
	reg.Get("CombineW2PC",w2pc);

	if (w1pc > UNDEF && w2pc > UNDEF)
	{
		mWeight1 = w1pc / 100.0f;
		mWeight2 = w2pc / 100.0f;
	}
	else if (w1pc > UNDEF)
	{
		mWeight1 = w1pc / 100.0f;
		mWeight2 = 1.0f - mWeight1;
	}
	else if (w2pc > UNDEF)
	{
		mWeight2 = w2pc / 100.0f;
		mWeight1 = 1.0f - mWeight2;
	}
	else
	{
		mWeight1 = 0.5f;
		mWeight2 = 0.5f;
	}
}
void CDiffData::SetColor(int iColor)
{
	if (iColor == miRgb)
		return;

	miRgb = iColor;
	ForceUpdate();
	gScreens.Refresh();
}
void CDiffData::PrintHistogram(CFileCompareBase &compFile)
{
	if (compFile.GetNDiffs() < 10)
		return;

	CString sHistFileName;
	FILE *pfHist = compFile.OpenCompLogFile("xls",sHistFileName);
	if (!pfHist)
		return;

	CImageStatistics::ReadRegistry();
	CImageStatistics::SetComputeHistogram(false);

	CMetaData *pPlus = mpDRPlus->GetCurrentData();
	CMetaData *pMinus = mpDRMinus->GetCurrentData();

	CImageStatistics ps1(pPlus->Name(), pfHist, pPlus->PageData());
	CImageStatistics ps2(pMinus->Name(), pfHist, pMinus->PageData());
	CImageStatistics psDiff(Name(), pfHist, PageData());

	ps1.ComputeAndPrint();
	ps2.ComputeAndPrint();
	psDiff.ComputeAndPrint();

	ps1.StartCommonHistogramRange(ps2);

	ps1.ComputeHistogram();
	ps2.ComputeHistogram();
	psDiff.ComputeHistogram();

	ps1.PrintHistogram3(ps2, psDiff);

	fclose(pfHist);
	CMyWindows::OpenExcel(sHistFileName);
}
bool CDiffData::FindBestFit(void)
{
	CLogFile log("CDiffData__FindBestFit.csv");
	CMetaData *pFirstData = mpDRPlus->GetCurrentData();
	CPosition *pFirstPos = pFirstData->GetPosition();
	CMetaData *pSecData = mpDRMinus->GetCurrentData();
	CPosition *pSecPos = pSecData->GetPosition();
	if (pFirstPos == pSecPos)
	{
		CMyWindows::MessBox("Can't find fit as both images use same position","Warning");
		return false;
	}

	if (log)
	{
		fprintf(log.mpf,"First %s pos %d\n", (const char*)pFirstData->Name(), pFirstPos->miCurrent);
		fprintf(log.mpf,"Second %s pos %d\n", (const char*)pSecData->Name(), pSecPos->miCurrent);
		fprintf(log.mpf,"\n");
	}
	pSecPos->SaveCurrentPosition();
	CMyProgress progressBar((short)pSecPos->miFirst, (short)pSecPos->miLast);

	int iBest = -1;
	double bestAvg = 1000000000;

	int nSeen = 0;
	int maxN = pSecData->GetNPages();
	mbLookingForBestFit = true;
	for (int i = pSecData->StartLoopI(); i >= 0; i = pSecData->LoopNext())
	{
		if (!pSecData->ForceUpdatePageData())
			break;

		if (!ForceUpdatePageData())
			break;

		double avg = mpPageData->GetAverageAbs();
		if (log)
		{
			fprintf(log.mpf,"%3d, %f,", pSecPos->miFirst + i, avg);
			if (mbTransform)
				fprintf(log.mpf, "%.3f, %.3ff,", mTransFactor, mTransBias);
		}

		if (iBest < 0 || avg < bestAvg)
		{
			iBest = i;
			bestAvg = avg;
			if (log)
				fprintf(log.mpf," - NEW BEST");
		}
		if (log)
			fprintf(log.mpf,"\n");
		progressBar.SetPos(i);
		nSeen++;
		if (nSeen > maxN)
		{
			CMyWindows::MessBox("<CDiffData::FindBestFit> went beyond loop limits", "SW Error");
			break;
		}
	}

	if (iBest >= 0)
		pSecPos->SetCurrent(pSecPos->miFirst + iBest);
	else
		pSecPos->RestoreSavedPosition();

	mbLookingForBestFit = false;
	pSecData->ForceUpdate();
	ForceUpdate();
	return true;
}
void CDiffData::ReadRegistry(CMyRegistry &myReg)
{
	myReg.Get("b_scale_diff", umbScaleDiff);
}
bool CDiffData::FindBestTransformation(bool bMessBox)
{
	CMetaData *pPlus = mpDRPlus->GetCurrentData();
	if (!pPlus)
	{
		mbCompareFailed = true;
		return false;
	}
	CMetaData *pMinus = mpDRMinus->GetCurrentData();
	if (!pMinus)
	{
		mbCompareFailed = true;
		return false;
	}
	if (!pPlus->IsDataOfType(DATA_TYPE_FLOAT) || !pPlus->IsDataOfType(DATA_TYPE_FLOAT))
	{
		CMyWindows::MessBox("<FindBestTransformation> is currently supported only for float", "Warning");
		return false;
	}
	float* pPlusData = (float *)pPlus->GetPageData();
	float* pMinusData = (float*)pMinus->GetPageData();
	CCompVector vPlus("vPlus", pPlusData, pPlus->GetNPixelsInPage());
	CCompVector vMinus("vMinus", pMinusData, pMinus->GetNPixelsInPage());

	static const int HIST_LEN = 1024;
	CHistogram hPlus("hPlus", HIST_LEN);
	CHistogram hMinus("hMinus", HIST_LEN);
	hPlus.Compute(vPlus);
	hMinus.Compute(vMinus);
	float lowPlus = hPlus.AverageInRange(0.1f, 0.5f);
	float highPlus = hPlus.AverageInRange(0.5f, 0.9f);
	float lowMinus = hMinus.AverageInRange(0.1f, 0.5f);
	float highMinus = hMinus.AverageInRange(0.5f, 0.9f);
	float spanSrc = highMinus - lowMinus;
	float spanTarget = highPlus - lowPlus;
	mTransFactor = spanTarget / spanSrc;

	float avgTarget = vPlus.ComputeAverage();
	float avgSrv = vMinus.ComputeAverage();
	mTransBias = avgTarget - mTransFactor * avgSrv;

	mbTransform = true;

	if (bMessBox)
	{
		char zBuf[128];
		sprintf_s(zBuf, sizeof(zBuf), "Transfer formula %.3f * x + %.3f", mTransFactor, mTransBias);
		CMyWindows::MessBox(zBuf, "Notice");
	}

	ForceUpdate();
	return true;
}