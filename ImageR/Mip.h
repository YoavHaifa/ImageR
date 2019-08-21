#pragma once
#include "afxtempl.h"
#include "MetaData.h"

class CMip :
    public virtual CMetaData
{
public:
    CMip(void);
    virtual ~CMip(void);

    virtual CString GetWindowName(void){return CString("MipWindow");}
    virtual bool IsMip(void){return true;}
    virtual CString GetZoomName(void){return CString("MipZoom");}
    virtual void OnAssignDR (void);
    virtual bool OnMousePos(CDataCoordinates &dataCoo);
    virtual bool OnMousePosOnRef(CDataCoordinates &dataCoo);
    virtual bool OnMousePosNotOnRef(void);
	virtual class CMip *PMip(void){return this;}
	virtual void OnPositionChange(void);
	virtual bool OnLButtonUp(class CGraphicElement *pGE);
	virtual bool OnRefDataUpdate(void);

protected:
    static class CGCursor *umpRefToImage;
    class CGCursor *mpRefToMip;
	static FILE *umpfLog;
    class CGEllipse *mpVoi;

    class CDataVolume *mpTopoVoi;
    class CGContour *mpTopoContour;
	class CDataRoiLineBounds *mpVoiBounds;

private:
    void UpdateRefToImagePos(void);
    bool UpdateImageRefToMip(void);
	bool VerifyPointer(void * p, const char *zText);

public:
    void SelectSlice(void);
public:
public:
	virtual void SetVoi(void){}
	virtual void SetTopoVoi(void){}
};
