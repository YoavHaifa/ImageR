#include "StdAfx.h"
#include "MultiPageLoader.h"
#include "MetaData.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\CsvFile.h"
#include "..\..\ImageRLib\Position.h"
#include "DRText.h"
#include "TypedData.h"

CMultiPageLoader::CMultiPageLoader(CMetaData *pData)
	: mpData(pData)
	, mnWanted(1)
	, mnLoaded(0)
	, mpSumPage(NULL)
	, mpImageText(NULL)
	, mbUseImagesCache(true)
	, miUpdate(0)
	, miInCache(0)
{
	// Init empty cache
	for (int i = 0; i < MAX_IN_CACHE; i++)
	{
		maLastUsed[i] = 0;
		maIndexLoaded[i] = -1000000;
	}
}
CMultiPageLoader::~CMultiPageLoader(void)
{
}
void CMultiPageLoader::SetNWanted(int n)
{
	if (n > MAX_IN_CACHE)
	{
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf),"No more than %d images can be averaged", MAX_IN_CACHE);
		CMyWindows::MessBox(zBuf,"Warning");
		n = MAX_IN_CACHE;
	}
	mnWanted = n;
}
bool CMultiPageLoader::AllocateImages(void)
{
	char zBuf[128];

	int n = mImages.GetSize();
	while (mImages.GetSize() < mnWanted)
	{
		CFileName fName(mpData->msName);
		CString sPageName (fName.Private());
		sPageName += "_page";
		sprintf_s(zBuf,sizeof(zBuf),"_%03d", ++n);
		sPageName += zBuf;

		CMyImage *pNew = mpData->CreatePage(sPageName);
		if (!pNew)
			return false;
		pNew->mIndex = -1000000; // NULL
		mImages.AddTail(pNew);
	}

	if (!mpSumPage)
	{
		CFileName fName(mpData->msName);
		CString sPageName (fName.Private());
		sPageName += "_sum";
		mpSumPage = mpData->CreateFloatPage(sPageName);
	}

	if (!mpImageText)
	{
		mpImageText = mpData->AddDataText("Averaging", false);
		mpImageText->meTextType = CDRText::TEXT_AVERAGING;
	}
	return true;
}
bool CMultiPageLoader::UpdatePage(void)
{
	if (!AllocateImages())
		return false;

	miUpdate++;
	CPosition *pPos = mpData->GetPosition();
	int iCur = pPos->miCurrent;
	int half = mnWanted / 2;
	miFirst = iCur - half;
	miLast = miFirst + mnWanted - 1;
	if (miFirst < pPos->miFirst)
		miFirst = pPos->miFirst;
	if (miLast > pPos->miLast)
		miLast = pPos->miLast;

	mpSumPage->Zero();
	CMyImage *pPageDataSave = mpData->mpPageData;
	mnLoaded = 0;
	bool bOK = true;
	if (mbUseImagesCache)
	{
		if (!LoadAndAverageImages())
			bOK = false;
	}
	else
	{
		if (!LoadAllImages())
			bOK = false;
	}

	//bool bOK = mpData->DoUpdatePageData();
	mpData->mpPageData = pPageDataSave;
	mpData->set_i(iCur);
	mpData->mpPageData->DivideFloatVector(mpSumPage->GetData(),mnLoaded);
	return bOK;
}
bool CMultiPageLoader::LoadAllImages(void)
{
	POSITION pos = mImages.GetHeadPosition();
	for (int iLoad = miFirst; iLoad <= miLast; iLoad++)
	{
		CMyImage *pTarget = mImages.GetNext(pos);
		if (!pTarget)
		{
			CMyWindows::MessBox("<CMultiPageLoader::UpdatePage> failed, missing target","SW Error");
			return false;
		}
		mpData->mpPageData = pTarget;

		if (!mpData->set_i(iLoad))
		{
			CMyWindows::MessBox("<CMultiPageLoader::UpdatePage> set_i failed","SW Error");
			return false;
		}
		if(!mpData->DoUpdatePageData())
		{
			CMyWindows::MessBox("<CMultiPageLoader::UpdatePage> DoUpdatePageData failed","SW Error");
			return false;
		}
		Add(pTarget);
	}
	return true;
}
int CMultiPageLoader::GetNWanted(void)
{
	return mnWanted;
}
void CMultiPageLoader::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("MultiPageLoader",xmlDump);
	xmlDump.Write("n_wanted",mnWanted);
	xmlDump.Write("n_loaded",mnLoaded);
	xmlDump.Write("i_first",miFirst);
	xmlDump.Write("i_last",miLast);
}
CString CMultiPageLoader::GetText(void)
{
	char zBuf[128];
	sprintf_s(zBuf,sizeof(zBuf),"Averaging %d images", mnLoaded);
	return CString(zBuf);
}
bool CMultiPageLoader::LoadAndAverageImages(void)
{
	int nToLoad = miLast - miFirst + 1;
	bool *pabLoaded = new bool[nToLoad];
	memset(pabLoaded,0,nToLoad*sizeof(pabLoaded[0]));
	bool bOK = true;

	// Look for all that are already loaded
	for (int iLoad = miFirst; iLoad <= miLast; iLoad++)
	{
		CMyImage *pTarget = GetTargetOf(iLoad);
		if (pTarget)
		{
			pabLoaded[iLoad-miFirst] = true;
			Add(pTarget);
		}
	}

	// Load rest of wanted images
	for (int iLoad = miFirst; iLoad <= miLast; iLoad++)
	{
		if (!pabLoaded[iLoad-miFirst])
		{
			CMyImage *pTarget = GetFreeTarget();
			if (pTarget)
			{
				if (!Load(pTarget, iLoad))
				{
					bOK = false;
					break;
				}
				Add(pTarget);
			}
		}
	}

	delete pabLoaded;
	return bOK;
}
CMyImage *CMultiPageLoader::GetFreeTarget(void)
{
	CMyImage *pBestTarget = NULL;
	int iBestUsed = miUpdate + 1;

	POSITION pos = mImages.GetHeadPosition();
	int iInCache = 0;
	while (pos)
	{
		CMyImage *pTarget = mImages.GetNext(pos);
		if (maLastUsed[iInCache] < iBestUsed)
		{
			pBestTarget = pTarget;
			iBestUsed = maLastUsed[iInCache];
			miInCache = iInCache;
		}
		iInCache++;
	}
	return pBestTarget;
}
CMyImage *CMultiPageLoader::GetTargetOf(int iLoad)
{
	POSITION pos = mImages.GetHeadPosition();
	miInCache = 0;
	while (pos)
	{
		CMyImage *pTarget = mImages.GetNext(pos);
		if (maIndexLoaded[miInCache] == iLoad)
			return pTarget;
		miInCache++;
	}
	return NULL;
}
void CMultiPageLoader::Add(CMyImage*pTarget)
{
	pTarget->AddToFloatVector(mpSumPage->GetData());
	maLastUsed[miInCache] = miUpdate;
	mnLoaded++;
}
bool CMultiPageLoader::Load(CMyImage*pTarget, int iLoad)
{
	mpData->mpPageData = pTarget;

	if (!mpData->set_i(iLoad))
	{
		CMyWindows::MessBox("<CMultiPageLoader::UpdatePage> set_i failed","SW Error");
		return false;
	}
	if(!mpData->DoUpdatePageData())
	{
		CMyWindows::MessBox("<CMultiPageLoader::UpdatePage> DoUpdatePageData failed","SW Error");
		return false;
	}
	maIndexLoaded[miInCache] = iLoad;
	return true;
}
void CMultiPageLoader::PrintValues2Excel(FILE *pf, int nCols, int iLine, int iCol)
{
	POSITION pos = mImages.GetHeadPosition();
	while (pos)
	{
		CMyImage *pPage = mImages.GetNext(pos);

		if (mpData->mpTypedData)
		{
			CString s(mpData->mpTypedData->Value2s(pPage->GetDataStart(), nCols, iLine, iCol,
				mpData->mRescaleSlope, mpData->mRescaleIntercept));
			fprintf(pf,"%s\t",s);
		}
		else
			fprintf(pf,"=??\t");
	}
}
void CMultiPageLoader::PrintValues2Csv(CCsvFile &csv, int nCols, int iLine, int iCol)
{
	POSITION pos = mImages.GetHeadPosition();
	while (pos)
	{
		CMyImage *pPage = mImages.GetNext(pos);

		if (mpData->mpTypedData)
		{
			CString s(mpData->mpTypedData->Value2s(pPage->GetDataStart(), nCols, iLine, iCol,
				mpData->mRescaleSlope, mpData->mRescaleIntercept));
			csv.PrintCell(s);
		}
		else
			csv.PrintCell("=??");
	}
}
