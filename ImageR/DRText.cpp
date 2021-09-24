#include "StdAfx.h"
#include "DRText.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\MyDicomPrint.h"
#include "..\..\ImageRLib\DataCursor.h"
#include "..\..\ImageRLib\RingLevelInfo.h"
#include "..\..\ImageRLib\Position.h"
#include "Screens.h"
#include "MetaData.h"
#include "DisplayRectangle.h"
	
int CDRText::I_FIRST_LINE = 10;
int CDRText::I_LINE_INC = 20;

CDRText::CDRText(const char *zText, int iLine, int iCol, bool bAbsolute)
: CGCursor ("Text")
, msText(zText)
, miLine(iLine)
, miCol(iCol)
, miTitle(0)
, miRightTitle(0)
, mbAbsoluteColumn(bAbsolute)
, mpData(NULL)
, mbDisableStateRestore(false)
, miDicom(0)
, meTextType(TEXT_NONE)
, miFrom(0)
, miTill(0)
{
	mbDisplayInactivePoints = false;
	mbDisplayStatisticsAsText = false;
}

CDRText::CDRText(CMetaData *pData, CDisplayRectangle *pDispRect)
: CGCursor (pData, pDispRect, "text")
, miLine(-1)
, miCol(-1)
, miTitle(0)
, miRightTitle(0)
, mbAbsoluteColumn(false)
, mpData(NULL)
, mbDisableStateRestore(false)
, miDicom(0)
, meTextType(TEXT_NONE)
{
	mbDisplayInactivePoints = false;
	mbDisplayStatisticsAsText = false;
}

CDRText::CDRText(class CXMLParseNode *pTextXml)
: CGCursor ("Text")
, msText(_T(""))
, miLine(0)
, miCol(0)
, miTitle(0)
, miRightTitle(0)
, mbAbsoluteColumn(false)
, mpData(NULL)
, mbDisableStateRestore(false)
, miDicom(0)
, meTextType(TEXT_NONE)
{
	mbDisplayInactivePoints = false;
	mbDisplayStatisticsAsText = false;
    pTextXml->GetValue("text", msText);
    pTextXml->GetValue("line", miLine);
    pTextXml->GetValue("column", miCol);
    pTextXml->GetValue("b_absolute", mbAbsoluteColumn);
}

CDRText::CDRText(class CDisplayRectangle *pDispRect, class CXMLParseNode *pTextXml)
: CGCursor(pDispRect, pTextXml)
, msText(_T(""))
, miLine(0)
, miCol(0)
, miTitle(0)
, miRightTitle(0)
, mbAbsoluteColumn(false)
, mpData(NULL)
, mbDisableStateRestore(false)
, miDicom(0)
, meTextType(TEXT_NONE)
{
	mbDisplayInactivePoints = false;
	mbDisplayStatisticsAsText = false;
}

CDRText::~CDRText(void)
{
}

void CDRText::Set(const char *zText)
{
	msText = zText;
}

//void CDRText::SaveState(class CXMLDump & xmlDump)
//{
//    CXMLDumpScope graph("DRText",xmlDump);
//    
//    //CNamedObject::SaveState(xmlDump);
//
//    xmlDump.Write("text", msText);
//	//xmlDump.Write("size", miSize);	// Modification
//    xmlDump.Write("line", miLine);
//    xmlDump.Write("column", miCol);
//	if (mbDisableStateRestore)
//		xmlDump.Write("disable_state_restore",mbDisableStateRestore);
//}
void CDRText::Update(int iText)
{
	if (!mpData)
		return;

	if (miDicom)
	{
		CMyDicom *pDicom = mpData->GetDicom();
		if (pDicom)
		{
			unsigned short group = (unsigned short)(miDicom >> 16);
			unsigned short num = (unsigned short)(miDicom % 0x10000);
			if ((group == PATIENT_NAME_GROUP) && (num == PATIENT_NAME_NUM) && !gScreens.mbEbwMode)
				Set(pDicom->GetPatientName());
			else
				Set(pDicom->GetTextField(group, num));
		}
	}
	else if (meTextType == TEXT_AVERAGING)
	{
		Set(mpData->GetAveragingText());
	}
	else if (meTextType == TEXT_XML)
	{
	}
	else if (mbDataSpecific)
		Set(mpData->GetPageDataText(iText));
}
void CDRText::SetAtLine(int iLine)
{
	miLine = miLine + iLine * I_LINE_INC;
}
