#pragma once
#include "GEllipse.h"

class CRingID :
	public CGEllipse
{
public:
	CRingID(class CDisplayRectangle *pDispRect, class CDataCoordinates &reconCenter, bool bCenterKnown);
	virtual ~CRingID(void);
    virtual bool GetStatisticsString(CMetaData *pData, CString &sValue, class CGraphicElementOnDR *pGEOnDR);
    virtual void OnChange (void);
	virtual bool GetDataDependantText(class CMetaData *pData, CString &s);

private:
	typedef struct SStatData
	{
		class CRingLevelInfo *mpRli;
		int mRadius;
		int mArea;
		float mSum;

		SStatData(class CRingLevelInfo *pRli, int radius, int area, float sum)
			: mpRli(pRli)
			, mRadius(radius)
			, mArea(area)
			, mSum(sum)
		{
		}

	} SStatData;

	static CList<SStatData *,SStatData *> mStatList;
	static void EmptyStatList(void);

	bool GetScanCenterInfo(CMetaData *pData, CString &sValue, bool bCustomerSupport = false);
	CString GetDeltaStat(class CMetaData *pData, class CRingLevelInfo * pRli, int radius);
	class CRingLevelInfo *mpRli;
	int miPos;
	static CString umsMissingDataMessage;

public:
	static int umMissingDataStartSizeMm;
	static CRingID *CreateRingID(class CDisplayRectangle *pDispRect, CMetaData *pData);
	static void SetMissingDataText(class CXMLParseNode *pNode);

	bool UpdateText(void);
	bool mbCenterKnown;
	virtual bool FixCenter(void);
};
