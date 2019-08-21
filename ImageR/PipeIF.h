#pragma once

#include "..\..\yUtils\PipeReader.h"
#include "MetaData.h"
#include "afxtempl.h"

class CPipeIF : public CPipeReader
{
public:
    CPipeIF(const char *zName, class CImageView *pViewer);
    ~CPipeIF(void);

    bool GetFirstFileName (CString &sName);

    static DWORD __stdcall StaticWaitImageThread (void  *p);
    DWORD __stdcall WaitImageThread (void);

protected:
    virtual void ProcessMessage (void);
    CList<CString *, CString *> mFilesList;
	virtual bool ShouldWait(void);

private:
    class CImageView *mpViewer;
public:
    CString msNextImageName;
    CString msImagePattern;
    int miNextImage;
    bool mbWaitInThread;
    int mnMaxToDisplay;
    bool mbWaitPrep;
    bool WaitPrep(void);
    void UpdateObjects(void);
    bool GetFirstSpecialName(CString & sName);
    bool GetFirstInt(int & value);
    bool GetFirstFloat(float & floatValue);

	void SendGraphicCommand(EGraphicCommand eGCom);

	void OnSetTitle(void);
	void OnSetPosition(void);
	void OnSetRelativePosition(void);
	void OnSetPosition2d(void);
	void OnSetZoom(void);
	void OnSetStatus(void);
	void OnGetWindow(void);
	void OnSetWindow(void);
	void OnSetWindowRange(void);
	void OnSetCursorBroadcast(void);
	int mNextFileUseSpecialPos;
	void OnDisplayColorMap(void);
	void OnDisplayImage(bool bReplace = false);
};



