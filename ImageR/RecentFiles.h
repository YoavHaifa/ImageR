#ifndef __RECENTFILES_H__
#define __RECENTFILES_H__

#include "afxtempl.h"

class CRecentFiles
{
public:
    static int umMaxRecentFiles;

    CRecentFiles();
    ~CRecentFiles();

    void init(CMenu *pSubMenu);
    void update_menu();
    void update(const char *zNewFileName);
    const char *MenuFileExist(int index);

private:
	void UpdateRegistry();
	void ReadFromRegistry();
	void ReadFromFile();
	void UpdateInFile();
	bool GetFileName();
	
	CString msfName;
	CList<CString*,CString*> mFilesMenuList;
    CList<CString*,CString*> mFilesRegistryList;

    CMenu *mpSubMenu;
    DWORD mnFirstItemId;

	static bool umbSaveInFile;
    static const CString umsRegKey;
	bool mbReportOnFailureToAccessRegistry;
};

extern CRecentFiles gRecentFiles;



#endif