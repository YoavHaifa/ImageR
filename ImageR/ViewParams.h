#pragma once
#include "..\..\ImageRLib\MyImage.h"


#define MAX_CT_VALUE  5199  // 4095 + 1024 bias
#define MIN_CT_VALUE  -2000

//#define MAX_DATA_SIZE 0x20000000

#define IPC_LOAD_FROM_FILE 0
#define IPC_LOAD_FROM_MEM  1
#define IPV_ADD_FROM_MEM   2

#define IMAGE_PAGE_SIZE  512

#define MAX_ZOOM         8
#define IVM_UPDATE_WIND_PARAMS (WM_APP+1)

//#define ZOOM_DUPLICATE   0
//#define ZOOM_INTERPOLATE 1
#define MAX_COLORS 256


struct SCommonParams
{
  int nRowsInFile; // # of rows in the whole file
  int nColsInFile; // # of columns in each file
  int Increment; // Increment to skip between successive rows
  ULONGLONG OffsetBytes; // Offset from start of file
  EMyImageDataType eDataType; // Data Unit Type
};

struct SImageViewParams
{
  float exFactor;
  int   exBias;
  int   nRowsInPage;
};

struct SRunTimeWindowParams
{
    float mMin;
    float mMax;
    float mFactor;

    void Init (class CWindowParams &params, bool bColorer);

    //void DisplayRgb(COLORREF *pScreen, S_RGB *pData, int n);

    void SetGrayRaster (unsigned char *pGray, float *pData, int n)
    {
        while (n--)
        {
            float value = *pData++;
            if (value <= mMin)
                *pGray++ =  0;
            else if (value >= mMax)
                *pGray++ =  0xff;
            else
                *pGray++ = (unsigned char)((value - mMin) * mFactor);
        }
    }
};

struct SZoomParams
{
  int ZoomFactor;
  int ZoomModeObsolete;
};

struct SRoi
{
	int xStart;
	int yStart;
	int xLen;
	int yLen;
};

typedef enum EGraphicCommand
{
	GCOM_NONE,
	GCOM_REMOVE,
	GCOM_DISABLE,
	GCOM_ENABLE,
	GCOM_FIX_CENTER,
	GCOM_LAST
} EGraphicCommand;

