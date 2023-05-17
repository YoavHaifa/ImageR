#pragma once
#include "metadata.h"
#include "..\..\ImageRLib\MyImage.h"

class CSharedMemFile :
    public virtual CMetaData
{
public:
    CSharedMemFile(class CFileMapping *pMapping, CMetaData *pOnData = NULL);
    virtual ~CSharedMemFile(void);

    virtual bool DoUpdatePageData(void);
    virtual bool ShouldUpdate(void);

    // Colorer Interface
    virtual bool IsColorer(void);
    virtual bool UpdateColorer(class CDisplayRectangle &displayRect);
	virtual float GetBias (void);

    virtual CString GetWindowName(void);
    virtual CString GetZoomName(void);
    virtual bool IsValid(void);
    virtual bool AllowColors(void);
    virtual void OnAssignDR (void);
	static DWORD umDebug;
    virtual void SaveState(class CXMLDump & xmlDump);
	virtual CString GetPageDataText(int i);
	virtual bool IsShared () {return true;}
	virtual bool WantContextMenu(int &oiMenu);
	bool GetColor(COLORREF &oColor);
	static void SetColorerDebugLog(FILE *pf){umpfColorer = pf;}

private:
	static FILE *umpfColorer;
	static bool umbDebugUpdateColorer;
    class CFileMapping *mpMapping;
    CMyImage::SSharedEntryHeader *mpaEntryHeaders;
    void *mpMem;
    int miLastRead;
    int miLastPageRead;
    int miLastRowRead;
    bool mbLastValid;
    bool mbImageValid;
    bool mbBitVolume;
    void DecodeBitVolumeImage(bool * pImage, unsigned int* pBitImage);
	int mnLinesInMultiDataPage;
	int mnLinesInFullData;
	CString msText;

protected:
    CMyImage::SSharedMemHeader *mpHeader;

public:
	virtual CMyImage::SSharedMemHeader *GetSharedHeader(void){return mpHeader;}

	virtual class CSharedMemFile *GetSharedMemFile(void){return this;}

	CPosition *GetPositionByNameAndRange(const char * zName, int iFirst, int iLast);
	CString GetNameFromHeader(void);
	CString GetCommand(void);
	CString GetDescription(void);
	bool BuildTextList(void);
	int GetContourThreshold(void);
};
