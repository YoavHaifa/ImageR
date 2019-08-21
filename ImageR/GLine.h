#pragma once
#include "..\..\ImageRLib\dataline.h"

class CGLine :
	public CDataLine
{
public:
	CGLine(class CDisplayRectangle *pDispRect, const char *zName, 
        COLORREF color = 0xffffff, int deltaMm = 10);
    CGLine(class CDisplayRectangle *pDispRect, class CXMLParseNode *pGLineXml);
	virtual ~CGLine(void);
    virtual void OnNewDisplay(CGraphicElementOnDR * pGEonDR);
    virtual bool Drag(CRect rect, CSize size, CGPoint *apPointsOnDR[]);
    virtual bool GetStatisticsString(CMetaData *pData, CString &sValue, class CGraphicElementOnDR *pGEOnDR);
	static const int N_SEEN_POINTS = 2;
	bool mbShowSlope;
	CGLine *mpUpper;
	CGLine *mpLower;
	void SetAbove(CGLine *pLower);
	void OnUserChange(void);
	float GetYOfXScreen(float x);
	virtual bool GetDataDependantText(class CMetaData * pData, CString &s);
	bool mbExtra;
	bool mbTieOrigins;
	bool mbEnableDistanceDisplay;
	static bool umbCreatOnly2Points;
    virtual void SaveState(class CXMLDump & xmlDump);
	virtual bool EditContextMenu(CMenu *pMenu);
	virtual void PrintInfo(CMetaData *pData);
};
