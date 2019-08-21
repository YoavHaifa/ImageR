#pragma once
#include <afxwin.h>

#define RAW_MENU_ID 0
#define OBLIQUE_MENU_ID 1
#define MIP_MENU_ID 5
#define MULTI_DATA_MENU_ID 6
#define DIFF_MENU_ID 7
#define AIR_TABLE_MENU_ID 8
#define WIND_A_ZOOM_MENU_ID 10
#define COLORING_MENU_ID 11

class CContextMenu
{
public:
	CContextMenu(void);
	~CContextMenu(void);

	bool SelectMenuForData(class CMetaData *pData, int &oMenuId);

	static const int USE_SUB_MENU = 12345;
	CMenu *GetMenu(void) {return mpSubMenu;}
	void OnPresetSelected(int iPreset);

private:
	class CXMLParse *mpXml;
	class CXMLParseNode *mpRoot;

	int miSubMenu;

	CMenu *mpPopMenu;
	CMenu *mpSubMenu;
	class CMetaData *mpData;

	bool Init(void);
	bool PrepareMenu(class CXMLParseNode *pMenuXml);
	void AdjastWindAndZoomMenu(class CMetaData *pData);

	static const int MAX_PRESETS = 10;
	float maPresetWidth[MAX_PRESETS];
	float maPresetCenter[MAX_PRESETS];
	int mnPresets;
};

extern CContextMenu gContextMenu;