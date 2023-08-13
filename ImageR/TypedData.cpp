#include "StdAfx.h"
#include ".\typeddata.h"
#include "ViewParams.h"
#include <float.h>


CTypedData::CTypedData(void)
	: mnChecked(0)
	, mnSummed(0)
	, mnNan(0)
	, mnNotZero(0)
	, mSum(0)
	, mSumAbs(0)
	, mpfLog(NULL)
{
	//static int count = 0;
	//mpfLog = CMyWindows::FOpenLogFile("CTypedData", count++);
}

CTypedData::~CTypedData(void)
{
	if (mpfLog)
	{
		fprintf(mpfLog, "<~CTypedData> mnChecked %d, mnSummed %d, mnNan %d\n", mnChecked, mnSummed, mnNan);
		fclose(mpfLog);
	}
}

CTypedData * CTypedData::GetNew (EMyImageDataType eDataType)
{
	switch (eDataType) 
	{
    case DATA_TYPE_SHORT:	
        return new TTypedData<short>;
    case DATA_TYPE_USHORT:
        return new TTypedData<unsigned short>;
    case DATA_TYPE_INT:
        return new TTypedData<int>;
    case DATA_TYPE_UINT:
        return new TTypedData<unsigned int>;
    case DATA_TYPE_FLOAT:
        return new TTypedData<float>;
    case DATA_TYPE_DOUBLE:
        return new TTypedData<double>;
    case DATA_TYPE_RGB:
        return new TTypedData<S_RGB>;
    case DATA_TYPE_BOOL:
        return new TTypedData<bool>;
    case DATA_TYPE_CHAR:
        return new TTypedData<char>;
    case DATA_TYPE_COLORREF:
        return new TTypedData<COLORREF>;
    }
    return NULL;
}
void TTypedData<S_RGB>::Init ()
{
    mnChecked = 0;
	mnSummed = 0;
    mnNotZero = 0;
	mnNan = 0;
	mSum = 0.0;
	mSumAbs = 0.0;
	mMin = 0;
    mMax = 0;
    InitFormat();
}

template <class T>
void TTypedData<T>::Init ()
{
	if (mpfLog)
	{
		fprintf(mpfLog, "<Init>\n");
		fflush(mpfLog);
	}
    mnChecked = 0;
	mnSummed = 0;
    mnNotZero = 0;
	mnNan = 0;
	mSum = 0.0;
	mSumAbs = 0.0;
	mMin = (T)0;
    mMax = (T)0;
    InitFormat();
}
template <class T>
void TTypedData<T>::OnRead (void *p, int n)
{
	if (mpfLog)
	{
		fprintf(mpfLog, "<OnRead> n %d\n", n);
		fflush(mpfLog);
	}
    mnChecked += n;
    T *pT = (T *)p;
    while (n--)
    {
        T value = *pT++;
		if (_isnan(value))
		{
			mnNan++;
			if (mpfLog)
			{
				fprintf(mpfLog, "<OnRead> mnNan %d\n", mnNan);
				fflush(mpfLog);
			}
		}
		else
		{
			if (!mnSummed)
			{
				mMin = value;
				mMax = value;
			}
			else
			{
				if (mMin > value)
					mMin = value;
				if (mMax < value)
					mMax = value;
			}
			mSum += value;
			mSumAbs += abs((double)value);
			mnSummed++;

			if (value)
				mnNotZero++;
		}
    }
}
template <class T>
void TTypedData<T>::OnReadLines (void *p, int nLines, int nCols, int nLastColsToIgnore)
{
    T *pT = (T *)p;

	int nColsToCheck = nCols - nLastColsToIgnore;
    mnChecked += nLines * nColsToCheck;
    while (nLines-- > 0)
    {
		int count = nColsToCheck;
		while (count -- > 0)
		{
			T value = *pT++;
			if (_isnan(value))
				mnNan++;
			else
			{
				if (!mnSummed)
				{
					mMin = value;
					mMax = value;
				}
				else
				{
					if (mMin > value)
						mMin = value;
					if (mMax < value)
						mMax = value;
				}

				mSum += value;
				mSumAbs += abs((double)value);
				mnSummed++;

				if (value)
					mnNotZero++;
			}
		}
		pT += nLastColsToIgnore;
    }
}
void TTypedData<short>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%d");
}
void TTypedData<unsigned short>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%u");
}
void TTypedData<int>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%d");
}
void TTypedData<unsigned int>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%u");
}
void TTypedData<float>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%f");
}
void TTypedData<double>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%f");
}
template<>
void TTypedData<S_RGB>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%x %x %x");
}
void TTypedData<COLORREF>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%x");
}
void TTypedData<bool>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%d");
}
void TTypedData<char>::InitFormat()
{
    strcpy_s (mzFormat, sizeof(mzFormat), "%d");
}
void CTypedData::SetFormat (const char *zFormat)
{
    strcpy_s (mzFormat, sizeof(mzFormat), zFormat);
}
template <class T>
CString TTypedData<T>::GetRangeText(void)
{
	if (mnSummed > 0)
	{
		CString sFormat("(");
		sFormat += mzFormat;
		sFormat += " - ";
		sFormat += mzFormat;
		sFormat += ")";
		char zBuf[256];
		sprintf_s(zBuf,sizeof(zBuf),sFormat, mMin, mMax);
		return CString(zBuf);
	}
	if (mnNan > 0)
		return CString("No Legal Values");

	return CString("No Data");
}
template <class T>
CString TTypedData<T>::GetRangeAndAverageText(bool bDiff)
{
	CString sText = GetRangeText();
	if (mnSummed > 0)
	{
		char zBuf[256];
		sprintf_s(zBuf,sizeof(zBuf)," Avg %f", Average());
		sText += zBuf;

		if (bDiff)
		{
			sprintf_s(zBuf,sizeof(zBuf)," AvgAbs %f", AverageAbs());
			sText += zBuf;
		}
	}
	if (mnNan > 0)
	{
		if (mnSummed > 0)
			sText += ",";
		char zBuf[256];
		sprintf_s(zBuf,sizeof(zBuf)," %d NAN", mnNan);
		sText += zBuf;
	}
	return sText;
}
CString CTypedData::GetRangeAndAverageText(bool /*bDiff*/)
{
	return CString(_T("")); 
}
/*static void func()
{
    TTypedData<short> TypedDataShort;
    TTypedData<int> TypedDataInt;
    TTypedData<unsigned short> TypedDataUShort;
    TTypedData<unsigned int> TypedDataUInt;
    //TTypedData<float> TypedDataFloat;


} */