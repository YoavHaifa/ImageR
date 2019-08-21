#pragma once
#include "..\..\ImageRLib\GraphicElement.h"

class CGGraph :
    public CGraphicElement
{
public:
    CGGraph(class CDisplayRectangle *pDispRect, class CXMLParseNode *pXml);
    virtual ~CGGraph(void);

    virtual bool SpecialDraw (class CDisplayRectangle &displayRect, 
        CDC &dc, CGPoint *apPointsOnDR[]);

private:
    typedef struct SLineInfo
    {
        int mnPoints;
        CString msName;
        COLORREF mColor;

        SLineInfo(class CXMLParseNode *pXml);
        SLineInfo(void)
            : mnPoints(0)
            , msName(_T(""))
            , mColor(0xffffff)
        {
        }
    } SLineInfo;

    CList<struct SLineInfo,struct SLineInfo> mLines;
};
