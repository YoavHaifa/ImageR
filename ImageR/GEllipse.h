#pragma once
#include "GRoi.h"

class CGEllipse :
    public CGRoi
{
public:
    CGEllipse(class CDisplayRectangle *pDispRect, const char *zName, 
        COLORREF color = GetDefColor(), int deltaMm = 30);
    CGEllipse(class CDisplayRectangle *pDispRect, class CXMLParseNode *pGRoiXml);
    virtual ~CGEllipse(void);

    virtual bool SpecialDraw (class CDisplayRectangle &displayRect, 
        CDC &dc, CGPoint *apPointsOnDR[]);

protected:
    virtual void AdjustOtherPointsToDrag (int iDragged, CGPoint *apPointsOnDR[]);
    virtual void SetRelativeClip(int iDragged, CRect &rClip, CGPoint *apPointsOnDR[]);
};
