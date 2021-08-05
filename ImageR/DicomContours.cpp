#include "StdAfx.h"
#include "DicomContours.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyDicomTagRef.h"


CDicomContours::CDicomContours(const char *zName, class CMyDicom *pDicom, class CXMLParseNode *pMetaDataXml)
	: CArchivesFile(zName, pDicom, pMetaDataXml)
	, mnContours(0)
	, mnPoints(0)
	, mX("X")
	, mY("Y")
	, mZ("Z")
	, mbFillValues(true)
	, mValueToFill(1024)
	, mDebug(15)
	, miLastFailedDicom(0)
	, miContour(0)
	, miImageLoaded(-1)
{
	mpfLog = CMyWindows::FOpenLogFile("CDicomContours");

    mMatrix = 512;
	mnCols = 512;
	mnLinesInPage = 512;

	VerifyImage();
}
CDicomContours::~CDicomContours(void)
{
	if (mpfLog)
	{
		fprintf(mpfLog, "\n<~CDicomContours>\n");
		fclose(mpfLog);
	}
}
bool CDicomContours::DoUpdatePageData(void)
{
	if (!VerifyImage())
		return false;

	if (mContoursNPointsTagsRefs.IsEmpty())
		return false;

	CPosition *pPos = mMultiPosition.GetPosition(1);
	if (pPos)
		miContour = pPos->miCurrent;
	else
		miContour = 0;

	CMyDicomTagRef *pRefN = mContoursNPointsTagsRefs.GetHead();
	CMyDicomTagRef *pRefPoints = mContoursDataTagsRefs.GetHead();

	
	POSITION posN = mContoursNPointsTagsRefs.GetHeadPosition();
	POSITION posData = mContoursDataTagsRefs.GetHeadPosition();
	int iPosContour = 0;
	while (posN && posData)
	{
		pRefN = mContoursNPointsTagsRefs.GetNext(posN);
		pRefPoints = mContoursDataTagsRefs.GetNext(posData);
		if (iPosContour == miContour)
			break;
		iPosContour++;
	}

	CString sn;
	if (!pRefN->GetText(sn))
		return false;
	mnPoints = atoi(sn);
	if (mnPoints < 1)
	{
		CMyWindows::MessBox("<CDicomContours::DoUpdatePageData> nPoints <= 0", "SW or DATA Error");
		return false;
	}

	mX.VerifySize(mnPoints);
	mY.VerifySize(mnPoints);
	mZ.VerifySize(mnPoints);

	mpChar = pRefPoints->GetDataStart();
	mpCharStart = mpChar;
	mpCharEnd = mpCharStart + pRefPoints->GetDataLength();
	float halfLines = mnLinesInPage / 2.0f;
	float halfCols = mnCols / 2.0f;

	mpPageData->Zero();
    unsigned short *pDest = (unsigned short *)mpPageData->GetDataStart();
	for (int i = 0; i < mnPoints; i++)
	{
		mX[i] = GetNextPointValue();
		mY[i] = GetNextPointValue();
		mZ[i] = GetNextPointValue();

		if (mbFillValues)
		{
			int iX = (int)(halfCols + mX[i] + 0.5f);
			int iY = (int)(halfLines + mY[i] + 0.5f);
			if (iX >= 0 && iX < mnCols)
			{
				if (iY >= 0 && iY < mnLinesInPage)
				{
					int offset = iY * mnCols + iX;
					pDest[offset] = mValueToFill;
				}
			}
		}
		if (mpfLog && ((mDebug & 0x100) == 0x100))
			fprintf(mpfLog,"%3d: %f %f %f\n", i, mX[i], mY[i], mZ[i]);
	}

	if (mpfLog)
	{
		mX.ShortPrint(mpfLog);
		mY.ShortPrint(mpfLog);
		mZ.ShortPrint(mpfLog);
		fprintf(mpfLog,"<DoUpdatePageData> nPoints %d\n", mnPoints);
		fprintf(mpfLog,"\n");
		fflush(mpfLog);
	}

	return true;
}
float CDicomContours::GetNextPointValue(void)
{
	CString s;
	//int iRead = 0;
	unsigned char nextChar = *mpChar;
	while (nextChar == ' ' || nextChar == '\\' || nextChar == '/')
	{
		mpChar++;
		nextChar = *mpChar;
	}
	float value = (float)atof((const char *)mpChar);
	while(isdigit(nextChar) || nextChar == '.' || nextChar == '-')
	{
		mpChar++;
		nextChar = *mpChar;	
	}
	return value;
}
bool CDicomContours::OnNewDicom(void)
{
	while (!mContoursDataTagsRefs.IsEmpty())
		mContoursDataTagsRefs.RemoveAll();
	while (!mContoursNPointsTagsRefs.IsEmpty())
		mContoursNPointsTagsRefs.RemoveAll();

	mnContours = mpDicom->GetListOfAllRefs(CONTOUR_DATA_GROUP, CONTOUR_DATA_NUM, mContoursDataTagsRefs);
	int nN = mpDicom->GetListOfAllRefs(CONTOUR_N_POINTS_GROUP, CONTOUR_N_POINTS_NUM, mContoursNPointsTagsRefs);
	if (nN != mnContours)
	{
		CMyWindows::MessBox("<CDicomContours> nNPoints != nContours", "SW or DATA Error");
	}
	if (mpfLog)
	{
		fprintf(mpfLog,"<CDicomContours> n %d\n", mnContours);
		fprintf(mpfLog,"\n");
	}
	CPosition *pPos = mMultiPosition.GetPosition(1);
	if (!pPos && mnContours > 1)
	{
		pPos = new CPosition("ContourSelect");
		pPos->SetLeafRange(0,mnContours-1);
		mMultiPosition.Add(pPos);
	}
	else
	{
		pPos->SetLeafRange(0,mnContours-1);
	}
	return true;
}
bool CDicomContours::VerifyImage(void)
{
	int iPosImage = GetCurrentPosition();
	if (iPosImage == miImageLoaded)
		return true;

    UpdateDicom();
	if (!mpDicom)
	{
		miLastFailedDicom = iPosImage;
		CMyWindows::MessBox("Faild to get Dicom data", Name());
		return false;
	}
	return OnNewDicom();
}

