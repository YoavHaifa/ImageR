// WindParams.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "WindowDlg.h"
#include "WindowParams.h"
#include "MetaData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWindowDlg dialog

CWindowDlg* CWindowDlg::umpCurrentDlg = NULL;
CWnd* CWindowDlg::umpImageViewWindow = NULL;
int CWindowDlg::umDebug = 0;

void CWindowDlg::SetImageView(CWnd* pImageViewWindow)
{
	umpImageViewWindow = pImageViewWindow;
}

CWindowDlg::CWindowDlg(CMetaData* pData, CWindowParams* pWindParams)
	: CDialog(CWindowDlg::IDD, umpImageViewWindow)
	, mpData(pData)
	, mpWindowParams(pWindParams)
	, mpfLog(NULL)
{
	if (umpCurrentDlg)
		delete umpCurrentDlg;
	umpCurrentDlg = this;

	if (pWindParams)
	{
		mSavedCenter = pWindParams->mCenter;
		mSaveWidth = pWindParams->mWidth;
	}
	if (umDebug)
	{
		static int count = 0;
		mpfLog = CMyWindows::FOpenLogFile("WindowDlg", count++);
		if (mpfLog)
		{
			fprintf(mpfLog, "<WindowDlg> %d %s - pWindParams %p\n", count, (const char*)pWindParams->Name(), pWindParams);
			fprintf(mpfLog, "Saved Center %f Width %f\n", mSavedCenter, mSaveWidth);
			if (mpWindowParams)
			{
				fprintf(mpfLog, "GUI Center %s Width %s\n", (const char*)mpWindowParams->GetCenterText(), (const char*)mpWindowParams->GetWidthText());
				fprintf(mpfLog, "min %f max %f range %f\n", mpWindowParams->mMin, mpWindowParams->mMax, mpWindowParams->mRange);
			}
			fprintf(mpfLog, "pData %p %s\n", pData, (const char*)pData->Name());
			fprintf(mpfLog, "\n");
			fflush(mpfLog);
		}
	}
}
CWindowDlg::~CWindowDlg(void)
{
	if (mpfLog)
	{
		fprintf(mpfLog, "\n<~WindowDlg> pWindParams %p\n", mpWindowParams);
		if (mpWindowParams)
		{
			fprintf(mpfLog, "Last Center %f Width %f\n", mpWindowParams->mCenter, mpWindowParams->mWidth);
			fprintf(mpfLog, "GUI Center %s Width %s\n", (const char*)mpWindowParams->GetCenterText(), (const char*)mpWindowParams->GetWidthText());
		}
		fclose(mpfLog);
	}
	if (umpCurrentDlg == this)
	{
		umpCurrentDlg = NULL;
	}
	mpData->ByeFromWindowDialog();
}
void CWindowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWindowDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWindowDlg, CDialog)
	//{{AFX_MSG_MAP(CWindowDlg)
	ON_WM_VSCROLL()
	ON_EN_KILLFOCUS(IDC_EDIT_WIDTH, OnKillfocusEditWidth)
	ON_EN_KILLFOCUS(IDC_EDIT_CENTER, OnKillfocusEditCenter)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWindowDlg message handlers

BOOL CWindowDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	HICON hIcon = AfxGetApp()->LoadIcon(IDI_ICON_WINDOW);
	SetIcon(hIcon, TRUE);			// Set big icon
	SetIcon(hIcon, FALSE);		// Set small icon

	SetWindowText(IDC_EDIT_CENTER, mpWindowParams->GetCenterText());
	SetWindowText(IDC_EDIT_WIDTH, mpWindowParams->GetWidthText());

	// Set width scroll
	SCROLLINFO info;
	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_ALL;
	info.nMin = 0;
	info.nMax = (int)mpWindowParams->mRange;
	info.nPage = 0;
	info.nPos = (int)(mpWindowParams->mRange - mpWindowParams->mWidth);
	info.nTrackPos = 0;
	CScrollBar* pScrBar = (CScrollBar*)GetDlgItem(IDC_SCROLLBAR_WIDTH);
	pScrBar->SetScrollInfo(&info);

	// Set mWidth scroll
	//info.nMin = mMin;
	//info.nMax = mMax;
	info.nPos = (int)(mpWindowParams->mMax - mpWindowParams->mCenter);
	pScrBar = (CScrollBar*)GetDlgItem(IDC_SCROLLBAR_CENTER);
	pScrBar->SetScrollInfo(&info);

	if (mpfLog)
	{
		fprintf(mpfLog, "<OnInitDialog>\n");
		fprintf(mpfLog, "nMax %d\n", info.nMax);
		fprintf(mpfLog, "nPos %d\n", info.nPos);
		fprintf(mpfLog, "\n");
		fflush(mpfLog);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CWindowDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar->GetDlgCtrlID() == IDC_SCROLLBAR_WIDTH) {
		on_width_scroll(nSBCode, nPos, pScrollBar);
	}
	else {
		on_center_scroll(nSBCode, nPos, pScrollBar);
	}

	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}
/*
void CWindowDlg::set_on(CWindowParams *pWindParams, int *pOn)
{
	mpOn=pOn;
	if (mpOn)
		*mpOn=true;

	mpWindowParams=pWindParams;
} */
void CWindowDlg::SetWindowText(int id, const char* zText)
{
	CWnd* pWnd = GetDlgItem(id);
	pWnd->SetWindowText(zText);
}
void CWindowDlg::UpdateWidthDisplay()
{
	CScrollBar* pScrollBar = (CScrollBar*)GetDlgItem(IDC_SCROLLBAR_WIDTH);
	pScrollBar->SetScrollPos((int)(mpWindowParams->mRange - mpWindowParams->mWidth));
	SetWindowText(IDC_EDIT_WIDTH, mpWindowParams->GetWidthText());
}
void CWindowDlg::UpdateCenterDisplay()
{
	CScrollBar* pScrollBar = (CScrollBar*)GetDlgItem(IDC_SCROLLBAR_CENTER);
	pScrollBar->SetScrollPos((int)(mpWindowParams->mMax - mpWindowParams->mCenter));
	SetWindowText(IDC_EDIT_CENTER, mpWindowParams->GetCenterText());
}
void CWindowDlg::update()
{
	UpdateWidthDisplay();
	UpdateCenterDisplay();
	InvalidateRect(NULL, false);
}

void CWindowDlg::on_width_scroll(UINT nSBCode, UINT nPos,
	CScrollBar*)
{
	if (mpfLog)
	{
		fprintf(mpfLog, "<on_width_scroll> nSBCode %d\n", nSBCode);
		fprintf(mpfLog, "nPos %d\n", nPos);
		fprintf(mpfLog, "\n");
		fflush(mpfLog);
	}
	float delta = 0.0f;

	switch (nSBCode)
	{
	case SB_LINEUP:
		if (mpWindowParams->mWidth < mpWindowParams->mRange)
			delta = 1.0f;
		break;

	case SB_LINEDOWN:
		if (mpWindowParams->mWidth > 1.0f)
			delta = -1.0f;
		break;

	case SB_PAGEUP:
		delta = min(WIND_PARAMS_PAGE_INCREMENT,
			(mpWindowParams->mRange - mpWindowParams->mWidth));
		break;

	case SB_PAGEDOWN:
		delta = max(-WIND_PARAMS_PAGE_INCREMENT,
			-mpWindowParams->mWidth);
		break;

	case SB_THUMBTRACK:
		delta = (mpWindowParams->mRange - nPos) - mpWindowParams->mWidth;
		break;

	default:
		delta = 0.0f;
		break;
	}

	if (delta)
	{
		if (mpfLog)
		{
			fprintf(mpfLog, "<on_width_scroll> END, delta %f\n", delta);
			fprintf(mpfLog, "SetWidth %f\n", mpWindowParams->mWidth + delta);
			fprintf(mpfLog, "\n");
			fflush(mpfLog);
		}
		mpWindowParams->SetWidth(mpWindowParams->mWidth + delta);
		UpdateWidthDisplay();
		InvalidateRect(NULL, false);
		if (umpImageViewWindow)
			umpImageViewWindow->SendMessage(IVM_UPDATE_WIND_PARAMS);
	}
}

void CWindowDlg::on_center_scroll(UINT nSBCode, UINT nPos,
	CScrollBar*)
{
	if (mpfLog)
	{
		fprintf(mpfLog, "<on_center_scroll> nSBCode %d\n", nSBCode);
		fprintf(mpfLog, "nPos %d\n", nPos);
		fprintf(mpfLog, "\n");
		fflush(mpfLog);
	}
	float delta = 0;

	switch (nSBCode)
	{
	case SB_LINEUP:
		if (mpWindowParams->mCenter < mpWindowParams->mMax)
			delta = 1.0f;
		break;

	case SB_LINEDOWN:
		if (mpWindowParams->mCenter > mpWindowParams->mMin)
			delta = -1.0f;
		break;

	case SB_PAGEUP:
		delta = min(WIND_PARAMS_PAGE_INCREMENT, (mpWindowParams->mMax - mpWindowParams->mCenter));
		break;

	case SB_PAGEDOWN:
		delta = max(-WIND_PARAMS_PAGE_INCREMENT, -(mpWindowParams->mCenter - mpWindowParams->mMin));
		break;

	case SB_THUMBTRACK:
		delta = (mpWindowParams->mMax - mpWindowParams->mCenter) - nPos;
		if (delta)
		{
			if (delta < -100.0f)
				delta = -100.0f;
			if (delta > 100.0f)
				delta = 100.0f;
		}
		break;

	default:
		delta = 0.0f;
		break;
	}
	if (delta)
	{
		if (mpfLog)
		{
			fprintf(mpfLog, "<on_center_scroll> END, delta %f\n", delta);
			fprintf(mpfLog, "SetCenter %f\n", mpWindowParams->mCenter + delta);
			fprintf(mpfLog, "\n");
			fflush(mpfLog);
		}
		mpWindowParams->SetCenter(mpWindowParams->mCenter + delta);

		UpdateCenterDisplay();
		InvalidateRect(NULL, false);
		if (umpImageViewWindow)
			umpImageViewWindow->SendMessage(IVM_UPDATE_WIND_PARAMS);
	}
}

float CWindowDlg::GetWindowValue(int id)
{
	char zText[32];
	CWnd* pWnd = GetDlgItem(id);
	pWnd->GetWindowText(zText, 31);
	return (float)atof(zText);
}
void CWindowDlg::OnKillfocusEditWidth()
{
	float newWidth = GetWindowValue(IDC_EDIT_WIDTH);

	/*
	if (newWidth == mpWindowParams->mWidth)
		return;

	if (newWidth  < 0.0f || newWidth > mpWindowParams->mRange)
	{
		CString sWarning ("The WIDTH parameter should be between 0 and ");
		char zBuf[128];
		float range = mpWindowParams->mRange;
		int iRange = (int)range;
		if ((float)iRange == range)
			sprintf_s (zBuf, 128, "%d", iRange);
		else
			sprintf_s (zBuf, 128, "%f", range);
		sWarning += zBuf;
		MessageBox(sWarning, "WARNING");
	} */
	if (!mpWindowParams->SetWidthFromGui(newWidth))
		return;

	UpdateWidthDisplay();
	InvalidateRect(NULL, false);
	if (umpImageViewWindow)
		umpImageViewWindow->SendMessage(IVM_UPDATE_WIND_PARAMS);
}

void CWindowDlg::OnKillfocusEditCenter()
{
	float newCenter = GetWindowValue(IDC_EDIT_CENTER);

	if (!mpWindowParams->SetCenterFromGui(newCenter))
		return;

	UpdateCenterDisplay();
	InvalidateRect(NULL, false);
	if (umpImageViewWindow)
		umpImageViewWindow->SendMessage(IVM_UPDATE_WIND_PARAMS);
}

void CWindowDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CWnd* pWnd = GetDlgItem(IDC_WINDPREVIEW);
	RECT ClientRect;
	CPaintDC WndBarDC(pWnd);

	pWnd->GetClientRect(&ClientRect);
	draw_bitmap(WndBarDC, &ClientRect);
}

void CWindowDlg::draw_bitmap(CDC& dc, RECT* pRect)
{
	float Scale = (float)(mpWindowParams->mRange + 1) / (float)pRect->bottom;
	int Start = (int)(pRect->bottom - ((mpWindowParams->mCenter - mpWindowParams->mMin) +
		mpWindowParams->mWidth / 2 + 1) / Scale);
	int Height = (int)((mpWindowParams->mWidth) / Scale + 0.5f);
	if (Height < 2)
		Height = 2;
	COLORREF clr = RGB(238, 238, 236);//GetSysColor(COLOR_WINDOWFRAME);

	CBitmap BitMap;
	BitMap.CreateCompatibleBitmap(&dc, pRect->right, Height);

	COLORREF* pColorBits = new COLORREF[pRect->right * Height];
	int i, j;
	int Step, Rem;
	int ColorLevel = 255;
	int Limit;
	COLORREF CurColor;

	if (Height < WND_NUM_COLORS) {
		Limit = WND_NUM_COLORS;
		for (i = 0; i < Height - 1; i++) {
			CurColor = RGB(ColorLevel, ColorLevel, ColorLevel);
			for (j = 0; j < pRect->right; j++) {
				pColorBits[i * pRect->right + j] = CurColor;
			}
			/*      if (i == Height-2) {
					FILE *pFd=MyFOpen("wnd.dat", "a");
					fprintf(pFd, "Height=%d, Color=%d, Step=%d, i=%d\n",
							Height, ColorLevel, Step, i);
					fclose(pFd);
				  } */
			Step = Limit / (Height - 1);
			Rem = Limit % (Height - 1);
			Limit = WND_NUM_COLORS + Rem;
			ColorLevel -= Step;
		}
	}
	else {
		Limit = Height;
		for (i = 0; i < Height - 1; i++) {
			CurColor = RGB(ColorLevel, ColorLevel, ColorLevel);
			for (j = 0; j < pRect->right; j++) {
				pColorBits[i * pRect->right + j] = CurColor;
			}
			Limit -= WND_NUM_COLORS;
			if (Limit < 0) {
				Limit += Height;
				ColorLevel--;
			}
		}
	}
	for (j = 0; j < pRect->right; j++) {
		pColorBits[(Height - 1) * pRect->right + j] = (COLORREF)0;
	}

	RECT BkGr;
	if (Start > 0) {
		BkGr.left = 0;
		BkGr.top = 0;
		BkGr.right = pRect->right;
		BkGr.bottom = Start;
		dc.FillSolidRect(&BkGr, clr);
	}
	if (Start + Height < pRect->bottom) {
		BkGr.left = 0;
		BkGr.top = Start + Height;
		BkGr.right = pRect->right;
		BkGr.bottom = pRect->bottom;
		dc.FillSolidRect(&BkGr, clr);
	}

	BitMap.SetBitmapBits(pRect->right * Height * sizeof(COLORREF),
		(void*)pColorBits);
	CDC MemDC;
	MemDC.CreateCompatibleDC(NULL);
	MemDC.SelectObject(&BitMap);

	CRgn ClipRgn;
	ClipRgn.CreateRectRgn(0, 0, pRect->right, pRect->bottom);
	dc.SelectClipRgn(&ClipRgn);

	dc.BitBlt(0, Start, pRect->right, Height, &MemDC,
		0, 0, SRCCOPY);
	delete pColorBits;
}
/*
void CWindowDlg::OnOK()
{
  if (mpOn) {
	*mpOn=false;
	DestroyWindow();
  } else {
	CDialog::OnOK();
  }
} */

void CWindowDlg::OnCancel()
{
	// Restore values
	mpWindowParams->SetCenter(mSavedCenter);
	mpWindowParams->SetWidth(mSaveWidth);
	if (umpImageViewWindow)
		umpImageViewWindow->SendMessage(IVM_UPDATE_WIND_PARAMS);


	CDialog::OnCancel();
}
