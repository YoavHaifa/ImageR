#pragma once
#include "afxtempl.h"

class CFileTypes
{
public:
	CFileTypes();
	void Init();
	class CFileType* GetFileType(const char *zfName);
	bool SearchForType(const char *zfName, class CMetaDataBase *pMetaData);

	bool mbArineta;
	CList<class CFileType*, class CFileType*> mTypes;
};

extern CFileTypes gFileTypes;