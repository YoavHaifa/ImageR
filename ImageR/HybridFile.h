#pragma once
#include "MetaData.h"
#include "afxtempl.h"

class CHybridFile :
    public CMetaData
{
public:
    CHybridFile(const char *zDirName, class CXMLParseNode *pMetaDataXml);
    ~CHybridFile(void);
    CList<CString*,CString*> mFileNamesList;
    virtual bool set_i(int iImage);
    virtual bool DoUpdatePageData(void);
	virtual bool SaveToRecentFiles(void){return true;}
	virtual CString NameToRecentFiles(void){return msDir;}
    virtual void SaveState(class CXMLDump & xmlDump);
    virtual bool IsHybrid (void) {return true;}
	virtual bool IsDicom (void) {return mpCurrentFile && mpCurrentFile->IsDicom();}
	virtual class CMyDicom *GetDicom(void) {return mpCurrentFile ? mpCurrentFile->GetDicom() : NULL;}
	virtual CMetaData *GetDataWithText(void);
    virtual float GetBias () {return mpCurrentFile ? mpCurrentFile->GetBias() : 0.0f;}
    virtual int GetNPages (void) {return (int)mFileNamesList.GetSize();}
    virtual bool GetPageDataByIndex(CMyImage * pResultImage, int iImage, CString *psName = NULL);
    virtual bool Is(const char * zName);

	static bool umbAutoFocusOnNewFile;
private:
	CString msDir;
	int miFile;
	void RemoveSomeFiles(void);
	CMetaData *mpCurrentFile;
	static int unInstances;
	static bool umbSkipMyParams;
public:
	void SetNewDate(const char * zNewDate);
};
