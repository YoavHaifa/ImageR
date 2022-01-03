#pragma once
#include "..\..\ImageRLib\Position.h"
#include "Oblique.h"
#include "afxtempl.h"

class CObliquePosition :
    public CPosition
{
public:
    CObliquePosition(class CXMLParseNode *pXmlNode);
    CObliquePosition(CObliquePosition &other);
    ~CObliquePosition(void);

    bool SetType(COblique::EOblType eType, int iLast);
    COblique::EOblType GetType(void){return meType;}

private:
    COblique::EOblType meType;
    CList<CObliquePosition*,CObliquePosition*> mSavedList;
    CObliquePosition *FindSaved(COblique::EOblType eType);

public:
    void SaveState(class CXMLDump & xmlDump);
    virtual void SetFromXml(class CXMLParseNode * pPositionXml);
    virtual bool AllowsSync();
};
