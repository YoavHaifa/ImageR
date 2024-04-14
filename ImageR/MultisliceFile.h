#pragma once
#include "./MultiDataFile.h"

class CMultisliceFile : public CMultiDataFile
{
public:
	CMultisliceFile(const char* zfName, CXMLParseNode* pMetaDataXml,
		unsigned int fileHeaderBytes = 0);
	~CMultisliceFile();

	virtual bool DoUpdatePageData(void) override;
	virtual void ComputeFilePageLenNeto() override;

protected:
	void ConvertAllMultislices();
	void ExpandMultislice(float* pIn, float* pOut);
	void ExpandLine(float* pInLeft, float* pInHR, float* pInRight, float* pOut);
	void ExpandLR(float* pIn, float* pOut, int n);

	int mnLRLeft;
	int mnLRRight;
	int mnHR;
	int mnMultiSlices;
	int mnMultiSliceSamplesInFile;
	int mnMultiSliceSizeInFileBytes;
	int mnPageSizeInFile;

	float* mpReadBuffer;
};

