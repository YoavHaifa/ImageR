// DataArea.cpp : implementation file
//

#include "stdafx.h"
#include "DataArea.h"
#include "..\..\yUtils\MyRegistry.h"
#include "..\..\yUtils\MyWindows.h"
#include "Screens.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataArea

CDataArea::CDataArea()
{
    mKeyboardMsg = RegisterWindowMessage("ImageRKeyboardMsg");
}

CDataArea::~CDataArea()
{
}


BEGIN_MESSAGE_MAP(CDataArea, CStatic)
	//{{AFX_MSG_MAP(CDataArea)
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDataArea message handlers

UINT CDataArea::OnGetDlgCode() 
{
  UINT Code = CWnd::OnGetDlgCode () | DLGC_WANTALLKEYS;
  return Code;
}

void CDataArea::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  CMyWindows::PostMessageToClient(mKeyboardMsg, nChar, 0);

  CWnd *pPar=GetParent();
  SHORT ShiftOn=GetKeyState(VK_SHIFT);
  SHORT CtrlOn=GetKeyState(VK_CONTROL);
  switch (nChar) {
    case 33:
      pPar->SendMessage(WM_VSCROLL, SB_PAGEUP, 0);
      break;
    case 34:
      pPar->SendMessage(WM_VSCROLL, SB_PAGEDOWN, 0);
      break;
    case 35:
      pPar->SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
      break;
    case 36:
      pPar->SendMessage(WM_VSCROLL, SB_TOP, 0);
      break;
    case 37:
      if (ShiftOn < 0) {
        pPar->SendMessage(WM_HSCROLL, SB_PAGELEFT, 0);
      } else if (CtrlOn < 0) {
        pPar->SendMessage(WM_HSCROLL, SB_TOP, 0);
      } else {
        pPar->SendMessage(WM_HSCROLL, SB_LINELEFT, 0);
      }
      break;
    case 38:
      pPar->SendMessage(WM_VSCROLL, SB_LINEUP, 0);
      break;
    case 39:
      if (ShiftOn < 0) {
        pPar->SendMessage(WM_HSCROLL, SB_PAGERIGHT, 0);
      } else if (CtrlOn < 0) {
        pPar->SendMessage(WM_HSCROLL, SB_BOTTOM, 0);
      } else {
        pPar->SendMessage(WM_HSCROLL, SB_LINERIGHT, 0);
      }
      break;
    case 40:
      pPar->SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);
      break;
    case 'F':
      if (CtrlOn < 0)
        pPar->SendMessage(WM_COMMAND, ID_WINDOW_AUTOFOCUS, 0);
      break;
    case 'L':
      if (CtrlOn < 0)
        pPar->SendMessage(WM_COMMAND, ID_WINDOW_LOCALFOCUS, 0);
      break;
    case 'G':
      if (CtrlOn < 0)
        pPar->SendMessage(WM_COMMAND, ID_EDIT_GOTO, 0);
      break;
    case 'C':
      if (CtrlOn < 0)
        pPar->SendMessage(WM_COMMAND, ID_VIEW_TOGGLECOLORS, 0);
      break;
    case 'T':
      if (CtrlOn < 0)
        pPar->SendMessage(WM_COMMAND, ID_VIEW_TOGGLETEXT, 0);
      break;
    case 'H':
      if (CtrlOn < 0)
        pPar->SendMessage(WM_COMMAND, ID_VIEW_TOGGLEGRAPHICS, 0);
      break;
    case 'N':
      if (CtrlOn < 0)
        pPar->SendMessage(WM_COMMAND, ID_VIEW_NEXT, 0);
      break;
    case 'P':
      if (CtrlOn < 0)
        pPar->SendMessage(WM_COMMAND, ID_VIEW_PREV, 0);
      break;
    case 'R':
      if (CtrlOn < 0) {
        pPar->SendMessage(WM_COMMAND, ID_VIEW_RESIZETOPAGESIZE, 0);
      }
      break;
    case 'Z':
      if (CtrlOn < 0) {
        pPar->SendMessage(WM_COMMAND, ID_VIEW_ZOOM, 0);
      }
      break;
    case 114:
      //if (ShiftOn < 0) {
      //  pPar->SendMessage(WM_COMMAND, ID_EDIT_FINDNEXTBACK, 0);
      //} else {
        pPar->SendMessage(WM_COMMAND, ID_VIEW_NEXT, 0);
      //}
      break;
    case 'W':
      if (CtrlOn < 0) {
        pPar->SendMessage(WM_COMMAND, ID_PARAMETERS_WINDOW, 0);
      }
      break;
    case 'S':
      if (CtrlOn < 0)
		{
			bool bEnable = false;
			if (CMyRegistry::GetAppReg("EnableSaveState",bEnable))
				if (bEnable)
					gScreens.SaveStateForUser();
		}	
      break;
    case 'D':
		{
			bool bEnable = false;
			if (CMyRegistry::GetAppReg("EnableDataDump",bEnable) && bEnable)
				pPar->SendMessage(WM_COMMAND, ID_PRINT_DUMPSLICE, 0);
		}
      break;
    case 'O':
      if (CtrlOn < 0) {
        pPar->SendMessage(WM_COMMAND, ID_FILE_OPEN, 0);
      }
      break;
    case 'M':
      if (CtrlOn < 0) {
        pPar->SendMessage(WM_COMMAND, ID_DICOM_PRINT, 0);
      }
      break;
    case 27:
      pPar->SendMessage(WM_USER+1000, 0, 0);
      break;
  }

	CStatic::OnKeyDown(nChar, nRepCnt, nFlags);
}
