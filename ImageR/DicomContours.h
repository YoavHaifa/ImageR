#pragma once
#include "ArchivesFile.h"
#include "..\..\yUtils\CompVector.h"

class CDicomContours :
	public CArchivesFile
{
public:
	CDicomContours(const char *zName, class CMyDicom *pDicom, class CXMLParseNode *pMetaDataXml);
	~CDicomContours(void);

	virtual bool DoUpdatePageData(void);

private:
	int mnContours;

	CList<class CMyDicomTagRef*,class CMyDicomTagRef*> mContoursDataTagsRefs;
	CList<class CMyDicomTagRef*,class CMyDicomTagRef*> mContoursNPointsTagsRefs;

	CCompVector mX;
	CCompVector mY;
	CCompVector mZ;
	int mnPoints;

	const unsigned char *mpChar;
	const unsigned char *mpCharStart;
	const unsigned char *mpCharEnd;
	float GetNextPointValue(void);
	bool OnNewDicom(void);

	bool mbFillValues;
	short mValueToFill;
	int mDebug;
	int miLastFailedDicom;
	int miContour;
public:
	int miImageLoaded;
	bool VerifyImage(void);
};

