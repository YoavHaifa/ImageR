#include "StdAfx.h"
#include ".\readingposition.h"
#include "ReadingsFile.h"
#include "SlicesDialog.h"
#include "Screens.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\MyMath.h"

CReadingPosition::CReadingPosition(const char *zName, CReadingsFile *pFile)
: CPosition (zName)
, miSlice(0)
, miFFS(0)
, mpFile(pFile)
{
}
CReadingPosition::CReadingPosition(CXMLParseNode *pPositionXml, CReadingsFile *pFile)
: CPosition (pPositionXml)
, miSlice(0)
, miFFS(0)
, mpFile (pFile)
{
    pPositionXml->GetValue("slice", miSlice);
    pPositionXml->GetValue("ffs", miFFS);
}
CReadingPosition::~CReadingPosition(void)
{
}
bool CReadingPosition::LineLeft(void)
{
    mbLastChangeHorizontal = true;

    if (miSlice > 0)
    {
        miSlice--;
        OnChange();
        return true;
    }
    else if (miFFS > 0)
    {
        miSlice = mpFile->mnSlices - 1;
        miFFS--;
        OnChange();
        return true;
    }
    return false;
}
bool CReadingPosition::LineRight(void)
{
    mbLastChangeHorizontal = true;

    if (miSlice < mpFile->mnSlices - 1)
    {
        miSlice++;
        OnChange();
        return true;
    }
    else if (miFFS < mpFile->mnFFS - 1)
    {
        miSlice = 0;
        miFFS++;
        OnChange();
        return true;
    }
    return false;
}
bool CReadingPosition::First(void)
{
    if (mbLastChangeHorizontal)
    {
        if (miSlice > 0)
        {
            miSlice = 0;
            OnChange();
            return true;
        }
        else if (miFFS > 0)
        {
            miFFS = 0;
            OnChange();
            return true;
        }
    }
    mbLastChangeHorizontal = false;
    return CPosition::First();
}
bool CReadingPosition::Last(void)
{
    if (mbLastChangeHorizontal)
    {
        if (miSlice < mpFile->mnSlices - 1)
        {
            miSlice = mpFile->mnSlices - 1;
            OnChange();
            return true;
        }
        else if (miFFS < mpFile->mnFFS - 1)
        {
            miFFS = mpFile->mnFFS - 1;
            OnChange();
            return true;
        }
    }
    mbLastChangeHorizontal = false;
    return CPosition::Last();
}
CString CReadingPosition::GetText(void)
{
    char zBuf[128];
    int iSlice = GetUISliceIndex();
    sprintf_s (zBuf, 128, "%d/%d", iSlice, mpFile->mnSlices);

    if (mpFile->mnFFS > 1)
    {
        char zFS[64];
        sprintf_s (zFS, 64, " FS%d", miFFS + 1);
        strcat_s (zBuf, 128, zFS);
    }

    char zReading[64];
    sprintf_s (zReading, 64, " (first %d/%d)", 
        miCurrent * mpFile->mnLinesInPage + 1, mpFile->mnFrames);
    strcat_s (zBuf, 128, zReading);

    return CString (zBuf);
}
void CReadingPosition::OnViewGotopage(void)
{
    static int nRowsInPageSave = 580;

    CSlicesDialog Dlg;
    Dlg.mnSlice = mpFile->mnSlices;
    Dlg.miSlice = GetUISliceIndex();
    Dlg.mlHeaderBytes = mpFile->mlFanHeaderBytes;
    Dlg.mlSlice = mpFile->mSliceLen;
    Dlg.mnFocalSpots = mpFile->mnFFS;
    Dlg.miFocalSpot = miFFS + 1;
    Dlg.mbViewReadings = mpFile->mnLinesInPage;

    if (Dlg.DoModal() == IDOK)
	{
        mpFile->mnSlices = Dlg.mnSlice;
        SetSliceIndexFromUI(Dlg.miSlice);
        mpFile->mlFanHeaderBytes = Dlg.mlHeaderBytes;
        mpFile->mSliceLen = Dlg.mlSlice;
        mpFile->mnFFS = Dlg.mnFocalSpots;
        if (Dlg.miFocalSpot > 0 && ((int)Dlg.miFocalSpot <= (int)mpFile->mnFFS))
            miFFS = Dlg.miFocalSpot - 1;
        if ((int)Dlg.mbViewReadings != mpFile->mnLinesInPage)
        {
            //if (Dlg.mbViewReadings)
            //    mpFile->mnLinesInPage = mImViewParams.nRowsInPage;
            //else
             //   mpFile->mnLinesInPage = nRowsInPageSave;
        }
        mpFile->OnChange();
        OnChange();
		gScreens.Refresh();
	}
}
void CReadingPosition::SaveState(class CXMLDump & xmlDump)
{
    CPosition::SaveState(xmlDump);
    xmlDump.Write ("slice", (int)miSlice);
    xmlDump.Write ("ffs", (int)miFFS);
}

CString CReadingPosition::GetFileNameText(void)
{
    char zBuf[128];
    sprintf_s (zBuf, 128, ".slice_%02d.ffs_%d", miSlice, miFFS);
    return CString(zBuf);
}

int CReadingPosition::GetUISliceIndex(void)
{
    return mpFile->mnSlices - miSlice;
}

void CReadingPosition::SetSliceIndexFromUI(int iUISlice)
{
    miSlice = mpFile->mnSlices - iUISlice;
    if (miSlice < 0)
        miSlice = 0;
    else if (miSlice >= mpFile->mnSlices)
        miSlice = mpFile->mnSlices - 1;
}
