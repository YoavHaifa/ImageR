#pragma once
#include "..\..\ImageRLib\Dependant.h"
#include "..\..\ImageRLib\TImage.h"

class CDisplayGE :
    public CDependant
{
public:
    CDisplayGE(class CGraphicElement *pGE);
    virtual ~CDisplayGE(void);

    class CGraphicElement *mpGE;
    class CGraphicElementOnDR * FindGeOnDr(class CDisplayRectangle * pDR, POSITION *pPos);
    bool AddDisplay(class CDisplayRectangle * pDR);
    int RemoveDisplay(class CDisplayRectangle * pDR); // Return number of remaining displays


    void UpdatePointsOnDrs(void);
    class CMetaData *GetFirstData();
	static CGraphicElementOnDR * GetGEOnDR (CGraphicElement *pGE);

protected:
    CList<class CGraphicElementOnDR *,class CGraphicElementOnDR *>mOnDRList;
};
