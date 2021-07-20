#include "stdafx.h"
#include "RecentFiles.h"
#include "..\..\yUtils\FileName.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyRegistry.h"

const CString CRecentFiles::umsRegKey("SOFTWARE\\ImageR\\RecentFiles\\RecentFile");

CRecentFiles gRecentFiles;
int CRecentFiles::umMaxRecentFiles = 7;
bool CRecentFiles::umbSaveInFile = true;

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

	if (umbSaveInFile)
		ReadFromFile();
	else
		ReadFromRegistry();


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

	if (umbSaveInFile)
		UpdateInFile();
	else
	    UpdateRegistry();
    //update_menu(); // Don't update menu here - menu stays the same after initializing
}
void CRecentFiles::ReadFromRegistry()
{
	HKEY hConfigsKey;
	LONG lRet;
	char KeyName[256];

	for (int i = 0; i < umMaxRecentFiles; i++)
	{
		sprintf_s(KeyName, 256, "%s%d", (const char*)umsRegKey, i);
		if ((lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			KeyName, 0, KEY_READ, &hConfigsKey)) != ERROR_SUCCESS)
		{
			continue;
		}
		char zBuf[256];
		DWORD bufLen = 255;
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
}
bool CRecentFiles::GetFileName()
{
	msfName = CMyWindows::GetApplicationPath();
	if (msfName.IsEmpty())
		return false;
	msfName += CMyWindows::GetApplicationName();
	msfName += ".RecentFiles.txt";
	return true;
}
void CRecentFiles::ReadFromFile()
{
	if (!GetFileName())
		return;

	FILE* pf = NULL;
	fopen_s(&pf, msfName, "r");
	if (!pf)
		return;

	char aLine[256];
	while (fgets(aLine, sizeof(aLine), pf))
	{
		int len = (int)strlen(aLine);
		if (aLine[len-1] == '\n')
			aLine[len-1] = 0;
		if (CFileName::Exist(aLine))
		{
			mFilesRegistryList.AddTail(new CString(aLine));
			mFilesMenuList.AddTail(new CString(aLine));
			if (mFilesRegistryList.GetSize() >= umMaxRecentFiles)
				break;
		}
	}

	fclose(pf);
}
void CRecentFiles::UpdateInFile()
{
	if (!GetFileName())
		return;
	FILE* pf = NULL;
	fopen_s(&pf, msfName, "w");
	if (!pf)
		return;

	int n = 0;
	POSITION pos = mFilesRegistryList.GetHeadPosition();
	while (pos && n < umMaxRecentFiles)
	{
		CString* psName = mFilesRegistryList.GetNext(pos);
		fprintf(pf, "%s\n", (const char*)*psName);
		n++;
	}
	fclose(pf);
}
void CRecentFiles::UpdateRegistry()
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
