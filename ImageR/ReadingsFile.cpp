#include "StdAfx.h"
#include "ReadingsFile.h"
#include "ReadingPosition.h"
#include ".\readingsfile.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\NameGetDialog.h"
#include "..\..\yUtils\MyProgress.h"

bool CReadingsFile::umbPrintBedPosition = true;

bool CReadingsFile::umbEditConstantPitch = false;
int umnConstantPitchLearn = 1000;
bool CReadingsFile::umbEditSkipStart = true;
bool umbEditSkipKeepBedPos = false;
bool umbEditSkipShiftBedPos = true;
int umnReadingsToSkip = 19200;
const int MAX_BADPOS = 140000;

CReadingsFile::CReadingsFile(const char *zfName)
: CMetaFile (zfName)
, mSliceLenWithoutFooter(0)
, mlFanHeaderBytes (0)
, mnSlices (0)
, mnFFS (1)
, mSliceLen (0)
, mReadingSize (0)
, mnFrames(0)
, mFrameSize(0)
, mDetectorSize(sizeof(unsigned short)) // May Change!
, mnDetectors(0)
, miFirstReadingInCurrentPage(0)
{
}

CReadingsFile::~CReadingsFile(void)
{
}
ULONGLONG CReadingsFile::GetOffsetToReading(int iFrame)
{
    ULONGLONG offset = mOffsetBytes;

    // Offset to current page
    offset += (ULONGLONG)iFrame * mFrameSize;

    // Offset to second focal spot
	CReadingPosition *pPos = (CReadingPosition*)GetPosition();
    if (pPos->miFFS)
        offset += mReadingSize;

    return offset;
}
bool CReadingsFile::DoUpdatePageData(void)
{
    open();
	CReadingPosition *pPos = (CReadingPosition*)GetPosition();
	miFirstReadingInCurrentPage = pPos->miCurrent * mnLinesInPage;
    ULONGLONG offset = GetOffsetToReading(miFirstReadingInCurrentPage);

    // Offset to 1st slice
    offset += mlFanHeaderBytes;

    // Offset to slice
    offset += pPos->miSlice * mSliceLen * sizeof(short);

    unsigned char *pDest = (unsigned char *)mpPageData->GetDataStart();

    DWORD nBytesToRead = mDetectorSize * mnCols;
    bool bEOF = false;
    for (int iLine = 0; iLine < mnLinesInPage; iLine++)
    {
        if (!bEOF)
        {
            LARGE_INTEGER liOffset;
            liOffset.QuadPart = offset;
            BOOL bOK = SetFilePointerEx(mhFile, liOffset, NULL, FILE_BEGIN);
            //if (fseek (pfRead, (int)offset, SEEK_SET) != 0)
            if (!bOK)
                bEOF = true;
        }

        if (bEOF)
            memset (pDest, 0, nBytesToRead);
        else
        {
            DWORD nBytesRead;
	        ReadFile (mhFile, pDest, nBytesToRead, &nBytesRead, NULL);
            if (nBytesRead != nBytesToRead)
                bEOF = true;
        }

        pDest += nBytesToRead;
        offset += mFrameSize;
    }
    close();
    return true;
}

void CReadingsFile::ComputeReadingSize(void)
{
    mReadingSize = mlFanHeaderBytes + mnSlices * mSliceLen * mDetectorSize;
    mFrameSize = mReadingSize * mnFFS;
}
void CReadingsFile::ParsePosition(class CXMLParseNode *pPosXmlNode)
{
    if (!HasPosition())
        SetPosition(new CReadingPosition(pPosXmlNode, this));
}

void CReadingsFile::InitPosition(void)
{
    if (!HasPosition())
        SetPosition(new CReadingPosition("ReadingsPos", this));

    int nPages = 1;
    if (mnFrames > 0 && mnLinesInPage > 0)
        nPages = (mnFrames + mnLinesInPage - 1) / mnLinesInPage;
    SetLeafRange(0, nPages-1);
}
CSize CReadingsFile::DefaultImageSize(void)
{
	if (mnLinesInPage >= 128 && mnLinesInPage <= 1024 )
		return CSize (mSliceLen, mnLinesInPage + 20);

    return CSize (mSliceLen, min(580,mnFrames));
}
void CReadingsFile::PrintReadingHeaders(void)
{
	if (mlFanHeaderBytes != sizeof(S7200RHeader))
	{
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf),"Reading Header size %d != %d expected", mlFanHeaderBytes, sizeof(S7200RHeader));
		CMyWindows::MessBox(zBuf,"Reading Headers Can't be Printed");
		return;
	}
    char zName[256];
    sprintf_s (zName, 256, "%s.headers.xls", PrivateName());
    CLogFile logFile ("Readings", zName);
    if (!logFile)
        return;

	CLogFile logBedFile;
	if (umbPrintBedPosition)
	{
	    sprintf_s (zName, 256, "%s.headersBed.xls", PrivateName());
		logBedFile.Open ("Readings", zName);
	}

  //  int headerSize = sizeof(S7200RHeader);

  //  unsigned int nShorts = mlFanHeaderBytes / sizeof (short);
    unsigned long offsetToNextReadingHeader = mReadingSize - mlFanHeaderBytes;

    char *pBuf = new char[mlFanHeaderBytes];
    S7200RHeader *pHdr = (S7200RHeader *)pBuf;
    pHdr->PrintExcelHeader (logFile.mpf);
    open();
	short *paBedPos = new short[MAX_BADPOS];

	float firstBed = 0;
	float lastBed = 0;
	float firstPitch = 0;
	float maxPitch = -100000;
	float minPitch = 100000;
	float firstSpeed = 0;
	float maxSpeed = -100000;
	float minSpeed = 100000;
	int nGood = 0;
	int nReadingsPerRotation = 2400;
	float collimation = 0;
	float singleRotationTimeSec = 0;
	float bedSpeedMmSec = 0;
	float pitchFromMdu = 0;

	if (mpDicom)
	{
		nReadingsPerRotation = mpDicom->GetNReadingsPerRotation();
		singleRotationTimeSec = mpDicom->SingleRotationTimeSec();
		bedSpeedMmSec = mpDicom->BedSpeedMmSec();
		pitchFromMdu = mpDicom->Pitch();
	}
	if (singleRotationTimeSec == 0)
		singleRotationTimeSec = 1;

	sprintf_s (zName, 256, "%s.Rotation.xls", PrivateName());
    CLogFile logRotation ("Readings", zName);
	if (logRotation)
		fprintf(logRotation.mpf,"i \t rot \t bed \t deltaBed \t speed \t pitch \n");

	int posStep = 20 * mnFFS;
	short nPosSteps = (short)(mnFrames / 20);
	CMyProgress progress(0, nPosSteps);
	int nWarn = 0;
	int iRead = 0;
    while (1)
    {
        DWORD nBytesRead;
	    ReadFile (mhFile, pBuf, mlFanHeaderBytes, &nBytesRead, NULL);
        if (nBytesRead != mlFanHeaderBytes)
            break;
		if (pHdr->NumOfSlices == 0)
		{
			if (iRead < MAX_BADPOS)
				paBedPos[iRead] = 0;
			if (nWarn == 0)
			{
				nWarn++;
				bool bOK = CMyWindows::WarnBox("Invalid Header. Save them also?", "Warning");
				if (!bOK)
					break;
			}
		}
		else
		{
			if (iRead < MAX_BADPOS)
				paBedPos[iRead] = pHdr->TablePosition;
			if (nGood < 1)
				firstBed = pHdr->TablePosition / 10.0f;
			lastBed = pHdr->TablePosition / 10.0f;
			nGood++;
		}
        pHdr->PrintExcel (logFile.mpf);
		if (logBedFile)
			fprintf(logBedFile.mpf,"%d\n", pHdr->TablePosition);

        int iReading = pHdr->ReadingNumber;
        if (iRead % 1000 == 0)
        {
            char zBuf[128];
            sprintf_s (zBuf, 128, "Reading %d - %d", iRead, iReading);
            CMyWindows::PrintStatus(zBuf);
        }

		if (iRead > 0 && (iRead % nReadingsPerRotation == 0))
		{
			// Compute Pitch
			int iRotation = iRead / nReadingsPerRotation;
			int deltaBedPerRotation = pHdr->TablePosition - paBedPos[iRead - nReadingsPerRotation];
			float deltaBedPerRotationMm = (float)deltaBedPerRotation / 10;
			float pitch = deltaBedPerRotationMm;
			if (collimation > 0)
				pitch /= collimation;
			if (firstPitch == 0)
				firstPitch = pitch;
			if (pitch > maxPitch)
				maxPitch = pitch;
			if (pitch < minPitch)
				minPitch = pitch;

			// Compute Speed
			float speed = deltaBedPerRotationMm / singleRotationTimeSec;
			if (firstSpeed == 0)
				firstSpeed = speed;
			if (speed > maxSpeed)
				maxSpeed = speed;
			if (speed < minSpeed)
				minSpeed = speed;

			if (logRotation)
				fprintf(logRotation.mpf,"%d \t %d \t %.3f \t %.3f \t %.3f \t %.3f \n", 
					iRead, iRotation, (float)pHdr->TablePosition / 10, deltaBedPerRotationMm, speed, pitch);
		}
        //unsigned short *pShort = (unsigned short *)pBuf;
        //for (unsigned int i = 0; i < nShorts; i++)
        //    fprintf (logFile.mpf, "%u\t", pShort[i]);
        //fprintf (logFile.mpf, "\n");

        LARGE_INTEGER liOffset;
        liOffset.QuadPart = offsetToNextReadingHeader;
        BOOL bOK = SetFilePointerEx(mhFile, liOffset, NULL, FILE_CURRENT);
        if (!bOK)
            break;
		iRead++;
		progress.SetPos(iRead / posStep);
    }
	char zBuf[256];
	sprintf_s(zBuf,sizeof(zBuf),"%d good headers, Pos %.3f - %.3f (%+.3f)", nGood, firstBed, lastBed, lastBed - firstBed);
	fprintf(logFile.mpf,"\n%s\n",zBuf);
	CMyWindows::PrintStatus(zBuf);

	fprintf(logFile.mpf,"Pitch: First %.3f, min %.3f - max %.3f (mdu %.3f)\n",
		firstPitch, minPitch, maxPitch, pitchFromMdu);
	fprintf(logFile.mpf,"Speed: First %.3f, min %.3f - max %.3f (mdu %.3f)\n",
		firstSpeed, minSpeed, maxSpeed, bedSpeedMmSec);

	if (nReadingsPerRotation > 0)
		fprintf(logFile.mpf,"nReadingsPerRotation %d, nRotations %.2f\n", nReadingsPerRotation, (float)nGood / nReadingsPerRotation);

	delete paBedPos;
    delete pBuf;
	close();
    logFile.Close();
	logBedFile.Close();

    CMyWindows::OpenExcel(logFile.mName);
}
void CReadingsFile::EditRaw(void)
{
	if (mlFanHeaderBytes != sizeof(S7200RHeader))
	{
		char zBuf[128];
		sprintf_s(zBuf,sizeof(zBuf),"Reading Header size %d != %d expected", mlFanHeaderBytes, sizeof(S7200RHeader));
		CMyWindows::MessBox(zBuf,"Reading Headers should be S7200RHeader");
		return;
	}

	int nReadingsPerRotation = 2400;

	//float collimation = 0;
	if (mpDicom)
	{
		nReadingsPerRotation = mpDicom->GetNReadingsPerRotation();
		//mpDicom->GetCollimation(collimation);
	}

	if (umbEditSkipStart)
	{
		int nRotationsToSkip = 1;
		if (CNameGetDialog::GetIntValue("Enter number of rotations to skip", nRotationsToSkip))
			umnReadingsToSkip = nReadingsPerRotation * nRotationsToSkip;
	}
	int maxNReadingsToTake = 0;
	CNameGetDialog::GetIntValue("Enter maximum number of reading to take (0 is all)", maxNReadingsToTake);

	CFileName fName(Name());
	CString sPath(fName.Path());
	sPath += "\\Edited";
	if (!CMyWindows::VerifyDirectory(sPath))
		return;

	CString sNewName(sPath + "\\");
	sNewName += PrivateName();

	FILE *pfWrite = MyFOpenWithErrorBox(sNewName,"wb","Edited Raw");
	if (!pfWrite)
		return;

    char zLogName[256];
    sprintf_s (zLogName, 256, "%s.edit.log", PrivateName());
    CLogFile logFile ("Readings", zLogName);
    if (!logFile)
        return;
	fprintf(logFile.mpf,"<CReadingsFile::EditRaw>\n");
	fprintf(logFile.mpf,"FROM: %s\n", Name());
	fprintf(logFile.mpf,"TO: %s\n", sNewName);
	fprintf(logFile.mpf,"\n");

    char *pBuf = new char[mReadingSize];
    S7200RHeader *pHdr = (S7200RHeader *)pBuf;

	short *paBedPos = new short[MAX_BADPOS];
	int *paTime = new int[MAX_BADPOS];

    //pHdr->PrintExcelHeader (logFile.mpf);
    open();

	int origFirstBed = 0;
	int firstBedWritten = 0;
	float firstBed = 0;
	float lastBed = 0;
	int nGood = 0;
	int nReadingsRead = 0;
	int nReadingsWritten = 0;

	int posStep = 20 * mnFFS;
	short nPosSteps = (short)(mnFrames / 20);
	CMyProgress progress(0, nPosSteps);

	int nWarn = 0;
	int iRead = 0;
	float pitch = 0;
    while (1)
    {
        DWORD nBytesRead;
	    ReadFile (mhFile, pBuf, mReadingSize, &nBytesRead, NULL);
        if (nBytesRead != mReadingSize)
            break;
		iRead++;
		nReadingsRead++;

		if (pHdr->NumOfSlices == 0)
		{
			if (nWarn == 0)
			{
				nWarn++;
				bool bOK = CMyWindows::WarnBox("Invalid Header. Save them also?", "Warning");
				if (!bOK)
					break;
			}
		}
		else
		{
			if (iRead <= MAX_BADPOS)
			{
				paBedPos[iRead - 1] = pHdr->TablePosition;
				paTime[iRead - 1] = pHdr->Timestamp;
			}
			if (nGood < 1)
			{
				origFirstBed = pHdr->TablePosition;
				firstBed = pHdr->TablePosition / 10.0f;
			}
			lastBed = pHdr->TablePosition / 10.0f;
			nGood++;
			if ((iRead > 1) && (iRead % 1000 == 1))
			{
				if (logFile && (iRead - 1001 < MAX_BADPOS))
				{
					int deltaBed = pHdr->TablePosition - paBedPos[iRead - 1001];
					fprintf(logFile.mpf,"Pitch %.3f\n", (float)deltaBed / 1000);
				}
			}
		}

		if (umbEditConstantPitch)
		{
			if (iRead == umnConstantPitchLearn)
			{
				int lastBed = pHdr->TablePosition;
				pitch = ((float)lastBed - (float)origFirstBed) / (iRead - 1);
			}
			else if (iRead > umnConstantPitchLearn)
			{
				pHdr->TablePosition = (short)origFirstBed + (short)(pitch * (iRead - 1));
				paBedPos[iRead - 1] = pHdr->TablePosition;
			}
		}

		if (!umbEditSkipStart || iRead > umnReadingsToSkip)
		{
			if (iRead == umnReadingsToSkip + 1)
				firstBedWritten = pHdr->TablePosition;
			//if (!umbEditConstantPitch)
			//{
				if (umbEditSkipShiftBedPos)
					pHdr->TablePosition = (short)origFirstBed + (short)(pHdr->TablePosition - firstBedWritten);
				else if (umbEditSkipKeepBedPos && (iRead - umnReadingsToSkip - 1 < MAX_BADPOS))
					pHdr->TablePosition = paBedPos[iRead - umnReadingsToSkip - 1];
			//}
			pHdr->Timestamp = paTime[iRead - umnReadingsToSkip - 1];
			pHdr->ReadingNumber = nReadingsWritten;
			pHdr->ReadingNumber2 = nReadingsWritten;

			size_t nWritten = fwrite(pBuf, mReadingSize, 1, pfWrite);
			if (nWritten != 1)
			{
				CMyWindows::MessBox(sNewName,"Failed to write to file");
				break;
			}
			nReadingsWritten++;
		}
		//if (logBedFile)
		//	fprintf(logBedFile.mpf,"%d\n", pHdr->TablePosition);

        int iReading = pHdr->ReadingNumber;
        if ((iRead == 1) || (iRead % 1000 == 0))
        {
            char zBuf[128];
            sprintf_s (zBuf, 128, "Reading %d - %d", iRead, iReading);
            CMyWindows::PrintStatus(zBuf);
        }

		progress.SetPos(iRead / posStep);
		if ((maxNReadingsToTake > 0) && (nReadingsWritten >=maxNReadingsToTake))
			break;
    }

	if (umbEditSkipStart)
		fprintf(logFile.mpf,"\nSkipping: nRead %d, nWritten %d, firstBed read %d written %d\n",
		nReadingsRead, nReadingsWritten, origFirstBed, firstBedWritten);
	char zBuf[256];
	sprintf_s(zBuf,sizeof(zBuf),"%d readings, Pos %.3f - %.3f (%+.3f)", nGood, firstBed, lastBed, lastBed - firstBed);
	fprintf(logFile.mpf,"\n%s\n",zBuf);
	CMyWindows::PrintStatus(zBuf);

	delete paBedPos;
	delete paTime;
    delete pBuf;
	close();
	fclose(pfWrite);
    logFile.Close();
	//logBedFile.Close();

    CMyWindows::OpenTextFile(logFile.mName);
}
void CReadingsFile::DisplaySinus(float initialAngle, float maxDetectorDistanceFromCenter, float deltaAngle)
{
	float centralDetector = mnDetectors / 2.0f;
	CReadingPosition *pPos = (CReadingPosition*)GetPosition();

	int iMax = maxDetectorDistanceFromCenter < 0 ? 
		(int)(maxDetectorDistanceFromCenter - 0.5f) : (int)(maxDetectorDistanceFromCenter + 0.5f);

    char zName[256];
	sprintf_s (zName, 256, "%s_sl%d_fs%d_ang%.3f_max%d.xls", PrivateName(), 
		pPos->miSlice, pPos->miFFS, initialAngle, iMax);
    CLogFile logFile ("Readings_Sinus", zName);
    if (!logFile)
        return;

	CLogFile log;
	

    unsigned long offsetToNextReadingHeader = mReadingSize;
	if (mnFFS > 0)
		offsetToNextReadingHeader = mReadingSize * mnFFS;

	unsigned long offsetToSliceInReading = mlFanHeaderBytes + pPos->miSlice * mSliceLen * sizeof(short);

    char *pBuf = new char[mlFanHeaderBytes];
    S7200RHeader *pHdr = (S7200RHeader *)pBuf;
	short value;

    open();

	int iRead = 0;
    LARGE_INTEGER liOffset;
	liOffset.QuadPart = GetOffsetToReading(0);

	if (log)
	{
		fprintf(log.mpf,"maxDetDistCenter    %f\n", maxDetectorDistanceFromCenter);
		fprintf(log.mpf,"initialAngle        %f\n", initialAngle);
		fprintf(log.mpf,"\n");
		fprintf(log.mpf,"mnDetectors         %d\n", mnDetectors);
		fprintf(log.mpf,"mnSlices            %d\n", mnSlices);
		fprintf(log.mpf,"mReadingSize        %d\n", mReadingSize);
		fprintf(log.mpf,"mnFFS               %d\n", mnFFS);
		fprintf(log.mpf,"offsetToNextReading %d\n", offsetToNextReadingHeader);
		unsigned long len = (unsigned long)CMyWindows::GetFileLength(msName);
		fprintf(log.mpf,"\n");
		fprintf(log.mpf,"File %s\n", msName);
		fprintf(log.mpf,"len                 %d\n", len);
		fprintf(log.mpf,"nFarmes             %d\n", len / offsetToNextReadingHeader);
		fprintf(log.mpf,"\n");
	}

    while (1)
    {
        BOOL bOK = SetFilePointerEx(mhFile, liOffset, NULL, FILE_BEGIN);
        if (!bOK)
		{
			if (log)
				fprintf(log.mpf,"SetFilePointerEx 1 failed %d\n", (int)liOffset.QuadPart);
            break;
		}

        DWORD nBytesRead;
	    ReadFile (mhFile, pBuf, mlFanHeaderBytes, &nBytesRead, NULL);
        if (nBytesRead != mlFanHeaderBytes)
		{
			if (log)
				fprintf(log.mpf,"ReadFile 1 failed, nRead %d / %d\n", (int)nBytesRead, mlFanHeaderBytes);
            break;
		}

        int iReading = pHdr->ReadingNumber;
        if (iRead % 1000 == 0)
        {
            char zBuf[128];
            sprintf_s (zBuf, 128, "Reading %d - %d", iRead, iReading);
            CMyWindows::PrintStatus(zBuf);
        }

		LARGE_INTEGER liOffsetValue = liOffset;
		liOffsetValue.QuadPart += offsetToSliceInReading;

		float angle = initialAngle + iRead * deltaAngle;
		float angSin = sin(angle);
		float xForLine = centralDetector + angSin * maxDetectorDistanceFromCenter;
		int iDet = (int)(xForLine + 0.5f);
		liOffsetValue.QuadPart += iDet * sizeof(short);

        bOK = SetFilePointerEx(mhFile, liOffsetValue, NULL, FILE_BEGIN);
        if (!bOK)
		{
			if (log)
				fprintf(log.mpf,"SetFilePointerEx 2 failed %d\n", (int)liOffsetValue.QuadPart);
            break;
		}

	    ReadFile (mhFile, &value, sizeof(value), &nBytesRead, NULL);
        if (nBytesRead != sizeof(value))
		{
			if (log)
				fprintf(log.mpf,"ReadFile 2 failed, nRead %d / %d\n", (int)nBytesRead, sizeof(value));
			break;
		}

		fprintf(logFile.mpf,"%d \t %d \t %d ", iRead, iReading, value);
		if (iRead == 1)
			fprintf(logFile.mpf,"\t max dist \t %f \t detectors", maxDetectorDistanceFromCenter);
		else if (iRead == 2)
			fprintf(logFile.mpf,"\t init angle \t %f ", initialAngle);
		fprintf(logFile.mpf,"\n");
		if (log)
			fprintf(log.mpf,"%u  -  %d \t %d \t %d \n", (unsigned int)liOffset.QuadPart, iRead, iReading, value);

        liOffset.QuadPart += offsetToNextReadingHeader;
		iRead++;
	}
	delete pBuf;
	close();
    logFile.Close();
    CMyWindows::OpenExcel(logFile.mName);
}
void CReadingsFile::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("ReadingsFile",xmlDump);
    CMetaFile::SaveState(xmlDump);
    xmlDump.Write("n_slices", (unsigned int)mnSlices);
    xmlDump.Write("n_FS", (unsigned int)mnFFS);
    xmlDump.Write("slice_len", (unsigned int)mSliceLen);
    xmlDump.Write("slice_le_without_footer", (unsigned int)mSliceLenWithoutFooter);
    xmlDump.Write("n_fan_header_bytes", (unsigned int)mlFanHeaderBytes);
    xmlDump.Write("reading_size", (unsigned int)mReadingSize);
    xmlDump.Write("frame_size", (unsigned int)mFrameSize);
    xmlDump.Write("detector_size", (unsigned int)mDetectorSize);
    xmlDump.Write("n_frames", (unsigned int)mnFrames);
}

