#include "StdAfx.h"
#include ".\displaycoordinates.h"
#include "MetaData.h"
#include "DisplayRectangle.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\ImageRLib\MyImage.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CScreenCoordinates::CScreenCoordinates(int setX, int setY)
: CCoordinates (setX, setY)
{
}
CScreenCoordinates::CScreenCoordinates(CPoint point)
: CCoordinates (point)
{
}
CScreenCoordinates::CScreenCoordinates(CRect &rect, CPoint point)
{
    x = rect.left + point.x;
    y = rect.top + point.y;
}
CScreenCoordinates::~CScreenCoordinates(void)
{
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CDRCoordinates::CDRCoordinates(CDisplayRectangle &rect, CScreenCoordinates screenCoordinates)
: mpRect (&rect)
, mbOnRect(false)
{
    x = screenCoordinates.x - rect.left;
    y = screenCoordinates.y - rect.top;
    CheckOnRect();
}
CDRCoordinates::CDRCoordinates(CDisplayRectangle &rect, CDRCenteredCoordinates &centeredCoo)
: mpRect (&rect)
{
    Update (centeredCoo);
}
CDRCoordinates::CDRCoordinates(class CDisplayRectangle &rect, 
                    class CZoomPanParams *pZoomParams, CDataCoordinates &dataCoo)
: mpRect (&rect)
{
    CDRCenteredCoordinates centeredCoo (pZoomParams, dataCoo);
    Update(centeredCoo);
}
CDRCoordinates::CDRCoordinates(CDisplayRectangle &rect, int setX, int setY)
: mpRect (&rect)
, CCoordinates (setX, setY)
{
    CheckOnRect();
}
CDRCoordinates::~CDRCoordinates()
{
}

void CDRCoordinates::Update (CDRCenteredCoordinates &centeredCoo)
{
    if (!mpRect)
        return;
    x = centeredCoo.x + (mpRect->right - mpRect->left) / 2;
    y = centeredCoo.y + (mpRect->bottom - mpRect->top) / 2;
    CheckOnRect();
}
//void CDRCoordinates::Update (const CDataCoordinates &dataCoo)
//{
//    CDRCenteredCoordinates centeredCoo (dataCoo);
//    Update(centeredCoo);
//}
void CDRCoordinates::Update (CDataCoordinates &dataCoo, CZoomPanParams *pZoomParams)
{
    CDRCenteredCoordinates centeredCoo (pZoomParams, dataCoo);
    Update(centeredCoo);
}
void CDRCoordinates::UpdateData (class CZoomPanParams *pZoomParams, class CDataCoordinates &dataCoo) const
{
    CDRCenteredCoordinates centeredCoo (pZoomParams, *this);
    centeredCoo.UpdateData(dataCoo);
}
 CDRCoordinates::operator CDRCenteredCoordinates (void)
{
    if (!mpRect)
	{
        CDRCenteredCoordinates coo(NULL, 0,0);
        return coo;
	}

	CDRCenteredCoordinates coo(mpRect->GetZoomParams(), x - (mpRect->right - mpRect->left) / 2, 
        y - (mpRect->bottom - mpRect->top) / 2);
    return coo;
}
bool CDRCoordinates::CheckOnRect(void)
{
    if (!mpRect)
        mbOnRect = false;
    else if (x < 0 || y < 0)
        mbOnRect = false;
    else if (x >= (mpRect->right - mpRect->left))
        mbOnRect = false;
    else if (y >= (mpRect->bottom - mpRect->top))
        mbOnRect = false;
    else
        mbOnRect = true;

    return mbOnRect;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CDRCenteredCoordinates::CDRCenteredCoordinates(CZoomPanParams *pParams, int setX, int setY)
: CCoordinates (setX, setY)
, mpParams (pParams)
{
}
CDRCenteredCoordinates::CDRCenteredCoordinates(CZoomPanParams *pParams, CDRCenteredCoordinates &other)
: CCoordinates (other.x, other.y)
, mpParams (pParams)
{
}
CDRCenteredCoordinates::CDRCenteredCoordinates(CZoomPanParams *pParams, const CDRCoordinates &drCoo)
: CCoordinates (drCoo.x, drCoo.y)
, mpParams (pParams)
{
    if (drCoo.mpRect)
    {
        x -= (drCoo.mpRect->right - drCoo.mpRect->left) / 2;
        y -= (drCoo.mpRect->bottom - drCoo.mpRect->top) / 2;
    }
}
CDRCenteredCoordinates::CDRCenteredCoordinates(CZoomPanParams *pParams, CDataCoordinates &dataCoo)
: mpParams (pParams)
{
    Update (dataCoo, mpParams);
}
CDRCenteredCoordinates::CDRCenteredCoordinates(const CDRCenteredCoordinates &other)
: CCoordinates (other)
, mpParams (other.mpParams)
{
}
//CDRCenteredCoordinates::CDRCenteredCoordinates(CDataCoordinates &dataCoo)
//: mpParams (dataCoo.mpData->GetZoomParams())
//{
//    Update (dataCoo, mpParams);
//}
CDRCenteredCoordinates::~CDRCenteredCoordinates(void)
{
}
void CDRCenteredCoordinates::Update (const CDataCoordinates &dataCoo, 
                                     const CZoomPanParams *pZoomParams)
{
    if (!dataCoo.mpImage)
        return;
    CFloatCoordinates dataCenter;
    dataCenter = dataCoo.mpImage->GetCenter();
    float shiftX = pZoomParams->XShiftMM() / dataCoo.mpImage->MmPerPixelWidth();
    float shiftY = pZoomParams->YShiftMM() / dataCoo.mpImage->MmPerPixelHeight();
    float centeredX = dataCoo.fx - dataCenter.fx - shiftX;
    float centeredY = dataCoo.fy - dataCenter.fy - shiftY;

    float MmPerPixScreen = pZoomParams->MmPerPixel();
    float fx = centeredX * dataCoo.mpImage->MmPerPixelWidth() / MmPerPixScreen;
    float fy = centeredY * dataCoo.mpImage->MmPerPixelHeight() / MmPerPixScreen;
    if (fx < 0)
        x = (LONG)(fx - 0.5f);
    else
        x = (LONG)(fx + 0.5f);
    if (fy < 0)
        y = (LONG)(fy - 0.5f);
    else
        y = (LONG)(fy + 0.5f);
}
void CDRCenteredCoordinates::UpdateData (CDataCoordinates &dataCoo) const
{
    if (!dataCoo.mpImage)
        return;
    float MmPerPixScreen = mpParams->MmPerPixel();
    float drfx = x * MmPerPixScreen / dataCoo.mpImage->MmPerPixelWidth();
    float drfy = y * MmPerPixScreen / dataCoo.mpImage->MmPerPixelHeight();

    float shiftX = mpParams->XShiftMM() / dataCoo.mpImage->MmPerPixelWidth();
    float shiftY = mpParams->YShiftMM() / dataCoo.mpImage->MmPerPixelHeight();

    CFloatCoordinates dataCenter;
    dataCenter = dataCoo.mpImage->GetCenter();
    dataCoo.fx = drfx + dataCenter.fx + shiftX;
    dataCoo.fy = drfy + dataCenter.fy + shiftY;
    dataCoo.CheckOnRaster();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*
CDataCoordinates::CDataCoordinates(CMetaData &data, CDRCoordinates &drCoo)
: mpData (&data)
{
    Update (drCoo);
} */

/*CDataCoordinates::CDataCoordinates(CMetaData &data, CDRCenteredCoordinates drCenteredCoo)
:mpData (&data)
{
    Update (drCenteredCoo);
} */
/*
CDataCoordinates::CDataCoordinates(class CMetaData &data, float setX, float setY)
: CFloatCoordinates (setX,setY)
, mpData (&data)
{
    CheckOnRaster();
}

CDataCoordinates::CDataCoordinates(CMetaData &data)
:mpData (&data)
{
    fx = 0;
    fy = 0;
    CheckOnRaster();
}
CDataCoordinates::CDataCoordinates(void)
: mpData (NULL)
, mbOnRaster (false)
{
}
CDataCoordinates::CDataCoordinates(CDataCoordinates &other)
: CFloatCoordinates(other)
, mpData (other.mpData)
, mbOnRaster (other.mbOnRaster)
{
}
CDataCoordinates::~CDataCoordinates(void)
{
};
void CDataCoordinates::Update (CMetaData *pData, CDRCoordinates &drCoo)
{
    mpData = pData;
    Update (drCoo);
};
void CDataCoordinates::Update (CDRCoordinates &drCoo)
{
    CDRCenteredCoordinates centeredCoo (drCoo.mpRect->GetZoomParams(), drCoo);
    Update (centeredCoo);
}
void CDataCoordinates::Update (CDRCenteredCoordinates &drCenteredCoo)
{
    float MmPerPixScreen = drCenteredCoo.mpParams->mMmPerPixel;
    float drfx = drCenteredCoo.x * MmPerPixScreen / mpData->MmPerPixelWidth();
    float drfy = drCenteredCoo.y * MmPerPixScreen / mpData->MmPerPixelHeight();

    float shiftX = drCenteredCoo.mpParams->mXShiftMM / mpData->MmPerPixelWidth();
    float shiftY = drCenteredCoo.mpParams->mYShiftMM / mpData->MmPerPixelHeight();

    CFloatCoordinates dataCenter (mpData->GetCenter());
    fx = drfx + dataCenter.fx + shiftX;
    fy = drfy + dataCenter.fy + shiftY;
    CheckOnRaster();
}
bool CDataCoordinates::CheckOnRaster(void)
{
    if (!mpData || !mpData->HasPageData())
        mbOnRaster = false;
    else
    {
        if (fx < 0.0f || fy < 0.0f)
            mbOnRaster = false;
        else if (fx > mpData->GetNCols() - 1)
            mbOnRaster = false;
        else if (fy > mpData->GetNLinesInPage() - 1)
            mbOnRaster = false;
        else
            mbOnRaster = true;
    }
    return mbOnRaster;
}
CDataCoordinates &CDataCoordinates::operator = (CDataCoordinates &other)
{
    fx = other.fx;
    fy = other.fy;
    mpData = other.mpData;
    mbOnRaster = other.mbOnRaster;
    return *this;
}
void CDataCoordinates::SaveState(class CXMLDump & xmlDump)
{
    CXMLDumpScope graph("data_coo",xmlDump);
    xmlDump.Write ("data_x", fx);
    xmlDump.Write ("data_y", fy);
}
void CDataCoordinates::InitFromXml(CXMLParseNode *pPointXml)
{
    pPointXml->GetValue ("data_x", fx);
    pPointXml->GetValue ("data_y", fy);
}
*/