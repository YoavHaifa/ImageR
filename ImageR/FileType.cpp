#include "StdAfx.h"
#include "FileType.h"
#include "..\..\yUtils\XMLParse.h"
#include "..\..\yUtils\FileName.h"
#include "..\..\ImageRLib\MetaDataBase.h"


CFileType::CFileType(CXMLParseNode* pTypeNode)
	: msName("UndefinedFileType")
	, meDataType(DATA_TYPE_FLOAT)
	, mnCols(512)
	, mnLines(512)
	, mbMultiSlice(false)
{
	pTypeNode->GetValue("name", msName);
	pTypeNode->GetValue("extension", msExtension);
	pTypeNode->GetValue("n_lines", mnLines);
	pTypeNode->GetValue("n_cols", mnCols);
}
bool CFileType::IsOfType(const char* zfName)
{
	if (msExtension.GetLength() > 0)
	{
		if (!CFileName::StaticIsOfType(zfName, msExtension))
			return false;
	}

	int nUnits = mnLines * mnCols;
	if (nUnits < 100)
		return false;

	unsigned int dataUnitSize = CMetaDataBase::DataUnitSizePerDataType(meDataType);
	unsigned int rasterSize = nUnits * dataUnitSize;

	__int64 length = CFileName::GetFileLength(zfName);
	if (length % rasterSize == 0)
		return true;

	return false;
}