#pragma once
#include "..\..\ImageRLib\DataCursor.h"
#include "GPoint.h"

class CGCursor :
    public CDataCursor
{
public:
    CGCursor(class CMetaData *pData, class CDisplayRectangle *pDispRect, const char *zName, 
		COLORREF color = 0xffffff);
    CGCursor(class CDisplayRectangle *pDispRect, class CXMLParseNode *pCursorXml);
    CGCursor(const char *zName = NULL);
    ~CGCursor(void);
    int mLen;

    virtual bool SpecialDraw (class CDisplayRectangle &displayRect, 
        CDC &dc, CGPoint *apPointsOnDR[]);
    virtual bool Drag(CRect rect, CSize size, CGPoint *apPointsOnDR[]);
    virtual bool GetStatisticsString(CMetaData *pData, CString &sValue, class CGraphicElementOnDR *pGEOnDR);
    //virtual void OnNewDisplay(CGraphicElementOnDR * pGEonDR);
};
