#pragma once

// MainFrm.h : interface of the CMainFrame class
//
#define WM_UPDATE_TRACELOG				WM_USER + 0
#define WM_UPDATE_STATUSTEXT			WM_USER + 1

//override CreateNewFrame to eliminate warning message:
//  "Warning: creating frame with no default view."


class CPestRidSplitterWnd;

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:
	
// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	// control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

	CPestRidSplitterWnd* m_pSplitterWnd;
	HICON m_hIcon;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam)
	{
		LRESULT lResult;
		ModifyStyle(WS_VISIBLE, 0);
		lResult = CFrameWnd::DefWindowProc(WM_SETTEXT, wParam, lParam);
		ModifyStyle(0, WS_VISIBLE);
		//entire frame needs to be invalidated
		//  perhaps only on first rendering? - check source
		Invalidate();
		//SendMessage(WM_NCPAINT, 1);
		return lResult;
	}
	afx_msg void OnNcPaint()
	{
		CRect captionRect(GetTitleRect());
		_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
		HRGN hRgn = (HRGN)pThreadState->m_lastSentMsg.wParam;
		CFrameWnd::OnNcPaint();
		//determine if title bar is being painted
		//hRgn = 1 indicates everything is redrawn
		if (hRgn == (HRGN)1 || RectInRegion(hRgn, captionRect)) {
			DrawCaptText(captionRect);
		}
	}
	afx_msg BOOL OnNcActivate(BOOL bActive)
	{
		BOOL bRet;
		bRet = CFrameWnd::OnNcActivate(bActive);
		SendMessage(WM_NCPAINT, 1);
		//Invalidate(); //invalidate if needed
		return bRet;
	}
	LRESULT OnUpdateStatusText(WPARAM, LPARAM);
	LRESULT OnUpdateTraceLog(WPARAM, LPARAM);
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	CRect GetTitleRect()
	{
		CRect rect;
		//to get width of area
		GetWindowRect(rect);
		ScreenToClient(rect);
		//skip past maximize, minimize and close icons 
        rect.right -= (	3 * GetSystemMetrics(SM_CXSIZE) +
						GetSystemMetrics(SM_CXFRAME));
		//rect.left = pixel length of current caption
		rect.left += GetSystemMetrics(SM_CXFRAME);
		//calculations
		rect.top = GetSystemMetrics(SM_CYFRAME);
		rect.bottom = rect.top + GetSystemMetrics(SM_CYCAPTION);
		return rect;
	}
	void DrawCaptText(CRect captionRect)
	{
		int iRet;
		int iOldMode;
		COLORREF crOldColor;
		TEXTMETRIC tm = { 0 };
		CGdiObject* pOldFont;
		CGdiObject NewFont;
		//must use CWindowDC to paint the non-client area
		CWindowDC pdc(this);
		if (!NewFont.Attach(GetStockObject(DEFAULT_GUI_FONT))) ASSERT(FALSE);
		pOldFont = pdc.SelectObject(&NewFont);
		if (!pdc.GetTextMetrics(&tm)) ASSERT(FALSE);
		//average is biased upwards
		captionRect.top += (GetSystemMetrics(SM_CYCAPTION) - tm.tmHeight -
							tm.tmExternalLeading - 1) / 2;
		captionRect.bottom =	captionRect.top + tm.tmHeight +
								tm.tmExternalLeading;
		//demo = yellow, free = green, professional/enterprise = black
		crOldColor = pdc.SetTextColor(0x00FF00);
		iOldMode = pdc.SetBkMode(TRANSPARENT);
		if (!((iRet = pdc.DrawText(	_T("Free"), captionRect,
										DT_RIGHT | DT_SINGLELINE)) >= 
				tm.tmHeight &&
				iRet <= tm.tmHeight + tm.tmExternalLeading)) ASSERT(FALSE);
		pdc.SetBkMode(iOldMode);
		pdc.SetTextColor(crOldColor);
		pdc.SelectObject(pOldFont);
		NewFont.Detach();
		//ReleaseDC(pdc);
	}

	DECLARE_MESSAGE_MAP()
};