// Gregory Morse

// PestRidDlg.cpp : implementation file
//

#include "stdafx.h"
#include <process.h>
#include "PestRid.h"
#include "PestRidDlg.h"
#include "MainFrm.h"
#include "afxpriv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CTabView, CCtrlView)
IMPLEMENT_DYNCREATE(CSplitterWndRight, CSplitterWndEx)
IMPLEMENT_DYNCREATE(CPestRidSplitterWnd, CSplitterWndEx)

BEGIN_MESSAGE_MAP(CTabView, CCtrlView)
//{{AFX_MSG_MAP(CTabView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelChange)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CSplitterWndRight, CSplitterWndEx)
//{{AFX_MSG_MAP(CPestRidSplitterWnd)
	ON_WM_DESTROY()
	ON_WM_NCDESTROY()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CPestRidSplitterWnd, CSplitterWndEx)
//{{AFX_MSG_MAP(CPestRidSplitterWnd)
	ON_WM_DESTROY()
	ON_WM_NCDESTROY()
	ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CPestRidSplitterWnd dialog

CPestRidSplitterWnd::CPestRidSplitterWnd(CWnd* /*pParent=NULL*/)
	: CSplitterWndEx(FALSE)
{
	m_pViewSelector = NULL;
	m_pCurrentView = NULL;
	m_hTermination = NULL;
	m_hWorkerThread = NULL;
	m_hDirty = NULL;
}

CPestRidSplitterWnd::~CPestRidSplitterWnd()
{
}


// CPestRidSplitterWnd message handlers

int CPestRidSplitterWnd::Init(CCreateContext* pContext)
{
	//create left pane tree
	if (!CreateView(0, 0, RUNTIME_CLASS(CCustomTreeView),
					CSize(200, 600), pContext)) return -1;
	//create right pane
	if (!CreateView(0, 1, RUNTIME_CLASS(CSplitterWndRight),
					CSize(0, 0), pContext)) return -1;
	m_pViewSelector = (CCustomTreeView*)GetPane(0, 0);
	if (((CSplitterWndRight*)GetPane(0, 1))->Init(pContext) == -1) return -1;
	m_pCurrentView = (CCustomMultiListView*)
							((CSplitterWndRight*)GetPane(0, 1))->GetPane(0, 0);

	return 0;
}

void CPestRidSplitterWnd::OnInitialUpdate()
{
	if ((m_hTermination = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
		return;
	}
	if ((m_hDirty = CreateEvent(NULL, FALSE, TRUE, NULL)) == NULL) {
		CloseHandle(m_hTermination);
		m_hTermination = NULL;
		return;
	}
	if ((m_hWorkerThread = 
			(HANDLE)_beginthreadex(
				NULL, 0, 
				&CPestRidSplitterWnd::QueryUpdateThread,
				this, 0, NULL)) == NULL ||
		(m_hWorkerThread == INVALID_HANDLE_VALUE)) {
		CloseHandle(m_hTermination);
		CloseHandle(m_hDirty);
		m_hTermination = m_hDirty = NULL;
	}
}

void CPestRidSplitterWnd::OnDestroy()
{
	if (m_ViewUpdateCS.Lock()) {
		SetEvent(m_hTermination);
		ResetEvent(m_hDirty);
		CFrameWnd* pFrameWnd = EnsureParentFrame();
		if (GetPane(0, 0) == pFrameWnd->GetActiveView())
			pFrameWnd->SetActiveView(NULL);
		DeleteView(0, 0);
		if (GetPane(0, 1) == pFrameWnd->GetActiveView())
			pFrameWnd->SetActiveView(NULL);
		GetPane(0, 1)->DestroyWindow(); //cannot DeleteView a non-CView derived object
		m_pViewSelector = NULL;
		m_pCurrentView = NULL;
		m_ViewUpdateCS.Unlock();
	}
	if (WaitForSingleObject(m_hWorkerThread, 
							THREAD_TERMINATE_TIMEOUT) == WAIT_TIMEOUT) {
		TerminateThread(m_hWorkerThread, 1);
	}
	CloseHandle(m_hDirty);
	CloseHandle(m_hTermination);
	CloseHandle(m_hWorkerThread);
	CSplitterWndEx::OnDestroy();
}

void CPestRidSplitterWnd::QueryThreadProc()
{
	DWORD dwObj;
	BOOL bTreeUpdate;
	BOOL bUpdate;
	DWORD dwTickDiff;
	DWORD dwOldTicks;
	DWORD dwWaitTime = THREAD_MAX_POLL_TIMEOUT;
	HANDLE paHandleArray[2] = { m_hTermination, m_hDirty };
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	while (	(dwObj = WaitForMultipleObjects(2, paHandleArray, FALSE,
											dwWaitTime)) !=
			WAIT_OBJECT_0) {
		ASSERT(dwObj == (WAIT_OBJECT_0 + 1) || dwObj == WAIT_TIMEOUT);
		dwOldTicks = GetTickCount();
		bTreeUpdate = FALSE;
		if (m_ViewUpdateCS.Lock()) {
			if (m_pViewSelector) bTreeUpdate = m_pViewSelector->DoQuery();
			m_ViewUpdateCS.Unlock();
		}
		dwTickDiff = GetTickCount() - dwOldTicks;
		dwOldTicks = dwTickDiff + dwOldTicks;
		bUpdate = FALSE;
		if (m_ViewUpdateCS.Lock()) {
			//cannot change current update list while updating
			if (m_pCurrentView && m_pCurrentView->GetCurrentModel()) {
				m_pCurrentView->GetCurrentModel()->OnUpdateDatabase();
				bUpdate = TRUE;
			}
			m_ViewUpdateCS.Unlock();
		}
		if (m_ViewUpdateCS.Lock()) {
			//must post messages at the same time due to race conditions when list depends on tree data
			if (bTreeUpdate && m_pViewSelector) m_pViewSelector->PostMessage(WM_PESTRID_UPDATEUI);
			if (bUpdate && m_pCurrentView) m_pCurrentView->PostMessage(WM_PESTRID_UPDATEUI);
			m_ViewUpdateCS.Unlock();
		}
		//Need to change to status bar
		if (m_StatusCS.Lock()) {
			m_StatusText.Format(_T("Tree Update: %lumsec  List Update: %lumsec"),
							dwTickDiff, GetTickCount() - dwOldTicks);
			m_StatusCS.Unlock();
		}
		if (dwWaitTime / 2 < GetTickCount() - dwOldTicks) dwWaitTime = (GetTickCount() - dwOldTicks) * 4;
		GetParent()->PostMessage(WM_UPDATE_STATUSTEXT, 0, 0);
		//ResetEvent(m_hDirty);
	}
	CoUninitialize();
}