//------------------------------------------------------------------------------------
//
//  GEllipse.cpp
//  ============
//  The ellipse is defined by closing rectangle, according to MFC "Ellipse" limitations.
//
//  Compute Ellipse Bitmap
//  ----------------------
//  For an ellipse around the center of the coordinates, he area is defined by:
//  A * X**2 + B * Y**2 <= 1
//
//  When:
//  A = 1 / (X0**2)
//  B = 1 / (Y0**2)
//  
#include "StdAfx.h"
#include ".\gellipse.h"
#include "DisplayRectangle.h"
#include "Screens.h"
#include "MetaData.h"
#include "Tissue.h"
#include "GraphicElementOnDR.h"
#include "DisplayGE.h"
#include "..\..\yUtils\MyMath.h"

CGEllipse::CGEllipse(class CDisplayRectangle *pDispRect, const char *zName, COLORREF color, int deltaMm)
: CGRoi(pDispRect, zName, color, deltaMm)
{
    CDataCoordinates dataCoo (pDispRect->GetSelectedDataCoo());
    InitEllipse(dataCoo, deltaMm / 2.0f);
	SetTextPoint(1);
}
CGEllipse::CGEllipse(class CDisplayRectangle *pDispRect, class CXMLParseNode *pGRoiXml)
: CGRoi(pDispRect, pGRoiXml)
{
    mbEllipse = true;
}
CGEllipse::~CGEllipse(void)
{
}
bool CGEllipse::SpecialDraw (CDisplayRectangle &displayRect, 
                             CDC &dc, CGPoint *apPointsOnDR[])
{
    CPen Pen(PS_SOLID, 1, mColor);
    dc.SelectObject(&Pen);
    dc.SelectStockObject(NULL_BRUSH);

    dc.Ellipse (displayRect.left + apPointsOnDR[0]->x, 
        displayRect.top + apPointsOnDR[1]->y, 
        displayRect.left + apPointsOnDR[2]->x, 
        displayRect.top + apPointsOnDR[3]->y);
    return true;
}
void CGEllipse::SetRelativeClip(int iDragged, CRect &rClip, CGPoint *apPointsOnDR[])
{
    if (iDragged == 0 || iDragged == 2)
    {
        rClip.top = apPointsOnDR[0]->y;
        rClip.bottom = apPointsOnDR[0]->y + 1;
        if (iDragged == 0)
            rClip.right = apPointsOnDR[2]->x - 5;
        else if (iDragged == 2)
            rClip.left = apPointsOnDR[0]->x + 5;
    }

    if (iDragged == 1 || iDragged == 3)
    {
        rClip.left = apPointsOnDR[1]->x;
        rClip.right = apPointsOnDR[1]->x + 1;
        if (iDragged == 1)
            rClip.bottom = apPointsOnDR[3]->y - 5;
        else if (iDragged == 3)
            rClip.top = apPointsOnDR[1]->y + 5;
    }
}
void CGEllipse::AdjustOtherPointsToDrag (int iDragged, CGPoint *apPointsOnDR[])
{
    // Keep Elliptic shape
    int iPolar = (iDragged + 2) % N_SEEN_POINTS;
    if (!apPointsOnDR[iPolar]->mbActive)
    {
        int iNext = (iDragged + 1) % N_SEEN_POINTS;
        int iPrev = (iDragged + N_SEEN_POINTS - 1) % N_SEEN_POINTS;

        if (iDragged & 1) // Upper or Lower
        {
            apPointsOnDR[iDragged]->x = apPointsOnDR[iPolar]->x;
            if (mbFixedCenter)
            {
                if (iDragged == 1)
                    apPointsOnDR[3]->y = apPointsOnDR[0]->y + (apPointsOnDR[0]->y - apPointsOnDR[1]->y);
                else
                    apPointsOnDR[1]->y = apPointsOnDR[0]->y - (apPointsOnDR[3]->y - apPointsOnDR[0]->y);
            }
            else
            {
                apPointsOnDR[iPrev]->y = (apPointsOnDR[iDragged]->y + apPointsOnDR[iPolar]->y) / 2;
                apPointsOnDR[iNext]->y = apPointsOnDR[iPrev]->y;
            }
            if (mbCircle)
            {
                int newRadius = abs(apPointsOnDR[iDragged]->y - apPointsOnDR[0]->y);
                apPointsOnDR[0]->x = apPointsOnDR[iDragged]->x - newRadius;
                apPointsOnDR[2]->x = apPointsOnDR[iDragged]->x + newRadius;
            }
        }
        else
        {
            apPointsOnDR[iDragged]->y = apPointsOnDR[iPolar]->y;
            if (mbFixedCenter)
            {
                if (iDragged == 0)
                    apPointsOnDR[2]->x = apPointsOnDR[1]->x + (apPointsOnDR[1]->x - apPointsOnDR[0]->x);
                else
                    apPointsOnDR[0]->x = apPointsOnDR[1]->x - (apPointsOnDR[2]->x - apPointsOnDR[1]->x);
            }
            else
            {
                apPointsOnDR[iPrev]->x = (apPointsOnDR[iDragged]->x + apPointsOnDR[iPolar]->x) / 2;
                apPointsOnDR[iNext]->x = apPointsOnDR[iPrev]->x;
            }
            if (mbCircle)
            {
                int newRadius = abs(apPointsOnDR[iDragged]->x - apPointsOnDR[1]->x);
                apPointsOnDR[1]->y = apPointsOnDR[iDragged]->y - newRadius;
                apPointsOnDR[3]->y = apPointsOnDR[iDragged]->y + newRadius;
            }
        }
        apPointsOnDR[iPrev]->ComputeDataPosition ();
        apPointsOnDR[iNext]->ComputeDataPosition ();
        if (mbFixedCenter)
            apPointsOnDR[iPolar]->ComputeDataPosition();
    }

    apPointsOnDR[4]->x = apPointsOnDR[1]->x;
    apPointsOnDR[4]->y = apPointsOnDR[0]->y;
    apPointsOnDR[4]->ComputeDataPosition ();
}
