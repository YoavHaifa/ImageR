#include "StdAfx.h"
#include "DisplayGE.h"
#include "DisplayRectangle.h"
#include "GraphicElementOnDR.h"
#include "..\..\ImageRLib\GraphicElement.h"
#include "Screens.h"

CDisplayGE::CDisplayGE(CGraphicElement *pGE)
: CDependant (CString ("Disp") + pGE->msName)
, mpGE (pGE)
{
    pGE->SetDisplayGE(this);
}

CDisplayGE::~CDisplayGE(void)
{
	if (!umDeleter.mbDoingGlobalDestruction)
	{
	    if (mpGE)
		{
			gScreens.OnGEDeleted(mpGE);
			delete mpGE;
		}
	}
}
bool CDisplayGE::AddDisplay(CDisplayRectangle * pDR)
{
    CGraphicElementOnDR *pGEonDR = FindGeOnDr(pDR, NULL);
    if (pGEonDR)
        return false;

    pGEonDR = new CGraphicElementOnDR(mpGE, pDR);
    mpGE->OnNewDisplay(pGEonDR);
    mOnDRList.AddTail(pGEonDR);
    return true;
}
int CDisplayGE::RemoveDisplay(CDisplayRectangle * pDR)
{
    POSITION pos;
    CGraphicElementOnDR *pGEonDR = FindGeOnDr(pDR, &pos);

    if (pGEonDR)
    {
        mOnDRList.RemoveAt(pos);
        delete pGEonDR;
    }
    return (int)mOnDRList.GetSize();
}
CGraphicElementOnDR * CDisplayGE::FindGeOnDr(CDisplayRectangle * pDR, POSITION *pPos)
{
    POSITION pos = mOnDRList.GetHeadPosition();
    while (pos)
    {
        if (pPos)
            *pPos = pos;
        CGraphicElementOnDR *pGEonDR = mOnDRList.GetNext(pos);
        if (!pDR || pGEonDR->IsOn(pDR))
            return pGEonDR;
    }
    if (pPos)
        *pPos = pos;
    return NULL;
}
void CDisplayGE::UpdatePointsOnDrs(void)
{
    POSITION pos = mOnDRList.GetHeadPosition();
    while (pos)
    {
        CGraphicElementOnDR *pGEonDR = mOnDRList.GetNext(pos);
        pGEonDR->UpdatePoints();
    }
}
class CMetaData * CDisplayGE::GetFirstData()
{
    return mOnDRList.GetHead()->GetDR()->GetCurrentData();
}
CGraphicElementOnDR * CDisplayGE::GetGEOnDR (CGraphicElement *pGE)
{
	return pGE->mpDisplayGE->FindGeOnDr(NULL,NULL);
}
