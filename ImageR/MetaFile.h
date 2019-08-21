#pragma once
#include "MetaData.h"
#include "..\..\yUtils\FileName.h"

class CMetaFile :
    public CMetaData
{
public:
    CMetaFile(const char *zName, class CXMLParseNode *pMetaDataXml = NULL);
    ~CMetaFile(void);

    virtual bool IsFile() {return true;}
    virtual bool HasSpecialParameters (void) {return mbSpecPar;}

    virtual class CMyDicom *GetDicom(void) {return mpDicom;}
    virtual void SetDicom(class CMyDicom *pDicom);
	virtual void ReadRescaleAndSlope(void);
    virtual void ReReadDicom(void);

	virtual HANDLE h(void) {return mhFile;}
    virtual bool IsDicom (void) {return mpDicom != NULL;}
    virtual bool IsDicomMatrix (void) {return mbDicomMatrix;}
    bool IsOfType(const char *z) {return mName.IsOfType(z);}
	virtual bool open(void);
	virtual void close(void);

     
    virtual float GetCtBias (void);
	static const char caDATA_VIEW_PAR_FILE_NAME[];
    virtual ULONGLONG GetOffsetBytes(void) {return mOffsetBytes;}
    void SetOffsetBytes(ULONGLONG offset);
    virtual ULONGLONG GetLength(void) {return mLength;}
    virtual bool ShouldUpdate(void);
    virtual void OnStartUpdate (void);
	//virtual bool GetBias(int &bias);
	virtual bool GetMmPerPixel(double &mmppWidth, double &mmppHeight);


protected:
	void GetFileLength(void);
    virtual void OnOpen(void);

    virtual bool guess_matrix_and_offset(void){return false;}
    float mCtBias;
	bool mbCtBiasSet;

    class CMyDicom *mpDicom;
	int mPlanarConfig;
    CFileName mName;
	HANDLE mhFile;
	bool mbDicomMatrix;
	ULONGLONG mOffsetBytes;
	ULONGLONG mLength;
	bool mbSpecPar;
    FILETIME mLastModificationRead;

    void SetDicomOnce();
	bool mbDicomSetOnce;

public:
    //void PostConstructInit(void);
    virtual void SaveState(class CXMLDump & xmlDump);
	bool GetDefaultNLines(void);
};
