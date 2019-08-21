#ifndef __RECENTFILES_H__
#define __RECENTFILES_H__

#include "afxtempl.h"

//#include <afxwin.h>         // MFC core and standard components
//#include <afxext.h>         // MFC extensions

class CRecentFiles
{
public:
    static int umMaxRecentFiles;

    CRecentFiles();
    ~CRecentFiles();

    void init(CMenu *pSubMenu);
    void update_menu();
    void update(const char *zNewFileName);
    void update_registry();
    const char *MenuFileExist(int index);

private:
    CList<CString*,CString*> mFilesMenuList;
    CList<CString*,CString*> mFilesRegistryList;

    CMenu *mpSubMenu;
    DWORD mnFirstItemId;

    static const CString umsRegKey;
	bool mbReportOnFailureToAccessRegistry;
};

extern CRecentFiles gRecentFiles;



#endif