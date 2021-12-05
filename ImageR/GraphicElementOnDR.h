#pragma once
#include "..\..\ImageRLib\GraphicElement.h"
#include "..\..\ImageRLib\Dependant.h"
#include "DisplayCoordinates.h"
#include "GPoint.h"
#include "afxtempl.h"

class CGraphicElementOnDR :
    public CGraphicElementOnDRBase
{
public:
    CGraphicElementOnDR(CGraphicElement *pGE, class CDisplayRectangle *pDispRect);
    ~CGraphicElementOnDR(void);

protected:
    class CZoomPanParams *mpZoomParams;
    friend class CGraphicElement;
    CGraphicElement *mpGE;
    CDisplayRectangle *mpDR;

    CList<class CGPoint*,class CGPoint*> mPoints;
	virtual void SaveStateForGE(class CXMLDump & xmlDump);

    bool mbActive;

    CGPoint **mapPointsOnDR;
    int mnPointsOnDR;
    bool mbAllActive;
	CString msImageText;

public:
	virtual bool GetStatOrDataDependantText(CString &s);
	void FixPoint(int i, float x, float y);
	void FreePoint(int i);
	void FreeAndActivatePoint(int i);
    bool Is (const char *zName) {return mpGE->msName == zName;}
    bool IsPrefixedBy (const char *zName) {return mpGE->msName.Find(zName) == 0;}
	bool IsSelectable(void) {return mpGE->IsSelectable();}
    CString GetName (void) {return mpGE->msName;}
    void SetName (const char *zName) {mpGE->SetName(zName);}
    bool IsValid(void){return mpGE->IsValid();}
    CGPoint **GetPointsMap(void);
	CGraphicElement* GetGE(void) { return mpGE; }
	bool IsGE(CGraphicElement* pGE) { return mpGE == pGE; }
	bool IsOn(class CDisplayRectangle *pDispRect){return pDispRect == mpDR;}
    class CDisplayRectangle *GetDR(void){return mpDR;}
    bool IsActive(void){return mbActive;}
    void SetPointsColor(COLORREF color);
    CGPoint *FindPointNear(CDRCoordinates &drCoo, int *pDistance = NULL);

	bool ComputePointsScreenPosition (void);
    void Draw (CDC &dc);
    bool OnLButtonDown(UINT nFlags, CDRCoordinates &drCoo);
    bool OnLButtonUp(UINT nFlags, CDRCoordinates &drCoo);
    bool OnRButtonDblClk(UINT nFlags, CDRCoordinates &drCoo);
    bool OnKillFocus(void);
	bool OnDataUpdate(void);

    bool Drag(CSize size);
    int mnSeenPoints;
    bool mbConnectPoints;
	bool mbConnectPointsPairs;
    bool mbConnectLastAndFirst;
    bool mbDisplayInactivePoints;
    bool mbAllowPermanentSelection;
    virtual void SaveState(class CXMLDump & xmlDump);

private:
    void DeleteAllPoints(void);
public:
    void BuildPoints();
    void UpdatePoints(void);
    bool IsRelevant(class CMetaData *pData);
public:
	bool mbDisabled;
public:
	bool Disable(void);
	bool Enable(void);
	void Print(FILE *pf);
	bool FixCenter(void);
	void FixAllPoints(void);
	void LogAllPoints(const char *zText);
	static bool umbCheckRange;
	float GetDataValue(class CMetaData *pData, int iPoint);
	void AdjustDataCoordinates(class CMetaData *pData, int iPoint, CDataCoordinates &oCoo);
	virtual void PrintExcelHeader(FILE *pf);
	virtual void PrintCsvHeader(class CCsvFile &csv);
	virtual CMetaDataBase *GetData(void);
	virtual void PrintValue2Excel(FILE *pf, int iLine, int iCol);
	virtual void PrintValue2Csv(class CCsvFile &csv, int iLine, int iCol);

};
