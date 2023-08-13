#pragma once

#include "PrintExcel.h"

class CTypedData
{
public:
    CTypedData(void);
    virtual ~CTypedData(void);

    virtual void Init () = 0;
    virtual void InitFormat () = 0;
    virtual void OnRead (void *p, int n) = 0;
    virtual void OnReadLines (void *p, int nLines, int nCols, int nLastColsToIgnore) = 0;
    virtual void OnReadBytes (void *p, int nBytes) = 0;

    virtual float Average(void) = 0;
    virtual float AverageAbs(void) = 0;
    virtual float NonZeroAverage (void) = 0;

    virtual float Center (void) = 0;
    virtual float Width (void) = 0;
    virtual float Min (void) = 0;
    virtual float Max (void) = 0;
    virtual unsigned int GetUnitSize (void) = 0;

    virtual CString PrintScreenToExcel (void *p, const char *zName,
        int nLines, int nCols, int nColsLoaded, const CString &sFileText) = 0;
    virtual CString Value2s (void *p, 
        int nCols, int iLine, int iCol, float slope, float bias) = 0;

	virtual CString GetRangeText(void){return CString(_T(""));}
    virtual CString GetRangeAndAverageText(bool bDiff);

    static CTypedData * GetNew (EMyImageDataType eDataType);

    char mzFormat[16];
    void SetFormat (const char *zFormat);

    int mnChecked;
    int mnSummed;
    double mSum;
    double mSumAbs;
    int mnNotZero;
	int mnNan;
	FILE *mpfLog;
};

template <typename T>
class TTypedData : public CTypedData
{
public:
    TTypedData(void)
    {
        Init();
    }
    virtual ~TTypedData(void)
    {
    }

    
    void Init ();
    
    void InitFormat ();

    void OnRead (void *p, int n);
    void OnReadLines (void *p, int nLines, int nCols, int nLastColsToIgnore);
    void OnReadBytes (void *p, int nBytes)
    {
        OnRead (p, nBytes / sizeof (T));
    }

    float Average(void)
    {
        if (!mnSummed)
            return 0.0f;
        return (float)(mSum) / mnSummed;
    }
    float AverageAbs(void)
    {
        if (!mnSummed)
            return 0.0f;
        return (float)(mSumAbs) / mnSummed;
    }
    float NonZeroAverage (void)
    {
        if (!mnNotZero)
            return 0.0f;
        return (float)(mSum) / mnNotZero;
    }
    float Min (void)
    {
        return (float)mMin;
    }
    float Max (void)
    {
        return (float)mMax;
    }
    float Center (void)
    {
        return (float)(mMin + mMax) / 2.0f;
    }
    float Width (void)
    {
        return (float)(mMax - mMin);
    }
    virtual unsigned int GetUnitSize (void) {return sizeof(T);}

    virtual CString PrintScreenToExcel (void *p, const char *zName,
        int nLines, int nCols, int nColsLoaded, const CString &sFileText)
    {
        return CPrintExcel::PrintScreen ((T *)p, mzFormat, nLines, nCols, nColsLoaded, 
                                            zName, sFileText);
    }
    virtual CString Value2s (void *p, 
        int nCols, int iLine, int iCol, float slope, float bias)
	{
        return CPrintExcel::Value2s ((T *)p, mzFormat, nCols, iLine, iCol, slope, bias);
	}

	virtual CString GetRangeText(void);
	virtual CString GetRangeAndAverageText(bool bDiff);

    T mMin;
    T mMax;
};

