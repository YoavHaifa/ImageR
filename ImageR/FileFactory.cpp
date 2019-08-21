#include "StdAfx.h"
#include "FileFactory.h"
#include ".\MetaFile.h"
#include ".\BmpFile.h"
#include ".\MultiFile.h"
//#include ".\ArchivesFile.h"
#include "Screens.h"
//#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\XMLParse.h"
#include "FloatPrep.h"
#include "DisplayRectangle.h"
#include "ProcImage.h"
#include "Sinograms.h"
#include "DiffData.h"
#include "Oblique.h"
#include "MipVolume.h"
#include "MultiDataFile.h"
#include "HybridFile.h"
#include <typeinfo.h>
#include "SharedMemFile.h"
#include "MipShared.h"
#include "..\..\ImageRLib\FileMapping.h"
#include "VolTissue.h"
#include "PipeBufReadings.h"
#include "PflhDicomFile.h"
#include "DicomContours.h"
#include "ObliquePosition.h"

#ifdef __VR__
#include "VolumeRenderingIF.h"
#endif

CFileFactory::CFileFactory(FILE *pfLog)
: mpfLog (pfLog)
, mpDicom (NULL)
, mbMultiFile(false)
, mpMetaDataXml(NULL)
, mbFileCreatedAndUsed(false)
, mbCloseLogFile(false)
{
    if (!mpfLog)
    {
        //CFileName fName("FileFactory");
        //mpfLog = CMyWindows::FOpenLogFile(fName,true/*bAppend*/);
        //mbCloseLogFile = true;
    }
}

CFileFactory::~CFileFactory(void)
{
    if (mpDicom)
        delete mpDicom;
    if (mpfLog && mbCloseLogFile)
        fclose (mpfLog);
}
CMetaData * CFileFactory::CreateFileObject(CXMLParseNode *pMetaDataXml)
{
    mpMetaDataXml = pMetaDataXml;

    CString sClass, sName;
    CXMLParseNode *pObjectXml = pMetaDataXml->GetFirstDeep("Object");
    if (!pObjectXml)
        return NULL;

    pObjectXml->GetValue("class",sClass);
    CMetaData *pData = NULL;


    if (sClass == "CMipVolume")
        pData = new CMipVolume(pMetaDataXml);

    else if (sClass == "CDiffData")
	{
		CDiffData *pDiff = new CDiffData(pMetaDataXml);
		if (pDiff->IsValid())
	        pData = pDiff;
		else
			delete pDiff;
	}

    else if (sClass == "COblique")
        pData = new COblique(pMetaDataXml);

    else if (sClass == "CProcImage")
        pData = new CProcImage(pMetaDataXml);

    else if (sClass == "CSinograms")
        pData = new CSinograms(pMetaDataXml);

    else if (pObjectXml->GetValue("name",sName))
       pData = TryCreateFileObject(sName);
    
    if (pData)
    {
        pData->SetXmlParseNode(pMetaDataXml);
        pData->SetParseId(pObjectXml);
        pData->PostConstruct(pMetaDataXml);
    }
    return pData;
}
bool CFileFactory::CreateAndDisplayFileObject(const char * zfName, bool bNewDisplay)
{
    CFileName fname (zfName);
    if (fname.IsOfType("files") || fname.IsOfType("txt"))
    {
        CreateFiles(zfName);
        return true;
    }

    CMetaData *pData = CreateFileObject(zfName);
    if (pData)
    {
        if (pData->IsColorer())
        {
            if (mpfLog)
				fprintf (mpfLog, "<CFileFactory::CreateAndDisplayFileObject> AddGlobalColorer %s\n", pData->Name());
            gScreens.AddGlobalColorer(pData);
            if (pData->GetVolume())
            {
                CDisplayRectangle *pDR = gScreens.GetMipDR();
                if (pDR)
                {
                    CVolTissue *pTissue = new CVolTissue(pData);
                    pTissue->SetLogFile(mpfLog);
                    pDR->AddColorer(pTissue);

                    CList<class CMetaData*,class CMetaData*> &list = pDR->GetDataList();

                    POSITION pos = list.GetHeadPosition();
                    while (pos)
                    {
                        CMetaData *pData = list.GetNext(pos);
						CMipVolume *pMipVolume = pData->PMipVolume();
                        if (pMipVolume)
						{
                            pMipVolume->AddTissue(pTissue);
							if (mpfLog)
								fprintf (mpfLog, "<CFileFactory::CreateAndDisplayFileObject> Add VolTissue %s to %s\n", 
								pTissue->Name(), pData->Name());
						}
                    }
                }
            }
            if (mpfLog)
				fprintf (mpfLog, "<CFileFactory::CreateAndDisplayFileObject> Colorer Added\n\n");
        }
        else
            gScreens.AddData (pData, bNewDisplay);
        gScreens.Refresh();
        return true;
    }
    return mbFileCreatedAndUsed;
}
CMetaData * CFileFactory::CreateFileObject(const char * zfName)
{
    if (mpfLog)
    {
        fprintf (mpfLog, "<CFileFactory::CreateFileObject> %s\n", zfName);
        fflush (mpfLog);
    }
    CMetaData *pData = NULL;
    mbFileCreatedAndUsed = false;

    if (_strnicmp (zfName, "Mem:", 4) == 0)
        pData = TryCreateMemFileObject(zfName + 4);
    else
        pData = TryCreateFileObject(zfName);

    if (pData)
    {
        pData->PostConstruct(NULL);
        if (mpDicom && pData->IsFile())
        {
            ((CMetaFile *)pData)->SetDicom (mpDicom);
            mpDicom = NULL;
        }
        if (mpfLog)
        {
            fprintf (mpfLog, "File created <%s>\n", zfName);
            fprintf (mpfLog, "File created typeid <%s>\n", typeid(*pData).name());
            fflush (mpfLog);
        }
    }
    else if (mpfLog && !mbFileCreatedAndUsed)
    {
        fprintf (mpfLog, "Failed to Create file <%s>\n", zfName);
        fflush (mpfLog);
    }

    return pData;
}
CMetaData * CFileFactory::TryCreateMemFileObject(const char * zfName)
{
    //static int nNotice = 0;
    //if (!nNotice++)
    //    MessageBox (NULL, "TryCreateMemFileObject", zfName, MB_OK);

    if (mpfLog)
        fprintf (mpfLog, "<CFileFactory::TryCreateMemFileObject> %s\n", zfName);

    CFileMapping *pMapping = new CFileMapping(zfName);
    if (!pMapping->Valid())
    {
        delete pMapping;
        return NULL;
    }

    // Look for data with this volume
    CMyImage::SSharedMemHeader *pHeader = pMapping->GetHeader();
	bool bColorer = pHeader->mDataType == DATA_TYPE_COLORREF;
    CString sPattern = pHeader->GetPattern();
	int patternLen = sPattern.GetLength();
    if (patternLen > 0)
    {
        if (mpfLog)
            fprintf (mpfLog, "<CFileFactory::TryCreateMemFileObject> sPattern %s\n", sPattern);
        CList<class CMetaData*,class CMetaData*> dataList;
        gScreens.ListData(dataList);
        POSITION pos = dataList.GetHeadPosition();
        while (pos)
        {
            CMetaData *pData = dataList.GetNext(pos);
			CString sName(pData->Name());
            if (pData->IsMultiFile())
            {
                CMultiFile *pMultiFile = (CMultiFile *)pData;
				sName = pMultiFile->GetNamePattern();
			}
            if (sName.Left(patternLen) == sPattern)
            {
				if (mpfLog)
					fprintf (mpfLog, "Check data FIT %s\n", sName);
				if (bColorer)
				{
					CMetaData * pColorer = new CSharedMemFile (pMapping, pData);
					gScreens.AddColorerOnData(pData, pColorer);
				}
				else
	                pData->AttachToSharedVolume(pMapping);

                mbFileCreatedAndUsed = true;
				if (mpfLog)
					fprintf (mpfLog, "<CFileFactory::TryCreateMemFileObject> AttachToSharedVolume %s\n\n", 
						sName);
                return NULL;
            }
			else
				if (mpfLog)
					fprintf (mpfLog, "Check data SKIP %s\n", sName);
        }
    }

	if (pHeader && pHeader->mbDisplayAsGraphicContour)
	{
        CDisplayRectangle *pDR = NULL;
        if (!pDR)
            pDR = gScreens.GetCurrentDisplayRectangle();
        if (pDR)
		{
			CSharedMemFile * pSharedMemFile = new CSharedMemFile (pMapping);
            if (pDR->AddGraphicElementFromSharedMemImage(pSharedMemFile))
            {
                gScreens.Refresh();
                mbFileCreatedAndUsed = true;
				if (mpfLog)
				{
					fprintf (mpfLog, "Graphic element Added\n");
					fprintf (mpfLog, "<CFileFactory::TryCreateMemFileObject> Created new graphic contour\n\n");
				}
				return NULL;
            }
		}
    }

	if (pHeader && pHeader->mbMip)
	{
		if (mpfLog)
			fprintf (mpfLog, "<CFileFactory::TryCreateMemFileObject> new CMipShared\n");
		CMetaData * pData = new CMipShared (pMapping);
		return pData;
	}

    if (mpfLog)
        fprintf (mpfLog, "<CFileFactory::TryCreateMemFileObject> new CSharedMemFile\n");
    CMetaData * pData = new CSharedMemFile (pMapping);
    return pData;
}
CMetaData * CFileFactory::AnalyzeDicomOptions(void)
{
    if (mName.IsOfType("archfiles"))
    {
		return new CArchivesFile(msFileName, NULL, NULL);
    }
        
    // Check Dicom & Dicom Raw
	bool bSingle = true;
    mpDicom = CFileName::GetDicomFile(msFileName, &bSingle);
    if (!mpDicom)
    {
        if (mpfLog)
            fprintf (mpfLog, "File is NOT a Dicom\n");
		return NULL;
    }
    if (mpfLog)
        fprintf (mpfLog, "File is Dicom\n");

  
	if (mpDicom->IsContours())
	{
		CDicomContours *pContours = new CDicomContours(mName, mpDicom, mpMetaDataXml);
		if (pContours->IsValid())
		{
			mpDicom = NULL;
			return pContours;
		}
		else
		{
			delete pContours;
			mpDicom = CFileName::GetDicomFile(msFileName, &bSingle);
		}
	}
	if (mName.IsImageFromHostArchives() || mpDicom->IsCtImage()) // Multi-File from nominal archives
	{
		CArchivesFile *pFile = new CArchivesFile(mName, mpDicom, mpMetaDataXml);
		mpDicom = NULL;
		return pFile;
	}

	if (mpDicom->IsPflh())
	{
		CPflhDicomFile *pFile = new CPflhDicomFile(mName, mpDicom, mpMetaDataXml);
		mpDicom = NULL;
		return pFile;
	}
	return NULL;
}
CMetaData * CFileFactory::TryCreateFileObject(const char * zfName)
{
    if (!CFileName::Exist(zfName))
    {
        if (mpfLog)
        {
            fprintf (mpfLog, "<CFileFactory::TryCreateFileObject> %s - File doesn't exist!\n", zfName);
            fflush (mpfLog);
        }
        return false;
    }
	// Copy Name to place
	char zLongFileName[1024];
    zLongFileName[0] = 0;
    GetLongPathName (zfName, zLongFileName, sizeof(zLongFileName)/sizeof(zLongFileName[0]));
	msFileName = zLongFileName;
    if (CMyWindows::IsDirectory(msFileName))
    {
		CCtImages::DisableWarnings();
        return new CHybridFile(msFileName, NULL);
    }

    FILE *pf = MyFOpen (msFileName, "rb");
    if (!pf)
    {
        if (mpfLog)
            fprintf (mpfLog, "<TryCreateFileObject> failed to open %s\n", msFileName);
        return NULL;
    }
    fclose (pf);

    mName.Set (msFileName);
	if (mName.IsOfType("ColorMap"))
	{
		gScreens.ReadColorMap(msFileName);
		return NULL;
	}

    if (mName.IsOfType("xml"))
    {
        if (mName.IsOfMetaType("ImageR"))
        {
            gScreens.LoadSavedState (msFileName);
            mbFileCreatedAndUsed = true;
        }

        else
            if (mName.IsOfMetaType("Graphic"))
        {
            CXMLParse parseFile (mName);
            CXMLParseNode *pRoot = parseFile.GetRoot();
            if (!pRoot)
                return false;

            CString sData = pRoot->GetFirstDeepString("data_name","");
            CDisplayRectangle *pDR = NULL;
            if (!sData.IsEmpty())
			{
				if (mpfLog)
					fprintf (mpfLog, "<CFileFactory::TryCreateFileObject> look for data %s\n", sData);
                pDR = gScreens.GetRectangleWithData(sData);
				if (!pDR && mpfLog)
				{
					fprintf (mpfLog, "<CFileFactory::TryCreateFileObject> ERROR: Failed to find %s\n\n", sData);
				}
			}
            if (!pDR)
                pDR = gScreens.GetCurrentDisplayRectangle();

            if (pDR)
                if (pDR->AddGraphicElementFromXml(pRoot))
                {
                    gScreens.Refresh();
                    mbFileCreatedAndUsed = true;
					if (mpfLog)
						fprintf (mpfLog, "Graphic element Added\n");
                }
        }
        return NULL;
    }

	CMetaData *pDicomBasedData = AnalyzeDicomOptions();
	if (pDicomBasedData)
		return pDicomBasedData;

	// Look for file type
	mbMultiFile = false;

	// Look for multi-file
	if (IsOfType ("aimg")) // Multi-file from IB-UT
    {
        return InitAImgFile();
    }
	else if (IsOfType ("bmp")) // colored image support
    {
        return new CBmpFile(msFileName);
    }
	else if (IsOfType ("float_prep") 
        || IsOfType ("logged_reading")
        || IsOfType ("UnLogged_reading")
        ) // colored image support
    {
        return new CFloatPrep(msFileName);
    }
	else if (IsOfType ("pipe"))
	{
		if (msFileName.Find("_header128") > 0)
			return new CPipeBufReadings(msFileName);
	}
	else
	{

		CFileNamePattern namePattern(msFileName, NULL);
		if (namePattern.IsMultiFile())
		{
			if (IsOfType ("prep"))
				CFileNamePattern::EnableMultiLevelFilesArray(2);

			mbMultiFile = true;
			CMultiFile *pFile = new CMultiDataFile(msFileName, mpMetaDataXml);
			return pFile;
		}
	}
    //        CheckForMx8000Prep();
	
    CMultiDataFile *pData = new CMultiDataFile (msFileName, mpMetaDataXml);
    return pData;
}

CMultiFile *CFileFactory::InitAImgFile()
{
	mbMultiFile = true;
    CMultiFile *pFile = new CMultiFile (msFileName, mpMetaDataXml);
    return pFile;
}
void CFileFactory::AddComputedData(EComputedDataType eType)
{
    if (eType == COMP_DATA_OBLIQUE || eType == COMP_DATA_MIP)
    {
        if (!gScreens.SelectMultiFile())
            return;
    }
    CDisplayRectangle *pDR = gScreens.GetCurrentDisplayRectangle();
    if (!pDR)
        return;

    CList<class CMetaData*,class CMetaData*> &dataList = pDR->GetDataList();
    class CObliquePosition *pOblPos = NULL;
    class CObliquePosition *pOtherOblPos = NULL;
    COblique *pOtherOblique = gScreens.GetOblique();
    if (pOtherOblique)
        pOtherOblPos = (class CObliquePosition *)pOtherOblique->GetPosition();

    int nAdded = 0;
    POSITION pos = dataList.GetHeadPosition();
    while (pos)
    {
        CMetaData *pData = dataList.GetNext(pos);
        switch (eType)
        {
        case COMP_DATA_SMOOTH:
            gScreens.AddData(new CProcImage (pData, "Smooth"), nAdded++ == 0);
            gScreens.Refresh();
            break;
        
        case COMP_DATA_BEAM_HARDENING:
            {
                CSinograms *pSinograms = new CSinograms(pData);
                gScreens.AddData(pSinograms, nAdded++ == 0);
                gScreens.Refresh();

                CSinograms *pAttenuation = new CSinograms(pSinograms);
                gScreens.AddData(pAttenuation, false);
                gScreens.Refresh();

                CSinograms *pCorrected = new CSinograms(pData, pAttenuation);
                gScreens.AddData(pCorrected, false);
                gScreens.Refresh();
            }
            break;

        case COMP_DATA_OBLIQUE:
            if (pData->IsMultiFile())
            {
				bool bMultiOblFromSameData = false;
                if (!pOblPos && pOtherOblPos)
                {
                    if (pOtherOblique->IsComputedFrom(pData))
						bMultiOblFromSameData = true;
					else
                        pOblPos = pOtherOblPos;
                }
                COblique *pOblique = new COblique ((CMultiFile *)pData, pOblPos);
                gScreens.AddData(pOblique, nAdded++ == 0);
                gScreens.Refresh();
                if (!pOblPos)
                    pOblPos = (class CObliquePosition *)pOblique->GetPosition();
				if (bMultiOblFromSameData)
				{
					if (pOblPos)
						pOblPos->mbPropagate = false;
					if (pOtherOblPos)
						pOtherOblPos->mbPropagate = false;
				}
            }
            break;

        case COMP_DATA_MIP:
            if (pData->IsMultiFile())
            {
#ifdef __VR__
				CRotationMatrix4::TestInverse(20);
                CVolumeRenderingIF *pMip = new CVolumeRenderingIF (pData);
                gScreens.AddData(pMip, nAdded++ == 0);
                gScreens.Refresh();
#else
                CMipVolume *pMip = new CMipVolume ((CMultiFile *)pData);
                gScreens.AddData(pMip, nAdded++ == 0);
                gScreens.Refresh();
#endif
            }
            break;

        default:
            break;
        }
    }

}
void CFileFactory::CreateFiles(const char * zFilesFileName)
{
    if (mpfLog)
    {
        fprintf (mpfLog, "<CFileFactory::CreateFiles> %s\n\n", zFilesFileName);
        fflush (mpfLog);
    }
    FILE *pf = MyFOpen (zFilesFileName, "r");
    if (!pf)
        return;

    CList<class CMetaData*,class CMetaData*> dataList;

    int count = 0;
    int nAdded = 0;
	CString sfName;
	while (CMyWindows::ReadFileLine(pf, sfName))
    {
        if ((count++ % 100 == 0) || count < 10)
        {
            if (mpfLog)
            {
                fprintf (mpfLog, "<CFileFactory::CreateFiles> %s\n", sfName);
                fflush (mpfLog);
            }
        }
        // Check if object already in list
        bool bFound = false;
        POSITION pos = dataList.GetHeadPosition();
        while (pos)
        {
            CMetaData *pData = dataList.GetNext(pos);
            if (pData->ContainsFile (sfName))
            {
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            CFileFactory factory(mpfLog);
            CMetaData *pNewData = factory.CreateFileObject(sfName);
            if (pNewData)
            {
                dataList.AddTail(pNewData);
                gScreens.AddData(pNewData,nAdded++ == 0);
            }
        }
    }

    fclose (pf);
    if (nAdded)
        gScreens.Refresh();
}
