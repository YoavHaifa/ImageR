#pragma once
#include "..\..\ImageRLib\Dependant.h"
#include "..\..\ImageRLib\TImage.h"

class CWindowParams :
    public CDependant
{
public:
    CWindowParams(class CMetaData *pData, bool bSetFocus = true);
    CWindowParams(class CXMLParseNode *pDRXml);
    ~CWindowParams(void);

    // Data Members
    float   mCenter;
    float   mWidth;

    int     mIndex;
    static int umCreateIndex;
    bool mbInitialized;

private:
	friend class CWindowDlg;

    // Change Controls
    float   mMin;
    float   mMax;
    //float   mExternalMin;
    //float   mExternalMax;
    float   mRange;
	bool    mbExternalRangeSet;

	float mLastMinUsed;
	float mLastMaxUsed;

    bool ReadInitialWindowFromRegistry(class CMetaData *pData);
	CString Value2StringByScale(float value, bool bInt = false);

public:
    // Methods
    void UpdateByMouseMove(int deltaX, int deltaY, class CMetaData* pData);
    float ScaleDelta (int delta, float range, int bInteger);

    void SetCt();
    void SetInRange(bool bCT, float min, float max);
    void SetRange (float min, float max);

    CString GetWidthText ();
    CString GetCenterText ();
    CString GetLowText ();
    CString GetHighText ();
    bool SetAutoFocus(class CMetaData *pData);
    bool SetAutoFocus(CTImage<float> * pRaster, CRect rect, float *poRatioNotZero = NULL);
    bool SetLocalFocus(CTImage<float> *pRaster, class CDRCoordinates coo);
    void SaveState(class CXMLDump & xmlDump);

    bool SetWidth(float width);
    bool SetCenter(float newCenter);
	void ExternalSetRange(float minVal, float maxVal);

    bool SetWidthFromGui(float width);
    bool SetCenterFromGui(float center);
	bool SetRangeFromGui(float minVal, float maxVal);

	void GetWindowMinMax(float &oMin, float &oMax, bool bColorer);

    CMetaData *mpCurrentData;

	static int umDebugWindow;
	static FILE *umpfLog;
	static float umWindowAccelerationFactor;
	static bool umbUseAllRangeForColorer;
	static void ReadRegistry(class CMyRegistry &myReg);
	void DebugPrint(const char *zText);
};
