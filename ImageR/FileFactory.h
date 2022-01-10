#pragma once
#include "MetaData.h"
#include "..\..\yUtils\FileName.h"

class CFileFactory
{
public:
    typedef enum EComputedDataType
    {
        COMP_DATA_UNDEF,
        COMP_DATA_SMOOTH,
        COMP_DATA_OBLIQUE,
        COMP_DATA_MIP,
        COMP_DATA_BEAM_HARDENING,
        COMP_DATA_LAST
    }EComputedDataType;

    CFileFactory(FILE *pfLog);
    ~CFileFactory(void);
    CMetaData * CreateFileObject(class CXMLParseNode *pMetaDataXml);
    bool CreateAndDisplayFileObject(const char * zfName, bool bNewDisplay);
    CMetaData * CreateFileObject(const char * zfName);
    FILE *mpfLog;

protected:
    CMetaData * TryCreateMemFileObject(const char * zfName);
    CMetaData * TryCreateFileObject(const char * zfName);
	CMetaData * AnalyzeDicomOptions(void);
    class CMultiFile *InitAImgFile(void);


    //bool CheckForMx8000Prep();

    bool IsOfType(const char *z) {return mName.IsOfType(z);}

    CString msFileName;
    class CMyDicom *mpDicom;
    bool mbMultiFile;
    CFileName mName;
    class CXMLParseNode *mpMetaDataXml;

    static CString umsReplaceFileName;
    static CString umsOrigDataRoot;
    static CString umsNewDataRoot;

public:
    void AddComputedData(EComputedDataType eType);
    void CreateFiles(const char * zFilesFileName);
    bool mbFileCreatedAndUsed;
    bool mbCloseLogFile;

    static void SetReplaceRoot(const char* zOrigRoot, const char* zNewRoot);
    static void SetReplaceFileName(const char* zfName);
};
