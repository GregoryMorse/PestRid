// Gregory Morse

// PestRidDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PestRid.h"
#include "PestRidDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CCustomListCtrl, CListCtrl)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SIZE()
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CCustomTreeListCtrl, CListCtrl)

BEGIN_MESSAGE_MAP(CCustomTreeListCtrl, CListCtrl)
	ON_WM_SIZE()
	ON_WM_NCCALCSIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CTabView, CCtrlView)

BEGIN_MESSAGE_MAP(CTabView, CCtrlView)
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, &CTabView::OnSelChange)
	ON_WM_SIZE()
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CSplitterWndTopRight, CSplitterWnd)
IMPLEMENT_DYNCREATE(CSplitterWndRight, CSplitterWnd)


// CPestRidSplitterWnd dialog

DWORD CPestRidSplitterWnd::m_OSMinorVersion = -1;
DWORD CPestRidSplitterWnd::m_FileTypeIndex = -1;
SC_HANDLE CPestRidSplitterWnd::m_hSCM = NULL;
HANDLE CPestRidSplitterWnd::m_hDriver = NULL;
BOOL CPestRidSplitterWnd::m_bUpdating = FALSE;

CPestRidSplitterWnd::CPestRidSplitterWnd(CWnd* pParent /*=NULL*/)
	: CSplitterWnd()
{
	HMODULE hModule;
	OSVERSIONINFO osvi;
	m_MainTab = NULL;
	m_ProcList = NULL;
	m_ProcTreeList = NULL;
	m_BottomTab = NULL;
	m_MainTree = NULL;
	m_TraceLogEdit = NULL;
	m_pspi = NULL;
	m_dwpspiSize = 0;
	m_CurrentSortItem = -1;
	m_SortAscending = true;
	m_hDummyIcon = NULL;
	m_ServiceInformation = NULL;
	m_ServiceInformationBufferSize = 2048;
	m_NumberOfServices = 0;
	m_DriverInformation = NULL;
	m_DriverInformationBufferSize = 2048;
	m_NumberOfDrivers = 0;
	m_ProcessTabs.Add(new ProcessListTabTree());
	m_ProcessTabs.Add(new ProcessListTabList());
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT || osvi.dwMajorVersion == 4) {
		AddTraceLog("APICall=GetVersionEx Error=Windows NT 4.0, 2000, XP, 2003 platform not detected will not run with all features\r\n");
	}
	InitializeCriticalSection(&m_ProtectDatabase);
	m_OSMinorVersion = osvi.dwMinorVersion;
	//SeLoadDriverPrivilege if using NtLoadDriver
	if (!EnablePrivilege(SE_SECURITY_NAME))
		AddTraceLog("MyCall=EnablePrivilege Privilege="SE_SECURITY_NAME" Error\r\n");
	if (!EnablePrivilege(SE_BACKUP_NAME))
		AddTraceLog("MyCall=EnablePrivilege Privilege="SE_BACKUP_NAME" Error\r\n");
	if (!EnablePrivilege(SE_DEBUG_NAME))
		AddTraceLog("MyCall=EnablePrivilege Privilege="SE_DEBUG_NAME" Error\r\n");
	if (!EnablePrivilege(SE_TAKE_OWNERSHIP_NAME))
		AddTraceLog("MyCall=EnablePrivilege Privilege="SE_TAKE_OWNERSHIP_NAME" Error\r\n");
	if (!EnablePrivilege(SE_TCB_NAME)) //grant SE_TCB_NAME if necessary
		AddTraceLog("MyCall=EnablePrivilege Privilege="SE_TCB_NAME" Error\r\n");
	m_ObjInf.hEventStart = m_ObjInf.hEventDone = m_hObjThread = INVALID_HANDLE_VALUE;
	if (hModule = GetModuleHandle("NTDLL.DLL")) {
		if (!(_NtQueryObject = (__NtQueryObject)GetProcAddress(hModule, "NtQueryObject"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=NtQueryObject Error=%08X\r\n", GetLastError());
		}
		if (!(_NtQuerySystemInformation = (__NtQuerySystemInformation)GetProcAddress(hModule, "NtQuerySystemInformation"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=NtQuerySystemInformation Error=%08X\r\n", GetLastError());
		}
		if (!(_RtlInitUnicodeString = (__RtlInitUnicodeString)GetProcAddress(hModule, "RtlInitUnicodeString"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=RtlInitUnicodeString Error=%08X\r\n", GetLastError());
		}
		if (!(_NtOpenDirectoryObject = (__NtOpenDirectoryObject)GetProcAddress(hModule, "NtOpenDirectoryObject"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=NtOpenDirectoryObject Error=%08X\r\n", GetLastError());
		}
		if (!(_NtQueryDirectoryObject = (__NtQueryDirectoryObject)GetProcAddress(hModule, "NtQueryDirectoryObject"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=NtQueryDirectoryObject Error=%08X\r\n", GetLastError());
		}
		if (!(_NtQueryInformationFile = (__NtQueryInformationFile)GetProcAddress(hModule, "NtQueryInformationFile"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=NtQueryInformationFile Error=%08X\r\n", GetLastError());
		}
		if (!(_NtSuspendProcess = (__NtSuspendProcess)GetProcAddress(hModule, "NtSuspendProcess"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=NtSuspendProcess Error=%08X\r\n", GetLastError());
		}
		if (!(_NtResumeProcess = (__NtSuspendProcess)GetProcAddress(hModule, "NtResumeProcess"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=NtResumeProcess Error=%08X\r\n", GetLastError());
		}
		if (!(_NtQueryInformationProcess = (__NtQueryInformationProcess)GetProcAddress(hModule, "NtQueryInformationProcess"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=NtQueryInformationProcess Error=%08X\r\n", GetLastError());
		}
	} else {
		AddTraceLog("APICall=GetModuleHandle ModuleName=NTDLL.DLL Error=%08X\r\n", GetLastError());
	}
	if (hModule = GetModuleHandle("user32.dll")) {
		if (!(_IsHungAppWindow = (__IsHungAppWindow)GetProcAddress(hModule, "IsHungAppWindow"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=IsHungAppWindow Error=%08X\r\n", GetLastError());
		}
	} else {
		AddTraceLog("APICall=GetModuleHandle ModuleName=user32.dll Error=%08X\r\n", GetLastError());
	}
	if (m_hPsapi = LoadLibrary("PSAPI.DLL")) {
		if (!(_GetModuleFileNameEx = (__GetModuleFileNameEx)GetProcAddress(m_hPsapi, "GetModuleFileNameExA"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=GetModuleFileNameExA Error=%08X\r\n", GetLastError());
		}
		if (!(_EnumProcesses = (__EnumProcesses)GetProcAddress(m_hPsapi, "EnumProcesses"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=EnumProcesses Error=%08X\r\n", GetLastError());
		}
		if (!(_GetProcessImageFileName = (__GetProcessImageFileName)GetProcAddress(m_hPsapi, "GetProcessImageFileNameA"))) {
			AddTraceLog("APICall=GetProcAddress FunctionName=GetProcessImageFileNameA Error=%08X\r\n", GetLastError());
		}
	} else {
		AddTraceLog("APICall=LoadLibrary LibraryName=PSAPI.DLL Error=%08X\r\n", GetLastError());
	}
}

CPestRidSplitterWnd::~CPestRidSplitterWnd()
{
	INT_PTR Counter;
	for (Counter = m_ProcessTabs.GetCount() - 1; Counter >= 0; Counter--) {
		delete m_ProcessTabs[Counter];
	}
	if (m_hObjThread && (m_hObjThread != INVALID_HANDLE_VALUE)) {
		if (!m_ObjInf.hEventStart || m_ObjInf.hEventStart == INVALID_HANDLE_VALUE) {
			if (!(m_ObjInf.hEventStart = CreateEvent(NULL, FALSE, FALSE, NULL))) {
				//AddTraceLog("APICall=CreateEvent Use=Start Error=%08X\r\n", GetLastError());
			}
		}
		if (!m_ObjInf.hEventDone || m_ObjInf.hEventDone == INVALID_HANDLE_VALUE) {
			if (!(m_ObjInf.hEventDone = CreateEvent(NULL, FALSE, FALSE, NULL))) {
				//AddTraceLog("APICall=CreateEvent Use=Done Error=%08X\r\n", GetLastError());
			}
		}
		m_ObjInf.hObject = INVALID_HANDLE_VALUE;
		if (!SetEvent(m_ObjInf.hEventStart)) {
			//AddTraceLog("APICall=SetEvent StartHandle=%08X Error=%08X\r\n", m_ObjInf.hEventStart, GetLastError());
		}
		if (WaitForSingleObject(m_ObjInf.hEventDone, 1000) == WAIT_TIMEOUT) {
			//AddTraceLog("APICall=WaitForSingleObject DoneEvent=%08X Error=Timeout TerminatingThread=%08X\r\n", m_ObjInf.hEventDone, m_hObjThread);
			if (!TerminateThread(m_hObjThread, 1)) {
				//AddTraceLog("APICall=TerminateThread Thread=%08X Error=%08X\r\n", m_hObjThread, GetLastError());
			}
			if (!CloseHandle(m_hObjThread)) {
				//AddTraceLog("APICall=CloseHandle Thread=%08X Error=%08X\r\n", m_hObjThread, GetLastError());
			}
		}
		m_hObjThread = INVALID_HANDLE_VALUE;
	}
	if (m_ObjInf.hEventStart && (m_ObjInf.hEventStart != INVALID_HANDLE_VALUE)) {
		if (!CloseHandle(m_ObjInf.hEventStart)) {
			//AddTraceLog("APICall=CloseHandle StartEvent=%08X Error=%08X\r\n", m_ObjInf.hEventStart, GetLastError());
		}
		m_ObjInf.hEventStart = NULL;
	}
	if (m_ObjInf.hEventDone && (m_ObjInf.hEventDone != INVALID_HANDLE_VALUE)) {
		if (!CloseHandle(m_ObjInf.hEventDone)) {
			//AddTraceLog("APICall=CloseHandle DoneEvent=%08X Error=%08X\r\n", m_ObjInf.hEventDone, GetLastError());
		}
		m_ObjInf.hEventDone = NULL;
	}
	if (m_hPsapi) {
		if (!FreeLibrary(m_hPsapi)) {
			//AddTraceLog("APICall=FreeLibrary Module=%08X Error=%08X\r\n", m_hPsapi, GetLastError());
		}
		m_hPsapi = NULL;
	}
	UnloadDriver(GPD_DOSDEVICE, GPD_DOSDEVICEGLOBAL, "PestRidDrv");
	CloseServiceManager();
	DeleteCriticalSection(&m_ProtectDatabase);
}

BEGIN_MESSAGE_MAP(CPestRidSplitterWnd, CSplitterWnd)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_NCDESTROY()
	ON_WM_DRAWITEM()
	ON_WM_CREATE()
	ON_MESSAGE(WM_APP_SYNCSCROLL, OnAppSyncScroll)
	ON_MESSAGE(WM_APP_SIZETREELIST, OnAppSizeTreeList)
	ON_MESSAGE(WM_APP_MOUSEWHEEL, OnAppMouseWheel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CPestRidSplitterWnd message handlers

int CPestRidSplitterWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CBitmap Bitmap;
	if (CSplitterWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	m_ProcessIdDatabase = new CMapWordToPtr(2048);
	m_ProcessIdDatabase->InitHashTable(49157); //http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
	EnumKernelNamespaceObjectTypes();
	m_DirectoryTreeRoot = new DirectoryTreeEntry();
	m_DirectoryTreeRoot->bDirty = FALSE;
	m_DirectoryTreeRoot->Index = 0;
	m_DirectoryTreeRoot->Name = new WCHAR[wcslen(L"\\") + 1];
	wcscpy(m_DirectoryTreeRoot->Name, L"\\");

	m_TreeImageList.Create(16, 16, ILC_COLOR, 2, 10);
	Bitmap.LoadBitmap(IDB_DIR);
	m_TreeImageList.Add(&Bitmap, RGB(0, 0, 0));
	Bitmap.DeleteObject();
	Bitmap.LoadBitmap(IDB_DIRSEL);
	m_TreeImageList.Add(&Bitmap, RGB(0, 0, 0));
	Bitmap.DeleteObject();
	m_ProcImageList.Create(16, 16, ILC_MASK, 1, 0);
	m_DummyImageList.Create(1, 16, ILC_MASK, 1, 0);
	BYTE* AndBuffer = new BYTE[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON) / 8];
	BYTE* XorBuffer = new BYTE[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON) / 8];
	memset(AndBuffer, 0, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON) / 8); 
	memset(XorBuffer, 0xFF, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON) / 8); 
	m_DummyImageList.Add(m_hDummyIcon = CreateIcon(AfxGetApp()->m_hInstance, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 1, AndBuffer, XorBuffer));
	delete [] AndBuffer;
	delete [] XorBuffer;
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_DEVICE32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_DIR32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_DRIVER32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_EVENT32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_KEY32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_MUTANT32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_PORT32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_PROFILE32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_SECTION32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_SEMAPHORE32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_SYMLINK32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_TIMER32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_UNK32));
	m_ProcImageList.Add(AfxGetApp()->LoadIcon(IDI_WINDOWSTATION32));

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	return 0;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPestRidSplitterWnd::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CSplitterWnd::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPestRidSplitterWnd::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPestRidSplitterWnd::OnLbnDblclkProclist()
{
	// need NSIS wrapper
	/*
	DWORD Count;
	DWORD Counter;
	HANDLE hFile;
	CStringArray FileNames;
	INT* SelItems;
	HANDLE* SelHandles;
	DWORD SelCount;
	int nItem = -1;
	if ((SelCount = GetProcList()->GetSelectedCount()) != -1) {
		SelItems = new INT[SelCount];
		SelHandles = new HANDLE[SelCount];
		for (Count = 0; Count < SelCount; Count++) {
			nItem = GetProcList()->GetNextItem(nItem, LVNI_SELECTED);
			SelItems[Count] = nItem;
		}
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		for (Counter = 0; Counter < SelCount; Counter++) {
			if ((SelHandles[Counter] = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)GetProcList()->GetItemData(SelItems[Counter]))) && (SelHandles[Counter] != INVALID_HANDLE_VALUE)) {
				if (m_PidPathArray[SelItems[Counter]].IsEmpty()) {
					FileNames.RemoveAll();
					Str.Format("Error getting image file name [%08X] code = %08X", GetProcList()->GetItemData(SelItems[Counter]), GetLastError());
					SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
					MessageBox(Str);
					break;
				} else
					FileNames.Add(m_PidPathArray[SelItems[Counter]]);
				TerminateProcess(SelHandles[Counter], 0);
				CloseHandle(SelHandles[Counter]);
			} else {
				FileNames.RemoveAll();
				Str.Format("Error opening process [%08X] code = %08X", GetProcList()->GetItemData(SelItems[Counter]), GetLastError());
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
				MessageBox(Str);
				break;
			}
		}
		for (Counter = 0; Counter < (DWORD)FileNames.GetCount(); Counter++) {
			Count = 0;
			while (true) {
				// try better strategies open with no share first, then also allow write share and fight, etc
				if ((hFile = CreateFile(FileNames[Counter], GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
					m_LockFileArray.Add(hFile);
					break;
				} else {
					if (GetLastError() != ERROR_SHARING_VIOLATION) {
						Str.Format("Create File failed with error %08X", GetLastError());
						MessageBox(Str);
						break;
					}
				}
				Count++;
				if (Count > 2000)
					Sleep(55);
			}
		}
		// should also enumerate through all handles on system and manually close any handles still open for the process
		// should also do the same for the file handle on the object not just the process handle
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		delete [] SelItems;
		delete [] SelHandles;
	}*/
}

void CPestRidSplitterWnd::OnDestroy()
{
	POSITION pos;
	POSITION posin;
	WORD key;
	HandleEntry* val;
	DirectoryEntry* DirEntry;
	CString Str;
	CMapWordToPtr* map;
	ProcessEntry* Entry;
	SetEvent(m_hTermination);
	if (WaitForSingleObject(m_hWorkerThread, 4000) == WAIT_TIMEOUT) {
		TerminateThread(m_hWorkerThread, 1);
	}
	CloseHandle(m_hTermination);
	CloseHandle(m_hWorkerThread);
	GetMainTree()->SetImageList(NULL, TVSIL_NORMAL);
	CSplitterWnd::OnDestroy();
	if (m_ServiceInformation) {
		delete [] m_ServiceInformation;
		m_NumberOfServices = 0;
		m_ServiceInformation = NULL;
	}
	if (m_DriverInformation) {
		delete [] m_DriverInformation;
		m_NumberOfDrivers = 0;
		m_DriverInformation = NULL;
	}
	pos = m_ProcessEntries.GetStartPosition();
	while (pos) {
		m_ProcessEntries.GetNextAssoc(pos, key, (void*&)Entry);
		DestroyIcon(Entry->hIcon);
		delete Entry;
	}
	m_ProcessEntries.RemoveAll();
	if (m_pspi) {
		delete [] m_pspi;
		m_pspi = NULL;
	}
	pos = m_DirectoryEntries.GetStartPosition();
	while (pos != NULL) {
		m_DirectoryEntries.GetNextAssoc(pos, Str, (void*&)DirEntry);
		delete [] DirEntry->Name;
		delete [] DirEntry->Type;
		delete DirEntry;
	}
	m_DirectoryEntries.RemoveAll();
	DeleteChildTree(m_DirectoryTreeRoot);
	m_DirectoryTreeRoot = NULL;
	if (m_ProcessIdDatabase) {
		pos = m_ProcessIdDatabase->GetStartPosition();
		while (pos) {
			m_ProcessIdDatabase->GetNextAssoc(pos, key, (void*&)map);
			posin = map->GetStartPosition();
			while (posin) {
				map->GetNextAssoc(posin, key, (void*&)val);
				delete [] val->Name;
				delete val;
			}
			delete map;
		}
		delete m_ProcessIdDatabase;
		m_ProcessIdDatabase = NULL;
	}
	if (m_hDummyIcon) {
		DestroyIcon(m_hDummyIcon);
		m_hDummyIcon = NULL;
	}
	while (m_LockFileArray.GetCount()) {
		if (!CloseHandle(m_LockFileArray[0])) {
			AddTraceLog("APICall=CloseHandle LockFile=%08X Error=%08X\r\n", m_LockFileArray[0], GetLastError());
		}
		m_LockFileArray.RemoveAt(0);
	}
}

void CPestRidSplitterWnd::OnTcnSelchangeMaintab(NMHDR *pNMHDR, LRESULT *pResult)
{
	ProcessEntry* Entry;
	if (GetItemType(GetMainTree()->GetSelectedItem()) == PROCESS_ITEM) {
		if (m_ProcessEntries.Lookup(0, (void*&)Entry))
			ExpandCollapseTree(Entry);
		GetProcList()->SortItems(SortFunc, (DWORD_PTR)this);
		m_ProcTreeList->SortItems(SortFunc, (DWORD_PTR)this);
	}	
	*pResult = 0;
}

void CPestRidSplitterWnd::OnTvnSelchangedMaintree(NMHDR *pNMHDR, LRESULT *pResult)
{
	USES_CONVERSION;
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	WORD key;
	ProcessEntry* Entry;
	DWORD Counter;
	POSITION pos;
	CString Str;
	int cx;
	DirectoryEntry* DirEntry;
	CRect rect;
	CTabView* OtherTab;
	DWORD Item = GetItemType(pNMTreeView->itemNew.hItem);
	DWORD TotalCols;
	OtherTab = (CTabView*)((CSplitterWndRight*)((CSplitterWnd*)GetPane(0, 1))->GetPane(0, 0))->GetPane(0, 1);
	OtherTab->GetClientRect(rect);
	if (m_ProcList)
		m_ProcList->DestroyWindow();
	m_ProcList->Create(LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_ALIGNLEFT | WS_TABSTOP | WS_CHILD | WS_VISIBLE, rect, OtherTab, LISTID);
	m_ProcList->SetExtendedStyle(m_ProcList->GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	m_ProcList->SetFont(GetMainTree()->GetFont());
	OtherTab->CreateChildWindow(m_ProcList);
	TotalCols = GetProcList()->GetHeaderCtrl()->GetItemCount();
	GetProcList()->DeleteAllItems();
	for (Counter = 0; Counter < TotalCols; Counter++) {
		GetProcList()->DeleteColumn(0);
	}
	TotalCols = m_ProcTreeList->GetHeaderCtrl()->GetItemCount();
	for (Counter = 0; Counter < TotalCols; Counter++) {
		m_ProcTreeList->DeleteColumn(0);
	}
	GetMainTab()->DeleteAllItems();	
	if (Item != PROCESS_ITEM)
		((CSplitterWndTopRight*)((CSplitterWndRight*)GetPane(0, 1))->GetPane(0, 0))->SetColumnInfo(0, 0, 0xFFFF);
	if (Item == PROCESS_ITEM) {
		((CSplitterWndTopRight*)((CSplitterWndRight*)GetPane(0, 1))->GetPane(0, 0))->SetColumnInfo(0, 200, 0);
		m_ProcList->SetImageList(&m_DummyImageList, LVSIL_SMALL);
		m_ProcTreeList->InsertColumn(0, ProcessNameColumn::_GetName());
		for (Counter = 0; Counter < (DWORD)m_ProcessTabs[0]->GetColumnCount(); Counter++) {
			GetProcList()->InsertColumn(Counter, m_ProcessTabs[0]->GetColumn(Counter)->GetName());
		}
		for (Counter = 0; Counter < (DWORD)m_ProcessTabs.GetCount(); Counter++) {
			GetMainTab()->InsertItem(Counter, m_ProcessTabs[Counter]->GetName());
		}
		GetMainTab()->SendMessage(WM_SIZE);
		OnAppSizeTreeList(0, 0);
		pos = m_ProcessEntries.GetStartPosition();
		while (pos) {
			m_ProcessEntries.GetNextAssoc(pos, key, (void*&)Entry);
			delete Entry;
		}
		m_ProcessEntries.RemoveAll();
		if (m_pspi) {
			delete [] m_pspi;
			m_pspi = NULL;
		}
		TotalCols = GetProcList()->GetHeaderCtrl()->GetItemCount();
		if ((m_CurrentSortItem != -1) && ((DWORD)m_CurrentSortItem >= TotalCols))
			m_CurrentSortItem = TotalCols - 1;
		m_ProcTreeList->SendMessage(WM_SIZE);
		UpdateProcessList();
	} else if (Item == SERVICE_ITEM) {
		GetProcList()->InsertColumn(0, "Index");
		GetProcList()->InsertColumn(1, "Service Name");
		GetProcList()->InsertColumn(2, "Display Name");
		GetProcList()->InsertColumn(3, "Status");
		GetProcList()->InsertColumn(4, "Service Type");
		GetProcList()->InsertColumn(5, "Startup Type");
		if (m_ServiceInformation) {
			delete [] m_ServiceInformation;
			m_ServiceInformation = NULL;
			m_NumberOfServices = 0;
		}
		UpdateServiceInformation();
	} else if (Item == DRIVER_ITEM) {
		GetProcList()->InsertColumn(0, "Index");
		GetProcList()->InsertColumn(1, "Driver Name");
		GetProcList()->InsertColumn(2, "Display Name");
		GetProcList()->InsertColumn(3, "Status");
		GetProcList()->InsertColumn(4, "Driver Type");
		GetProcList()->InsertColumn(5, "Startup Type");
		if (m_DriverInformation) {
			delete [] m_DriverInformation;
			m_DriverInformation = NULL;
			m_NumberOfDrivers = 0;
		}
		UpdateDriverInformation();
	} else if (Item == ROOT_ITEM) {
		m_ProcList->SetImageList(&m_ProcImageList, LVSIL_SMALL);
		GetProcList()->InsertColumn(0, "Index");
		GetProcList()->InsertColumn(1, "Name");
		GetProcList()->InsertColumn(2, "Type");
		pos = m_DirectoryEntries.GetStartPosition();
		while (pos != NULL) {
			m_DirectoryEntries.GetNextAssoc(pos, Str, (void*&)DirEntry);
			delete [] DirEntry->Name;
			delete [] DirEntry->Type;
			delete DirEntry;
		}
		m_DirectoryEntries.RemoveAll();
		QueryDirectory(A2W(GetTreeDirectoryPath(((NMTREEVIEW*)pNMHDR)->itemNew.hItem)));
	} else {
		m_ProcList->SetImageList(&m_ProcImageList, LVSIL_SMALL);
		GetProcList()->InsertColumn(0, "Name");
		GetProcList()->InsertColumn(1, "PID");
		GetProcList()->InsertColumn(2, "Handle");
		GetProcList()->InsertColumn(3, "Index");
		UpdateHandleDatabaseList();
	}
	((CSplitterWndTopRight*)((CSplitterWndRight*)GetPane(0, 1))->GetPane(0, 0))->RecalcLayout();
	TotalCols = GetProcList()->GetHeaderCtrl()->GetItemCount();
	if ((m_CurrentSortItem == -1) && (Item != PROCESS_ITEM) || ((DWORD)m_CurrentSortItem >= TotalCols))
		m_CurrentSortItem = (m_CurrentSortItem == -1) ? 0 : (TotalCols - 1);
	// these calculations are too slow and inefficient should be done manually for speedup
	for (Counter = 0; Counter < TotalCols; Counter++) {
		GetProcList()->SetColumnWidth(Counter, LVSCW_AUTOSIZE_USEHEADER);
		cx = GetProcList()->GetColumnWidth(Counter);
		GetProcList()->SetColumnWidth(Counter, LVSCW_AUTOSIZE);
		if (GetProcList()->GetColumnWidth(Counter) < cx)
			GetProcList()->SetColumnWidth(Counter, cx);
	}
	HDITEM HeaderItem;
	HeaderItem.mask = HDI_FORMAT | HDI_BITMAP;
	CHeaderCtrl* HeaderCtrl = ((m_CurrentSortItem == -1) ? m_ProcTreeList : GetProcList())->GetHeaderCtrl();
	HeaderCtrl->GetItem((m_CurrentSortItem == -1) ? 0 : m_CurrentSortItem, &HeaderItem);
	if (HeaderItem.hbm != 0) {
		DeleteObject(HeaderItem.hbm);
		HeaderItem.hbm = 0;
	}
	HeaderItem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
	HeaderItem.hbm = (HBITMAP)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_SortAscending ? IDB_UP : IDB_DOWN), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	HeaderCtrl->SetItem((m_CurrentSortItem == -1) ? 0 : m_CurrentSortItem, &HeaderItem);
	*pResult = 0;
}

void CPestRidSplitterWnd::OnLvnGetdispinfoProclist(NMHDR *pNMHDR, LRESULT *pResult)
{
	USES_CONVERSION;
	static const char* ServiceStatus[] = { "Unknown", "Stopped", "Start Pending", "Stop Pending", "Running", "Continue Pending", "Pause Pending", "Paused" };
	static CString Str;
	WCHAR* Type;
	ProcessEntry* Entry;
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	if (pDispInfo->item.mask & LVIF_TEXT) {
		Str.Empty();
		switch (GetItemType(GetMainTree()->GetSelectedItem())) {
		case PROCESS_ITEM:
			if (m_ProcessEntries.Lookup((WORD)pDispInfo->item.lParam, (void*&)Entry)) {
				Str = m_ProcessTabs[GetMainTab()->GetCurSel()]->GetColumn(pDispInfo->item.iSubItem)->GetDispInfo(Entry);
			}
			break;
		case SERVICE_ITEM:
			switch (pDispInfo->item.iSubItem) {
			case 0:
				Str.Format("%lu", pDispInfo->item.lParam);
				break;
			case 1:
				Str = m_ServiceInformation[pDispInfo->item.lParam].lpServiceName;
				break;
			case 2:
				Str = m_ServiceInformation[pDispInfo->item.lParam].lpDisplayName;
				break;
			case 3:
				Str = ServiceStatus[(m_ServiceInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwCurrentState > SERVICE_PAUSED) ? 0 : m_ServiceInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwCurrentState];
				break;
			case 4:
				Str.Format("%s%s%s", m_ServiceInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwServiceType & SERVICE_INTERACTIVE_PROCESS ? "Interactive " : "", m_ServiceInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwServiceType & SERVICE_WIN32_OWN_PROCESS ? "Own Process" : "", m_ServiceInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwServiceType & SERVICE_WIN32_SHARE_PROCESS ? "Share Process" : "");
				break;
			}
			break;
		case DRIVER_ITEM:
			switch (pDispInfo->item.iSubItem) {
			case 0:
				Str.Format("%lu", pDispInfo->item.lParam);
				break;
			case 1:
				Str = m_DriverInformation[pDispInfo->item.lParam].lpServiceName;
				break;
			case 2:
				Str = m_DriverInformation[pDispInfo->item.lParam].lpDisplayName;
				break;
			case 3:
				Str = ServiceStatus[(m_DriverInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwCurrentState > SERVICE_PAUSED) ? 0 : m_DriverInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwCurrentState];
				break;
			case 4:
				Str.Format("%s%s%s%s", m_DriverInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwServiceType & SERVICE_KERNEL_DRIVER ? "Kernel " : "", m_DriverInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwServiceType & SERVICE_FILE_SYSTEM_DRIVER ? "File System" : "", m_DriverInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwServiceType & SERVICE_ADAPTER ? "Adapter" : "", m_DriverInformation[pDispInfo->item.lParam].ServiceStatusProcess.dwServiceType & SERVICE_RECOGNIZER_DRIVER ? "Recognizer" : "");
				break;
			}
			break;
		case ROOT_ITEM:
			switch (pDispInfo->item.iSubItem) {
			case 0:
				Str.Format("%lu", ((DirectoryEntry*)pDispInfo->item.lParam)->Index);
				break;
			case 1:
				Str.Format("%S", ((DirectoryEntry*)pDispInfo->item.lParam)->Name);
				break;
			case 2:
				Str.Format("%S", ((DirectoryEntry*)pDispInfo->item.lParam)->Type);
				break;
			}
			break;
		case OBJECTTYPES_ITEM:
			switch (pDispInfo->item.iSubItem) {
			case 0:
				Str = W2A(((HandleEntry*)pDispInfo->item.lParam)->Name);
				break;
			case 1:
				Str.Format("%04X", (WORD)((HandleEntry*)pDispInfo->item.lParam)->HandleInfo.uIdProcess);
				break;
			case 2:
				Str.Format("%04X", ((HandleEntry*)pDispInfo->item.lParam)->HandleInfo.Handle);
				break;
			case 3:
				Str.Format("%04X", ((HandleEntry*)pDispInfo->item.lParam)->SystemIndex);
				break;
			}
			break;
		}
		strcpy(pDispInfo->item.pszText, Str);
	}
	if (pDispInfo->item.mask & LVIF_IMAGE) {
		Type = NULL;
		switch (GetItemType(GetMainTree()->GetSelectedItem())) {
		case PROCESS_ITEM:
			break;
		case SERVICE_ITEM:
			break;
		case DRIVER_ITEM:
			break;
		case ROOT_ITEM:
			Type = ((DirectoryEntry*)pDispInfo->item.lParam)->Type;
			break;
		case OBJECTTYPES_ITEM:
			Type = m_KernelObjectNames[((HandleEntry*)pDispInfo->item.lParam)->HandleInfo.ObjectTypeIndex - 1];
			break;
		}
		if (Type == NULL) {
			pDispInfo->item.iImage = 12;
		} else if (_wcsicmp(Type, L"Device") == 0) {
			pDispInfo->item.iImage = 0;
		} else if (_wcsicmp(Type, L"Directory") == 0) {
			pDispInfo->item.iImage = 1;
		} else if (_wcsicmp(Type, L"Driver") == 0) {
			pDispInfo->item.iImage = 2;
		} else if (_wcsicmp(Type, L"Event") == 0) {
			pDispInfo->item.iImage = 3;
		} else if (_wcsicmp(Type, L"Key") == 0) {
			pDispInfo->item.iImage = 4;
		} else if (_wcsicmp(Type, L"Mutant") == 0) {
			pDispInfo->item.iImage = 5;
		} else if (_wcsicmp(Type, L"Port") == 0) {
			pDispInfo->item.iImage = 6;
		} else if (_wcsicmp(Type, L"Profile") == 0) {
			pDispInfo->item.iImage = 7;
		} else if (_wcsicmp(Type, L"Section") == 0) {
			pDispInfo->item.iImage = 8;
		} else if (_wcsicmp(Type, L"Semaphore") == 0) {
			pDispInfo->item.iImage = 9;
		} else if (_wcsicmp(Type, L"SymbolicLink") == 0) {
			pDispInfo->item.iImage = 10;
		} else if (_wcsicmp(Type, L"Timer") == 0) {
			pDispInfo->item.iImage = 11;
		} else if (_wcsicmp(Type, L"WindowStation") == 0) {
			pDispInfo->item.iImage = 13;
		} else {
			pDispInfo->item.iImage = 12;
		}
	}
	*pResult = 0;
}

void CPestRidSplitterWnd::OnTvnGetdispinfoMaintree(NMHDR *pNMHDR, LRESULT *pResult)
{
	USES_CONVERSION;
	NMTVDISPINFO *pDispInfo = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
	strcpy(pDispInfo->item.pszText, W2A(((DirectoryTreeEntry*)pDispInfo->item.lParam)->Name));
	*pResult = 0;
}

void CPestRidSplitterWnd::OnNcDestroy()
{
	AfxGetApp()->m_pMainWnd = NULL;
	CSplitterWnd::OnNcDestroy();
}

BOOL CPestRidSplitterWnd::CreateView(int row, int col, CRuntimeClass* pViewClass, SIZE sizeInit, CCreateContext* pContext)
{
	USES_CONVERSION;
	HTREEITEM hItem;
	HTREEITEM hProcItem;
	DWORD Item;
	CRect rect;
	BOOL bRet = TRUE;
	CEdit* NewEdit;
	if (pViewClass)
		bRet = CSplitterWnd::CreateView(row, col, pViewClass, sizeInit, pContext);
	if ((row == 0) && (col == 0)) {
		m_MainTree = &((CTreeView*)GetPane(0, 0))->GetTreeCtrl();
		hItem = GetMainTree()->InsertItem("\\", GetMainTree()->GetRootItem());
		GetMainTree()->ModifyStyle(0, TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_TABSTOP | WS_CHILD | WS_VISIBLE);
		GetMainTree()->SetItemImage(hItem, 0, 1);
		GetMainTree()->SetItemData(hItem, ROOT_ITEM);
		GetMainTree()->SetItemData(hProcItem = GetMainTree()->InsertItem("Processes"), PROCESS_ITEM);
		GetMainTree()->SetItemImage(hProcItem, 0, 1);
		GetMainTree()->SetItemData(GetMainTree()->InsertItem("Services"), SERVICE_ITEM);
		GetMainTree()->SetItemData(GetMainTree()->InsertItem("Drivers"), DRIVER_ITEM);
		GetMainTree()->SetImageList(&m_TreeImageList, TVSIL_NORMAL);
	} else if ((row == 0) && (col == 1)) {
		m_MainTab = &((CTabView*)((CSplitterWndTopRight*)((CSplitterWndRight*)GetPane(0, 1))->GetPane(0, 0))->GetPane(0, 0))->GetTabCtrl();
		m_BottomTab = &((CTabView*)((CSplitterWndRight*)GetPane(0, 1))->GetPane(1, 0))->GetTabCtrl();
		m_ProcList = new CCustomListCtrl();
		m_ProcTreeList = new CCustomTreeListCtrl();
		GetMainTab()->GetClientRect(rect);
		m_ProcTreeList->Create(LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_ALIGNLEFT | WS_TABSTOP | WS_CHILD | WS_VISIBLE, rect, GetMainTab(), TREELISTID);
		m_ProcTreeList->SetExtendedStyle(m_ProcTreeList->GetExtendedStyle() | LVS_EX_FULLROWSELECT);
		m_ProcTreeList->SetImageList(&m_ProcImageList, LVSIL_SMALL);
		m_ProcTreeList->SetImageList(NULL, LVSIL_SMALL);
		((CTabView*)m_MainTab)->CreateChildWindow(m_ProcTreeList);
		m_TraceLogEdit = NewEdit = new CEdit();
		GetBottomTab()->GetClientRect(rect);
		NewEdit->Create(ES_LEFT | ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_HSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rect, GetBottomTab(), TRACEEDITID);
		NewEdit->SetFont(GetFont());
		((CTabView*)GetBottomTab())->CreateChildWindow(NewEdit);
		QueryDirectoryTree(NULL, L"\\", GetMainTree()->GetRootItem(), m_DirectoryTreeRoot);
		if (hItem = GetMainTree()->GetSelectedItem()) {
			Item = GetItemType(hItem);
			if (Item == PROCESS_ITEM) {
				UpdateProcessList();
			} else if (Item == SERVICE_ITEM) {
				UpdateServiceInformation();
			} else if (Item == DRIVER_ITEM) {
				UpdateDriverInformation();
			} else if (Item == ROOT_ITEM) {
				QueryDirectory(A2W(GetTreeDirectoryPath(hItem)));
			} else {
				UpdateHandleDatabaseList();
			}
		}
		GetMainTree()->SelectItem(hItem = GetMainTree()->GetRootItem());
		GetMainTree()->Expand(hItem, TVE_EXPAND);
		GetMainTab()->SetFont(GetMainTree()->GetFont());
		m_ProcTreeList->SetFont(GetMainTree()->GetFont());
		GetBottomTab()->SetFont(GetMainTree()->GetFont());
		NewEdit->SetFont(GetMainTree()->GetFont());
		m_hTermination = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!(m_hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, &CPestRidSplitterWnd::ProcessHandleDatabaseUpdateThread, this, 0, NULL)) || (m_hWorkerThread == INVALID_HANDLE_VALUE)) {
			CloseHandle(m_hTermination);
			m_hTermination = NULL;
		}
	}
	return bRet;
}

BOOL CPestRidSplitterWnd::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	HTREEITEM hItem;
	BOOL bEntry = FALSE;
	if (((NMHDR*)lParam)->hwndFrom == GetMainTab()->GetSafeHwnd()) {
		switch (((NMHDR*)lParam)->code) {
		case TCN_SELCHANGE:
			bEntry = TRUE;
			EnterCriticalSection(&m_ProtectDatabase);
			OnTcnSelchangeMaintab((NMHDR*)lParam, pResult);
			break;
		}
	} else if (((NMHDR*)lParam)->hwndFrom == GetMainTree()->GetSafeHwnd()) {
		switch (((NMHDR*)lParam)->code) {
		case TVN_GETDISPINFO:
			if (!m_bUpdating) {
				bEntry = TRUE;
				EnterCriticalSection(&m_ProtectDatabase);
			}
			OnTvnGetdispinfoMaintree((NMHDR*)lParam, pResult);
			break;
		case TVN_SELCHANGED:
			if (!m_bUpdating) {
				bEntry = TRUE;
				EnterCriticalSection(&m_ProtectDatabase);
			}
			OnTvnSelchangedMaintree((NMHDR*)lParam, pResult);
			break;
		}
	} else if (((NMHDR*)lParam)->hwndFrom == GetProcList()->GetSafeHwnd()) {
		switch (((NMHDR*)lParam)->code) {
		case NM_RCLICK:
			bEntry = TRUE;
			EnterCriticalSection(&m_ProtectDatabase);
			OnLvnRClickProcList((NMHDR*)lParam, pResult);
			break;
		case LVN_GETDISPINFO:
			if (!m_bUpdating) {
				bEntry = TRUE;
				EnterCriticalSection(&m_ProtectDatabase);
			}
			OnLvnGetdispinfoProclist((NMHDR*)lParam, pResult);
			break;
		case LVN_COLUMNCLICK:
			bEntry = TRUE;
			EnterCriticalSection(&m_ProtectDatabase);
			OnLvnColumnclick((NMHDR*)lParam, pResult);
			break;
		case LVN_ITEMCHANGED:
			if (!m_bUpdating) {
				bEntry = TRUE;
				EnterCriticalSection(&m_ProtectDatabase);
			}
			if (hItem = GetMainTree()->GetSelectedItem()) {
				if (GetItemType(hItem) == PROCESS_ITEM) {
					if (((LPNMLISTVIEW)lParam)->uOldState != ((LPNMLISTVIEW)lParam)->uNewState) {
						m_ProcTreeList->SetItemState(((LPNMLISTVIEW)lParam)->iItem, ((LPNMLISTVIEW)lParam)->uNewState, -1);
					}
				}
			}
			break;
		}
	} else if (((NMHDR*)lParam)->hwndFrom == m_ProcTreeList->GetSafeHwnd()) {
		if (hItem = GetMainTree()->GetSelectedItem()) {
			if (GetItemType(hItem) == PROCESS_ITEM) {
				switch (((NMHDR*)lParam)->code) {
				case NM_CLICK:
					bEntry = TRUE;
					EnterCriticalSection(&m_ProtectDatabase);
					OnLvnClickProcList((NMHDR*)lParam, pResult);
					break;
				case NM_RCLICK:
					bEntry = TRUE;
					EnterCriticalSection(&m_ProtectDatabase);
					OnLvnRClickProcList((NMHDR*)lParam, pResult);
					break;
				case NM_CUSTOMDRAW:
					EnterCriticalSection(&m_ProtectDatabase);
					// must be passed up through parent windows
					OnLvnCustomdrawProcList((NMHDR*)lParam, pResult);
					LeaveCriticalSection(&m_ProtectDatabase);
					return TRUE;
					break;
				case LVN_COLUMNCLICK:
					bEntry = TRUE;
					EnterCriticalSection(&m_ProtectDatabase);
					OnLvnColumnclick((NMHDR*)lParam, pResult);
					break;
				case LVN_ITEMCHANGED:
					if (!m_bUpdating) {
						bEntry = TRUE;
						EnterCriticalSection(&m_ProtectDatabase);
					}
					if (((LPNMLISTVIEW)lParam)->uOldState != ((LPNMLISTVIEW)lParam)->uNewState) {
						GetProcList()->SetItemState(((LPNMLISTVIEW)lParam)->iItem, ((LPNMLISTVIEW)lParam)->uNewState, -1);
					}
					break;
				}
			}
		}
	}
	if (bEntry)
		LeaveCriticalSection(&m_ProtectDatabase);
	return CSplitterWnd::OnNotify(wParam, lParam, pResult);
}
