#pragma once
#include "multifile.h"
#include "..\..\ImageRLib\CtImages.h"

class CArchivesFile :
    public CMultiFile, public CCtImages
{
public:
    CArchivesFile(const char *zName, class CMyDicom *pDicom, class CXMLParseNode *pMetaDataXml);
    ~CArchivesFile(void);
    void UpdateDicom(void);

    virtual bool DoUpdatePageData(void);
    virtual ULONGLONG GetLength(void);
    virtual CString GetPageText (void);
    bool GetInstanceNumberByIndex(int iImage, int &number);
    virtual bool GetPageDataByIndex(class CMyImage * pResultImage, int iImage, CString *psName = NULL);
	virtual bool GetMmPerPixel(double &mmppWidth, double &mmppHeight);
	virtual CString GetPageDataText(int i);
	virtual int NeedDataTexts(void){return umbNeedDataText ? 1 : 0;}
    virtual bool DoLoadVolumeImage(int iLoad, void *pVolumePage);
	static bool umbNeedDataText;

protected:
    bool GetPageDataFromDicom(CMyDicom *pDicom, unsigned short *pPage);
	class CRingLevelInfo *mpRli;
	bool mbLookedForRli;

public:
    static CArchivesFile * CreateFile(CFileName & name, CMyDicom * pDicom, 
        CXMLParseNode *pMetaDataXml);
    virtual void SaveState(class CXMLDump & xmlDump);
    static bool umbSaveRaster;
	size_t mnPixelsLen;
	virtual class CRingLevelInfo *GetRli(void);
    virtual bool SetCurrent(int iCurrent);
    virtual bool SetCurrentPositionOnly(int iCurrent);
	virtual CMyDicom * GetDicomByIndex(int iImage);
	void ReorderRgbData(char * pBuf);
};
