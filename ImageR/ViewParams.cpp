// ViewParams.cpp : implementation file
//

#include "stdafx.h"
#include "ViewParams.h"
#include "Screens.h"
#include "WindowParams.h"

void SRunTimeWindowParams::Init (CWindowParams &params, bool bColorer)
{
	params.GetWindowMinMax(mMin, mMax, bColorer);
	float width = mMax - mMin;
    mFactor = 0;
    if (width > 0)
        mFactor = MAX_COLORS / width;
}
/*void SRunTimeWindowParams::DisplayRgb(COLORREF *pScreen, S_RGB *pData, int n)
{
    COLORREF *pS = pScreen;
    COLORREF cref = 0;
    S_RGB rgb;

    while (n--)
    {
        rgb = *pData++;
        cref = RGB(rgb.RGB[0], rgb.RGB[1], rgb.RGB[2]);
        pS = pScreen;
        *pScreen++ = cref;
    }
} */
