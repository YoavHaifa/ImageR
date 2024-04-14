#pragma once
#include "MultiFile.h"

class CMultiDataFile : public CMultiFile
{
public:
    CMultiDataFile(const char *zfName, 
		class CXMLParseNode *pMetaDataXml, 
		unsigned int fileHeaderBytes = 0, 
		EMyImageDataType eDefaultDataType = DATA_TYPE_UNDEF,
		bool bComputeNPages = true);
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
	virtual bool DoLoadVolume();

protected:
	char *mpPageHeader;
	void LoadPrologue();
	unsigned int mnPages;
	unsigned int mFilePageLenNeto;
	unsigned int mDisplayPageLenNeto;
    unsigned int mFilePageLenBruto;
	bool ReadPageHeader(void);
	bool SetOffset(ULONGLONG offset);
	DWORD ReadPage(unsigned char* pDest);

private:
    bool TryToGetImageSizeFromXml(void);
    bool GuessMatrix(void);
	void GetInfoFromXMLHeader();
	unsigned long ReadXMLOffset();

public:
	virtual void ComputeFilePageLenNeto();
	unsigned int ComputeNPages();
	unsigned int GetFilePageSizeBruto(void){return mFilePageLenBruto;}

	virtual bool DesplayXmlHeader(void);
};
