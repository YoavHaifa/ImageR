#include "StdAfx.h"
#include "SharedMemFile.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyRegistry.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\ImageRLib\Position.h"
#include "..\..\ImageRLib\FileMapping.h"
#include "..\..\ImageRLib\TVolume.h"
#include "DisplayRectangle.h"
#include "Screens.h"

static bool umbDebugShouldUpdate = false;
bool CSharedMemFile::umbDebugUpdateColorer = false;
DWORD CSharedMemFile::umDebug = 0;
FILE *CSharedMemFile::umpfColorer = NULL;

CSharedMemFile::CSharedMemFile(CFileMapping *pMapping, CMetaData *pOnData)
: CMetaData (pMapping->msName)
, mpMapping(pMapping)
, mpHeader(pMapping->GetHeader())
, mpMem(NULL)
, mpaEntryHeaders(NULL)
, miLastRead(-1)
, miLastPageRead(-1)
, miLastRowRead(-1)
, mbLastValid(false)
, mbImageValid(true)
, mbBitVolume(false)
, mnLinesInMultiDataPage(0)
, mnLinesInFullData(0)
, msText(_T(""))
{
	static bool ubOnce = false;
	if (!ubOnce)
	{
		ubOnce = true;
		if (CMyRegistry::GetAppReg("DebugSharedMemory",umDebug))
		{
			umbDebugShouldUpdate = (umDebug & 2) != 0;
			umbDebugUpdateColorer = (umDebug & 4) != 0;
		}
	}
	//if (!upfLog)
	//	upfLog = CMyWindows::FOpenLogFile("CSharedMemFile");
	//DebugBreak();

    mbBitVolume = msName.Find("BitVolume") >= 0;

	CString sPositionName(mpHeader->mzPositionName);

	if (umDebug && !mpfLog)
	{
		mpfLog = CMyWindows::FOpenLogFile(Name());
		if (!mpfLog)
			mpfLog = CMyWindows::FOpenLogFile(Name() + "InViewer");
		if (mpfLog)
		{
			static int count = 0;
			fprintf (mpfLog, "<CSharedMemFile::CSharedMemFile> %d %p %s\n", ++count, this, (const char*)Name());
			fprintf (mpfLog, "mpHeader %p\n", mpHeader);
			fprintf (mpfLog, "Position Name <%s>\n", (const char*)sPositionName);
			if (mbBitVolume)
				fprintf (mpfLog, "Bit Volume\n");
		}
	}

	if (pOnData)
	{
		SetPosition(pOnData->GetPosition());
	}
	else if (sPositionName.IsEmpty())
	{
		CMetaData *pData = gScreens.GetCurrentData();
		if (pData)
		{
			SetPositionLike(pData);
			if (mpfLog)
				fprintf (mpfLog, "Set Position Like <%s>\n", (const char*)pData->Name());
		}
	}
	
	if (HasPosition())
	{
	}
	else if (sPositionName.IsEmpty() || sPositionName == "Shared")
	{
	    SetNewPosition("Shared", mpHeader->miFirst, mpHeader->miLast);
		if (mpfLog)
			fprintf (mpfLog, "Set Common Position %s\n", (const char*)sPositionName);
		CPosition *pPosition = GetPosition();
		if (pPosition)
		{
			CMetaDataBase *pUser = NULL; // pPosition->GetFirstUser();
			if (pUser)
			{
				if (mpfLog)
					fprintf (mpfLog, "First user of Position %p %s\n", pUser, (const char*)pUser->Name());
				if (pUser->IsMultiDataFile())
					mnLinesInMultiDataPage = pUser->GetNLinesInPage();
				else if (pUser == this)
				{
					int nLines = mpHeader->mnLines;
					if (mpfLog)
						fprintf (mpfLog, "First User is me, nLines %d\n", nLines);
					if (nLines > 512 && (pPosition->GetNFiles() < 2))
					{
						mnLinesInMultiDataPage = 512;
						int nPages = (nLines + (mnLinesInMultiDataPage - 1)) / mnLinesInMultiDataPage;
						SetLeafRange(0, nPages - 1);
					}
				}
				else if (pUser->GetClass() == GetClass())
				{
					CSharedMemFile *pOther = pUser->GetSharedMemFile();
					if (pOther)
					{
						mnLinesInMultiDataPage = pOther->mnLinesInMultiDataPage;
						if (mpfLog)
							fprintf (mpfLog, "First User same class, pOther %p %s\n", pOther, (const char*)pOther->Name());
					}
				}
				if (pUser->GetPosition(1))
				{
					CPosition *pSecPos = pUser->GetPosition(1);
					SetPosition(pSecPos,1);
				}
			}
		}
		if (mpfLog)
			fprintf (mpfLog, "Set Common Position <%s> (nLinesInMultiPage %d)\n", (const char*)sPositionName, mnLinesInMultiDataPage);
	}
	else if (sPositionName == "Shared2d" && mpHeader->mnImagesInRow > 1)
	{
		if (mpfLog)
			fprintf (mpfLog, "Set 2D Positions %s\n", (const char*)sPositionName);

		CPosition *pPosition = GetPositionByNameAndRange("Shared",
			mpHeader->miFirst, mpHeader->miFirst + mpHeader->mnImagesInRow - 1);
		SetPosition(pPosition);

		CPosition *pSecPos = GetPositionByNameAndRange("Shared2",
			0, max (0, mpHeader->mnPages / mpHeader->mnImagesInRow - 1));
		SetPosition(pSecPos,1);
		if (mpfLog)
			fprintf (mpfLog, "pSecPos %p %s\n", pSecPos, (const char*)pSecPos->Name());
	}
	else
	{
		CPosition *pPosition = GetPositionByNameAndRange(sPositionName,mpHeader->miFirst, mpHeader->miLast);
		SetPosition(pPosition);
	}
	CPosition *pPosition = GetPosition();
	if ((mpHeader->miLast > mpHeader->miFirst) && (pPosition->mStep == 1))
		pPosition->mStep = max (1, mpHeader->mStep);
    mColor = mpHeader->mColor;

    if (mbBitVolume)
    {
        mnCols = (mpHeader->mnCols - 2) * 32;
        mnLinesInPage = mpHeader->mnLines - 2;
		SetCtImage(false, true /*force*/);
		SetTypedDataHandler(DATA_TYPE_BOOL);
    }
    else
    {
        mnCols = mpHeader->mnCols;
		if (mnLinesInMultiDataPage)
		{
	        mnLinesInPage = mnLinesInMultiDataPage;
			mnLinesInFullData = mpHeader->mnLines;
		}
		else
	        mnLinesInPage = mpHeader->mnLines;
		SetCtImage(mpHeader->mbCtImage, true /*bForce*/);
        SetTypedDataHandler(mpHeader->mDataType);
    }


    mpaEntryHeaders = (CMyImage::SSharedEntryHeader *)(((char *)mpHeader) + mpHeader->mSpecialHeaderOffset);
    if (mpHeader->mbVolume)
    {
		//volatile bool bBreak = false;
		//for (int i = 1; !bBreak && i < 1000000; i++)
		//{
		//	Sleep(1);
		//}
        mpVolume = CVolume::GetNew(pMapping, false);
		if (mpVolume)
		{
	        CMyImage::SSharedMemHeader *pHeader = mpVolume->GetHeader();
		    if (pHeader)
			{
				mMmPerPixelPage = abs(pHeader->mSecondBedPos - pHeader->mFirstBedPos);
	        }
		}
    }
    else
        mpMem = ((char *)mpHeader) + mpHeader->mDataOffset;
    if (mpHeader && mpHeader->mMmPerPixelHeight != 0.0f)
        SetMmPerPixel(mpHeader->mMmPerPixelWidth, mpHeader->mMmPerPixelHeight);

	BuildTextList();
	if (mpfLog)
	{
		CPosition *pPos = GetPosition();
		if (pPos)
			fprintf(mpfLog, "Position %p %s\n", pPos, (const char*)pPos->Name());
		if (mpVolume)
			fprintf(mpfLog, "Volume %s\n", (const char*)mpVolume->Name());
		fprintf (mpfLog, "<CSharedMemFile::CSharedMemFile> End %s\n\n", (const char*)Name());
	}
}

CSharedMemFile::~CSharedMemFile(void)
{
    if (mpMapping)
        delete mpMapping;

	if (mpfLog)
	{
		fprintf(mpfLog, "<CSharedMemFile::~CSharedMemFile>\n");
		fclose(mpfLog);
		mpfLog = NULL;
	}
}
CString CSharedMemFile::GetNameFromHeader(void)
{
	if (!mpHeader)
		return CString();
	return CString(mpHeader->mzName);
}
CPosition *CSharedMemFile::GetPositionByNameAndRange(const char * zName, int iFirst, int iLast)
{
	CPosition *pPosition = NULL;
	CNamedObject *pObject = GetNamed(zName);
	if (pObject && (pObject->GetClass() == "CPosition"))
	{
		CPosition *pFoundPosition = (CPosition *) pObject;
		if (mpfLog)
			fprintf (mpfLog, "Get position by name <%s> found\n", zName);
		if (pFoundPosition->miFirst == iFirst && pFoundPosition->miLast == iLast)
			pPosition = pFoundPosition;
		else
		{
			if (mpfLog)
				fprintf (mpfLog, "Named position has different range [%d - %d] != [%d - %d]\n",
					pFoundPosition->miFirst, pFoundPosition->miLast, iFirst, iLast);
		}
	}
	if (!pPosition)
	{
		pPosition = new CPosition(zName);
		pPosition->SetLeafRange(iFirst, iLast);
	}
	return pPosition;
}
CString CSharedMemFile::GetWindowName(void)
{
    if (mpHeader && mpHeader->mzWindowName[0])
        return CString(mpHeader->mzWindowName);
    return CString("SharedWind");
}
CString CSharedMemFile::GetZoomName(void)
{
    if (mpHeader && mpHeader->mzZoomName[0])
        return CString(mpHeader->mzZoomName);
	return CMetaData::GetZoomName();
}
CString CSharedMemFile::GetCommand(void)
{
    if (mpHeader && mpHeader->mzCommand[0])
        return CString(mpHeader->mzCommand);
	return CString();
}
CString CSharedMemFile::GetDescription(void)
{
    if (mpHeader && mpHeader->mzDescription[0])
        return CString(mpHeader->mzDescription);
	return CString();
}
int CSharedMemFile::GetContourThreshold(void)
{
    if (mpHeader)
        return mpHeader->mContourThreshold;
	return 1;
}
bool CSharedMemFile::IsValid(void)
{
    if (mpHeader)
		if (mpHeader->mbSuspended)
			return false;

	return mbValid;
}
void CSharedMemFile::OnAssignDR (void)
{
	mpDR->mbDisableStateRestore = true;
}
bool CSharedMemFile::AllowColors(void)
{
    if (mpHeader)
		if (mpHeader->mbDisableColors)
			return false;
	return true;
}
bool CSharedMemFile::IsColorer(void)
{
	if (!mpHeader)
		return false;
	if (mpHeader->mColor != 0)
		return true;
	if (mpHeader->mDataType == DATA_TYPE_RGB)
		return true;
	if (mpHeader->mDataType == DATA_TYPE_COLORREF)
		return true;
	return false;
}
bool CSharedMemFile::DoUpdatePageData()
{
	static int count = 0;
	count++;

	int iRelativePage = GetRelativePosition();

	CPosition *pPosition = GetPosition();
	CPosition *pSecondPosition = GetPosition(1);
    if (mpVolume)
	{
		mpMem = mpVolume->GetPageRelativeVP(iRelativePage);
		//if (pSecondPosition)
		//	mpMem = mpVolume->GetPageVP2d(pSecondPosition->miCurrent, pPosition->miCurrent);
		//else
	    //    mpMem = mpVolume->GetPageVP(pPosition->miCurrent);
	}
    if (mpfLog)
    {
        fprintf (mpfLog, "\n<CSharedMemFile::DoUpdatePageData> %s %d\n", (const char*)msName, count);
        if (mbBitVolume)
            fprintf (mpfLog, "Bit Volume: miCurrent %d\n", pPosition->miCurrent);
		else if (mpVolume)
            fprintf (mpfLog, "Volume: miCurrent %d %d\n", 
			pPosition->miCurrent, pSecondPosition ? pSecondPosition->miCurrent : 0);

        fprintf (mpfLog, "<CSharedMemFile::DoUpdatePageData> %s mpMem %p\n", 
            mpVolume ? (const char*)mpVolume->msName : "", mpMem);
        fflush (mpfLog);
    }
    if (!mpMem || !mbImageValid)
    {
        if (mpfLog)
        {
            fprintf (mpfLog, "<CSharedMemFile::DoUpdatePageData> mpMem %p mbImageValid %d -> Zero\n",
                mpMem, (int)mbImageValid);
            fflush (mpfLog);
        }
        mbLastValid = false;
        mbImageValid = false;
        mpPageData->Zero();
        return true;
    }

    void *pResult = (unsigned short *)mpPageData->GetDataStart();
    if (mpfLog)
    {
        fprintf (mpfLog, "memcpy(pResult %p, mpMem %p, mpPageData->GetByteSize() %d)\n", 
            pResult, mpMem, mpPageData->GetByteSize());
        if (mpHeader)
            fprintf (mpfLog, "<CSharedMemFile::DoUpdatePageData> mSync.miWriteStart %d end %d\n",
                                    mpHeader->mSync.miWriteStart, mpHeader->mSync.miWriteEnd);
        fflush (mpfLog);
    }

	int iLastRead = 0;
    if (mpHeader)
    {
        if (mpHeader->mbVolume)
		{
            iLastRead = mpHeader->mSync.miWriteStart;
			if (mpVolume)
				mpVolume->StartReadRelative(iRelativePage);
		}
        else
		{
            iLastRead = mpHeader->mSync.miWriteEnd;
			// mpHeader->mSync.miReadStart = iLastRead;
		}
	}

    if (mbBitVolume)
        DecodeBitVolumeImage((bool *)pResult, (unsigned int *)mpMem);
    else if (mnLinesInMultiDataPage)
	{
		unsigned int iRelative = pPosition->GetCurrentRelative();
		unsigned int lineLen = mnCols * GetUnitSize();
		unsigned int pageSize = lineLen * mnLinesInMultiDataPage;
		unsigned int offset = pageSize * iRelative;

		unsigned int fullLength = mnLinesInFullData * lineLen;
		unsigned int nToCopy = mpPageData->GetByteSize();
		if (offset + nToCopy > fullLength)
		{
			nToCopy = fullLength - offset;
			mpPageData->Zero();
		}

		if (nToCopy > 0)
	        memcpy (pResult, (char *)mpMem + offset, nToCopy);
	}
	else
        memcpy (pResult, mpMem, mpPageData->GetByteSize());

    miLastPageRead = pPosition->miCurrent;
	if (pSecondPosition)
		miLastRowRead = pSecondPosition->miCurrent;
    if (mpHeader)
	{
		miLastRead = iLastRead;
        if (mpHeader->mbVolume && mpVolume)
			mpVolume->EndRead(pPosition->miCurrent);
	}

    if (mpfLog)
    {
        fprintf (mpfLog, "<CSharedMemFile::DoUpdatePageData> miLastPageRead %d %d (from %p)\n", 
            miLastPageRead, miLastRowRead, mpMem);
        if (mpHeader)
            fprintf (mpfLog, "<CSharedMemFile::DoUpdatePageData> mSync.miWriteStart %d end %d - iLastRead %d\n\n",
                            mpHeader->mSync.miWriteStart, mpHeader->mSync.miWriteEnd, miLastRead);
		double sum = mpPageData->Sum();
		fprintf (mpfLog, "<CSharedMemFile::DoUpdatePageData> sum %f\n", sum);
        fflush (mpfLog);
    }
    mbImageValid = true;
    mbLastValid = mbImageValid;
	BuildTextList();
    return true;
}
bool CSharedMemFile::ShouldUpdate(void)
{
    if (mbBitVolume)
    {
        if (mpfLog && umbDebugShouldUpdate)
            fprintf (mpfLog, "<CSharedMemFile::ShouldUpdate> Bit Volume %s\n", (const char*)msName);
    }
	if (mpHeader && mpfLog && (umDebug & 8))
	{
		fprintf (mpfLog, "<CSharedMemFile::ShouldUpdate> %s\n", (const char*)msName);
		mpHeader->Print(mpfLog);
	}

	CPosition *pPosition = GetPosition();
	CPosition *pSecondPosition = GetPosition(1);
    if (mpHeader && mpaEntryHeaders)
    {
        mbImageValid = true;
        if (mpVolume)
            mbImageValid = mpVolume->IsImageValid(pPosition->miCurrent);

        if (!mbLastValid && !mbImageValid)
        {
            if (mpfLog && umbDebugShouldUpdate)
                fprintf (mpfLog, "<CSharedMemFile::ShouldUpdate> mbLastValid %d mbImageValid %d --> false\n\n",
                    (int)mbLastValid, (int)mbImageValid);
            return false;
        }

        if ((miLastPageRead == pPosition->miCurrent)
            && (mbLastValid == mbImageValid)
            && (miLastRead == (int)mpHeader->mSync.miWriteEnd)
            && (miLastRead == (int)mpHeader->mSync.miWriteStart)
			&& (!pSecondPosition || (miLastRowRead == pSecondPosition->miCurrent)))
        {
            if (mpfLog && umbDebugShouldUpdate)
                fprintf (mpfLog, "<CSharedMemFile::ShouldUpdate> miLastPageRead %d miLastRead %d --> false\n\n",
                    miLastPageRead, miLastRead);
            return false;
        }
        if (mpfLog && umbDebugShouldUpdate)
            fprintf (mpfLog, "<CSharedMemFile::ShouldUpdate> mpaEntryHeaders, return true\n\n");
        return true;
    }
    return CMetaData::ShouldUpdate();
}
bool CSharedMemFile::UpdateColorer(CDisplayRectangle &displayRect)
{
	FILE *pfLog = mpfLog;
	if (umpfColorer)
		pfLog = umpfColorer;

    if (mbBitVolume)
    {
        if (pfLog)
            fprintf (pfLog, "<CSharedMemFile::UpdateColorer> %s\n", (const char*)msName);
    }
    CMetaData *pData = displayRect.GetCurrentData();

	if (!umpfColorer && umbDebugUpdateColorer)
	{
		umpfColorer = CMyWindows::FOpenLogFile("CSharedMemFile_UpdateColorer");
		pfLog = umpfColorer;
	}
	if (pfLog)
		fprintf (pfLog, "\n<CSharedMemFile::UpdateColorer> %s\n", (const char*)msName);

	CPosition *pPosition = GetPosition();
    if (pData->GetPosition() != pPosition)
	{
		if (pfLog)
		{
			fprintf (pfLog, "Not Same Pos <%s> != ", (const char*)pData->GetPosition()->Name());
			if (pPosition)
				fprintf (pfLog, "<%s>\n", (const char*)pPosition->Name());
			else
				fprintf (pfLog, "NULL\n");
		}
        return false;
	}

    if (!mbMmPerPixelSet)
        SetMmPerPixel(*pData);

    if (mpHeader->mbVolume)
    {
        if (((int)mpHeader->miFirst > pPosition->miCurrent) || 
                        ((int)mpHeader->miLast < pPosition->miCurrent))
		{
			if (pfLog)
				fprintf (pfLog, "Volume out of range miCurrent %d\n", pPosition->miCurrent);
            return false;
		}
    }
    else
    {
        if ((int)mpHeader->mIndex != pPosition->miCurrent)
		{
			if (pfLog)
				fprintf (pfLog, "Not same mpHeader->mIndex %d != %d pPosition->miCurrent\n",
					mpHeader->mIndex, pPosition->miCurrent);
            return false;
		}
		if (pfLog)
			fprintf (pfLog, "mpHeader->mIndex %d, lastWrite %d - lastRead %d\n",
				mpHeader->mIndex, mpHeader->mSync.miWriteEnd, miLastRead);
    }

    if (ShouldUpdate())
	{
		if (pfLog)
			fprintf (pfLog, "Call UpdatePageData\n");
        UpdatePageData();
		if (pfLog)
			fprintf (pfLog, "After <UpdatePageData> mpHeader->mIndex %d, lastWrite %d - lastRead %d\n\n",
				mpHeader->mIndex, mpHeader->mSync.miWriteEnd, miLastRead);
	}
	else
		if (pfLog)
			fprintf (pfLog, "No need to UpdatePageData\n");

    SetCommonZoom (pData->GetZoomParams());
	if (pfLog)
		fprintf (pfLog, "Done \n");
    return FillZoomedRaster(displayRect);
}

void CSharedMemFile::DecodeBitVolumeImage(bool * pImage, unsigned int * pBitImage)
{
    int nWordsInBitLine = mnCols / 32 + 2;
    mpPageData->Zero();
    int nOn = 0;
    for (int iLine = 0; iLine < mnLinesInPage; iLine++)
    {
        unsigned int *pBitLine = pBitImage + nWordsInBitLine * (iLine + 1);
        bool *pPageLine = pImage + iLine * mnCols;
        unsigned int bitsInWord = 31;
        unsigned int bitsWord = *pBitLine++;
        bitsWord = bitsWord >> 1;

        int iPix = 0;
        while (iPix < mnCols)
        {
            while (bitsWord && bitsInWord > 0)
            {
                if (bitsWord & 1)
                {
                    pPageLine[iPix] = true;
                    nOn++;
                }
                bitsWord = bitsWord >> 1;
                bitsInWord--;
                iPix++;
            }
            if (bitsInWord)
            {
                iPix += bitsInWord;
            }
            bitsWord = *pBitLine++;
            bitsInWord = 32;
        }
    }
	if (mpfLog)
		fprintf (mpfLog, "<DecodeBitVolumeImage> nOn %d\n", nOn);
}
void CSharedMemFile::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("SharedMemFile",xmlDump);
    CMetaData::SaveState(xmlDump);
    if (mDumpCount)
        return;

	xmlDump.Write("miLastRead", miLastRead);
	xmlDump.Write("miLastPageRead", miLastPageRead);
	xmlDump.Write("mbImageValid", mbImageValid);
	xmlDump.Write("mbLastValid", mbLastValid);
	xmlDump.Write("mbBitVolume", mbBitVolume);
	xmlDump.Write("mnLinesInMultiDataPage", mnLinesInMultiDataPage);
	xmlDump.Write("mnLinesInFullData", mnLinesInFullData);

	if (mpHeader)
		mpHeader->SaveState(xmlDump);
}
CString CSharedMemFile::GetPageDataText(int /*i*/)
{
	if (mpHeader && mpHeader->mzName[0])
		return CString(mpHeader->mzName);
	return CString("");
}
float CSharedMemFile::GetBias (void)
{
	return IsCtImage() ? CMyDicom::GetDefaultCtBias() : 0.0f;
}
bool CSharedMemFile::WantContextMenu(int &oiMenu)
{
	// Allow XML configuration to override anything in the SW!
	if (CMetaData::WantContextMenu(oiMenu)) 
		return true;

	if (mpHeader)
	{
		if (mpHeader->miContextMenu)
		{
			oiMenu = mpHeader->miContextMenu;
			return true;
		}
	}
	return false;
}
bool CSharedMemFile::GetColor(COLORREF &oColor)
{
	if (!mpHeader)
		return false;
	if (mpHeader->mColor == 0)
		return false;
	oColor = mpHeader->mColor;
	return true;
}
bool CSharedMemFile::BuildTextList(void)
{
	static bool bOnce = false;
	CString sText = mpHeader->GetText();
	if (bOnce)
	{
		if (sText == msText)
			return mDataTextList.GetSize() > 0;
		DeleteAllText();
	}

	if (mpHeader && mpHeader->mzName[0])
		AddDataText(mpHeader->mzName);

	msText = sText;
	while (sText.GetLength() > 0)
	{
		int iNewLine = sText.Find('\n');
		if (iNewLine > 0)
		{
			CString sLine(sText.Left(iNewLine));
			AddDataText(sLine, false); // not "data specific"...
			sText = sText.Right(sText.GetLength() - iNewLine - 1);
		}
		else if (iNewLine == 0)
		{
			sText = sText.Right(sText.GetLength() - 1);
		}
		else
		{
			AddDataText(sText, false); // not "data specific"...
			break;
		}
	}

	bOnce = true;
	return mDataTextList.GetSize() > 0;
}
