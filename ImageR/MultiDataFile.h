#pragma once
#include "MultiFile.h"

class CMultiDataFile :
    public CMultiFile
{
public:
    CMultiDataFile(const char *zfName, class CXMLParseNode *pMetaDataXml, 
		unsigned int fileHeaderBytes = 0, EMyImageDataType eDefaultDataType = DATA_TYPE_UNDEF);
    ~CMultiDataFile(void);

    virtual bool DoUpdatePageData(void);
    virtual bool IsMultiDataFile() {return true;}
	static DWORD WINAPI StaticSetImageSize (LPVOID p);
	virtual bool set_i (int i);
    virtual void SaveState(class CXMLDump & xmlDump);
	virtual void ShowHeader(void){}
	virtual void EditContextMenu(CMenu *pPopup);
	virtual bool AddToTitleText(CString &iosTitle);
	virtual int GetNPages (void) {return mnPages;}

protected:
	char *mpPageHeader;

private:
	unsigned int mnPages;
    unsigned int mPageLenNeto;
    unsigned int mPageLenBruto;

    bool TryToGetImageSizeFromXml(void);
    bool GuessMatrix(void);
	void GetInfoFromXMLHeader();
	unsigned long ReadXMLOffset();
	bool ReadPageHeader(void);

public:
	unsigned int ComputeNPages(void);
	unsigned int GetPageSizeBruto(void){return mPageLenBruto;}

	virtual bool DesplayXmlHeader(void);
};
