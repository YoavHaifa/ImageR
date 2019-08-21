#pragma once

class CMenuController 
{
public:
	// Some constructors / destructors
	CMenuController(void);
	~CMenuController(void){}

	void RunXML(void);

private:
	// Menu XML commands
	void RemoveItems(class CXMLParseNode * pRemoveNode);

	int FindMenuItem(CMenu* Menu, LPCTSTR MenuString);
	void RemoveMenuItem(const char *zString, int id);
	int Convert(CString XMLString);
public:
	CMenu *mpMenu;
};

extern CMenuController gMenuController;