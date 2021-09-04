#pragma once
#include "MetaData.h"
#include "..\..\yUtils\CompVector.h"

typedef enum EDiffColor
{
	DIFF_COL_BLUE = 0,
	DIFF_COL_GREEN = 1,
	DIFF_COL_RED = 2
} EDiffColor;

class CDiffData :
    public CMetaData
{
public:
    CDiffData(class CDisplayRectangle *pDRPlus, class CDisplayRectangle *pDRMinus, const char *zName = NULL);
    CDiffData(class CXMLParseNode *pXmlNode);
    ~CDiffData(void);

    virtual bool IsDiff () {return true;}
 	virtual int n_diff (void) {return mnDiff;}
    virtual bool DoUpdatePageData(void);
	virtual void EditContextMenu(CMenu *pPopup);
	virtual int NeedDataTexts(void){return 2;}
	virtual CString GetPageDataText(int i);

    virtual CString GetWindowName(void){return CString("DiffWindow");}
    virtual void SaveState(class CXMLDump & xmlDump);
	bool IsValid(void){return ((mpDRPlus != NULL) && (mpDRMinus != NULL));}
	void SetColor(int iColor);

protected:
	class CDisplayRectangle *mpDRPlus;
	class CDisplayRectangle *mpDRMinus;
	EMyImageDataType meDataTypePlus;
	EMyImageDataType meDataTypeMinus;
	void ReportTypeError(void);

	bool mbCompareTried;
	bool mbCompareFailed;
    int mnDiff;

	// Support RGB
	bool mbRgb;
	int manRgbDiff[3];
	int miRgb;
	int mnAnyDiffForRgb;

    class CCompareManager *mpCompareManager;
	CList <CMetaData *, CMetaData *> mDataList;
	class CDisplayRectangle *GetDRAt(int iWanted);
	void GetCombineParams(void);
	bool CompareTypedData(void *pPlus, void *pMinus, void *pResult, int n);
	template <typename TINP>
	bool CompareTypedDataShort(TINP *pPlus, void *pMinus, void *pResult, int n);
	template <typename TINP>
	bool CompareTypedDataInt(TINP *pPlus, void *pMinus, void *pResult, int n);
	bool CompareTypedDataFloat(float *pPlus, void *pMinus, void *pResult, int n);
	bool CompareTypedDataDouble(double *pPlus, void *pMinus, double *pResult, int n);

	bool mbShouldScale;
	CCompVector mScaledPlus;
	CCompVector mScaledMinus;

public:
    void SetDiffDataType(void);
    void InitOnConstruction(void);
    bool mbRatio;
	bool mbCombine;
	float mWeight1;
	float mWeight2;
    void StartCompare();
	int miPlus;
	int miMinus;
	int miDiff;
	bool ChangePlus();
	bool ChangeMinus();
	void PrintStatistics();
	bool FindBestFit();
	bool FindBestTransformation(bool bMessBox = false);
	void PrintHistogram(class CFileCompareBase &compFile);
	bool mbLookingForBestFit;
	//bool mbFirstWithBestFit;


	static bool umbAllowDiffOfDifferentSize;
	int mnLinesToCompare;
	int mnColsToCompare;
	bool mbPrintHistogram;

	bool mbTransform;
	float mTransFactor;
	float mTransBias;

	static bool umbScaleDiff;
	static void ReadRegistry(class CMyRegistry &myReg);
};
