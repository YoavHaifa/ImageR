#include "StdAfx.h"
#include "ContextMenu.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\XMLParse.h"
#include "Screens.h"
#include "MetaData.h"
#include "resource.h"

CContextMenu gContextMenu;

CContextMenu::CContextMenu(void)
: mpXml(NULL)
, mpRoot(NULL)
, mpData(NULL)
, mnPresets(0)
, mpPopMenu(NULL)
, mpSubMenu(NULL)
{
}
CContextMenu::~CContextMenu(void)
{
	if (mpXml)
		delete mpXml;
	if (mpPopMenu)
		delete mpPopMenu;
}
bool CContextMenu::SelectMenuForData(class CMetaData *pData, int &oMenuId)
{
	if (!pData)
		return false;
	mpData = pData;

	if (!mpXml)
		if (!Init())
			return false;

	if (!mpRoot)
		return false;

	miSubMenu = -1;
	CString sClass = pData->GetClass();
	CString sName = pData->Name();

	oMenuId = -1;
	CXMLParseNode *pMenuXml = mpRoot->GetFirst("menu");
	while (pMenuXml)
	{
		CString sMenuDataName;
		if (pMenuXml->GetValue("data_name", sMenuDataName))
		{
			if (sName.Find(sMenuDataName) == 0) // Any Prefix will do
			{
				PrepareMenu(pMenuXml);
			}
		}

		CString sMenuClass;
		if (pMenuXml->GetValue("data_class", sMenuClass))
		{
			if (sClass.Find(sMenuClass) == 0) // Any Prefix will do
			{
				PrepareMenu(pMenuXml);
			}
		}
		pMenuXml = mpRoot->GetNextDFS("menu", &pMenuXml);
	}

	if (miSubMenu >= 0)
	{
		oMenuId = USE_SUB_MENU;
	}
	if (miSubMenu == WIND_A_ZOOM_MENU_ID)
		AdjastWindAndZoomMenu(pData);
	return true; // NO MENU (-1)
}
bool CContextMenu::Init(void)
{
	static bool ubOnce = false;
	if (!ubOnce)
	{
		ubOnce = true;
		CString sfName(CMyWindows::GetApplicationPath());
		sfName += "ContextMenu.xml";
		mpXml = new CXMLParse(sfName);
		mpRoot = mpXml->GetRoot();
	}
	if (!mpRoot)
		return false; // No XML Definition

	return true;
}
bool CContextMenu::PrepareMenu(class CXMLParseNode *pMenuXml)
{
	if (!pMenuXml->GetValue("id",miSubMenu))
		miSubMenu = WIND_A_ZOOM_MENU_ID; // Generic

	if (mpPopMenu)
		delete mpPopMenu;
	mpPopMenu = new CMenu;
	mpPopMenu->LoadMenu(IDR_MENU_POP);
	mpSubMenu = mpPopMenu->GetSubMenu(miSubMenu);
	if (!mpSubMenu)
		return false;

	mnPresets = 0;
	CXMLParseNode *pEntryXml = pMenuXml->GetFirst("preset_window");
	while (pEntryXml)
	{
		float width = 100;
		float center = 0;
		CString sText(_T("Text"));
		pEntryXml->GetValue("text",sText);
		pEntryXml->GetValue("center",center);
		pEntryXml->GetValue("width",width);
		if (mnPresets >= MAX_PRESETS)
		{
			char zBuf[128];
			sprintf_s(zBuf,sizeof(zBuf),"Only %d preset windows are allowed for each menu",MAX_PRESETS);
			CMyWindows::MessBox(zBuf,"XML Def Error");
			break;
		}
		mpSubMenu->AppendMenu(MF_STRING, ID_GENERIC_PRESET_WINDOW0 + mnPresets, (const char *)sText);
		maPresetCenter[mnPresets] = center;
		maPresetWidth[mnPresets] = width;
		mnPresets++;
		pEntryXml = pMenuXml->GetNextDFS("preset_window", &pEntryXml);
	}

	return true;
}
void CContextMenu::OnPresetSelected(int iPreset)
{
	if (iPreset >= mnPresets)
		return;
	if (!mpData)
		return;

	//float center = mpData->Unscale(maPresetCenter[iPreset]);
	//float width = mpData->UnscaleWidth(maPresetWidth[iPreset]);
	float center = maPresetCenter[iPreset]; // It will be unscaled in "SetWindowParamsFromGui"
	float width = maPresetWidth[iPreset];

	mpData->SetWindowParamsFromGui(center, width);
	gScreens.Refresh();
}
void CContextMenu::AdjastWindAndZoomMenu(CMetaData *pData)
{
	if (!mpSubMenu)
		return;

	if (pData->UseSpecialWindow())
		mpSubMenu->ModifyMenu(ID_GENERIC_SPECIALWINDOW,MF_BYCOMMAND,ID_GENERIC_SPECIALWINDOW,"Common Window");
	else
		mpSubMenu->ModifyMenu(ID_GENERIC_SPECIALWINDOW,MF_BYCOMMAND,ID_GENERIC_SPECIALWINDOW,"Special Window");

	if (pData->UseSpecialZoom())
		mpSubMenu->ModifyMenu(ID_GENERIC_SPECIALZOOM,MF_BYCOMMAND,ID_GENERIC_SPECIALZOOM,"Common Zoom");
	else
		mpSubMenu->ModifyMenu(ID_GENERIC_SPECIALZOOM,MF_BYCOMMAND,ID_GENERIC_SPECIALZOOM,"Special Zoom");
}
