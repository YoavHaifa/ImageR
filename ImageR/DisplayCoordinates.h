#pragma once
#include "atltypes.h"
#include "..\..\ImageRLib\DataCoordinates.h"

class CScreenCoordinates :
    public CCoordinates
{
public:
    CScreenCoordinates(int x, int y);
    CScreenCoordinates(CPoint point);
    CScreenCoordinates(CRect &rect, CPoint point);
    ~CScreenCoordinates(void);
};
class CDRCenteredCoordinates :
    public CCoordinates
{
public:
    CDRCenteredCoordinates(class CZoomPanParams *pParams, class CDataCoordinates &dataCoo);
    CDRCenteredCoordinates(const CDRCenteredCoordinates &other);
    //CDRCenteredCoordinates(class CDataCoordinates &dataCoo);
    CDRCenteredCoordinates(class CZoomPanParams *pParams, int setX, int setY);
    CDRCenteredCoordinates(class CZoomPanParams *pParams, CDRCenteredCoordinates &other);
    CDRCenteredCoordinates(class CZoomPanParams *pParams, const class CDRCoordinates &drCoo);
    ~CDRCenteredCoordinates(void);

    class CZoomPanParams *mpParams;

    void Update (const class CDataCoordinates &dataCoo, const class CZoomPanParams *pZoomParams);
    void UpdateData (class CDataCoordinates &dataCoo) const;
};
class CDRCoordinates :
    public CCoordinates
{
public:
    CDRCoordinates(class CDisplayRectangle &rect, CScreenCoordinates screenCoordinates);
    CDRCoordinates(class CDisplayRectangle &rect, 
                    class CZoomPanParams *pZoomParams, class CDataCoordinates &dataCoo);
    CDRCoordinates(class CDisplayRectangle &rect, CDRCenteredCoordinates &centeredCoo);
    CDRCoordinates(class CDisplayRectangle &rect, int setX, int setY);
    ~CDRCoordinates(void);

    void Update (CDRCenteredCoordinates &centeredCoo);
    //void Update (const class CDataCoordinates &dataCoo);
    void UpdateData (class CZoomPanParams *pZoomParams, class CDataCoordinates &dataCoo) const;
    void Update (class CDataCoordinates &dataCoo, class CZoomPanParams *pZoomParams);
    operator CDRCenteredCoordinates (void);
    class CDisplayRectangle *mpRect;
    bool mbOnRect;
    bool CheckOnRect(void);
};

