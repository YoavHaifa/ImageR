#include "StdAfx.h"
#include "MenuController.h"
#include "ImageView.h"
#include "RecentFiles.h"
#include "Resource.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\XMLParse.h"

#define XMLFILE "menu.xml"

CMenuController::CMenuController(void)
: mpMenu(NULL)
{
	CWnd * pMain = AfxGetMainWnd();
	if (pMain)
		mpMenu = pMain->GetMenu();   
}

int CMenuController::Convert(CString XMLString)
{
	if (XMLString == "file/open")
		return ID_FILE_OPEN;
	if (XMLString == "file/add/file")
		return ID_ADD_FILE;
	if (XMLString == "file/add/diff")
		return ID_ADD_DIFF;
	if (XMLString == "file/add/ratio")
		return ID_ADD_RATIO;
	if (XMLString == "file/add/coronal")
		return ID_ADD_CORONAL;
	if (XMLString == "file/add/smooth")
		return ID_ADD_SMOOTH;
	if (XMLString == "file/add/mip")
		return ID_ADD_MIP;
	if (XMLString == "file/open directory")
		return ID_FILE_OPENDIRECTORY;
	if (XMLString == "file/save state")
		return ID_FILE_SAVE_STATE;
	if (XMLString == "file/delete")
		return ID_FILE_DELETE;
	if (XMLString == "file/exit")
		return ID_FILE_EXIT1;
	if (XMLString == "view/statistics")
		return ID_VIEW_STATISTICS;
	if (XMLString == "view/go to...")
		return ID_EDIT_GOTO;
	if (XMLString == "view/zoom")
		return ID_VIEW_ZOOM;
	if (XMLString == "view/new zoom")
		return ID_VIEW_NEWZOOM;
	if (XMLString == "view/prev file")
		return ID_VIEW_PREV;
	if (XMLString == "view/next file")
		return ID_VIEW_NEXT;
	if (XMLString == "view/toggle colors")
		return ID_VIEW_TOGGLECOLORS;
	if (XMLString == "view/toggle text")
		return ID_VIEW_TOGGLETEXT;
	if (XMLString == "view/data coo")
		return ID_VIEW_DATACOO;
	if (XMLString == "view/refresh")
		return ID_VIEW_REFRESH;
	if (XMLString == "settings/new position")
		return ID_SETTINGS_NEWPOSITION;
	if (XMLString == "settings/set full roi")
		return ID_SETTINGS_SETFULLROI;
	if (XMLString == "settings/set display center")
		return ID_SETTINGS_SETDISPLAYCENTER;
	if (XMLString == "settings/select images by parameters")
		return ID_SETTINGS_SELECTIMAGESBYPARAMETERS;
	if (XMLString == "settings/interactive tuning")
		return ID_SETTINGS_INTERACTIVETUNING;
	if (XMLString == "window/window")
		return ID_PARAMETERS_WINDOW;	
	if (XMLString == "window/auto wide")
		return ID_WINDOW_AUTO;
	if (XMLString == "window/auto focus")
		return ID_WINDOW_AUTOFOCUS;
	if (XMLString == "window/new window")
		return ID_SETTINGS_SETSECONDWINDOW;
	if (XMLString == "window/select window")
		return ID_WINDOW_SELECTWINDOW;
	if (XMLString == "window/local focus")
		return ID_WINDOW_LOCALFOCUS;
	if (XMLString == "graphic/add cursor")
		return ID_GRAPHIC_ADDCURSOR;
	if (XMLString == "graphic/add rectangle")
		return ID_GRAPHIC_ADDRECTANGLE;
	if (XMLString == "graphic/add ellipse")
		return ID_GRAPHIC_ADDELLIPSE;
	if (XMLString == "graphic/add circle")
		return ID_GRAPHIC_ADDCIRCLR;
	if (XMLString == "graphic/add line")
		return ID_GRAPHIC_ADDLINE;
	if (XMLString == "graphic/add contour")
		return ID_GRAPHIC_ADDCONTOUR;
	if (XMLString == "graphic/add screen text")
		return ID_GRAPHIC_ADDTEXT;
	if (XMLString == "graphic/delete all graphics")
		return ID_GRAPHIC_DELETEALLGRAPHICS;
	if (XMLString == "graphic/delete all texts")
		return ID_GRAPHIC_DELETEALLTEXTS;
	if (XMLString == "print/col to excel")
		return ID_PRINT_EXCEL;
	if (XMLString == "print/screen to excel")
		return ID_PRINT_SCREENTOEXCEL;
	if (XMLString == "print/dump slice")
		return ID_PRINT_DUMPSLICE;
	if (XMLString == "print/dump all slices")
		return ID_PRINT_DUMPALLSLICES;
	if (XMLString == "print/save zoomed")
		return ID_PRINT_SAVEZOOMED;
	if (XMLString == "dicom/print")
		return ID_DICOM_PRINT;
	if (XMLString == "dicom/view tables")
		return ID_DICOM_VIEWTABLES;
	if (XMLString == "dicom/add air table")
		return ID_DICOM_CHANGETABLE;
	if (XMLString == "dicom/edit text fields")
		return ID_DICOM_EDITTEXTFIELDS;
	if (XMLString == "dicom/amend all by xml")
		return ID_DICOM_AMENDALLBYXML;
	if (XMLString == "dicom/show scan center")
		return ID_DICOM_SHOWSCANCENTER;
	if (XMLString == "dicom/check series")
		return ID_DICOM_CHECKSERIES;
	if (XMLString == "help/open manual")
		return ID_HELP_OPENMANUAL;	

	return -1;	// if the string wasn't found
}

int CMenuController::FindMenuItem(CMenu* Menu, LPCTSTR MenuString)
{
    ASSERT(Menu);
    ASSERT(::IsMenu(Menu->GetSafeHmenu()));
	
    int count = Menu->GetMenuItemCount();
    for (int i = 0; i < count; i++) 
    {
        CString str;
		if (Menu->GetMenuString(i, str, MF_BYPOSITION) &&
                            (strcmp(str, MenuString) == 0))
            return i;
    }

	return -1;   // If the menu string wasn't found
}

void CMenuController::RemoveMenuItem(const char *zString, int id)
{
    int pos = FindMenuItem(mpMenu, zString);

    if (pos != -1) 
	{    
		CMenu *pSubMenu = mpMenu->GetSubMenu(pos);
		if (pSubMenu)
			pSubMenu->RemoveMenu(id, MF_BYCOMMAND);
    }
}

void CMenuController::RemoveItems(CXMLParseNode * pRemoveNode)
{
	CXMLParseNode * pXMLNode = pRemoveNode->GetFirst("MenuItem"); // Finds the beginning of the file
	
	// If a <MenuItem> was found, do this...
	while (pXMLNode)	
	{
		CString sCaption = pXMLNode->msValue;
		int iPosition = Convert(sCaption.MakeLower());

		if (iPosition != -1)
		{
			mpMenu->RemoveMenu(iPosition,MF_BYCOMMAND);
		}
		else 
		{
			CString ErrorStr("Couldn\'t find menu item: ");
			ErrorStr.Append(sCaption);
			ErrorStr.Append("\nVerify its correctness and try again");
			MessageBox(NULL, ErrorStr, "Error", MB_OK);
		}
		pXMLNode = pRemoveNode->GetNext("MenuItem", pXMLNode);	// Move to next menu item
	}
}

void CMenuController::RunXML(void)
{
	CString sTempFileName(CMyWindows::GetApplicationPath());
	sTempFileName.Append("DefaultMenu.xml");

	CXMLParse xmlParse(sTempFileName,0);
    CXMLParseNode *pRoot = xmlParse.GetRoot();
	
	if (!pRoot)
        return;

	// Going over the XML file and deleting the menu items
	CXMLParseNode * pRemoveNode = pRoot->GetFirst("remove"); // Finds the beginning of the file
	if (pRemoveNode)
		RemoveItems(pRemoveNode);
}
