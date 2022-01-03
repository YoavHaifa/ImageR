#include "StdAfx.h"
#include "ObliquePosition.h"
#include "..\..\yUtils\XMLDump.h"
#include "..\..\yUtils\XMLParse.h"

CObliquePosition::CObliquePosition(CXMLParseNode *pXmlNode)
: CPosition("OblPos")
, meType (COblique::OBL_UNDEF)
{
    if (pXmlNode)
    {
        SetFromXml(pXmlNode);
        SetParseId(pXmlNode);
    }
}
CObliquePosition::CObliquePosition(CObliquePosition &other)
: CPosition(other)
, meType (other.meType)
{
}
CObliquePosition::~CObliquePosition(void)
{
}
bool CObliquePosition::AllowsSync()
{
    return false;
}
bool CObliquePosition::SetType(COblique::EOblType eType, int iLast)
{
    if (meType == eType)
        return false;

    // Save current position per type
    if (meType != COblique::OBL_UNDEF)
    {
        CObliquePosition *pSaved = FindSaved(meType);
        if (pSaved)
            pSaved->miCurrent = miCurrent;
        else
            mSavedList.AddTail (new CObliquePosition(*this));
    }

    // Set position to saved value of new type
    CObliquePosition *pSaved = FindSaved(eType);
    if (pSaved)
        miCurrent = pSaved->miCurrent;
	miLast = iLast;

    meType = eType;
    OnChange();
    return true;
}
void CObliquePosition::SaveState(class CXMLDump & xmlDump)
{
    CPosition::SaveState(xmlDump);
    xmlDump.Write ("oblique_type", COblique::GetTypeName(meType));

    if (!mSavedList.IsEmpty())
    {
        CXMLDumpScope savedList("saved_positions", xmlDump);
        POSITION pos = mSavedList.GetHeadPosition();
        while (pos)
        {
            CObliquePosition *pSaved = mSavedList.GetNext(pos);
            CXMLDumpScope savedPos (COblique::GetTypeName(pSaved->meType), xmlDump);
            xmlDump.Write ("current", pSaved->miCurrent);
        }
    }
}
void CObliquePosition::SetFromXml(CXMLParseNode * pPositionXml)
{
    CPosition::SetFromXml(pPositionXml);
    CString sType;
    if (pPositionXml->GetValue ("oblique_type", sType))
        meType = COblique::GetType(sType);
}
CObliquePosition *CObliquePosition::FindSaved(COblique::EOblType eType)
{
    POSITION pos = mSavedList.GetHeadPosition();
    while (pos)
    {
        CObliquePosition *pSaved = mSavedList.GetNext(pos);
        if (pSaved->meType == eType)
            return pSaved;
    }
    return NULL;
}