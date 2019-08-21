#pragma once
#include "GLine.h"
#include "Oblique.h"

class CObliqueRef :
	public CGLine
{
public:
	CObliqueRef(class CDisplayRectangle *pDispRect, bool bOnImage, 
				class COblique *pOblique, CMetaData *pSrcData);
	~CObliqueRef(void);
	virtual void AutoUpdateForDraw(void);

private:
	CObliquePosition *mpOblPos;
    COblique::EOblType meType;

	void UpdateOnImage(void);
	void UpdateOnObl(void);
	COblique *mpOblique;
	CMetaData *mpSrcData;
	bool mbOnImage;
};

