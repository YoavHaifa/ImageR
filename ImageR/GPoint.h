#pragma once
#include "atltypes.h"
#include "DisplayCoordinates.h"

class CGPoint :
    public CDRCoordinates
{
public:
    CGPoint(CDataCoordinates *pDataCoo, 
            class CDisplayRectangle *pDispRect,
            class CZoomPanParams *pZoomParams);
    ~CGPoint(void);

    // Data Members
    CDataCoordinates *mpData;
    class CZoomPanParams *mpZoomParams;
    bool mbActive;
    bool mbSelectedActive;
    COLORREF mColor;
    COLORREF mActiveColor;
    bool mbActivateAll;
	bool mbSelectable;

    // Static data
    static const int CURSOR_LEN = 10;
    static const int SELECT_SIZE = 20;

    // Methods
    //void SetData (float xData, float yData);
    void ComputeScreenPosition(CZoomPanParams *pZoomParams);
    void ComputeDataPosition(void);
    void DrawCursor(class CRect &rect, CDC &dc);
    //void SetDataPosition(CDataCoordinates & dataCoo);

    bool OnLButtonDown(UINT nFlags, CDRCoordinates &drCoo);
    bool OnLButtonUp(UINT nFlags, CDRCoordinates &drCoo);
    bool OnRButtonDblClk(UINT nFlags, CDRCoordinates &drCoo);

    bool ClipDrag(CSize rectSize, CSize &size);
    bool ClipDrag(CRect rect, CSize &size);

    bool IsNear(CDRCoordinates &drCoo);
    int DistanceFrom(CDRCoordinates &drCoo);
	void Print(FILE *pf);
	void PrintMore(FILE *pf);
	void Average(CDRCoordinates &drCoo1,CDRCoordinates &drCoo2);

    //operator CDataCoordinates(void) {return mData;}
};
