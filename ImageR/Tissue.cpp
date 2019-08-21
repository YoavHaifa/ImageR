#include "StdAfx.h"
#include "Tissue.h"
#include "DisplayRectangle.h"
#include "MetaData.h"
#include ".\tissue.h"

CTissue::CTissue(int nLines, int nCols, const char *zName)
: CMetaData (zName)
{
    mbTransparent = true;
    mColor = 0x00ffff;
    SetTypedDataHandler(DATA_TYPE_BOOL);
    mnLinesInPage = nLines;
    mnCols = nCols;
}

CTissue::~CTissue(void)
{
}
CTImage<bool> &CTissue::Raster(void)
{
    CTImage<bool> *pPage = (CTImage<bool> *)mpPageData;
    return *pPage;
}
void CTissue::AdjustColors(CDisplayRectangle &dr)
{
	CTImage<COLORREF> &colorRaster = dr.GetColorRaster();
    if (!colorRaster.IsOfShape(mnLinesInPage, mnCols))
        return;
	if (!mpPageData)
		return;

    int n = colorRaster.GetNPixels();
    COLORREF *pColors = colorRaster.GetData();

    bool *pFlags = (bool *)mpPageData->GetDataStart();
    int nFlags = mpPageData->GetNPixels();
    if (n > nFlags)
        n = nFlags;

    if (mbTransparent)
    {
        while (n--)
        {
            if (*pFlags++)
                *pColors &= mColor;
            pColors++;
        }
    }
    else
    {
        while (n--)
        {
            if (*pFlags++)
                *pColors = mColor;
            pColors++;
        }
    }
}
bool CTissue::SetShape(CDisplayRectangle &displayRect)
{
    mnLinesInPage = displayRect.bottom - displayRect.top;
    mnCols = displayRect.right - displayRect.left;
    return SetSize(mnLinesInPage, mnCols);
}
bool * CTissue::GetMaskStart(void)
{   
    if (!mpPageData)
        return NULL;
    return (bool *)mpPageData->GetDataStart();
}
