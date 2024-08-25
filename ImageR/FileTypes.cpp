#include "StdAfx.h"
#include "FileTypes.h"
#include "FileType.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\ImageRLib\MetaDataBase.h"

CFileTypes gFileTypes;

CFileTypes::CFileTypes()
	: mbArineta(false)
{
}

void CFileTypes::Init()
{
	CXMLParse parseFile(CMyWindows::GetApplicationPath() + "FileTypes.xml");
	CXMLParseNode* pRoot = parseFile.GetRoot();
	if (!pRoot)
		return;

	pRoot->GetValue("bArineta", mbArineta);

	CXMLParseNode* pTypeNode = pRoot->GetFirst("type");
	while (pTypeNode)
	{
		CFileType* pType = new CFileType(pTypeNode);
		mTypes.AddTail(pType);

		pTypeNode = pRoot->GetNext("type", pTypeNode);
	}
}
CFileType* CFileTypes::GetFileType(const char* zfName)
{
	if (mTypes.IsEmpty())
		return NULL;

	POSITION pos = mTypes.GetHeadPosition();
	while (pos)
	{
		CFileType* pType = mTypes.GetNext(pos);
		if (pType->IsOfType(zfName))
			return pType;
	}
	return NULL;
}
bool CFileTypes::SearchForType(const char* zfName, class CMetaDataBase* pMetaData)
{
	CFileType* pType = GetFileType(zfName);
	if (!pType)
		return NULL;

	pMetaData->SetNCols(pType->mnCols);
	pMetaData->SetNLinesInPage(pType->mnLines);
	pMetaData->SetDataType(pType->meDataType);
	return true;
}