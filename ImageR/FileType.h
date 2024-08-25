#pragma once
#include "..\..\ImageRLib\MyImage.h"

class CFileType
{
public:
	CFileType(class CXMLParseNode* pTypeNode);
	bool IsOfType(const char *zfName);

	CString msName;
	CString msExtension;
	EMyImageDataType meDataType;
	int mnLines;
	int mnCols;
	bool mbMultiSlice;
};

