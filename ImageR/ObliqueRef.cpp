#include "StdAfx.h"

#include "ObliqueRef.h"
#include "Oblique.h"
#include "ObliquePosition.h"


CObliqueRef::CObliqueRef(CDisplayRectangle *pDispRect, bool bOnImage, 
				COblique *pOblique, CMetaData *pSrcData)
	: CGLine(pDispRect, bOnImage ? "OblOnIm" : "ImOnObl", 0x00c0c0)
	, mbOnImage(bOnImage)
	, mpOblique(pOblique)
	, mpSrcData(pSrcData)
	, mpOblPos(NULL)
{
	mbSelectable = false;
	mbDisableDrag = true;
}
CObliqueRef::~CObliqueRef(void)
{
}
void CObliqueRef::AutoUpdateForDraw(void)
{
	mpOblPos = mpOblique->GetOblPos();
	meType = mpOblPos->GetType();

	if (mbOnImage)
		UpdateOnImage();
	else
		UpdateOnObl();
}
void CObliqueRef::UpdateOnImage(void)
{
	float currentObl = (float)mpOblPos->miCurrent;

	switch(meType)
	{
	case COblique::OBL_CORONAL:
		SetDataPointCoo(0, 0, currentObl);
		SetDataPointCoo(1, (float)(mpSrcData->GetNCols() - 1), currentObl);
		break;

	case COblique::OBL_SAGITTAL:
		SetDataPointCoo(0, currentObl, 0);
		SetDataPointCoo(1, currentObl, (float)(mpSrcData->GetNLinesInPage() - 1));
		break;
	}
}
void CObliqueRef::UpdateOnObl(void)
{
	CPosition *pSrcPos = mpSrcData->GetPosition();
	float currentDataRel = (float)pSrcPos->GetCurrentRelative();
	if (mpOblique->IsInvertedUpDown())
		currentDataRel = mpOblique->GetNLinesInPage() - 1 - currentDataRel;

	switch(meType)
	{
	case COblique::OBL_CORONAL:
	case COblique::OBL_SAGITTAL:
		SetDataPointCoo(0, 0, currentDataRel);
		SetDataPointCoo(1, (float)(mpOblique->GetNCols() - 1), currentDataRel);
		break;
	}
}

