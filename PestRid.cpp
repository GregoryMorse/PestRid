// PestRid.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PestRid.h"
#include "PestRidDlg.h"
#include "MainFrm.h"
#include "SysSeeAllDoc.h"
#include "Utility.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()



// CPestRidApp

BEGIN_MESSAGE_MAP(CPestRidApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CPestRidApp::OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPestRidApp construction

CPestRidApp::CPestRidApp()
{
}

CPestRidSettings g_PestRidSettings;

// The one and only CPestRidApp object

CPestRidApp theApp;

// CPestRidApp initialization

BOOL CPestRidApp::InitInstance()
{
	CSingleDocTemplate* pDocTemplate;
	INITCOMMONCONTROLSEX InitCtrls;
	CCommandLineInfo cmdInfo;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	if (!CWinApp::InitInstance()) return FALSE;
	SetRegistryKey(_T("PestRid"));

	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	if (!InitCommonControlsEx(&InitCtrls)) return FALSE;

	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CSysSeeAllDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		NULL); //since CSplitterWnd does not derive from CView
	if (!pDocTemplate) return FALSE;
	AddDocTemplate(pDocTemplate);

	ParseCommandLine(cmdInfo);
	if (!ProcessShellCommand(cmdInfo)) return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}

void CPestRidApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
