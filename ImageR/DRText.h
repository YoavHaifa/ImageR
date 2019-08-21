#pragma once

#include "GCursor.h"

class CDRText
	: public CGCursor
{
public:
	typedef enum ETextType
	{
		TEXT_NONE,
		TEXT_RESOLUTION,
		TEXT_IMAGE_NUMBER,
		TEXT_IMAGE_BALANCE,
		TEXT_RECON_ANGLE,
		TEXT_XML,
		TEXT_RING_LEVEL,
		TEXT_RING_LEVEL_SUM,
		TEXT_AVERAGING,
		TEXT_LAST
	} ETextType;

	static int I_FIRST_LINE;
	static int I_LINE_INC;

    CDRText(const char *zText, int iLine = 10, int iCol = 10, bool bAbsolute = false);
	CDRText(CMetaData *pData, CDisplayRectangle *pDispRect);
    CDRText(class CXMLParseNode *pTextXml);
    CDRText(class CDisplayRectangle *pDispRect, class CXMLParseNode *pTextXml);
    virtual ~CDRText(void);

    bool operator == (CDRText &other)
    {
        return ((msText == other.msText) 
			&& (miLine == other.miLine) 
			&& (miCol == other.miCol)
			&& (mpData == other.mpData));
    }
	void Set(const char *zText);
	CString GetText(void) {return msText;}
	void SetOnData(class CMetaData *pData){mpData = pData;}

private:
	friend class CDisplayRectangle;
    CString msText;
	CString msTitle;
	int miTitle;
	int miRightTitle;
    int miLine;
    int miCol;
	class CMetaData *mpData;
	bool mbDisableStateRestore;
	bool mbDataSpecific;
	bool mbAbsoluteColumn;

public:
	ETextType meTextType;

	void SetDataSpecific(bool flag){mbDataSpecific = flag;}
    //virtual void SaveState(class CXMLDump & xmlDump);
	void DisableStateRestore(void){mbDisableStateRestore = true;}
	void Update(int iText);
	unsigned long miDicom;
	void SetDicom(unsigned long iDicom){miDicom = iDicom;}
	void SetTitle(const char *zTitle){msTitle = zTitle;}
	void SetTitle(int id){miTitle = id;}
	void SetRightTitle(int id){miRightTitle = id;}
	void SetAtLine(int iLine);
	int miFrom;
	int miTill;
};
