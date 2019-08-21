#pragma once
#include "..\..\yUtils\XmlParse.h"

class CTextController
{
public:
	CTextController(void);
public:
	virtual ~CTextController(void);
public:
	CXMLParse *mpAppParse;
	CXMLParseNode *mpAppRoot;

	bool Init(void);

	void SetDataTexts(class CMetaData *pData);
	bool SetApplicationTexts(class CMetaData *pData);
	void SetPathTexts(class CMetaData *pData);
	bool SetPathImageText(class CMetaData *pData, CList<class CDRText*,class CDRText*> &list);

	static CString umsTextsFileName;
};

extern CTextController gTexts;
