#include "StdAfx.h"
#include ".\cardiacsegmentation.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\MyImage\Position.h"

extern "C"
{
extern int uncompress(unsigned char *puBuff, unsigned long &dataLen, unsigned char *pcBuff, unsigned long cLen); 
}

CCardiacSegmentation::CCardiacSegmentation(const char *zName, CMyDicom *pDicom)
: CMetaFile(zName)
, mnSlices(0)
, mpData(NULL)
{
    mpDicom = pDicom;

    short *pDim = (short *)mpDicom->GetTagData(SEGMENTATION_DIMENSIONS_GROUP, 
        SEGMENTATION_DIMENSIONS_NUM);
    if (!pDim)
    {
		pDim = (short *)mpDicom->GetTagData(SEGMENTATION_DIMENSIONS_CCA4_GROUP, 
			SEGMENTATION_DIMENSIONS_CCA4_NUM);
		if (!pDim)
		{
	        MessageBox(NULL, "Failed to find segmentation dimensions", zName, MB_OK);
		    return;
		}
    }
    mnCols = pDim[0];
    mnLinesInPage = pDim[1];
    mnSlices = pDim[2];

    SetTypedDataHandler(DATA_TYPE_CHAR);
    SetNewPosition("CardSegPos", 1, mnSlices);

    unsigned int imageLen = mnCols * mnLinesInPage;
    unsigned int dataLen = imageLen * mnSlices;
    mpData = new unsigned char[dataLen];
    if (!mpData)
    {
        char zBuf[256];
        sprintf_s (zBuf, 256, "Failed to allocate memory %d * %d * %d => %d", mnCols, mnLinesInPage, mnSlices, dataLen);
        MessageBox(NULL, zBuf, zName, MB_OK);
    }

    unsigned int *pCompressed = (unsigned int *)mpDicom->GetTagData(CARDIAC_SEGMENTATION_GROUP, 
        CARDIAC_SEGMENTATION_NUM);
    if (!pCompressed)
    {
		pCompressed = (unsigned int *)mpDicom->GetTagData(CARDIAC_SEGMENTATION_CCA4_GROUP, 
			CARDIAC_SEGMENTATION_CCA4_NUM);
		if (!pCompressed)
		{
	        MessageBox(NULL, "Failed to find segmentation dimensions", zName, MB_OK);
			return;
		}
    }
    unsigned long compressedLen = pCompressed[0];
    unsigned long unComLen = dataLen;
    unsigned char *pCompressedChars = (unsigned char *)(pCompressed + 1);
    int rc = uncompress (mpData, unComLen, pCompressedChars, compressedLen);
    if (rc != 0)
    {
        char zBuf[256];
        sprintf_s (zBuf, 256, "<uncompress> ERROR %d", rc);
        MessageBox(NULL, zBuf, zName, MB_OK);
    }
    else
    {
        CMyWindows::PrintStatus("Uncompressing...");
        CString sOutName(msName);
        sOutName += ".char.uncompressed";
        FILE *pf = MyFOpen (sOutName, "wb");
        if (pf)
        {
            fwrite (mpData, sizeof(unsigned char), dataLen, pf);
            fclose (pf);
            MessageBox(NULL, sOutName, "Uncompressed File Ready", MB_OK);
        }
    }
}

CCardiacSegmentation::~CCardiacSegmentation(void)
{
    if (mpData)
        delete mpData;
}
bool CCardiacSegmentation::DoUpdatePageData(void)
{
    if (!mpData)
        return false;

    unsigned int pageLen = mnLinesInPage * mnCols;
	unsigned int offset = GetRelativePosition() * pageLen;

    unsigned char *pDest = (unsigned char *)mpPageData->GetDataStart();

    memcpy (pDest, mpData + offset, pageLen * sizeof(mpData[0]));
    return true;
}