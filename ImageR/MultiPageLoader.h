#pragma once
#include "afxtempl.h"
#include "..\..\ImageRLib\TImage.h"

class CMultiPageLoader
{
public:
	CMultiPageLoader(class CMetaData *pData);
	~CMultiPageLoader(void);

	void SetNWanted(int n);
	bool UpdatePage(void);
	int GetNWanted(void);
    virtual void SaveState(class CXMLDump & xmlDump);
	CString GetText(void);
	bool AllocateImages(void);
	void PrintValues2Excel(FILE *pf, int nCols, int iLine, int iCol);
	void PrintValues2Csv(class CCsvFile &csv, int nCols, int iLine, int iCol);

	POSITION GetHeadPosition(void){return mImages.GetHeadPosition();}
	class CMyImage *GetNext(POSITION &pos){return mImages.GetNext(pos);}

private:
	class CMetaData *mpData;
	int mnWanted;
	int mnLoaded;
	int miFirst;
	int miLast;

    CList<class CMyImage*,class CMyImage*> mImages;
	CTImage<float> *mpSumPage;
	class CDRText *mpImageText;

	bool LoadAllImages(void);
	bool mbUseImagesCache;
	bool LoadAndAverageImages(void);
	class CMyImage *GetTargetOf(int iLoad);
	class CMyImage *GetFreeTarget(void);
	void Add(class CMyImage*pTarget);
	bool Load(class CMyImage*pTarget, int iLoad);
	static const int MAX_IN_CACHE = 2048;
	int miUpdate;
	int miInCache;
	int maIndexLoaded[MAX_IN_CACHE];
	int maLastUsed[MAX_IN_CACHE];
};

