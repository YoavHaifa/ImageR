#pragma once
#include "Volumetric.h"

class COblique :
    public CVolumetric
{
public:
    typedef enum EOblType
    {
        OBL_UNDEF,
        OBL_CORONAL,
        OBL_SAGITTAL,
        OBL_GENERAL,
        OBL_LAST
    } EOblType;

    COblique(class CMultiFile *pData, class CObliquePosition *pOblPos = NULL);
    COblique(class CXMLParseNode *pXmlNode);
    ~COblique(void);

    void InitOnConstruction(class CXMLParseNode *pXmlNode = NULL);

    virtual bool DoUpdatePageData(void);
    virtual CString GetPageText (void) {return CString("Obl");}
    virtual bool IsOblique() {return true;}
    void SetCoronal(void);
    void SetSagittal(void);
    virtual CString GetZoomName(void);
    virtual void OnAssignDR (void);
	class CObliquePosition *GetOblPos(void) {return mpOblPos;}

protected:
    class CObliquePosition &GetPos(void){return *(class CObliquePosition *)GetPosition();}
	class CObliquePosition *mpOblPos;
	CString GetCurrentImageName();
	class CObliqueRef* mpOblRefOnIm;

public:
    static CString GetTypeName (EOblType eType);
    static EOblType GetType (const char *zType);
};
