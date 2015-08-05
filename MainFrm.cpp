// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "PestRid.h"

#include "MainFrm.h"
#include "PestRidDlg.h"
#include "TraceLog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SYSCOMMAND()
	ON_WM_NCPAINT()
	ON_WM_NCACTIVATE()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_MESSAGE(WM_UPDATE_TRACELOG, OnUpdateTraceLog)
	ON_MESSAGE(WM_UPDATE_STATUSTEXT, OnUpdateStatusText)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pSplitterWnd = NULL;
}

CMainFrame::~CMainFrame()
{
	if (m_hIcon) DestroyIcon(m_hIcon);
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(	this,
								TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE |
								CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS |
								CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		//TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators) / sizeof(UINT))) {
		//TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	//Make the toolbar dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	if ((m_pSplitterWnd = new CPestRidSplitterWnd()) == NULL) return FALSE;
	if (!m_pSplitterWnd->CreateStatic(this, 1, 2)) return FALSE;
	if (!CFrameWnd::OnCreateClient(lpcs, pContext)) return FALSE;
	if (m_pSplitterWnd->Init(pContext) == -1) return FALSE;

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL) { 
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	return TRUE;
}

void CMainFrame::OnDestroy()
{
	m_pSplitterWnd->DestroyWindow();
	SetActiveView(NULL);
	CFrameWnd::OnDestroy();
	PostQuitMessage(0);
}



// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMainFrame::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CFrameWnd::OnPaint();
	}
}

//The system calls this function to obtain the cursor to display
//  while the user drags the minimized window.
HCURSOR CMainFrame::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		((CPestRidApp*)AfxGetApp())->OnAppAbout();
	} else {
		CFrameWnd::OnSysCommand(nID, lParam);
	}
}

LRESULT CMainFrame::OnUpdateStatusText(WPARAM, LPARAM)
{
	CString String;
	m_pSplitterWnd->GetStatusText(String);
	SetMessageText(String);
	return 0;
}

LPARAM CMainFrame::OnUpdateTraceLog(WPARAM, LPARAM)
{
	CString String;
	GetTraceLog(String);
	if (!String.IsEmpty()) {
		CEdit* TraceLogEdit;
		CString Str;
		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
		BOOL bScroll;
		TraceLogEdit = ((CTabView*)
								((CSplitterWndRight*)m_pSplitterWnd->GetPane(0, 1))->GetPane(1, 0))->GetTraceEdit();
		TraceLogEdit->GetWindowText(Str);
		TraceLogEdit->GetScrollInfo(SB_VERT, &si);
		bScroll = ((UINT)si.nPos >= (UINT)si.nMax - si.nPage);
		TraceLogEdit->SetWindowText(Str + String);
		if (bScroll) {
			TraceLogEdit->GetScrollRange(SB_VERT, &si.nMin, &si.nMax);
			TraceLogEdit->LineScroll(si.nMax - si.nMin);
		} else {
			TraceLogEdit->LineScroll(si.nPos);
		}
	}
	return 0;
}