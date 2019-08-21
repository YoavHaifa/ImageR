#pragma once
#include "Volumetric.h"
#include "Mip.h"
#pragma warning(disable:4250) 

class CMipVolume :
	public CVolumetric, public CMip
{
public:
    CMipVolume(CMetaData *pData);
    CMipVolume(class CXMLParseNode *pXmlNode);
	virtual ~CMipVolume(void);

public:
    virtual bool ShouldUpdate(void);
    virtual bool DoUpdatePageData(void);
	virtual class CMipVolume *PMipVolume(void){return this;}
    void AddTissue(class CVolTissue * pTissue);
	virtual void SetVoi(void);
	virtual void SetTopoVoi(void);
    virtual void Bye(CDependant *pObject);

private:
    CList<class CVolTissue *, class CVolTissue *> mTissuesList;
    CList<class CVolTissue *, class CVolTissue *> mTissuesToRemove;
    CTImage<bool> *mpVOIMask;
    class CLinesTable *mpVOILinesTable;
    void InitOnConstruction(void);
    int mnImageLines;
    int mnImagePixels;

    void ProjectImageByMask(unsigned short * pImage, unsigned short * pMipLine);
    void ProjectImageRemoveTissue(unsigned short * pImage, bool *pTissue, unsigned short * pMipLine);
    void ProjectImageRemove2Tissues(unsigned short * pImage, bool *pTissue1, bool *pTissue2, unsigned short * pMipLine);
    int FindTissueToRemove(void);
	int mRemovedTissuesId;
	static int umnMipVois;
};
