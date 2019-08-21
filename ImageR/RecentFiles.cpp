#include "stdafx.h"
#include "RecentFiles.h"
#include "..\..\yUtils\FileName.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyRegistry.h"

const CString CRecentFiles::umsRegKey("SOFTWARE\\ImageR\\RecentFiles\\RecentFile");

CRecentFiles gRecentFiles;
int CRecentFiles::umMaxRecentFiles = 7;

CRecentFiles::CRecentFiles()
: mpSubMenu(NULL)
, mnFirstItemId(0)
, mbReportOnFailureToAccessRegistry(false)
{
}
CRecentFiles::~CRecentFiles()
{
}
void CRecentFiles::init(CMenu *pSubMenu)
{
    if (mpSubMenu)
        return;

    CMyRegistry myReg(CMyWindows::GetApplicationRegistryName(), KEY_READ);
    if (myReg)
	{
        myReg.Get("nRecentFiles", umMaxRecentFiles);
	}

    mpSubMenu = pSubMenu;
    mnFirstItemId = mpSubMenu->GetMenuItemID(0);

    HKEY hConfigsKey;
    LONG lRet;
    char KeyName[256];
 	
    for (int i = 0; i < umMaxRecentFiles; i++) 
    {
        sprintf_s(KeyName, 256, "%s%d", (const char *)umsRegKey, i);
        if ((lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
             KeyName, 0, KEY_READ, &hConfigsKey)) != ERROR_SUCCESS) 
        {
            continue;
        }
        char zBuf[256];
        DWORD bufLen=255;
        if (RegQueryValueEx(hConfigsKey, NULL, 0, NULL, (LPBYTE)zBuf, 
                    &bufLen) != ERROR_SUCCESS) 
        {
            continue;
        }
        RegCloseKey(hConfigsKey);
        if (CFileName::Exist(zBuf))
        {
            mFilesRegistryList.AddTail(new CString(zBuf));
            mFilesMenuList.AddTail(new CString(zBuf));
        }
    }
    update_menu();
}
void CRecentFiles::update_menu()
{
    if (!mpSubMenu)
        return;

    int nItems=mpSubMenu->GetMenuItemCount();
    POSITION pos = mFilesMenuList.GetHeadPosition();
    INT_PTR nRecent = mFilesMenuList.GetSize();
	int i;
    for (i=0; i < nItems && i < nRecent; i++) 
    {
        CString *psName = mFilesMenuList.GetNext(pos);
        mpSubMenu->ModifyMenu(i, MF_BYPOSITION, mnFirstItemId+i, (const char *)*psName);
        if (i == 0)
            mpSubMenu->EnableMenuItem(i, MF_BYPOSITION | MF_ENABLED);
    }

    if (nItems > nRecent) 
    {
        for (; i < nItems; i++) 
        {
            if (i == 0) 
            {
                mpSubMenu->ModifyMenu(i, MF_BYPOSITION, mnFirstItemId, "None");
                mpSubMenu->EnableMenuItem(i, MF_BYPOSITION | MF_GRAYED );
            } 
            else
                mpSubMenu->RemoveMenu(i, MF_BYPOSITION);
        }
    } 
    else 
    {
        for (; i < nRecent; i++) 
        {
            CString *psName = mFilesMenuList.GetNext(pos);
            mpSubMenu->AppendMenu(MF_STRING, mnFirstItemId+i, (const char *)*psName);
        }
    }
}
void CRecentFiles::update(const char *zNewFileName)
{
    if (!zNewFileName || !CFileName::Exist(zNewFileName))
		return;

    mFilesRegistryList.AddHead (new CString(zNewFileName));

    POSITION pos = mFilesRegistryList.GetHeadPosition();
    mFilesRegistryList.GetNext(pos);
    while (pos)
    {
        POSITION lastPos = pos;
        CString *psName = mFilesRegistryList.GetNext(pos);
        if (*psName == zNewFileName)
        {
            mFilesRegistryList.RemoveAt(lastPos);
            break;
        }
    }

    if (mFilesRegistryList.GetSize() > umMaxRecentFiles)
    {
        CString *psName = mFilesRegistryList.RemoveTail();
        delete psName;
    }
    update_registry();
    //update_menu(); // Don't update menu here - menu stays the same after initializing
}
void CRecentFiles::update_registry()
{
    HKEY hConfigsKey;
    LONG lRet;
    char KeyName[256];

    POSITION pos = mFilesRegistryList.GetHeadPosition();

    for (int i = 0; i < umMaxRecentFiles; i++) 
    {
        sprintf_s(KeyName, 256, "%s%d", (const char *)umsRegKey, i);
        if ((lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, NULL, 0,
                        KEY_WRITE, NULL, &hConfigsKey, NULL)) != ERROR_SUCCESS) 
		{
			if (mbReportOnFailureToAccessRegistry)
			{
				char zBuf [256];
				sprintf_s (zBuf, 256, "Can't create key in REGISTRY %s", KeyName);
			    MessageBox(NULL, zBuf, "Recent Files", MB_OK);
			}
            break;
        }
        if (pos) 
        {
            CString *psName = mFilesRegistryList.GetNext(pos);
            const char *zName = (const char *)*psName;
            size_t bufLen = psName->GetLength() + 1; // Add NULL terminator
            if (RegSetValueEx(hConfigsKey, NULL, 0, REG_SZ, (LPBYTE)zName, 
                                        (DWORD)bufLen) != ERROR_SUCCESS) 
            {
                RegCloseKey(hConfigsKey);
                break;
            }
        } 
        else
            RegDeleteValue(hConfigsKey, NULL);
        RegCloseKey(hConfigsKey);
    }
}
const char *CRecentFiles::MenuFileExist(int index)
{
    POSITION pos = mFilesMenuList.FindIndex(index);
    if (!pos)
        return NULL;
    CString *psName = mFilesMenuList.GetAt(pos);
    if (CFileName::Exist(*psName))
        return *psName;
         
    mFilesMenuList.RemoveAt(pos);
    update_menu();
    CMyWindows::PrintStatus("Missing Recent File");
    return NULL;
}
