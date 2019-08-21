#include "StdAfx.h"
#include "TextController.h"
#include "MetaData.h"
#include "DRText.h"

//------------------------------------------------------------
// Text Controller
// ---------------
// The text controller is a global object
// In the constructor it reads an xml file to get wanted configuration
// 
// Requirements
// ------------
// Different sets of texts for different types/names of images
// Any number of text items per image
// Initially all texts are on the left upper corner, one under the other
// Support bed position text as it is currently shown by Archives File class
// Support any number of dicom text fields
//
// Implementation
// --------------
// Current bed position is defined as data-related text
//		In this case - the initiative comes from the data...
// Dicom text is relevant when dicom is available
//
// Cooperation with Display Rectangle (DR)
// ---------------------------------------
// Policy is checked when new data is introduced to DR
//		Text Objects are created and assigned to DR, Data Dependant
// Text content should be updated for any new page!
//		Updates depend on the text object itself, without global controller.
//   

CTextController gTexts;
CString CTextController::umsTextsFileName(_T("ImageRTexts.xml"));

CTextController::CTextController(void)
: mpAppParse(NULL)
, mpAppRoot(NULL)
{
}

CTextController::~CTextController(void)
{
	if (mpAppParse)
		delete mpAppParse;
}
bool CTextController::Init(void)
{
	if (mpAppParse || mpAppRoot)
		return mpAppRoot != NULL;

	//CLogFile log("CTextController_Init");
	CString sClientName;
	CString sClientWithPath;
	CString sClientPath;
	if (CMyWindows::GetClientName(sClientName, &sClientWithPath))
	{
		CFileName fName(sClientWithPath);
		sClientPath = fName.Path();
	}
	//if (log.mpf)
	//{
	//	fprintf (log.mpf, "sClientName %s\n", sClientName);
	//	fprintf (log.mpf, "sClientWithPath %s\n", sClientWithPath);
	//	fprintf (log.mpf, "sClientPath %s\n", sClientPath);
	//}

	CString sParseFileName;
	if (!sClientPath.IsEmpty())
		sParseFileName = sClientPath + umsTextsFileName;
	if (sParseFileName.IsEmpty() || !CFileName::Exist(sParseFileName))
	{
		//if (log.mpf)
		//	fprintf (log.mpf, "NOT FOUND: sParseFileName %s\n", sParseFileName);
		sParseFileName = CMyWindows::GetApplicationPath() + umsTextsFileName;
	}
	
	mpAppParse = new CXMLParse(sParseFileName);
	mpAppRoot = mpAppParse->GetRoot();
	//if (log.mpf)
	//	fprintf (log.mpf, "sParseFileName %s\n", sParseFileName);

	if (mpAppRoot && !sClientName.IsEmpty())
	{
		CXMLParseNode *pNode = mpAppRoot->GetFirst(sClientName);
		if (pNode)
			mpAppRoot = pNode;
		//if (log.mpf)
		//	fprintf (log.mpf, "mpAppRoot %s\n", mpAppRoot->msName);
	}
	return (mpAppRoot != NULL);
}
bool CTextController::SetApplicationTexts(CMetaData *pData)
{
	if (!Init())
		return false;

	CString sClass(pData->GetClass());

	// Look for definition of wanted texts
	CXMLParseNode *pDefNode = mpAppRoot->GetFirstDeep("def");
	while (pDefNode)
	{
		if (pDefNode->GetFirstDeep(sClass))
			break;
		pDefNode = mpAppRoot->GetNextDFS("def", &pDefNode);
	}
	if (!pDefNode)
		return false;

	CXMLParseNode *pTextNode = pDefNode->GetFirstDeep("text");
	while (pTextNode)
	{
		unsigned long iDicom;
		if (pTextNode->GetValue("dicom",iDicom))
		{
			CDRText *pText = pData->AddDataText("dicom");
			pText->SetDicom(iDicom);
			CString sTitle;
			if (pTextNode->GetValue("title",sTitle))
				pText->SetTitle(sTitle);
			int titleId = 0;
			if (pTextNode->GetValue("title_id",titleId))
				pText->SetTitle(titleId);
			if (pTextNode->GetValue("right_title_id",titleId))
				pText->SetRightTitle(titleId);
		}
		else if (pTextNode->GetFirstDeep("data_text"))
		{
			pData->AddDataText("DataText");
		}
		else if (pTextNode->GetFirstDeep("resolution"))
		{
			CDRText *pText = pData->AddDataText("resolution");
			pText->meTextType = CDRText::TEXT_RESOLUTION;
		}
		else if (pTextNode->GetFirstDeep("image_number"))
		{
			CDRText *pText = pData->AddDataText("iNumber");
			pText->meTextType = CDRText::TEXT_IMAGE_NUMBER;
		}
		else if (pTextNode->GetFirstDeep("image_balance"))
		{
			CDRText *pText = pData->AddDataText("bBalance");
			pText->meTextType = CDRText::TEXT_IMAGE_BALANCE;
		}
		else if (pTextNode->GetFirstDeep("ring_level"))
		{
			CDRText *pText = pData->AddDataText("RLI");
			pText->meTextType = CDRText::TEXT_RING_LEVEL;
			pTextNode->GetValue("from",pText->miFrom);
			pTextNode->GetValue("till",pText->miTill);
		}
		else if (pTextNode->GetFirstDeep("ring_level_sum"))
		{
			CDRText *pText = pData->AddDataText("RLI_SUM");
			pText->meTextType = CDRText::TEXT_RING_LEVEL_SUM;
			pTextNode->GetValue("from",pText->miFrom);
			pTextNode->GetValue("till",pText->miTill);
		}
		else if (pTextNode->GetFirstDeep("recon_angle"))
		{
			CDRText *pText = pData->AddDataText("reconAngle");
			pText->meTextType = CDRText::TEXT_RECON_ANGLE;
		}
		pTextNode = pDefNode->GetNextDFS("text", &pTextNode);
	}
	return true;
}
bool CTextController::SetPathImageText(class CMetaData *pData, CList<class CDRText*,class CDRText*> &list)
{
	while (!list.IsEmpty())
	{
		delete list.GetTail();
		list.RemoveTail();
	}

	CFileName fName(pData->Name());
	CString sPath(fName.Path());
	sPath += "\\ImageRTexts\\";
	sPath += fName.Private();
	sPath += ".txt";
	if (!CFileName::Exist(sPath))
		return false;

	//CString sText;
	FILE *pf = MyFOpen(sPath, "r");

	char zBuf[256];
	while (fgets(zBuf,sizeof(zBuf)-1, pf))
	{
		//sText += zBuf;
		CDRText *pText = new CDRText(zBuf);
		pText->SetOnData(pData);
		pText->DisableStateRestore();
		pText->SetDataSpecific(true);
		pText->SetAtLine((int)list.GetSize());
		pText->meTextType = CDRText::TEXT_XML;
		list.AddTail(pText);
	}

	fclose(pf);
	return true;
}
void CTextController::SetPathTexts(CMetaData *pData)
{
	CFileName fName(pData->Name());
	CString sPath(fName.Path());
	sPath += "\\MyParams\\ImageRDirTexts.xml";
	if (!CFileName::Exist(sPath))
		return;

	CXMLParse xmlParse(sPath);
	CXMLParseNode *pRoot = xmlParse.GetRoot();

	POSITION pos = pRoot->GetHeadPosition();
	while (pos)
	{
		CXMLParseNode *pField = pRoot->GetNext(pos);
		if (pField)
		{
			CXMLParseNode *pValue = pField->GetFirst("value");
			if (pValue)
			{
				char zBuf[128];
				sprintf_s (zBuf, 128, "%s %s", pField->msName, pValue->msValue);
				CDRText *pText = pData->AddDataText(zBuf);
				pText->meTextType = CDRText::TEXT_XML;
			}
		}
	}

}
void CTextController::SetDataTexts(CMetaData *pData)
{
	if (!SetApplicationTexts(pData))
	{
		int nTexts = pData->NeedDataTexts();
		while (nTexts-- > 0)
			pData->AddDataText("DataText");
	}

	if (pData->IsFile())
		SetPathTexts(pData);
}
