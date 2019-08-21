#pragma once
#include "MetaFile.h"

class CPflhDicomFile :
	public CMetaFile
{
public:
	CPflhDicomFile(const char *zName, class CMyDicom *pDicom, class CXMLParseNode *pMetaDataXml = NULL);
	virtual ~CPflhDicomFile(void);
    virtual void SaveState(class CXMLDump & xmlDump);

    virtual void SetDicom(class CMyDicom *pDicom);
    virtual bool DoUpdatePageData(void);
	int mnPages;
	unsigned int mFrameSizeBytes;
	unsigned int mLastFrameSizeBytes;
	unsigned long mLastReadOffset;
    unsigned short mDicomDataGroup;
    unsigned short mDicomDataNum;
};

