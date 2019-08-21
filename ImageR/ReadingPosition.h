#pragma once
#include "..\..\ImageRLib\Position.h"

class CReadingPosition :
    public CPosition
{
public:
    CReadingPosition(const char *zName, class CReadingsFile *pFile);
    CReadingPosition(class CXMLParseNode *pPositionXml, class CReadingsFile *pFile);
    ~CReadingPosition(void);

    unsigned long miSlice;
    unsigned long miFFS;
    CReadingsFile *mpFile;

    virtual bool First(void);
    virtual bool Last(void);
    virtual CString GetText(void);
    virtual int GetTextSize(void) {return 170;}
    virtual bool UseSpecialGoto(void){return true;}
    virtual void OnViewGotopage(void);

    virtual bool LineLeft(void);
    virtual bool LineRight(void);

    virtual void SaveState(class CXMLDump & xmlDump);
    virtual CString GetFileNameText(void);
    int GetUISliceIndex(void);
    void SetSliceIndexFromUI(int iUISlice);
};
