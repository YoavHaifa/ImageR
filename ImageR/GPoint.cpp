#include "StdAfx.h"
#include ".\gpoint.h"
#include "MetaData.h"
#include "DisplayRectangle.h"

CGPoint::CGPoint(CDataCoordinates *pDataCoo, CDisplayRectangle *pDispRect,
            CZoomPanParams *pZoomParams)
: CDRCoordinates (*pDispRect, 0,0)
, mbActive(false)
, mColor (0xffffff)
, mActiveColor (0xffa0a0)
, mbSelectedActive(false)
, mbActivateAll(false)
, mpData (pDataCoo)
, mpZoomParams(pZoomParams)
, mbSelectable(true)
{
}
CGPoint::~CGPoint(void)
{
}
int CGPoint::DistanceFrom(CDRCoordinates &drCoo)
{
    int distanceX = (drCoo.x < x) ? x - drCoo.x : drCoo.x - x;
    int distanceY = (drCoo.y < y) ? y - drCoo.y : drCoo.y - y;
    if (distanceX > distanceY)
        return distanceX;
    return distanceY;
}
bool CGPoint::IsNear(CDRCoordinates &drCoo)
{
    if (!mbOnRect)
        return false;
    if (drCoo.x < x - SELECT_SIZE || drCoo.x > x + SELECT_SIZE)
        return false;
    if (drCoo.y < y - SELECT_SIZE || drCoo.y > y + SELECT_SIZE)
        return false;
    return true;
}

bool CGPoint::OnLButtonDown(UINT , CDRCoordinates &drCoo)
{
    if (!mbSelectable || mbActive || !IsNear(drCoo))
        return false;
    mbActive = true;
    return true;
}
bool CGPoint::OnLButtonUp(UINT , CDRCoordinates &)
{
    if (!mbActive || mbSelectedActive)
        return false;
    mbActive = false;
    return true;
}
bool CGPoint::OnRButtonDblClk(UINT , CDRCoordinates &drCoo)
{
    if (!IsNear(drCoo))
        return false;
    mbSelectedActive = !mbSelectedActive;
    mbActive = mbSelectedActive;
    return true;
}

bool CGPoint::ClipDrag(CSize rectSize, CSize &size)
{
    CPoint target ((CPoint)*this + size);

    if (target.x >= rectSize.cx)
        size.cx -= target.x - rectSize.cx + 1;
    else if (target.x < 0)
        size.cx -= target.x;

    if (target.y >= rectSize.cy)
        size.cy -= target.y - rectSize.cy + 1;
    else if (target.y < 0)
        size.cy -= target.y;

    return (size.cy ||size.cx);
}
bool CGPoint::ClipDrag(CRect rect, CSize &size)
{
    CPoint target ((CPoint)*this + size);

    if (target.x >= rect.right)
        size.cx -= target.x - rect.right + 1;
    else if (target.x < rect.left)
        size.cx += rect.left - target.x;

    if (target.y >= rect.bottom)
        size.cy -= target.y - rect.bottom + 1;
    else if (target.y < rect.top)
        size.cy += rect.top - target.y;

    return (size.cy ||size.cx);
}

void CGPoint::ComputeScreenPosition(CZoomPanParams *pZoomParams)
{
    Update (*mpData, pZoomParams);
}
void CGPoint::ComputeDataPosition()
{
    UpdateData(mpZoomParams, *mpData);
}

void CGPoint::DrawCursor(class CRect &rect, CDC &dc)
{
    CScreenCoordinates screenCoo (rect, *this);
    if (!PtInRect(&rect,screenCoo))
        return;

    CPen Pen(PS_SOLID, 1, mbActive ? mActiveColor : mColor);
    dc.SelectObject(&Pen);

	POINT aPoints[2];

    aPoints[0].x = screenCoo.x - CURSOR_LEN;
    aPoints[0].y = screenCoo.y;

	aPoints[1].x = screenCoo.x + CURSOR_LEN;
    aPoints[1].y = screenCoo.y;

    dc.Polyline (aPoints, 2);

    aPoints[0].x = screenCoo.x;
    aPoints[0].y = screenCoo.y - CURSOR_LEN;

	aPoints[1].x = screenCoo.x;
    aPoints[1].y = screenCoo.y + CURSOR_LEN;

    dc.Polyline (aPoints, 2);
}
void CGPoint::Print(FILE *pf)
{
	fprintf (pf, "[x %d y %d]\n", x, y);
}
void CGPoint::PrintMore(FILE *pf)
{
	fprintf (pf, "[x %d y %d] ", x, y);
	fprintf (pf, "%s\n", mbSelectable ? "Selectable" : "Fixed");
	fprintf (pf, "\n");
}
void CGPoint::Average(CDRCoordinates &drCoo1,CDRCoordinates &drCoo2)
{
    x = (drCoo1.x + drCoo2.x) / 2;
    y = (drCoo1.y + drCoo2.y) / 2;
    ComputeDataPosition ();
}
