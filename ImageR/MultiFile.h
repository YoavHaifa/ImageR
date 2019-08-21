// MultiFile.h: interface for the CMultiFile class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "..\..\yUtils\FileName.h"
#include "MetaFile.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\ImageRLib\FileNamePattern.h"


//---------------------------------------------------------------------------------

class CMultiFile : public CMetaFile
{
public:
	CMultiFile(const char *zfName, class CXMLParseNode *pMetaDataXml);
	virtual ~CMultiFile(void);

    // bool read_image (int i, class MyImage &Image);
   
	int mMatrix;
	//bool read (FILE *pf);

	void set_file_offset(ULONGLONG Offset);

    virtual bool DoUpdatePageData(void);
    virtual bool DoLoadVolume();
    virtual bool DoLoadVolumeImage(int iLoad, void *pVolumePage);

	virtual bool set_i (int i);

    virtual void PrintId(FILE * pf, const char * zText);

	int i() {return miFile;}
    bool IsMultiFile() {return mbSingle ? false : true;}


    bool IsMx8000Prep (void) {return mbIsMx8000Prep;}
    virtual int GetMatrix(void) {return mMatrix;}
    virtual int GetNPages (void) {return mNamePattern.GetNFiles();}
    virtual bool ContainsFile(const char *zName);
    virtual void SetDicom(class CMyDicom *pDicom);

    bool mbSingle;

    const CString GetNamePattern(void){return mNamePattern.GetPattern();}
    virtual CString PermanentName(void){return GetNamePattern();}

protected:
	virtual bool guess_matrix_and_offset(void);

	void convert_char (const char *zfName, const char *zfOut);

	bool mbIsMx8000Prep;

    int  miFile;
    CFileNamePattern mNamePattern;

public:
	virtual void SetSecondLeafingIndex(void);
	virtual void SetNewPosition(void);

    virtual void SaveState(class CXMLDump & xmlDump);

	static DWORD WINAPI StaticSelectByParameters (LPVOID p);
    bool SelectByParameters(void);
    bool mbUpdateViewerOnSetName;
    virtual CString GetFileNameOfPage(int iPage);
    void ChangeDicomDateToAllFiles(CString sNewDate);
    virtual bool Is(const char * zName);
	bool SetDir(int iDir);
	void AmendAllByXml(void);
	void CheckDicomSeries(void);
	CString GetFileName(void){return mNamePattern.GetName();}
	virtual CString GetLastName(void);
	void ReorderFiles(int iFirst, int iLast, int step = 1, 
		int iOut = 1, int iOutStep = 1, const char *zPrefix = NULL);
	void ReorderFiles(void);
	void ReverseFilesOrder(void);
};

