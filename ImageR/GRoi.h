#pragma once
#include "..\..\ImageRLib\DataRoi.h"

class CGRoi :
    public CDataRoi
{
public:
    CGRoi(class CDisplayRectangle *pDispRect, const char *zName, 
        COLORREF color = GetDefColor(), int deltaMm = 10);
    CGRoi(class CDisplayRectangle *pDispRect, class CXMLParseNode *pGRoiXml);
    virtual ~CGRoi(void);
    virtual bool Drag(CRect rect, CSize size, CGPoint *apPointsOnDR[]);

    bool SetDataMask (class CTImage<bool> &mask);

    virtual bool GetStatisticsString(CMetaData *pData, CString &sValue, class CGraphicElementOnDR *pGEOnDR);
    virtual void OnNewDisplay(CGraphicElementOnDR * pGEonDR);

protected:
    void InitPoints(class CDisplayRectangle *pDispRect, int deltaMm);
    virtual void AdjustOtherPointsToDrag (int i, CGPoint *apPointsOnDR[]);
    virtual void SetRelativeClip(int i, CRect &rClip, CGPoint *apPointsOnDR[]);
};
