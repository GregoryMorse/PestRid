#pragma once

// Gregory Morse
//Makes use of MFC resources from app wizard template:
//C:\program files\Microsoft Visual Studio .NET 2003\
//  Vc7\VCWizards\mfcappwiz\templates\1033
/*

Life's So Complicated

Manage whatever is important to you including your
  personal life, business life, social life and computer
  Life Manager
Automated downloader with appropriate action recorder for login pages 
Advanced job and task scheduler
Autorun change monitor
System Emergency Maintenance Tool


Under the Hood

Take Control of Your System

Verify all kernel API calls are not hooked
Verify all ntdll.dll and underneath kernel Read+Execute code checksums
Verify PSAPI.dll, tlhelp32.dll, advapi32.dll
Use kernel to dump:
All kernel objects
All processes/threads/modules
All drivers/services
All GDI objects
All User/window objects
All files (including com, lpt, reserved names)
All NTFS alternate data streams
All IFS
All registry keys
All autoruns



TODO
----
Support viewing process information
Process group support and job objects
Service detection for processes
Mouse drags causes scroll handle synchronization

*/

// PestRidDlg.h : header file
//

// SymLink info,
// tabs for mutant information
// more handle columns we have more information!!!
// windows security information for objects

#include <afxmt.h>

#include "Modules.h"
#include "CustomSplit.h"
#include "CustomTree.h"
#include "CustomListCtrl.h"

#define THREAD_TERMINATE_TIMEOUT	10000
#define THREAD_MAX_POLL_TIMEOUT		8000

#define ID_QUERYHANDLE 10
#define TRACEEDITID	1000

//are nested splitter control IDs conflicting
//  or can duplicate IDs but different parents work okay


class CTabView : public CCtrlView
{
public:
	//TCS_SINGLELINE could be a setting...
	CTabView() : CCtrlView(	WC_TABCONTROL,
							WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_MULTILINE)
	{ m_Edit = NULL; }
	virtual ~CTabView()
	{
	}
	void DoSize()
	{
		DWORD Counter;
		CRect rect;
		CRect testrect;
		//hide/show and size appropriate set of controls
		if (m_Edit) {
			GetClientRect(rect);
			//go through all tabs and set the top to the largest bottom value
			for (	Counter = 0;
					Counter < (DWORD)GetTabCtrl().GetItemCount(); Counter++) {
				if (GetTabCtrl().GetItemRect(Counter, testrect)) {
					if (testrect.bottom > rect.top) {
						rect.top = testrect.bottom;
					}
				}
			}
			m_Edit->SetWindowPos(	&wndTop, rect.left, rect.top,
									rect.Width(), rect.Height(), 0);
		}
	}
	CTabCtrl & GetTabCtrl() const { return *((CTabCtrl*)this); }
	CEdit* GetTraceEdit() { return m_Edit; }
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		CRect rect;
		int iRet = CCtrlView::OnCreate(lpCreateStruct); 
		SetFont(GetParent()->GetFont());
		GetClientRect(rect);
		m_Edit = new CEdit();
		if (!m_Edit) return -1;
		if (!m_Edit->Create(	ES_LEFT | ES_MULTILINE | ES_READONLY | WS_VSCROLL |
						WS_HSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rect,
						this, TRACEEDITID)) return -1;
		m_Edit->SetFont(GetFont());
		return iRet;
	}
	afx_msg void OnDestroy()
	{
		m_Edit->DestroyWindow();
		delete m_Edit;
	}
	afx_msg void OnSize(UINT nType, int cx, int cy)
	{
		//must call parent first to update tab control
		CCtrlView::OnSize(nType, cx, cy);
		DoSize();
	}
	afx_msg void OnSelChange(NMHDR * /*pNotifyStruct*/, LRESULT* /*result*/)
	{
		DoSize();
	}
	CEdit* m_Edit;
	DECLARE_DYNCREATE(CTabView)
	DECLARE_MESSAGE_MAP()
};

class CSplitterWndRight : public CSplitterWndEx
{
	DECLARE_DYNCREATE(CSplitterWndRight)
	DECLARE_MESSAGE_MAP()
public:
	CSplitterWndRight() : CSplitterWndEx(FALSE) {}
	BOOL Create(LPCTSTR /*lpszClassName*/, LPCTSTR /*lpszWindowName*/,
				DWORD dwStyle, const RECT& /*rect*/, CWnd* pParentWnd, UINT nID, 
				CCreateContext* /*pContext*/)
	{
		return CreateStatic(pParentWnd, 2, 1, dwStyle | WS_CHILD | WS_VISIBLE,
							nID);
	}
	afx_msg void OnDestroy()
	{
		CFrameWnd* pFrameWnd = EnsureParentFrame();
		if (GetPane(1, 0) == pFrameWnd->GetActiveView())
			pFrameWnd->SetActiveView(NULL);
		DeleteView(1, 0);
		if (GetPane(0, 0) == pFrameWnd->GetActiveView())
			pFrameWnd->SetActiveView(NULL);
		GetPane(0, 0)->DestroyWindow(); //cannot DeleteView a non-CView derived object
		CSplitterWndEx::OnDestroy();
	}
	void PostNcDestroy()
	{
		// default for views is to allocate them on the heap
		//  the default post-cleanup is to 'delete this'.
		//  never explicitly call 'delete' on a view
		delete this;
	}
	int Init(CCreateContext* pContext)
	{
		CRect rect;
		int iRet = 0;
		SetFont(GetParent()->GetFont());
		SetRowInfo(0, 500, 100);
		if (!CreateView(0, 0, RUNTIME_CLASS(CCustomMultiListView),
						CSize(0, 440), pContext) ||
			!CreateView(1, 0, RUNTIME_CLASS(CTabView),
						CSize(300, 100), pContext)) {
			iRet = -1;
		}
		m_Context.m_pCurrentDoc = pContext->m_pCurrentDoc;
		m_Context.m_pCurrentFrame = pContext->m_pCurrentFrame;
		m_Context.m_pLastView = pContext->m_pLastView;
		m_Context.m_pNewDocTemplate = pContext->m_pNewDocTemplate;
		m_Context.m_pNewViewClass = pContext->m_pNewViewClass;
		//future: load default start view setting from registry
		//causing problems with taskbar icon and artifacts on change focus?
		//((CCustomMultiListView*)GetPane(0, 0))->OnSelChange(NULL, NULL);
		return iRet;
	}
	void InitDisplay()
	{
		CreateView(0, 0, RUNTIME_CLASS(CCustomMultiListView),
						CSize(0, 440), &m_Context);
		RecalcLayout();
	}
	void KillDisplay()
	{
		CFrameWnd* pFrameWnd = EnsureParentFrame();
		if (GetPane(0, 0) == pFrameWnd->GetActiveView())
			pFrameWnd->SetActiveView(NULL);
		GetPane(0, 0)->DestroyWindow(); //cannot DeleteView a non-CView derived object
	}
	CTabCtrl* GetTabCtrl()
	{ return &((CTabView*)GetPane(1, 0))->GetTabCtrl(); }
private:
	CCreateContext m_Context;
};

// CPestRidSplitterWnd dialog

//window needs splitter, sizable
class CPestRidSplitterWnd : public CSplitterWndEx
{
// Construction
public:
	CPestRidSplitterWnd(CWnd* pParent = NULL);	// standard constructor
	virtual ~CPestRidSplitterWnd();
	int Init(CCreateContext* pContext);
	void OnSelChange(CEnumModel* pModel, DisplaySet* pSet, void* pvKey)
	{
		if (m_ViewUpdateCS.Lock()) {
			if (pSet->paDisplayColumns[0].pRendererInfo.pRenderer == NULL && m_pCurrentView) {
				((CSplitterWndRight*)GetPane(0, 1))->KillDisplay();
				m_pCurrentView = NULL;
				((CSplitterWndRight*)GetPane(0, 1))->InitDisplay();
				m_pCurrentView = (CCustomMultiListView*)
											((CSplitterWndRight*)GetPane(0, 1))->GetPane(0, 0);
			} else {
				if (!m_pCurrentView) {
					((CSplitterWndRight*)GetPane(0, 1))->InitDisplay();
					m_pCurrentView = (CCustomMultiListView*)
											((CSplitterWndRight*)GetPane(0, 1))->GetPane(0, 0);
				}
				m_pCurrentView->OnSelChange(pSet, pModel, pvKey);
			}
			m_ViewUpdateCS.Unlock();
			SetEvent(m_hDirty);
		}
	}
	void GetStatusText(CString& String)
	{
		if (m_StatusCS.Lock()) {
			String = m_StatusText;
			m_StatusCS.Unlock();
		}
	}
protected:
	void OnInitialUpdate();
	afx_msg void OnDestroy();
	void PostNcDestroy()
	{
		// default for views is to allocate them on the heap
		//  the default post-cleanup is to 'delete this'.
		//  never explicitly call 'delete' on a view
		delete this;
	}

	void QueryThreadProc();
	static unsigned int WINAPI QueryUpdateThread(void* lParam)
	{
		((CPestRidSplitterWnd*)lParam)->QueryThreadProc();
		return 0;
	}
	CCriticalSection m_StatusCS;
	CString m_StatusText;
	CCriticalSection m_ViewUpdateCS;

	//must cache for thread use as GetPane uses the thread AFX message map
	CCustomTreeView* m_pViewSelector;
	CCustomMultiListView* m_pCurrentView;

	HANDLE m_hWorkerThread;
	HANDLE m_hTermination;
	HANDLE m_hDirty;

	DECLARE_DYNCREATE(CPestRidSplitterWnd)
	DECLARE_MESSAGE_MAP()
};