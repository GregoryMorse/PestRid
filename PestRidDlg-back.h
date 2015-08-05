// Gregory Morse

/*

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




*/

// PestRidDlg.h : header file
//

// SymLink info,
// tabs for mutant information
// more handle columns we have more information!!!
// windows security information for objects

#pragma once
#include "afxwin.h"
#include <psapi.h>
#include <tlhelp32.h>
#include <afxtempl.h>
#include "afxcmn.h"
#include <process.h>
#include <winternl.h>
#include <winsvc.h>

#include <stdio.h>
#include <stdarg.h>


#include "devioctl.h"

#include <pshpack4.h>		// allow use in driver and application build!
// 4 byte alignment required for all NT structures

#define MEM_EXECUTE_OPTION_DISABLE  0x1
#define MEM_EXECUTE_OPTION_ENABLE 0x2

#define OBJ_INHERIT              Ox00000002L 
#define OBJ_PERMANENT            0x00000010L 
#define OBJ_EXCLUSIVE            0x00000020L 
#define OBJ_CASE_INSENSITIVE     0x00000040L 
#define OBJ_OPENIF               0x00000080L 
#define OBJ_VALID_ATTRIBUTES     0x000000F2L 

#define OBJ_NAME_PATH_SEPARATOR ((WCHAR)L'\\')

#define DIRECTORY_QUERY               (0x0001) 
#define DIRECTORY_TRAVERSE            (0x0002) 
#define DIRECTORY_CREATE_OBJECT       (0x0004) 
#define DIRECTORY_CREATE_SUBDIRECTORY (0x0008) 
#define DIRECTORY_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED | 0xF) 

typedef enum _DIRECTORYINFOCLASS { 
    ObjectArray, 
    ObjectByOne 
} DIRECTORYINFOCLASS, *PDIRECTORYINFOCLASS; 

#define QUERY_DIRECTORY_BUF_SIZE 0x200 

typedef struct _OBJECT_NAMETYPE_INFO { 
	UNICODE_STRING   ObjectName; 
    UNICODE_STRING   ObjectType; 
} OBJECT_NAMETYPE_INFO, *POBJECT_NAMETYPE_INFO; 

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;
typedef DWORD KWAIT_REASON;
typedef LONG KPRIORITY;
typedef struct _VM_COUNTERS {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;

typedef struct _SYSTEM_THREAD
    {
/*000*/ FILETIME   ftKernelTime;   // 100 nsec units
/*008*/ FILETIME   ftUserTime;    // 100 nsec units
/*010*/ FILETIME   ftCreateTime;   // relative to 01-01-1601
/*018*/ DWORD    dWaitTime;
/*01C*/ PVOID    pStartAddress;
/*020*/ CLIENT_ID  Cid;        // process/thread ids
/*028*/ DWORD    dPriority;
/*02C*/ DWORD    dBasePriority;
/*030*/ DWORD    dContextSwitches;
/*034*/ DWORD    dThreadState;   // 2=running, 5=waiting
/*038*/ KWAIT_REASON WaitReason;
/*03C*/ DWORD    dReserved01;
/*040*/ }
    SYSTEM_THREAD, *PSYSTEM_THREAD;
#define SYSTEM_THREAD_ sizeof (SYSTEM_THREAD)
// -----------------------------------------------------------------
typedef struct _SYSTEM_PROCESS     // common members
    {
/*000*/ DWORD     dNext;      // relative offset
/*004*/ DWORD     dThreadCount;
/*008*/ DWORD     dReserved01;
/*00C*/ DWORD     dReserved02;
/*010*/ DWORD     dReserved03;
/*014*/ DWORD     dReserved04;
/*018*/ DWORD     dReserved05;
/*01C*/ DWORD     dReserved06;
/*020*/ FILETIME    ftCreateTime;  // relative to 01-01-1601
/*028*/ FILETIME    ftUserTime;   // 100 nsec units
/*030*/ FILETIME    ftKernelTime;  // 100 nsec units
/*038*/ UNICODE_STRING usName;
/*040*/ KPRIORITY   BasePriority;
/*044*/ DWORD     dUniqueProcessId;
/*048*/ DWORD     dInheritedFromUniqueProcessId;
/*04C*/ DWORD     dHandleCount;
/*050*/ DWORD     dReserved07;
/*054*/ DWORD     dReserved08;
/*058*/ VM_COUNTERS  VmCounters;   // see ntddk.h
/*084*/ DWORD     dCommitCharge;  // bytes
/*088*/ }
    SYSTEM_PROCESS, *PSYSTEM_PROCESS;
#define SYSTEM_PROCESS_ sizeof (SYSTEM_PROCESS)
// -----------------------------------------------------------------
typedef struct _SYSTEM_PROCESS_NT4   // Windows NT 4.0
    {
/*000*/ SYSTEM_PROCESS Process;     // common members
/*088*/ SYSTEM_THREAD aThreads [1];   // thread array
/*088*/ }
    SYSTEM_PROCESS_NT4, *PSYSTEM_PROCESS_NT4;
#define SYSTEM_PROCESS_NT4_ sizeof (SYSTEM_PROCESS_NT4)
// -----------------------------------------------------------------
typedef struct _SYSTEM_PROCESS_NT5   // Windows 2000
    {
/*000*/ SYSTEM_PROCESS Process;     // common members
/*088*/ IO_COUNTERS  IoCounters;   // see ntddk.h
/*0B8*/ SYSTEM_THREAD aThreads [1];   // thread array
/*0B8*/ }
    SYSTEM_PROCESS_NT5, *PSYSTEM_PROCESS_NT5;
#define SYSTEM_PROCESS_NT5_ sizeof (SYSTEM_PROCESS_NT5)
// -----------------------------------------------------------------
typedef union __SYSTEM_PROCESS_INFORMATION
    {
/*000*/ SYSTEM_PROCESS   Process;
/*000*/ SYSTEM_PROCESS_NT4 Process_NT4;
/*000*/ SYSTEM_PROCESS_NT5 Process_NT5;
/*0B8*/ }
    __SYSTEM_PROCESS_INFORMATION, *__PSYSTEM_PROCESS_INFORMATION;

typedef struct _RTL_DRIVE_LETTER_CURDIR { 
        USHORT Flags; 
        USHORT Length; 
        ULONG TimeStamp; 
        UNICODE_STRING DosPath; 
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR; 

typedef struct _RTL_USER_PROCESS_PARAMETERS { 
        ULONG MaximumLength; // 0x0 
        ULONG Length; 
        ULONG Flags; 
        ULONG DebugFlags; 
        PVOID ConsoleHandle; //0x10
        ULONG ConsoleFlags; 
        HANDLE StdInputHandle; 
        HANDLE StdOutputHandle; 
        HANDLE StdErrorHandle; //0x20
        UNICODE_STRING CurrentDirectoryPath; 
        HANDLE CurrentDirectoryHandle; 
        UNICODE_STRING DllPath; //0x30
        UNICODE_STRING ImagePathName; 
        UNICODE_STRING CommandLine; //0x40
        PVOID Environment;
        ULONG StartingPositionLeft; 
        ULONG StartingPositionTop; //0x50
        ULONG Width; 
        ULONG Height; 
        ULONG CharWidth; 
        ULONG CharHeight; //0x60
        ULONG ConsoleTextAttributes; 
        ULONG WindowFlags; 
        ULONG ShowWindowFlags; 
        UNICODE_STRING WindowTitle; //0x70
        UNICODE_STRING DesktopName; 
        UNICODE_STRING ShellInfo; //0x80
        UNICODE_STRING RuntimeData; 
        RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20]; //0x90
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS; 

/*typedef enum _PROCESSINFOCLASS { 
    ProcessBasicInformation = 0, 
    ProcessWow64Information = 26 
} PROCESSINFOCLASS; */

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO {
    USHORT uIdProcess;
    USHORT CreatorBackTraceIndex;
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT Handle;
    PVOID Object;
    ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG uCount;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _OBJECT_NAME_INFORMATION {
	UNICODE_STRING ObjectName;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType,
    // end_wdm
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,
    // begin_wdm
} POOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION {
	UNICODE_STRING Name;
	ULONG ObjectCount;
	ULONG HandleCount;
	ULONG Reserved1[4];
	ULONG PeakObjectCount;
	ULONG PeakHandleCount;
	ULONG Reserved2[4];
	ULONG InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccess;
	UCHAR Unknown;
	BOOLEAN MaintainHandleDatabase;
	POOL_TYPE PoolType;
	ULONG PagedPoolUsage;
	ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_ALL_TYPES_INFORMATION {
	ULONG NumberOfTypes;
	OBJECT_TYPE_INFORMATION TypeInformation;
} OBJECT_ALL_TYPES_INFORMATION, *POBJECT_ALL_TYPES_INFORMATION;

typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllTypesInformation,
    ObjectHandleInformation
} OBJECT_INFORMATION_CLASS;

#define STATUS_BUFFER_OVERFLOW ((NTSTATUS)0x80000005)
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef enum ___SYSTEM_INFORMATION_CLASS { 
    _SystemBasicInformation,                 // 0 
    SystemProcessorInformation,             // 1 
    _SystemPerformanceInformation,             // 2
    _SystemTimeOfDayInformation,             // 3
    SystemNotImplemented1,                     // 4
    SystemProcessesAndThreadsInformation,     // 5
    SystemCallCounts,                         // 6
    SystemConfigurationInformation,         // 7
    SystemProcessorTimes,                     // 8
    SystemGlobalFlag,                         // 9
    SystemNotImplemented2,                     // 10
    SystemModuleInformation,                 // 11
    SystemLockInformation,                     // 12
    SystemNotImplemented3,                     // 13
    SystemNotImplemented4,                     // 14
    SystemNotImplemented5,                     // 15
    SystemHandleInformation,                 // 16
    SystemObjectInformation,                 // 17
    SystemPagefileInformation,                 // 18
    SystemInstructionEmulationCounts,         // 19
    SystemInvalidInfoClass1,                 // 20
    SystemCacheInformation,                 // 21
    SystemPoolTagInformation,                 // 22
    SystemProcessorStatistics,                 // 23
    SystemDpcInformation,                     // 24
    SystemNotImplemented6,                     // 25
    SystemLoadImage,                         // 26
    SystemUnloadImage,                         // 27
    SystemTimeAdjustment,                     // 28
    SystemNotImplemented7,                     // 29
    SystemNotImplemented8,                     // 30
    SystemNotImplemented9,                     // 31
    SystemCrashDumpInformation,             // 32
    _SystemExceptionInformation,             // 33
    SystemCrashDumpStateInformation,         // 34
    SystemKernelDebuggerInformation,         // 35
    SystemContextSwitchInformation,         // 36
    _SystemRegistryQuotaInformation,         // 37
    SystemLoadAndCallImage,                 // 38
    SystemPrioritySeparation,                 // 39
    SystemNotImplemented10,                 // 40
    SystemNotImplemented11,                 // 41
    SystemInvalidInfoClass2,                 // 42
    SystemInvalidInfoClass3,                 // 43
    SystemTimeZoneInformation,                 // 44
    _SystemLookasideInformation,             // 45
    SystemSetTimeSlipEvent,                 // 46
    SystemCreateSession,                     // 47
    SystemDeleteSession,                     // 48
    SystemInvalidInfoClass4,                 // 49
    SystemRangeStartInformation,             // 50
    SystemVerifierInformation,                 // 51
    SystemAddVerifier,                         // 52
    SystemSessionProcessesInformation         // 53
} __SYSTEM_INFORMATION_CLASS;

typedef enum ___FILE_INFORMATION_CLASS {
	FileModeInformation = 16,
    FilePipeInformation = 23,            // 23
    FilePipeLocalInformation = 24       // 24
} __FILE_INFORMATION_CLASS, *__PFILE_INFORMATION_CLASS;

typedef struct _FILE_PIPE_INFORMATION {
	ULONG ReadMode;
	ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
	ULONG NamedPipeType;
	ULONG NamedPipeConfiguration;
	ULONG MaximumInstances;
	ULONG CurrentInstances;
	ULONG InboundQuota;
	ULONG ReadDataAvailable;
	ULONG OutboundQuota;
	ULONG WriteQuotaAvailable;
	ULONG NamedPipeState;
	ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_MODE_INFORMATION {
 ULONG Mode; 
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION; 

#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH	((NTSTATUS)0xC0000004L)
#endif
#define STATUS_NO_MORE_ENTRIES           ((NTSTATUS)0x8000001AL)

#define QUERY_STATE 1 // query state for timer, mutex, etc

typedef VOID (NTAPI *__RtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
typedef VOID (NTAPI *__RtlFreeUnicodeString)(PUNICODE_STRING);

typedef NTSTATUS (NTAPI *__NtOpenDirectoryObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
typedef NTSTATUS (NTAPI *__NtQueryDirectoryObject)(HANDLE, PVOID, ULONG, DIRECTORYINFOCLASS, BOOLEAN, PULONG, PULONG);
typedef NTSTATUS (NTAPI *__NtQueryObject)(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS (NTAPI *__NtQuerySystemInformation)(ULONG, PVOID, ULONG, PULONG);
typedef NTSTATUS (NTAPI *__NtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
typedef NTSTATUS (NTAPI *__NtResumeProcess)(HANDLE);
typedef NTSTATUS (NTAPI *__NtSuspendProcess)(HANDLE);
typedef NTSTATUS (NTAPI *__NtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

typedef DWORD (WINAPI *__GetModuleFileNameEx)(HANDLE, HMODULE, LPTSTR, DWORD);
typedef BOOL (WINAPI *__EnumProcesses)(DWORD*, DWORD, DWORD*);
typedef DWORD (WINAPI *__GetProcessImageFileName)(HANDLE, LPTSTR, DWORD);

typedef BOOL (WINAPI *__IsHungAppWindow)(HWND);

static __NtQueryObject _NtQueryObject;
static __NtQuerySystemInformation _NtQuerySystemInformation;
static __RtlInitUnicodeString _RtlInitUnicodeString;
static __NtOpenDirectoryObject _NtOpenDirectoryObject;
static __NtQueryDirectoryObject _NtQueryDirectoryObject;
static __NtQueryInformationFile _NtQueryInformationFile;
static __GetModuleFileNameEx _GetModuleFileNameEx;
static __EnumProcesses _EnumProcesses;
static __GetProcessImageFileName _GetProcessImageFileName;
static __NtResumeProcess _NtResumeProcess;
static __NtSuspendProcess _NtSuspendProcess;
static __NtQueryInformationProcess _NtQueryInformationProcess;
static __IsHungAppWindow _IsHungAppWindow;


// Device type           -- in the "User Defined" range."
#define GPD_TYPE    40000

// Dos-Device name
#define	GPD_DOSDEVICE	"\\\\.\\PestRidDrv"
#define GPD_DOSDEVICEGLOBAL "\\\\.\\Global\\PestRidDrv"

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.

#define IOCTL_FOI_GETOBJECTNAME CTL_CODE( GPD_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_CLOSEHANDLE CTL_CODE( GPD_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_COMPAREVERSION CTL_CODE( GPD_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_OPENPROCESSTOKEN CTL_CODE( GPD_TYPE, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_DUPLICATEOBJECT CTL_CODE( GPD_TYPE, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETFILEDEVICEOBJECT CTL_CODE( GPD_TYPE, 0x908, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READKSTACK CTL_CODE( GPD_TYPE, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETKCONTEXT CTL_CODE( GPD_TYPE, 0x90A, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETMUTANTOWNER CTL_CODE( GPD_TYPE, 0x90B, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_COPYMEMORY CTL_CODE( GPD_TYPE, 0x90C, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_QUERYDEP CTL_CODE( GPD_TYPE, 0x90D, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETFILESHAREACCESS CTL_CODE( GPD_TYPE, 0x90E, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_OPENPROCESS CTL_CODE( GPD_TYPE, 0x90F, METHOD_BUFFERED, FILE_ANY_ACCESS )

#include <poppack.h>

#define ROOT_ITEM 0x80000000
#define OBJECTTYPES_ITEM 0x80000001
#define PROCESS_ITEM 0x80000002
#define SERVICE_ITEM 0x80000003
#define DRIVER_ITEM 0x80000004

#define WM_APP_SYNCSCROLL WM_APP + 0
#define WM_APP_SIZETREELIST WM_APP + 1
#define WM_APP_MOUSEWHEEL WM_APP + 2
#define ID_QUERYHANDLE 10
#define TREELISTID 1000
#define LISTID 1001
#define TRACEEDITID 1002
#define MAINTABID 1003

#include "afxpriv.h"

class CCustomListCtrl : public CListCtrl
{
public:
	CCustomListCtrl() : CListCtrl() {}
protected:
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
	{
		NMITEMACTIVATE nmiact;
		if ((wParam == 0) && (((NMHDR*)lParam)->code == NM_RCLICK)) {
			// handle header control right clicks
			nmiact.hdr.code = ((NMHDR*)lParam)->code;
			nmiact.hdr.hwndFrom = GetSafeHwnd();
			nmiact.hdr.idFrom = LISTID;
			nmiact.iItem = nmiact.iSubItem = -1;
			GetParent()->SendMessage(WM_NOTIFY, nmiact.hdr.idFrom, (LPARAM)&nmiact);
		}
		return CListCtrl::OnNotify(wParam, lParam, pResult);
	}
	void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
	{
		CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
		GetParent()->GetParent()->GetParent()->GetParent()->SendMessage(WM_APP_SYNCSCROLL, 0, (LPARAM)GetSafeHwnd());
	}
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
	{
		BOOL bRet = CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
		GetParent()->GetParent()->GetParent()->GetParent()->SendMessage(WM_APP_SYNCSCROLL, 0, (LPARAM)GetSafeHwnd());
		return bRet;
	}
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
		GetParent()->GetParent()->GetParent()->GetParent()->SendMessage(WM_APP_SYNCSCROLL, 0, (LPARAM)GetSafeHwnd());
	}
	void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		CListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
		GetParent()->GetParent()->GetParent()->GetParent()->SendMessage(WM_APP_SYNCSCROLL, 0, (LPARAM)GetSafeHwnd());
	}
	afx_msg void OnSize(UINT nType, int cx, int cy)
	{
		GetParent()->GetParent()->GetParent()->GetParent()->SendMessage(WM_APP_SIZETREELIST);
		CListCtrl::OnSize(nType, cx, cy);
	}
	DECLARE_MESSAGE_MAP()
};

class CCustomTreeListCtrl : public CListCtrl
{
protected:
	afx_msg void OnSize(UINT nType, int cx, int cy)
	{
		CRect Rect;
		GetClientRect(Rect);
		SetColumnWidth(0, Rect.Width());
		CListCtrl::OnSize(nType, cx, cy);
	}
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS *lpncsp)
	{
		ModifyStyle(WS_HSCROLL | WS_VSCROLL, 0, 0);//SWP_FRAMECHANGED
		CListCtrl::OnNcCalcSize(bCalcValidRects, lpncsp);
	}
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
	{
		BOOL bRet = CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
		GetParent()->GetParent()->GetParent()->GetParent()->SendMessage(WM_APP_MOUSEWHEEL, nFlags | (zDelta << 16), pt.x | (pt.y << 16));
		return bRet;
	}
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
		GetParent()->GetParent()->GetParent()->GetParent()->SendMessage(WM_APP_SYNCSCROLL, 0, (LPARAM)GetSafeHwnd());
	}
	void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		CListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
		GetParent()->GetParent()->GetParent()->GetParent()->SendMessage(WM_APP_SYNCSCROLL, 0, (LPARAM)GetSafeHwnd());
	}
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult)
	{
		NMITEMACTIVATE nmiact;
		if ((((NMHDR*)lParam)->code == HDN_BEGINTRACKA) || (((NMHDR*)lParam)->code == HDN_BEGINTRACKW)) {
			*pResult = TRUE;
			return TRUE;
		} else if ((wParam == 0) && (((NMHDR*)lParam)->code == NM_RCLICK)) {
			nmiact.hdr.code = ((NMHDR*)lParam)->code;
			nmiact.hdr.hwndFrom = GetSafeHwnd();
			nmiact.hdr.idFrom = TREELISTID;
			nmiact.iItem = nmiact.iSubItem = -1;
			GetParent()->SendMessage(WM_NOTIFY, nmiact.hdr.idFrom, (LPARAM)&nmiact);
		}
		return CListCtrl::OnNotify(wParam, lParam, pResult);
	}
	DECLARE_DYNCREATE(CCustomTreeListCtrl)
	DECLARE_MESSAGE_MAP()
};

class CTabView : public CCtrlView
{
public:
	CTabView() : CCtrlView(WC_TABCONTROL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_MULTILINE) { m_ChildWnd = NULL; }
	virtual ~CTabView()
	{
		if (m_ChildWnd) {
			m_ChildWnd->DestroyWindow();
			delete m_ChildWnd;
		}
	}
	void DoSize()
	{
		DWORD Counter;
		CRect rect;
		CRect testrect;
		if (m_ChildWnd) {
			GetClientRect(rect);
			for (Counter = 0; Counter < (DWORD)GetTabCtrl().GetItemCount(); Counter++) {
				GetTabCtrl().GetItemRect(Counter, testrect);
				if (testrect.bottom > rect.top) {
					rect.top = testrect.bottom;
				}
			}
			m_ChildWnd->SetWindowPos(&wndTop, rect.left, rect.top, rect.Width(), rect.Height(), 0);
		}
	}
	void CreateChildWindow(CWnd* ChildWnd) { m_ChildWnd = ChildWnd; }
	CWnd* GetChildWindow() { return m_ChildWnd; }
	CTabCtrl & GetTabCtrl() const { return *((CTabCtrl*)this); }
protected:
	afx_msg void OnSize(UINT nType, int cx, int cy)
	{
		DoSize();
		CCtrlView::OnSize(nType, cx, cy);
	}
	afx_msg void OnSelChange( NMHDR * pNotifyStruct, LRESULT* result )
	{
		DoSize();
	}
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult)
	{
		if ((((NMHDR*)lParam)->code == NM_CUSTOMDRAW) && (((NMHDR*)lParam)->hwndFrom == GetChildWindow()->GetSafeHwnd())) {
			*pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
			return TRUE;
		} else {
			return CCtrlView::OnNotify(wParam, lParam, pResult);
		}
	}
	DECLARE_DYNCREATE(CTabView)
	DECLARE_MESSAGE_MAP()
	CWnd* m_ChildWnd;
};

class CSplitterWndTopRight : public CSplitterWnd
{
public:
	CSplitterWndTopRight() {}
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
	{
		if ((((NMHDR*)lParam)->hwndFrom == ((CTabView*)GetPane(0, 0))->GetTabCtrl().GetSafeHwnd()) || (((NMHDR*)lParam)->hwndFrom == ((CTabView*)GetPane(0, 0))->GetChildWindow()->GetSafeHwnd()) || (((NMHDR*)lParam)->hwndFrom == ((CTabView*)GetPane(0, 1))->GetChildWindow()->GetSafeHwnd()) || (((NMHDR*)lParam)->hwndFrom == ((CTabView*)GetPane(0, 1))->GetTabCtrl().GetSafeHwnd())) {
			*pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
			return TRUE;
		} else {
			return CSplitterWnd::OnNotify(wParam, lParam, pResult);
		}
	}

private:
	DECLARE_DYNCREATE(CSplitterWndTopRight)
};

class CSplitterWndRight : public CSplitterWnd
{
public:
	CSplitterWndRight() {}
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
	{
		if ((((NMHDR*)lParam)->hwndFrom == ((CTabView*)((CSplitterWndTopRight*)GetPane(0, 0))->GetPane(0, 0))->GetTabCtrl().GetSafeHwnd()) || (((NMHDR*)lParam)->hwndFrom == ((CTabView*)((CSplitterWndTopRight*)GetPane(0, 0))->GetPane(0, 0))->GetChildWindow()->GetSafeHwnd()) || (((NMHDR*)lParam)->hwndFrom == ((CTabView*)((CSplitterWndTopRight*)GetPane(0, 0))->GetPane(0, 1))->GetChildWindow()->GetSafeHwnd()) || (((NMHDR*)lParam)->hwndFrom == ((CTabView*)((CSplitterWndTopRight*)GetPane(0, 0))->GetPane(0, 1))->GetTabCtrl().GetSafeHwnd()) || (((NMHDR*)lParam)->hwndFrom == ((CSplitterWndTopRight*)GetPane(0, 0))->GetSafeHwnd())) {
			*pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
			return TRUE;
		} else {
			return CSplitterWnd::OnNotify(wParam, lParam, pResult);
		}
	}

private:
	DECLARE_DYNCREATE(CSplitterWndRight)
};

// CPestRidSplitterWnd dialog

//window needs splitter, sizable
class CPestRidSplitterWnd : public CSplitterWnd
{
// Construction
public:
	CPestRidSplitterWnd(CWnd* pParent = NULL);	// standard constructor
	virtual ~CPestRidSplitterWnd();

	CImageList m_TreeImageList;
	CImageList m_ProcImageList;
	CImageList m_DummyImageList;
	HICON m_hDummyIcon;
	int GetCurrentSortItem() { return m_CurrentSortItem; }
	BOOL GetSortAscending() { return m_SortAscending; }
	CEdit* m_TraceLogEdit;
	CTreeCtrl* m_MainTree;
	CCustomListCtrl*  m_ProcList;
	CTabCtrl* m_MainTab;
	CTabCtrl* m_BottomTab;
	CListCtrl* m_ProcTreeList;
	CEdit* GetTraceLogEdit() { return m_TraceLogEdit; }
	CTreeCtrl* GetMainTree() { return m_MainTree; }
	CListCtrl* GetProcList() { return  m_ProcList; }
	CTabCtrl* GetMainTab() { return m_MainTab; }
	CTabCtrl* GetBottomTab() { return m_BottomTab; }
	virtual BOOL CreateView(int row, int col, CRuntimeClass* pViewClass, SIZE sizeInit, CCreateContext* pContext);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTcnSelchangeMaintab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnSelchangedMaintree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetdispinfoProclist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnGetdispinfoMaintree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLbnDblclkProclist();
	afx_msg void OnDestroy();
	afx_msg void OnNcDestroy();
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
	{
		LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
		HDITEM HeaderItem;
		HeaderItem.mask = HDI_FORMAT | HDI_BITMAP;
		CHeaderCtrl* HeaderCtrl = ((pNMHDR->hwndFrom == GetProcList()->GetSafeHwnd()) ? GetProcList() : m_ProcTreeList)->GetHeaderCtrl();
		HeaderCtrl->GetItem(pNMLV->iSubItem, &HeaderItem);
		if (HeaderItem.hbm != 0) {
			DeleteObject(HeaderItem.hbm);
			HeaderItem.hbm = 0;
		}
		HeaderItem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
		m_SortAscending = ((m_CurrentSortItem == -1) ? (pNMHDR->hwndFrom == GetProcList()->GetSafeHwnd()) : ((pNMHDR->hwndFrom != GetProcList()->GetSafeHwnd()) || (m_CurrentSortItem != pNMLV->iSubItem))) ? true : !m_SortAscending;
		HeaderItem.hbm = (HBITMAP)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_SortAscending ? IDB_UP : IDB_DOWN), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
		HeaderCtrl->SetItem(pNMLV->iSubItem, &HeaderItem);
		if (((m_CurrentSortItem == -1) ? (pNMHDR->hwndFrom == GetProcList()->GetSafeHwnd()) : ((pNMHDR->hwndFrom != GetProcList()->GetSafeHwnd()) || (m_CurrentSortItem != pNMLV->iSubItem)))) {
			HeaderCtrl = ((m_CurrentSortItem == -1) ? m_ProcTreeList : GetProcList())->GetHeaderCtrl();
			HeaderCtrl->GetItem((m_CurrentSortItem == -1) ? 0 : m_CurrentSortItem, &HeaderItem);
			HeaderItem.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
			if (HeaderItem.hbm != 0) {
				DeleteObject(HeaderItem.hbm);
				HeaderItem.hbm = 0;
			}
			HeaderCtrl->SetItem((m_CurrentSortItem == -1) ? 0 : m_CurrentSortItem, &HeaderItem);
		}
		m_CurrentSortItem = (pNMHDR->hwndFrom == GetProcList()->GetSafeHwnd()) ? pNMLV->iSubItem : -1;
		GetProcList()->SortItems(SortFunc, (DWORD_PTR)this);
		m_ProcTreeList->SortItems(SortFunc, (DWORD_PTR)this);
		*pResult = 0;
	}
	static int CALLBACK SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		BOOL bAscending = ((CPestRidSplitterWnd*)lParamSort)->GetSortAscending();
		ProcessEntry* Entry1;
		ProcessEntry* Entry2;
		WORD PID1;
		WORD PID2;
		DWORD Counter;
		CArray<WORD, WORD>* PIDArray1;
		CArray<WORD, WORD>* PIDArray2;
		switch (((CPestRidSplitterWnd*)lParamSort)->GetItemType(((CPestRidSplitterWnd*)lParamSort)->GetMainTree()->GetSelectedItem())) {
		case PROCESS_ITEM:
			if (((CPestRidSplitterWnd*)lParamSort)->GetMainTab()->GetCurSel() == 0) {
				PIDArray1 = new CArray<WORD, WORD>();
				PIDArray2 = new CArray<WORD, WORD>();
				PIDArray1->InsertAt(0, PID1 = (WORD)lParam1);
				while (PID1 && ((CPestRidSplitterWnd*)lParamSort)->m_ProcessEntries.Lookup((WORD)PID1, (void*&)Entry1)) {
					PIDArray1->InsertAt(0, PID1 = (WORD)Entry1->lppe.th32ParentProcessID);
				}
				if (PID1) {
					PIDArray1->SetAt(0, PID1 = 0);
				}
				PIDArray2->InsertAt(0, PID2 = (WORD)lParam2);
				while (PID2 && ((CPestRidSplitterWnd*)lParamSort)->m_ProcessEntries.Lookup((WORD)PID2, (void*&)Entry2)) {
					PIDArray2->InsertAt(0, PID2 = (WORD)Entry2->lppe.th32ParentProcessID);
				}
				if (PID2) {
					PIDArray2->SetAt(0, PID2 = 0);
				}
				for (Counter = 0; Counter < (DWORD)(min(PIDArray1->GetCount(), PIDArray2->GetCount())); Counter++) {
					if (PIDArray1->GetAt(Counter) != PIDArray2->GetAt(Counter)) {
						break;
					}
				}
				if (Counter == (DWORD)(min(PIDArray1->GetCount(), PIDArray2->GetCount()))) {
					Counter = (PIDArray1->GetCount() == PIDArray2->GetCount()) ? 0 : ((PIDArray1->GetCount() > PIDArray2->GetCount()) ? 1 : -1);
					delete PIDArray1;
					delete PIDArray2;
					return Counter;
				}
				PID1 = PIDArray1->GetAt(Counter);
				PID2 = PIDArray2->GetAt(Counter);
				delete PIDArray1;
				delete PIDArray2;
			} else {
				PID1 = (WORD)lParam1;
				PID2 = (WORD)lParam2;
			}
			// only 0 if PIDs are equal!!!
			// 0 - equal makes it impossible to maintain tree relationships so must handle the secondary ordering using PIDs myself
			if (((CPestRidSplitterWnd*)lParamSort)->m_ProcessEntries.Lookup((WORD)PID1, (void*&)Entry1)) {
				if (((CPestRidSplitterWnd*)lParamSort)->m_ProcessEntries.Lookup((WORD)PID2, (void*&)Entry2)) {
					return (((CPestRidSplitterWnd*)lParamSort)->GetCurrentSortItem() == -1) ? ProcessNameColumn::_CompareFunction(Entry1, Entry2, bAscending) : ((CPestRidSplitterWnd*)lParamSort)->m_ProcessTabs[((CPestRidSplitterWnd*)lParamSort)->GetMainTab()->GetCurSel()]->GetColumn(((CPestRidSplitterWnd*)lParamSort)->GetCurrentSortItem())->CompareFunction(Entry1, Entry2, bAscending);
				} else {
					return (bAscending ? 1 : -1);
				}
			} else {
				if (((CPestRidSplitterWnd*)lParamSort)->m_ProcessEntries.Lookup((WORD)PID2, (void*&)Entry2)) {
					return (bAscending ? -1 : 1);
				} else {
					return (PID1 == PID2) ? 0 : (((PID1 > PID2) == bAscending) ? 1 : -1);
				}
			}
			break;
		case SERVICE_ITEM:
			switch (((CPestRidSplitterWnd*)lParamSort)->GetCurrentSortItem()) {
			case 0:
				return (lParam1 == lParam2) ? 0 : (((lParam1 > lParam2) == bAscending) ? 1 : -1);
				break;
			case 1:
				return _stricmp(((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[bAscending ? lParam1 : lParam2].lpServiceName, ((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[bAscending ? lParam2 : lParam1].lpServiceName);
				break;
			case 2:
				return _stricmp(((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[bAscending ? lParam1 : lParam2].lpDisplayName, ((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[bAscending ? lParam2 : lParam1].lpDisplayName);
				break;
			case 3:
				return (((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[lParam1].ServiceStatusProcess.dwCurrentState == ((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[lParam2].ServiceStatusProcess.dwCurrentState) ? 0 : (((((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[lParam1].ServiceStatusProcess.dwCurrentState > ((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[lParam2].ServiceStatusProcess.dwCurrentState) == bAscending) ? 1 : -1);
				break;
			case 4:
				return (((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[lParam1].ServiceStatusProcess.dwServiceType == ((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[lParam2].ServiceStatusProcess.dwServiceType) ? 0 : (((((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[lParam1].ServiceStatusProcess.dwServiceType > ((CPestRidSplitterWnd*)lParamSort)->m_ServiceInformation[lParam2].ServiceStatusProcess.dwServiceType) == bAscending) ? 1 : -1);
				break;
			}
			break;
		case DRIVER_ITEM:
			switch (((CPestRidSplitterWnd*)lParamSort)->GetCurrentSortItem()) {
			case 0:
				return (lParam1 == lParam2) ? 0 : (((lParam1 > lParam2) == bAscending) ? 1 : -1);
				break;
			case 1:
				return _stricmp(((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[bAscending ? lParam1 : lParam2].lpServiceName, ((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[bAscending ? lParam2 : lParam1].lpServiceName);
				break;
			case 2:
				return _stricmp(((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[bAscending ? lParam1 : lParam2].lpDisplayName, ((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[bAscending ? lParam2 : lParam1].lpDisplayName);
				break;
			case 3:
				return (((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[lParam1].ServiceStatusProcess.dwCurrentState == ((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[lParam2].ServiceStatusProcess.dwCurrentState) ? 0 : (((((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[lParam1].ServiceStatusProcess.dwCurrentState > ((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[lParam2].ServiceStatusProcess.dwCurrentState) == bAscending) ? 1 : -1);
				break;
			case 4:
				return (((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[lParam1].ServiceStatusProcess.dwServiceType == ((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[lParam2].ServiceStatusProcess.dwServiceType) ? 0 : (((((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[lParam1].ServiceStatusProcess.dwServiceType > ((CPestRidSplitterWnd*)lParamSort)->m_DriverInformation[lParam2].ServiceStatusProcess.dwServiceType) == bAscending) ? 1 : -1);
				break;
			}
			break;
		case ROOT_ITEM:
			switch (((CPestRidSplitterWnd*)lParamSort)->GetCurrentSortItem()) {
			case 0:
				return (((DirectoryEntry*)lParam1)->Index == ((DirectoryEntry*)lParam2)->Index) ? 0 : (((((DirectoryEntry*)lParam1)->Index > ((DirectoryEntry*)lParam2)->Index) == bAscending) ? 1 : -1);
				break;
			case 1:
				return _wcsicmp(((DirectoryEntry*)(bAscending ? lParam1 : lParam2))->Name, ((DirectoryEntry*)(bAscending ? lParam2 : lParam1))->Name);
				break;
			case 2:
				return _wcsicmp(((DirectoryEntry*)(bAscending ? lParam1 : lParam2))->Type, ((DirectoryEntry*)(bAscending ? lParam2 : lParam1))->Type);
				break;
			}
			break;
		case OBJECTTYPES_ITEM:
			switch (((CPestRidSplitterWnd*)lParamSort)->GetCurrentSortItem()) {
			case 0:
				if (((HandleEntry*)lParam1)->Name && ((HandleEntry*)lParam2)->Name)
					return _wcsicmp(((HandleEntry*)(bAscending ? lParam1 : lParam2))->Name, ((HandleEntry*)(bAscending ? lParam2 : lParam1))->Name);
				else if (((HandleEntry*)lParam1)->Name)
					return (bAscending ? 1 : -1);
				else if (((HandleEntry*)lParam2)->Name)
					return (bAscending ? -1 : 1);
				else
					return 0;
				break;
			case 1:
				return (((HandleEntry*)lParam1)->HandleInfo.uIdProcess == ((HandleEntry*)lParam2)->HandleInfo.uIdProcess) ? 0 : (((((HandleEntry*)lParam1)->HandleInfo.uIdProcess > ((HandleEntry*)lParam2)->HandleInfo.uIdProcess) == bAscending) ? 1 : -1);
				break;
			case 2:
				return (((HandleEntry*)lParam1)->HandleInfo.Handle == ((HandleEntry*)lParam2)->HandleInfo.Handle) ? 0 : (((((HandleEntry*)lParam1)->HandleInfo.Handle > ((HandleEntry*)lParam2)->HandleInfo.Handle) == bAscending) ? 1 : -1);
				break;
			case 3:
				return (((HandleEntry*)lParam1)->SystemIndex == ((HandleEntry*)lParam2)->SystemIndex) ? 0 : (((((HandleEntry*)lParam1)->SystemIndex > ((HandleEntry*)lParam2)->SystemIndex) == bAscending) ? 1 : -1);
				break;
			}
			break;
		}
		return 0;
	}

protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	HICON m_hIcon;
	CArray<HANDLE, HANDLE> m_LockFileArray;
	CArray<CComBSTR, CComBSTR> m_KernelObjectNames;
	typedef struct DirectoryTreeEntry
	{
		DWORD Index;
		WCHAR* Name;
		CMapStringToPtr Children;
		BOOL bDirty;
	};
	typedef struct DirectoryEntry
	{
		DWORD Index;
		WCHAR* Name;
		WCHAR* Type;
		BOOL bDirty;
	};
	typedef struct HandleEntry
	{
		DWORD SystemIndex;
		SYSTEM_HANDLE_TABLE_ENTRY_INFO HandleInfo;
		WCHAR* Name;
		BOOL bDirty;
	};
	typedef struct ProcessEntry
	{
		DWORD ProcessThreadInformationOffset;
		PROCESSENTRY32 lppe;
		HICON hIcon;
		BOOL bExpanded;
		BOOL bDirty;
		BYTE DepStatus;
		BYTE WindowStatus;
		DWORD SessionId;
		CArray<WORD, WORD> ChildProcessIds;
		CString CommandLine;
		CString ImagePathName;
		CString FileDescription;
		CString CompanyName;
		CString WindowTitle;
		CString Version;
		CString UserName;
	};
	class ProcessListColumn
	{
	public:
		virtual LPCTSTR GetName() = 0;
		virtual int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending) = 0;
		virtual CString GetDispInfo(ProcessEntry* Entry) = 0;
	};
	class ProcessNameColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return _GetName(); }
		static LPCTSTR _GetName() { return "Name"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending) { return _CompareFunction(Entry1, Entry2, bAscending); }
		static int _CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			int Counter;
			return (Counter = _stricmp((bAscending ? Entry1 : Entry2)->lppe.szExeFile, (bAscending ? Entry2 : Entry1)->lppe.szExeFile)) == 0 ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : Counter;
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str;
			Str = Entry->lppe.szExeFile;
			return Str;
		}
	};
	class ProcessIdColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "ID"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str;
			Str.Format("%04X", Entry->lppe.th32ProcessID);
			return Str;
		}
	};
	class ProcessUserNameColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "User Name"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->UserName.CompareNoCase(Entry2->UserName) == 0) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (bAscending ? Entry1 : Entry2)->UserName.CompareNoCase((bAscending ? Entry2 : Entry1)->UserName);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str = Entry->UserName;
			return Str;
		}
	};
	class ProcessCommandLineColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "Command Line"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->CommandLine.CompareNoCase(Entry2->CommandLine) == 0) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (bAscending ? Entry1 : Entry2)->CommandLine.CompareNoCase((bAscending ? Entry2 : Entry1)->CommandLine);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str = Entry->CommandLine;
			return Str;
		}
	};
	class ProcessFileDescriptionColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "Description"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->FileDescription.CompareNoCase(Entry2->FileDescription) == 0) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (bAscending ? Entry1 : Entry2)->FileDescription.CompareNoCase((bAscending ? Entry2 : Entry1)->FileDescription);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str = Entry->FileDescription;
			return Str;
		}
	};
	class ProcessCompanyNameColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "Company Name"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->CompanyName.CompareNoCase(Entry2->CompanyName) == 0) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (bAscending ? Entry1 : Entry2)->CompanyName.CompareNoCase((bAscending ? Entry2 : Entry1)->CompanyName);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str = Entry->CompanyName;
			return Str;
		}
	};
	class ProcessVersionColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "Version"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->Version.CompareNoCase(Entry2->Version) == 0) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (bAscending ? Entry1 : Entry2)->Version.CompareNoCase((bAscending ? Entry2 : Entry1)->Version);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str = Entry->Version;
			return Str;
		}
	};
	class ProcessParentIdColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "Parent ID"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->lppe.th32ParentProcessID == Entry2->lppe.th32ParentProcessID) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (((Entry1->lppe.th32ParentProcessID > Entry2->lppe.th32ParentProcessID) == bAscending) ? 1 : -1);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str;
			Str.Format("%04X", Entry->lppe.th32ParentProcessID);
			return Str;
		}
	};
	class ProcessDepStatusColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "DEP Status"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->DepStatus == Entry2->DepStatus) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (((Entry1->DepStatus > Entry2->DepStatus) == bAscending) ? 1 : -1);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str;
			if (Entry->DepStatus == 0xFF) {
				Str = "<Unknown>";
			} else if (Entry->DepStatus & (MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_ENABLE)) {
				Str = "Off";
			} else {
				Str = "On";
			}
			return Str;
		}
	};
	class ProcessWindowTitleColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "Window Title"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->WindowTitle.CompareNoCase(Entry2->WindowTitle) == 0) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (bAscending ? Entry1 : Entry2)->WindowTitle.CompareNoCase((bAscending ? Entry2 : Entry1)->WindowTitle);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str = Entry->WindowTitle;
			return Str;
		}
	};
	class ProcessWindowStatusColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "Window Status"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->WindowStatus == Entry2->WindowStatus) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (((Entry1->WindowStatus > Entry2->WindowStatus) == bAscending) ? 1 : -1);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str;
			if (Entry->WindowStatus == 1) {
				Str = "Running";
			} else if (Entry->WindowStatus == 0) {
				Str = "Not Responding";
			}
			return Str;
		}
	};
	class ProcessSessionIdColumn : public ProcessListColumn
	{
	public:
		LPCTSTR GetName() { return "Session Id"; }
		int CompareFunction(ProcessEntry* Entry1, ProcessEntry* Entry2, BOOL bAscending)
		{
			return (Entry1->SessionId == Entry2->SessionId) ? ((Entry1->lppe.th32ProcessID == Entry2->lppe.th32ProcessID) ? 0 : (((Entry1->lppe.th32ProcessID > Entry2->lppe.th32ProcessID) == bAscending) ? 1 : -1)) : (((Entry1->SessionId > Entry2->SessionId) == bAscending) ? 1 : -1);
		}
		CString GetDispInfo(ProcessEntry* Entry)
		{
			CString Str;
			if (Entry->SessionId != -1)
				Str.Format("%lu", Entry->SessionId);
			return Str;
		}
	};
	class ProcessListTab
	{
	public:
		virtual ~ProcessListTab()
		{
			INT_PTR Counter;
			for (Counter = m_Columns.GetCount() - 1; Counter >= 0; Counter--) {
				delete m_Columns[Counter];
			}
		};
		virtual LPCTSTR GetName() = 0;
		ProcessListColumn* GetColumn(INT_PTR Index) { return m_Columns[Index]; }
		INT_PTR GetColumnCount() { return m_Columns.GetCount(); }
	protected:
		CArray<ProcessListColumn*, ProcessListColumn*> m_Columns;
	};
	class ProcessListTabList : public ProcessListTab
	{
	public:
		ProcessListTabList() { m_Columns.Add(new ProcessIdColumn()); m_Columns.Add(new ProcessParentIdColumn()); m_Columns.Add(new ProcessUserNameColumn()); m_Columns.Add(new ProcessCompanyNameColumn()); m_Columns.Add(new ProcessFileDescriptionColumn()); m_Columns.Add(new ProcessVersionColumn()); m_Columns.Add(new ProcessCommandLineColumn()); m_Columns.Add(new ProcessDepStatusColumn()); m_Columns.Add(new ProcessWindowTitleColumn()); m_Columns.Add(new ProcessWindowStatusColumn()); m_Columns.Add(new ProcessSessionIdColumn()); }
		LPCTSTR GetName() { return "List Mode"; }
	};
	class ProcessListTabTree : public ProcessListTab
	{
	public:
		ProcessListTabTree() { m_Columns.Add(new ProcessIdColumn()); m_Columns.Add(new ProcessParentIdColumn()); m_Columns.Add(new ProcessUserNameColumn()); m_Columns.Add(new ProcessCompanyNameColumn()); m_Columns.Add(new ProcessFileDescriptionColumn()); m_Columns.Add(new ProcessVersionColumn()); m_Columns.Add(new ProcessCommandLineColumn()); m_Columns.Add(new ProcessDepStatusColumn()); m_Columns.Add(new ProcessWindowTitleColumn()); m_Columns.Add(new ProcessWindowStatusColumn()); m_Columns.Add(new ProcessSessionIdColumn()); }
		LPCTSTR GetName() { return "Tree Mode"; }
	};
	CArray<ProcessListTab*, ProcessListTab*> m_ProcessTabs;
	CMapStringToPtr m_DirectoryEntries;
	DirectoryTreeEntry* m_DirectoryTreeRoot;
	ENUM_SERVICE_STATUS_PROCESS* m_ServiceInformation;
	DWORD m_ServiceInformationBufferSize;
	DWORD m_NumberOfServices;
	ENUM_SERVICE_STATUS_PROCESS* m_DriverInformation;
	DWORD m_DriverInformationBufferSize;
	DWORD m_NumberOfDrivers;
	DWORD m_dwpspiSize;
	__PSYSTEM_PROCESS_INFORMATION m_pspi;
	static DWORD m_FileTypeIndex;
	CMapWordToPtr* m_ProcessIdDatabase;
	CMapWordToPtr m_ProcessEntries;
	HANDLE m_hObjThread;
	HANDLE m_hWorkerThread;
	HMODULE m_hPsapi;
	HANDLE m_hTermination;
	CRITICAL_SECTION m_ProtectDatabase;
	static BOOL m_bUpdating;
	static DWORD m_OSMinorVersion;
	static SC_HANDLE m_hSCM;
	static HANDLE m_hDriver;
	//thread communication structure only one thread actually runs at a time so safe
	struct OBJINF {
		HANDLE hObject;//[in]
		SYSTEM_HANDLE_TABLE_ENTRY_INFO* pHandleInfo;//[in]
		HANDLE hEventStart;//[in]
		HANDLE hEventDone;//[in]
		BOOL bSuccess;//[out]
		LPWSTR lpwsReturn;//[out]
	} m_ObjInf;
	int m_CurrentSortItem;
	BOOL m_SortAscending;

	void AddTraceLog(char* FormatString, ...)
	{
		va_list args;
		va_start(args, FormatString);
		AddTraceLog(GetTraceLogEdit(), FormatString, args);
		va_end(args);
	}
	static void AddTraceLog(CEdit* TraceLogEdit, char* FormatString, ...)
	{
		va_list args;
		va_start(args, FormatString);
		AddTraceLog(TraceLogEdit, FormatString, args);
		va_end(args);
	}
	static void AddTraceLog(CEdit* TraceLogEdit, char* FormatString, va_list args)
	{
		CString String;
		CString Str;
		vsprintf(String.GetBuffer(_vscprintf(FormatString, args) + 1), FormatString, args);
		String.ReleaseBuffer();
		if (TraceLogEdit) {
			TraceLogEdit->GetWindowText(Str);
			Str += String;
			TraceLogEdit->SetWindowText(Str);
			TraceLogEdit->LineScroll(TraceLogEdit->GetLineCount());
		}
	}
	DWORD GetItemType(HTREEITEM hItem)
	{
		HTREEITEM hParentItem;
		BOOL bObjMode = TRUE;
		if (!hItem)
			return -1;
		if ((hParentItem = GetMainTree()->GetParentItem(hItem)) && (hParentItem != GetMainTree()->GetRootItem())) {
			if (GetMainTree()->GetItemData(hParentItem) == OBJECTTYPES_ITEM)
				bObjMode = FALSE;
		}
		if (GetMainTree()->GetItemData(hItem) == PROCESS_ITEM) {
			return PROCESS_ITEM;
		} else if (GetMainTree()->GetItemData(hItem) == SERVICE_ITEM) {
			return SERVICE_ITEM;
		} else if (GetMainTree()->GetItemData(hItem) == DRIVER_ITEM) {
			return DRIVER_ITEM;
		} else if (bObjMode) {
			return ROOT_ITEM;
		} else {
			return OBJECTTYPES_ITEM;
		}
	}
	static BOOL OpenServiceManager(CEdit* TraceLogEdit = NULL)
	{
		if (m_hSCM || (m_hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
			return TRUE;
		} else {
			AddTraceLog(TraceLogEdit, "APICall=OpenSCManager DesiredAccess=SC_MANAGER_ALL_ACCESS Error=%08X\r\n", GetLastError());
			return FALSE;
		}
	}
	static void CloseServiceManager(CEdit* TraceLogEdit = NULL)
	{
		if (m_hSCM) {
			if (!CloseServiceHandle(m_hSCM)) {
				AddTraceLog(TraceLogEdit, "APICall=CloseServiceHandle SCManagerHandle=%08X Error=%08X\r\n", m_hSCM, GetLastError());
			}
		}
		m_hSCM = NULL;
	}
	static BOOL InstallAndStartDriver(LPCTSTR DriverName, LPCTSTR ServiceExe, CEdit* TraceLogEdit = NULL)
	{
		SC_HANDLE hSrv;
		BOOL bRet = FALSE;
		if (OpenServiceManager(TraceLogEdit)) {
			if (hSrv = CreateService(m_hSCM, DriverName, DriverName, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, ServiceExe, NULL, NULL, NULL, NULL, NULL)) {
				if (!(bRet = StartService(hSrv, 0, NULL))) {
					AddTraceLog(TraceLogEdit, "APICall=StartService ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
				}
				if (!CloseServiceHandle(hSrv)) {
					AddTraceLog(TraceLogEdit, "APICall=CloseServiceHandle ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
				}
			} else if (GetLastError() == ERROR_SERVICE_EXISTS) {
				if (hSrv = OpenService(m_hSCM, DriverName, SERVICE_ALL_ACCESS)) {
					if (!(bRet = StartService(hSrv, 0, NULL))) {
						AddTraceLog(TraceLogEdit, "APICall=StartService ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
					}
					if (!CloseServiceHandle(hSrv)) {
						AddTraceLog(TraceLogEdit, "APICall=CloseServiceHandle ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
					}
				} else {
					AddTraceLog(TraceLogEdit, "APICall=OpenService ServiceName=%s Error=%08X\r\n", DriverName, GetLastError());
				}
			} else {
				AddTraceLog(TraceLogEdit, "APICall=CreateService ServiceName=%s Error=%08X\r\n", DriverName, GetLastError());
			}
		}
		return bRet;
	}
	static BOOL StartDriver(LPCTSTR DriverName, CEdit* TraceLogEdit = NULL)
	{
		SC_HANDLE hSrv;
		BOOL bRet = FALSE;
		if (OpenServiceManager(TraceLogEdit)) {
			if (hSrv = OpenService(m_hSCM, DriverName, SERVICE_ALL_ACCESS)) {
				if (!(bRet = StartService(hSrv, 0, NULL))) {
					AddTraceLog(TraceLogEdit, "APICall=StartService ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
				}
				if (!CloseServiceHandle(hSrv)) {
					AddTraceLog(TraceLogEdit, "APICall=CloseServiceHandle ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
				}
			} else {
				AddTraceLog(TraceLogEdit, "APICall=OpenService ServiceName=%s Error=%08X\r\n", DriverName, GetLastError());
			}
		}
		return bRet;
	}
	static BOOL UnloadDriver(LPCTSTR DosName, LPCTSTR DosNameGlobal, LPCTSTR DriverName, CEdit* TraceLogEdit = NULL)
	{
		SC_HANDLE hSrv;
		SERVICE_STATUS serviceStatus;
		OFSTRUCT of;
		BOOL bReturn;
		if (m_hDriver && (m_hDriver != INVALID_HANDLE_VALUE)) {
			CloseHandle(m_hDriver);
			m_hDriver = INVALID_HANDLE_VALUE;
		}
		if (!OpenFile(DosName, &of, OF_DELETE)) {
		}
		if (!OpenFile(DosNameGlobal, &of, OF_DELETE)) {
		}
		if (OpenServiceManager(TraceLogEdit)) {
			if (hSrv = OpenService(m_hSCM, DriverName, SERVICE_ALL_ACCESS)) {
				if (!(bReturn = ControlService(hSrv, SERVICE_CONTROL_STOP, &serviceStatus))) {
					AddTraceLog(TraceLogEdit, "APICall=ControlService ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
				}
				if (!(bReturn = DeleteService(hSrv))) {
					AddTraceLog(TraceLogEdit, "APICall=DeleteService ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
				}
				if (!CloseServiceHandle(hSrv)) {
					AddTraceLog(TraceLogEdit, "APICall=CloseServiceHandle ServiceName=%s ServiceHandle=%08X Error=%08X\r\n", DriverName, hSrv, GetLastError());
				}
			} else {
				AddTraceLog(TraceLogEdit, "APICall=OpenService ServiceName=%s Error=%08X\r\n", DriverName, GetLastError());
			}
		}
		return bReturn;
	}
	static HANDLE LoadDriver(BOOL *fNTDynaLoaded, LPCTSTR DosName, LPCTSTR DosNameGlobal, LPCTSTR DriverName, LPCTSTR ServicePathExe, CEdit* TraceLogEdit = NULL)
	{
		HANDLE hDev;
		*fNTDynaLoaded = FALSE;
		hDev = CreateFile(DosName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE, 0);
		if (hDev == INVALID_HANDLE_VALUE) {
			hDev = CreateFile(DosNameGlobal, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE, 0);
			if (hDev == INVALID_HANDLE_VALUE) {
				if (InstallAndStartDriver(DriverName, ServicePathExe, TraceLogEdit)) {
					if ((hDev = CreateFile(DosName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE, 0)) == INVALID_HANDLE_VALUE) {
						AddTraceLog(TraceLogEdit, "APICall=CreateFile ServiceDosName=%s Error=%08X\r\n", DosName, GetLastError());
					} else {
						*fNTDynaLoaded = TRUE;
					}
				} else {
					AddTraceLog(TraceLogEdit, "MyCall=InstallAndStartDriver Error=Unable to install and start driver\r\n", DosName, GetLastError());
				}
			}
		}
		return hDev;
	}
	static BOOL DriverQuery(DWORD IoControlCode, LPVOID InputBuffer, DWORD InputBufferLength, LPVOID OutputBuffer, DWORD OutputBufferLength, CEdit* TraceLogEdit = NULL)
	{
		HINSTANCE ghInst;
		HRSRC hRsrc;
		HGLOBAL hDrvRsrc;
		DWORD dwDriverSize;
		LPVOID lpvDriver;
		HFILE hfTempFile;
		char srvexe[MAX_PATH << 4];
		BOOL bLoaded = FALSE;
		DWORD dw;
		OFSTRUCT of;
		if (!m_hDriver || (m_hDriver == INVALID_HANDLE_VALUE)) {
			if (ghInst = GetModuleHandle(NULL)) {
				if ((m_OSMinorVersion >= 0 && m_OSMinorVersion <= 2) && (hRsrc = FindResource(ghInst, MAKEINTRESOURCE((m_OSMinorVersion == 0) ? ID_BINRES2000 : ((m_OSMinorVersion == 1) ? ID_BINRESXP : ID_BINRES2003)), "BINRES"))) {
					if (hDrvRsrc = LoadResource(ghInst, hRsrc)) {
						if (dwDriverSize = SizeofResource(ghInst, hRsrc)) {
							if (lpvDriver = LockResource(hDrvRsrc)) {
								srvexe[0] = 0;
								if (!GetCurrentDirectory(MAX_PATH << 4, srvexe)) {
									AddTraceLog(TraceLogEdit, "APICall=GetCurrentDirectory Error=%08X\r\n", GetLastError());
								}
								strcat(srvexe, "\\PestRidDrv.Sys");
								if ((hfTempFile = _lcreat(srvexe, NULL)) != HFILE_ERROR) {
									if (_lwrite(hfTempFile, (char*)lpvDriver, dwDriverSize) != HFILE_ERROR) {
										if (_lclose(hfTempFile) == HFILE_ERROR) {
											AddTraceLog(TraceLogEdit, "APICall=_lclose File=%s Handle=%08X Error=%08X\r\n", srvexe, hfTempFile, GetLastError());
										}
										if ((m_hDriver = LoadDriver(&bLoaded, GPD_DOSDEVICE, GPD_DOSDEVICEGLOBAL, "PestRidDrv", srvexe)) != INVALID_HANDLE_VALUE && m_hDriver && bLoaded) {
										} else {
											AddTraceLog(TraceLogEdit, "MyCall=LoadDriver Error=Unable to load driver\r\n");
										}
										if (!OpenFile(srvexe, &of, OF_DELETE)) {
											AddTraceLog(TraceLogEdit, "APICall=OpenFile Error=%08X\r\n", GetLastError());
										}
									} else {
										AddTraceLog(TraceLogEdit, "APICall=_lwrite File=%s Handle=%08X Error=%08X\r\n", srvexe, hfTempFile, GetLastError());
									}
								} else {
									AddTraceLog(TraceLogEdit, "APICall=_lcreat File=%s Error=%08X\r\n", srvexe, GetLastError());
								}
							} else {
								AddTraceLog(TraceLogEdit, "APICall=LockResource Error=Failed\r\n");
							}
						} else {
							AddTraceLog(TraceLogEdit, "APICall=SizeofResource Error=%08X\r\n", GetLastError());
						}
					} else {
						AddTraceLog(TraceLogEdit, "APICall=LoadResource Error=%08X\r\n", GetLastError());
					}
				} else {
					if (m_OSMinorVersion >= 0 && m_OSMinorVersion <= 2)
						AddTraceLog(TraceLogEdit, "APICall=FindResource Error=%08X\r\n", GetLastError());
				}
			} else {
				AddTraceLog(TraceLogEdit, "APICall=GetModuleHandle Error=%08X\r\n", GetLastError());
			}
		}
		if (m_hDriver && (m_hDriver != INVALID_HANDLE_VALUE)) {
			if (DeviceIoControl(m_hDriver, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, &dw, NULL)) {
				return TRUE;
			} else {
				AddTraceLog(TraceLogEdit, "APICall=DeviceIoControl Error=%08X\r\n", GetLastError());
			}
		}
		return FALSE;
	}
	static BOOL EnablePrivilege(PCSTR name, CEdit* TraceLogEdit = NULL)
	{
		HANDLE hToken;
		BOOL rv = FALSE;
		TOKEN_PRIVILEGES priv = {1, {0, 0, SE_PRIVILEGE_ENABLED}};
		if (LookupPrivilegeValue(0, name, &priv.Privileges[0].Luid)) {
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
				if (AdjustTokenPrivileges(hToken, FALSE, &priv, sizeof priv, 0, 0)) {
					rv = TRUE;
				} else {
					AddTraceLog(TraceLogEdit, "APICall=AdjustTokenPrivileges Token=%08X PrivilegeName=%s Error=%08X\r\n", hToken, name, GetLastError());
				}
				if (!CloseHandle(hToken)) {
					AddTraceLog(TraceLogEdit, "APICall=CloseHandle Token=%08X Error=%08X\r\n", hToken, GetLastError());
				}
			} else {
				AddTraceLog(TraceLogEdit, "APICall=OpenProcessToken Rights=ADJUST_PRIVILEGES Process=Self Error=%08X\r\n", GetLastError());
			}
		} else {
			AddTraceLog(TraceLogEdit, "APICall=LookupPrivilegeValue PrivilegeName=%s Error=%08X\r\n", name, GetLastError());
		}
		return rv;
	}
	CString GetTreeDirectoryPath(HTREEITEM hItem)
	{
		if (hItem == NULL || hItem == GetMainTree()->GetRootItem())
			return "\\";
		else
			return ((GetMainTree()->GetParentItem(hItem) != GetMainTree()->GetRootItem()) ? GetTreeDirectoryPath(GetMainTree()->GetParentItem(hItem)) : CString("")) + "\\" + GetMainTree()->GetItemText(hItem);
	}
	void QueryDirectory(LPWSTR Path)
	{
		CString Str;
		UNICODE_STRING ustr;
		HANDLE h;
		NTSTATUS st;
		OBJECT_ATTRIBUTES obj;
		POSITION pos;
		DWORD Counter;
		LVFINDINFO fi;
		CStringArray DeleteStrArray;
		CHAR buf[QUERY_DIRECTORY_BUF_SIZE];
		BOOLEAN first  = TRUE;
		ULONG index  = 0;
		ULONG retlen = 0;
		DirectoryEntry* DirEntry;
		fi.psz = NULL;
		fi.vkDirection = 0;
		fi.pt.x = 0;
		fi.pt.y = 0;
		if (_RtlInitUnicodeString && _NtOpenDirectoryObject && _NtQueryDirectoryObject) {
			_RtlInitUnicodeString(&ustr, Path);
			obj.Length = sizeof(OBJECT_ATTRIBUTES);
			obj.RootDirectory = NULL;
			obj.Attributes = OBJ_CASE_INSENSITIVE;
			obj.ObjectName = &ustr;
			obj.SecurityDescriptor = NULL;
			obj.SecurityQualityOfService = NULL;
			pos = m_DirectoryEntries.GetStartPosition();
			while (pos != NULL) {
				m_DirectoryEntries.GetNextAssoc(pos, Str, (void*&)DirEntry);
				DirEntry->bDirty = TRUE;
			}
			if ((st = _NtOpenDirectoryObject(&h, DIRECTORY_QUERY, &obj)) == STATUS_SUCCESS) {
				while ((st = _NtQueryDirectoryObject(h, buf, QUERY_DIRECTORY_BUF_SIZE, ObjectByOne, first, &index, &retlen)) == STATUS_SUCCESS) {
					// all of this so that the strings are null terminated WCHAR* and not length indicated unicode strings
					WCHAR* SafePath = (WCHAR*)_malloca(((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Length + sizeof(WCHAR));
					WCHAR* SafeType = (WCHAR*)_malloca(((POBJECT_NAMETYPE_INFO)buf)->ObjectType.Length + sizeof(WCHAR));
					CopyMemory(SafePath, ((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Buffer, ((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Length);
					CopyMemory(SafeType, ((POBJECT_NAMETYPE_INFO)buf)->ObjectType.Buffer, ((POBJECT_NAMETYPE_INFO)buf)->ObjectType.Length);
					*((WCHAR*)&(((CHAR*)SafePath)[((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Length])) = (L"")[0];
					*((WCHAR*)&(((CHAR*)SafeType)[((POBJECT_NAMETYPE_INFO)buf)->ObjectType.Length])) = (L"")[0];
					Str.Format("%S%S%lX", SafePath, SafeType, index); // key with name, type and index which have to be unique!
					if (!m_DirectoryEntries.Lookup(Str, (void*&)DirEntry)) {
						DirEntry = new DirectoryEntry();
						DirEntry->Index = index;
						DirEntry->Name = new WCHAR[wcslen(SafePath) + 1];
						CopyMemory(DirEntry->Name, SafePath, (wcslen(SafePath) + 1) * sizeof(WCHAR));
						DirEntry->Type = new WCHAR[wcslen(SafeType) + 1];
						CopyMemory(DirEntry->Type, SafeType, (wcslen(SafeType) + 1) * sizeof(WCHAR));
						m_DirectoryEntries.SetAt(Str, DirEntry);
						GetProcList()->SetItemData(GetProcList()->InsertItem(GetProcList()->GetItemCount(), LPSTR_TEXTCALLBACK, I_IMAGECALLBACK), (DWORD_PTR)DirEntry);
					}
					DirEntry->bDirty = FALSE;
					_freea(SafeType);
					_freea(SafePath);
					first = FALSE;
				}
				if (st != STATUS_NO_MORE_ENTRIES) {
					AddTraceLog("APICall=NtQueryDirectoryObject ObjectTypesDirectory=%08X Name=%S Error=%08X\r\n", h, Path, st);
				}
				if (!CloseHandle(h)) {
					AddTraceLog("APICall=CloseHandle ObjectTypesDirectory=%08X Error=%08X\r\n", h, GetLastError());
				}
			} else {
				AddTraceLog("APICall=NtOpenDirectoryObject Name=%S Rights=QUERY Error=%08X\r\n", Path, st);
			}
			pos = m_DirectoryEntries.GetStartPosition();
			while (pos != NULL) {
				m_DirectoryEntries.GetNextAssoc(pos, Str, (void*&)DirEntry);
				if (DirEntry->bDirty) {
					AddTraceLog("Event=Directory Object Deletion Name=%S Type=%S Index=%lu\r\n", DirEntry->Name, DirEntry->Type, DirEntry->Index);
					delete [] DirEntry->Name;
					delete [] DirEntry->Type;
					delete DirEntry;
					DeleteStrArray.Add(Str);
					fi.flags = LVFI_PARAM;
					fi.lParam = (LPARAM)DirEntry;
					if ((Counter = GetProcList()->FindItem(&fi)) != -1)
						GetProcList()->DeleteItem(Counter);
				}
			}
			for (Counter = 0; Counter < (DWORD)DeleteStrArray.GetCount(); Counter++) {
				m_DirectoryEntries.RemoveKey(DeleteStrArray[Counter]);
			}
			GetProcList()->SortItems(SortFunc, (DWORD_PTR)this);
		}
	}
	void QueryDirectoryTree(HANDLE ParentHandle, LPWSTR Path, HTREEITEM hParent, DirectoryTreeEntry* TreeEntry)
	{
		// add option on tree right click to have tree Index sorted ascending/descending or Name sorted ascending/descending
		USES_CONVERSION;
		HTREEITEM hItem;
		CString Str;
		UNICODE_STRING ustr;
		HANDLE h;
		NTSTATUS st;
		POSITION pos;
		OBJECT_ATTRIBUTES obj;
		CStringArray DeleteStrArray;
		DirectoryTreeEntry* NextTreeEntry;
		CHAR buf[QUERY_DIRECTORY_BUF_SIZE];
		BOOLEAN first  = TRUE;
		ULONG index  = 0;
		ULONG retlen = 0;
		DWORD Counter;
		if (_RtlInitUnicodeString && _NtOpenDirectoryObject && _NtQueryDirectoryObject) {
			_RtlInitUnicodeString(&ustr, Path);
			obj.Length = sizeof(OBJECT_ATTRIBUTES);
			obj.RootDirectory = ParentHandle;
			obj.Attributes = OBJ_CASE_INSENSITIVE;
			obj.ObjectName = &ustr;
			obj.SecurityDescriptor = NULL;
			obj.SecurityQualityOfService = NULL;
			pos = TreeEntry->Children.GetStartPosition();
			while (pos) {
				TreeEntry->Children.GetNextAssoc(pos, Str, (void*&)NextTreeEntry);
				NextTreeEntry->bDirty = TRUE;
			}
			if ((st = _NtOpenDirectoryObject(&h, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &obj)) == STATUS_SUCCESS) {
				while ((st = _NtQueryDirectoryObject(h, buf, QUERY_DIRECTORY_BUF_SIZE, ObjectByOne, first, &index, &retlen)) == STATUS_SUCCESS) {
					if ((_wcsnicmp(((POBJECT_NAMETYPE_INFO)buf)->ObjectType.Buffer, L"Directory", wcslen(L"Directory")) == 0) || (GetMainTree()->GetItemData(hParent) == OBJECTTYPES_ITEM)) {
						WCHAR* SafePath = (WCHAR*)_malloca(((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Length + sizeof(WCHAR));
						CopyMemory(SafePath, ((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Buffer, ((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Length);
						*((WCHAR*)&(((CHAR*)SafePath)[((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Length])) = (L"")[0];
						if (GetMainTree()->GetItemData(hParent) == OBJECTTYPES_ITEM) {
							for (Counter = 0; Counter < (DWORD)m_KernelObjectNames.GetCount(); Counter++) {
								if (_wcsicmp(SafePath, m_KernelObjectNames[Counter]) == 0) {
									break;
								}
							}
						}
						if (!TreeEntry->Children.Lookup(CString(W2A(SafePath)), (void*&)NextTreeEntry)) {
							NextTreeEntry = new DirectoryTreeEntry();
							NextTreeEntry->Index = index;
							NextTreeEntry->Name = new WCHAR[wcslen(SafePath) + 1];
							CopyMemory(NextTreeEntry->Name, SafePath, (wcslen(SafePath) + 1) * sizeof(WCHAR));
							TreeEntry->Children.SetAt(CString(W2A(SafePath)), NextTreeEntry);
							hItem = GetMainTree()->InsertItem(LPSTR_TEXTCALLBACK, hParent);
							GetMainTree()->SetItemImage(hItem, 0, 1);
							GetMainTree()->SetItemData(hItem, (DWORD_PTR)NextTreeEntry);
						} else {
							hItem = GetMainTree()->GetChildItem(hParent);
							while (hItem) {
								if (GetMainTree()->GetItemData(hParent) == OBJECTTYPES_ITEM) {
									if (Counter == GetMainTree()->GetItemData(hItem))
										break;
								} else if (GetMainTree()->GetItemData(hItem) == OBJECTTYPES_ITEM) {
									if ((_wcsnicmp(((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Buffer, L"ObjectTypes", wcslen(L"ObjectTypes")) == 0) && GetMainTree()->GetItemData(hParent) == ROOT_ITEM)
										break;
								} else if (GetMainTree()->GetItemData(hItem) == (DWORD_PTR)NextTreeEntry) {
									break;
								}
								hItem = GetMainTree()->GetNextItem(hItem, TVGN_NEXT);
							}
						}
						NextTreeEntry->bDirty = FALSE;
						if ((_wcsnicmp(((POBJECT_NAMETYPE_INFO)buf)->ObjectName.Buffer, L"ObjectTypes", wcslen(L"ObjectTypes")) == 0) && GetMainTree()->GetItemData(hParent) == ROOT_ITEM) {
							GetMainTree()->SetItemData(hItem, OBJECTTYPES_ITEM);
							GetMainTree()->SetItemState(hItem, TVIS_BOLD, TVIS_BOLD);
							GetMainTree()->SetItemText(hItem, W2A(SafePath));
						}
						if (GetMainTree()->GetItemData(hParent) == OBJECTTYPES_ITEM) {
							GetMainTree()->SetItemData(hItem, Counter);
							GetMainTree()->SetItemText(hItem, W2A(SafePath));
						} else {
							QueryDirectoryTree(h, SafePath, hItem, NextTreeEntry);
						}
						_freea(SafePath);
					}
					first = FALSE;
				}
				if (st != STATUS_NO_MORE_ENTRIES) {
					AddTraceLog("APICall=NtQueryDirectoryObject ObjectTypesDirectory=%08X Name=%S Error=%08X\r\n", h, Path, st);
				}
				if (!CloseHandle(h)) {
					AddTraceLog("APICall=CloseHandle ObjectTypesDirectory=%08X Name=%S Error=%08X\r\n", h, Path, GetLastError());
				}
			} else {
				AddTraceLog("APICall=NtOpenDirectoryObject Name=%S Rights=QUERY|TRAVERSE Error=%08X\r\n", Path, st);
			}
			pos = TreeEntry->Children.GetStartPosition();
			while (pos) {
				TreeEntry->Children.GetNextAssoc(pos, Str, (void*&)NextTreeEntry);
				if (NextTreeEntry->bDirty) {
					AddTraceLog("Event=Delete Directory Type Object Deletion Name=%S Index=%lu", NextTreeEntry->Name, NextTreeEntry->Index);
					DeleteChildTree(NextTreeEntry);
					DeleteStrArray.Add(Str);
					hItem = GetMainTree()->GetChildItem(hParent);
					while (hItem) {
						if (GetMainTree()->GetItemData(hItem) == (DWORD_PTR)NextTreeEntry) {
							GetMainTree()->DeleteItem(hItem);
							break;
						}
						hItem = GetMainTree()->GetNextItem(hItem, TVGN_NEXT);
					}
				}
			}
			for (Counter = 0; Counter < (DWORD)DeleteStrArray.GetCount(); Counter++) {
				TreeEntry->Children.RemoveKey(DeleteStrArray[Counter]);
			}
			//calling RtlFreeUnicodeString causes heap corruption since it uses local stack in RtlInitUnicodeString for Buffer allocation
			//so do not call it as it is unneccessary the memory is automatically freed
		}
	}
	void DeleteChildTree(DirectoryTreeEntry* TreeEntry)
	{
		DirectoryTreeEntry* NextTreeEntry;
		POSITION pos;
		CString Str;
		pos = TreeEntry->Children.GetStartPosition();
		while (pos) {
			TreeEntry->Children.GetNextAssoc(pos, Str, (void*&)NextTreeEntry);
			DeleteChildTree(NextTreeEntry);
		}
		delete [] TreeEntry->Name;
		delete TreeEntry;
	}
	void EnumKernelNamespaceObjectTypes()
	{
		USES_CONVERSION;
		UNICODE_STRING ustr;
		HANDLE h;
		NTSTATUS st;
		OBJECT_ATTRIBUTES obj;
		if (_RtlInitUnicodeString && _NtOpenDirectoryObject && _NtQueryDirectoryObject) {
			_RtlInitUnicodeString(&ustr, L"\\ObjectTypes");
			obj.Length = sizeof(OBJECT_ATTRIBUTES);
			obj.RootDirectory = NULL;
			obj.Attributes = OBJ_CASE_INSENSITIVE;
			obj.ObjectName = &ustr;
			obj.SecurityDescriptor = NULL;
			obj.SecurityQualityOfService = NULL;
			if ((st = _NtOpenDirectoryObject(&h, DIRECTORY_QUERY, &obj)) == STATUS_SUCCESS) {
				if (_NtQueryObject) {
					DWORD dwSize = sizeof(POBJECT_ALL_TYPES_INFORMATION);
					POBJECT_ALL_TYPES_INFORMATION pObjectInfo = (POBJECT_ALL_TYPES_INFORMATION)_malloca(dwSize);
					st = _NtQueryObject(h, ObjectAllTypesInformation, pObjectInfo, dwSize, &dwSize); 
					while (st == STATUS_INFO_LENGTH_MISMATCH) { 
						_freea(pObjectInfo);
						pObjectInfo = (POBJECT_ALL_TYPES_INFORMATION)_malloca(dwSize);
						st = _NtQueryObject(h, ObjectAllTypesInformation, pObjectInfo, dwSize, &dwSize); 
					}
					if (st == STATUS_SUCCESS) {
						DWORD Counter;
						POBJECT_TYPE_INFORMATION TypePtr = &pObjectInfo->TypeInformation;
						m_FileTypeIndex = -1;
						for (Counter = 0; Counter < pObjectInfo->NumberOfTypes; Counter++) {
							WCHAR* SafePath = (WCHAR*)_malloca(TypePtr->Name.Length + sizeof(WCHAR));
							CopyMemory(SafePath, TypePtr->Name.Buffer, TypePtr->Name.Length);
							*((WCHAR*)&(((CHAR*)SafePath)[TypePtr->Name.Length])) = (L"")[0];
							if (_wcsicmp(SafePath, L"File") == 0)
								m_FileTypeIndex = Counter + 1;
							m_KernelObjectNames.Add(SafePath);
							TypePtr = (POBJECT_TYPE_INFORMATION)((char*)TypePtr + sizeof(OBJECT_TYPE_INFORMATION) + (wcslen(TypePtr->Name.Buffer) + 1) * sizeof(WCHAR));
							if ((*((DWORD*)&TypePtr) & 3) != 0) {
								TypePtr = (POBJECT_TYPE_INFORMATION)((char*)TypePtr + (4 - (*((DWORD*)&TypePtr) & 3)));
							}
							_freea(SafePath);
						}
					} else {
						AddTraceLog("APICall=NtQueryObject Info=ObjectAllTypesInformation ObjectTypesDirectory=%08X Error=%08X\r\n", h, st);
					}
					_freea(pObjectInfo);
				}
				if (!CloseHandle(h)) {
					AddTraceLog("APICall=CloseHandle ObjectTypesDirectory=%08X Error=%08X\r\n", h, GetLastError());
				}
			} else {
				AddTraceLog("APICall=NtOpenDirectoryObject Name=\\ObjectTypes Rights=QUERY Error=%08X\r\n", st);
			}
			//calling RtlFreeUnicodeString causes heap corruption since it uses local stack in RtlInitUnicodeString for Buffer allocation
			//so do not call it as it is unneccessary the memory is automatically freed
		}
	}
	void QueryThreadProc()
	{
		USES_CONVERSION;
		HTREEITEM hItem;
		DWORD Item;
		while (WaitForSingleObject(m_hTermination, 3000) == WAIT_TIMEOUT) {
			EnterCriticalSection(&m_ProtectDatabase);
			m_bUpdating = TRUE;
			DWORD OldTicks = GetTickCount();
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
			m_bUpdating = FALSE;
			LeaveCriticalSection(&m_ProtectDatabase);
			AddTraceLog("Query Time: %lums\r\n", GetTickCount() - OldTicks);
		}
	}
	static unsigned int WINAPI ProcessHandleDatabaseUpdateThread(void* lParam)
	{
		((CPestRidSplitterWnd*)lParam)->QueryThreadProc();
		return 0;
	}
	// This function must not use any resources or heap memory before the NtQueryObject in case of deadlock terminate thread causes leaks
	// note that in debug builds memory leaks still show up because _malloca/_freea use normal dynamic memory not the stack
	static LPWSTR GetObjectNameInfo(HANDLE hObject, HANDLE hEventStart, HANDLE hEventDone, BOOL* bSuccess) 
	{
		LPWSTR lpwsReturn = NULL; 
		DWORD dwSize = sizeof(OBJECT_NAME_INFORMATION) + 2048;
		POBJECT_NAME_INFORMATION pObjectInfo = (POBJECT_NAME_INFORMATION)_malloca(dwSize);
		if (_NtQueryObject) {
			NTSTATUS ntReturn = _NtQueryObject(hObject, ObjectNameInformation, pObjectInfo, dwSize, &dwSize);
			while (ntReturn == STATUS_INFO_LENGTH_MISMATCH) {
				_freea(pObjectInfo);
				pObjectInfo = (POBJECT_NAME_INFORMATION)_malloca(dwSize);
				ntReturn = _NtQueryObject(hObject, ObjectNameInformation, pObjectInfo, dwSize, &dwSize); 
			}
			if (ntReturn == STATUS_SUCCESS) {
				*bSuccess = TRUE;
				SetEvent(hEventDone); // starting allocation
				WaitForSingleObject(hEventStart, INFINITE);
				if (pObjectInfo->ObjectName.Buffer != NULL) {
					lpwsReturn = (LPWSTR) new BYTE[pObjectInfo->ObjectName.Length + sizeof(WCHAR)]; 
					*((WCHAR*)&(((CHAR*)lpwsReturn)[pObjectInfo->ObjectName.Length])) = (L"")[0];
					CopyMemory(lpwsReturn, pObjectInfo->ObjectName.Buffer, pObjectInfo->ObjectName.Length); 
				}
			}
		}
		_freea(pObjectInfo);
		return lpwsReturn; 
	}
	static unsigned int WINAPI GetObjectNameThread(void* lParam)
	{
		struct _DriverNameQuery
		{
			ULONG PID;
			PVOID ObjectAddress;
			HANDLE ObjectHandle;
		} DriverNameQuery;
		USES_CONVERSION;
		char* Buf = (char*)_malloca(8192);
		while ((WaitForSingleObject(((OBJINF*)lParam)->hEventStart, INFINITE) == WAIT_OBJECT_0) && (((OBJINF*)lParam)->hObject != INVALID_HANDLE_VALUE)) {
			// ObQueryNameString will hang when querying the names of pipes that have been opened for synchronous access (FILE_SYNCHRONOUS_IO_ALERT or FILE_SYNCHRONOUS_IO_NOALERT) and that have a pending read or write operation
			// Named pipes are file type objects and its hard to determine anymore in user mode without risking the permanently stranded thread
			// Must use kernel mode driver cannot be done in user space
			// PFILE_OBJECT has PDEVICE_OBJECT->DeviceType == FILE_DEVICE_NAMED_PIPE(0x11) or Flags = FO_NAMED_PIPE
			((OBJINF*)lParam)->lpwsReturn = NULL;
			((OBJINF*)lParam)->bSuccess = FALSE;
			DriverNameQuery.PID = ((OBJINF*)lParam)->pHandleInfo->uIdProcess;
			DriverNameQuery.ObjectAddress = ((OBJINF*)lParam)->pHandleInfo->Object;
			DriverNameQuery.ObjectHandle = (HANDLE)((OBJINF*)lParam)->pHandleInfo->Handle;
			ZeroMemory(Buf, 8192);
			if (DriverQuery(IOCTL_FOI_GETOBJECTNAME, &DriverNameQuery, sizeof(DriverNameQuery), Buf, 8192)) {
				((OBJINF*)lParam)->bSuccess = TRUE;
				SetEvent(((OBJINF*)lParam)->hEventDone); // starting allocation
				WaitForSingleObject(((OBJINF*)lParam)->hEventStart, INFINITE);
				((OBJINF*)lParam)->lpwsReturn = _wcsdup(A2W(Buf));
			} else if (((OBJINF*)lParam)->pHandleInfo->ObjectTypeIndex != m_FileTypeIndex) {
				// should use driver ISFILEOBJECT to detect file objects with invalid length also since we can query them
				((OBJINF*)lParam)->lpwsReturn = GetObjectNameInfo(((OBJINF*)lParam)->hObject, ((OBJINF*)lParam)->hEventStart, ((OBJINF*)lParam)->hEventDone, &((OBJINF*)lParam)->bSuccess);
			}
			SetEvent(((OBJINF*)lParam)->hEventDone); // done
		}
		_freea(Buf);
		return 0;
	}
	void GetHandleName()
	{
		HANDLE hProcess;
		if ((hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, m_ObjInf.pHandleInfo->uIdProcess)) && hProcess != INVALID_HANDLE_VALUE) {
			if (DuplicateHandle(hProcess, (HANDLE)m_ObjInf.pHandleInfo->Handle, GetCurrentProcess(), &m_ObjInf.hObject, 0, FALSE, 0)) {
				if (!m_ObjInf.hEventStart || m_ObjInf.hEventStart == INVALID_HANDLE_VALUE) {
					if (!(m_ObjInf.hEventStart = CreateEvent(NULL, FALSE, FALSE, NULL))) {
						AddTraceLog("APICall=CreateEvent Use=Start Error=%08X\r\n", GetLastError());
					}
				}
				if (!m_ObjInf.hEventDone || m_ObjInf.hEventDone == INVALID_HANDLE_VALUE) {
					if (!(m_ObjInf.hEventDone = CreateEvent(NULL, FALSE, FALSE, NULL))) {
						AddTraceLog("APICall=CreateEvent Use=Done Error=%08X\r\n", GetLastError());
					}
				}
				if (!m_hObjThread || m_hObjThread == INVALID_HANDLE_VALUE) {
					if (!(m_hObjThread = (HANDLE)_beginthreadex(NULL, 0, &CPestRidSplitterWnd::GetObjectNameThread, &m_ObjInf, 0, NULL)) || (m_hObjThread == INVALID_HANDLE_VALUE)) {
						AddTraceLog("LibraryCall=_beginthreadex Error=%08X\r\n", errno);
					}
				}
				if (!SetEvent(m_ObjInf.hEventStart)) {
					AddTraceLog("Start event SetEvent returns error %08X\r\n", GetLastError());
				}
				if (WaitForSingleObject(m_ObjInf.hEventDone, 1000) == WAIT_TIMEOUT) {
					AddTraceLog("APICall=WaitForSingleObject Error=WAIT_TIMEOUT Thread=NtQueryObject CurHandleEntry=[Type=%S PID=%04X Handle=%04X]\r\n", m_KernelObjectNames[m_ObjInf.pHandleInfo->ObjectTypeIndex - 1], m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.hObject);
					if (!TerminateThread(m_hObjThread, 1)) {
						AddTraceLog("APICall=TerminateThread Thread=%08X Error=%08X\r\n", m_hObjThread, GetLastError());
					}
					if (!CloseHandle(m_hObjThread)) {
						AddTraceLog("APICall=CloseHandle Thread=%08X Error=%08X\r\n", m_hObjThread, GetLastError());
					}
					m_hObjThread = INVALID_HANDLE_VALUE;
					m_ObjInf.lpwsReturn = new WCHAR[wcslen(L"<UNABLE TO QUERY NAME>") + 1];
					wcscpy(m_ObjInf.lpwsReturn, L"<UNABLE TO QUERY NAME>");
				} else {
					if (m_ObjInf.bSuccess) {
						if (!SetEvent(m_ObjInf.hEventStart)) {
							AddTraceLog("Start event SetEvent returns error %08X\r\n", GetLastError());
						}
						WaitForSingleObject(m_ObjInf.hEventDone, INFINITE);
					} else {
						AddTraceLog("MyCall=GetHandleName CurHandleEntry=[Type=%S PID=%04X Handle=%04X]\r\n", m_KernelObjectNames[m_ObjInf.pHandleInfo->ObjectTypeIndex - 1], m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
					}
				}
				if (!CloseHandle(m_ObjInf.hObject)) {
					AddTraceLog("APICall=CloseHandle DuplicatedHandle=%08X Error=%08X CurHandleEntry=[Type=%S PID=%04X Handle=%04X]\r\n", m_ObjInf.hObject, GetLastError(), m_KernelObjectNames[m_ObjInf.pHandleInfo->ObjectTypeIndex - 1], m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
				}
			} else {
				AddTraceLog("APICall=DuplicateHandle Error=%08X CurHandleEntry=[Type=%S PID=%04X Handle=%04X]\r\n", GetLastError(), m_KernelObjectNames[m_ObjInf.pHandleInfo->ObjectTypeIndex - 1], m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
			}
			if (!CloseHandle(hProcess)) {
				AddTraceLog("APICall=CloseHandle Process=%08X Error=%08X CurHandleEntry=[Type=%S PID=%04X Handle=%04X]\r\n", hProcess, GetLastError(), m_KernelObjectNames[m_ObjInf.pHandleInfo->ObjectTypeIndex - 1], m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
			}
		} else {
			if (hProcess != INVALID_HANDLE_VALUE) {
				AddTraceLog("APICall=OpenProcess Rights=PROCESS_DUP_HANDLE Error=%08X CurHandleEntry=[Type=%S PID=%04X Handle=%04X]\r\n", GetLastError(), m_KernelObjectNames[m_ObjInf.pHandleInfo->ObjectTypeIndex - 1], m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
			} else {
				AddTraceLog("APICall=OpenProcess Rights=PROCESS_DUP_HANDLE Error=INVALID_HANDLE_VALUE CurHandleEntry=[Type=%S PID=%04X Handle=%04X]\r\n", m_KernelObjectNames[m_ObjInf.pHandleInfo->ObjectTypeIndex - 1], m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
			}
		}
	}
	// move to another thread and use postmessage this function is potentially to slow
	// eventually should have driver hook CreateProcess and allow user to confirm what new processes to allow creation to
	// also hook all functions that can create new handles so this is not trivial...
	void UpdateHandleDatabaseList()
	{
		USES_CONVERSION;
		POSITION pos;
		POSITION npos;
		WORD key;
		WORD keyback;
		DWORD cb;
		CMapWordToPtr* ProcHandleTable;
		HANDLE hObject = NULL;
		DWORD Index = 0;
		HandleEntry* NewEntry;
		CArray<WORD, WORD> DeleteArray;
		CArray<WORD, WORD> DeleteProcArray;
		LVFINDINFO fi;
		fi.psz = NULL;
		fi.vkDirection = 0;
		fi.pt.x = 0;
		fi.pt.y = 0;
		if (_NtQueryObject && _NtQuerySystemInformation) {
			DWORD dwSize = sizeof(SYSTEM_HANDLE_INFORMATION); 
			PSYSTEM_HANDLE_INFORMATION pHandleInfo = (PSYSTEM_HANDLE_INFORMATION)new BYTE[dwSize];
			pHandleInfo->uCount = 0;
			NTSTATUS ntReturn = _NtQuerySystemInformation(SystemHandleInformation, pHandleInfo, dwSize, &dwSize); 
			while (ntReturn == STATUS_INFO_LENGTH_MISMATCH) { 
				delete [] pHandleInfo;
				pHandleInfo = (PSYSTEM_HANDLE_INFORMATION) new BYTE[dwSize];
				ntReturn = _NtQuerySystemInformation(SystemHandleInformation, pHandleInfo, dwSize, &dwSize);
			}
			if (ntReturn != STATUS_SUCCESS) {
				AddTraceLog("Call=NtQuerySystemInformation Type=HandleInformation Error=%08X\r\n", ntReturn);
				pHandleInfo->uCount = 0;
			}
			pos = m_ProcessIdDatabase->GetStartPosition();
			while (pos) {
				m_ProcessIdDatabase->GetNextAssoc(pos, key, (void*&)ProcHandleTable);
				npos = ProcHandleTable->GetStartPosition();
				while (npos) {
					ProcHandleTable->GetNextAssoc(npos, key, (void*&)NewEntry);
					NewEntry->bDirty = TRUE;
				}
			}
			if (pHandleInfo) {
				for (cb = 0; cb < pHandleInfo->uCount; cb++) {
					NewEntry = NULL;
					if (!m_ProcessIdDatabase->Lookup(pHandleInfo->Handles[cb].uIdProcess, (void*&)ProcHandleTable)) {
						ProcHandleTable = new CMapWordToPtr(2048);
						ProcHandleTable->InitHashTable(12289);
						m_ProcessIdDatabase->SetAt(pHandleInfo->Handles[cb].uIdProcess, (void*&)ProcHandleTable);
					}
					Index = 0;
					if (!ProcHandleTable->Lookup(pHandleInfo->Handles[cb].Handle, (void*&)NewEntry)) {
						NewEntry = new HandleEntry;
						NewEntry->Name = NULL;
						Index = -1;
					} else if (NewEntry->HandleInfo.ObjectTypeIndex != pHandleInfo->Handles[cb].ObjectTypeIndex) {
						if ((WORD)(GetMainTree()->GetItemData(GetMainTree()->GetSelectedItem()) + 1) == NewEntry->HandleInfo.ObjectTypeIndex) {
							fi.flags = LVFI_PARAM;
							fi.lParam = (LPARAM)NewEntry;
							if ((Index = GetProcList()->FindItem(&fi)) != -1)
								GetProcList()->DeleteItem(Index);
						}
					}
					NewEntry->SystemIndex = cb;
					CopyMemory(&NewEntry->HandleInfo, &pHandleInfo->Handles[cb], sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO));
					ProcHandleTable->SetAt(pHandleInfo->Handles[cb].Handle, NewEntry);
					if ((WORD)(GetMainTree()->GetItemData(GetMainTree()->GetSelectedItem()) + 1) == NewEntry->HandleInfo.ObjectTypeIndex) {
						m_ObjInf.pHandleInfo = &NewEntry->HandleInfo;
						GetHandleName();
						if (NewEntry->Name) {
							delete [] NewEntry->Name;
							NewEntry->Name = NULL;
						}
						if (m_ObjInf.lpwsReturn) {
							NewEntry->Name = new WCHAR[wcslen(m_ObjInf.lpwsReturn) + 1];
							CopyMemory(NewEntry->Name, m_ObjInf.lpwsReturn, (wcslen(m_ObjInf.lpwsReturn) + 1) * sizeof(WCHAR));
							delete [] m_ObjInf.lpwsReturn;
							m_ObjInf.lpwsReturn = NULL;
						}
						fi.flags = LVFI_PARAM;
						fi.lParam = (LPARAM)NewEntry;
						if ((Index == -1) || (Index = GetProcList()->FindItem(&fi)) == -1) {
							Index = GetProcList()->InsertItem(GetProcList()->GetItemCount(), LPSTR_TEXTCALLBACK, I_IMAGECALLBACK);
						}
						GetProcList()->SetItemData(Index, (DWORD_PTR)NewEntry);
					}
					if (NewEntry)
						NewEntry->bDirty = FALSE;
				}
			}
			pos = m_ProcessIdDatabase->GetStartPosition();
			while (pos) {
				m_ProcessIdDatabase->GetNextAssoc(pos, keyback, (void*&)ProcHandleTable);
				npos = ProcHandleTable->GetStartPosition();
				while (npos) {
					ProcHandleTable->GetNextAssoc(npos, key, (void*&)NewEntry);
					if (NewEntry->bDirty) {
						AddTraceLog("Event=Handle Deletion HandleEntry=[Type=%S PID=%04X Handle=%04X]\r\n", m_KernelObjectNames[NewEntry->HandleInfo.ObjectTypeIndex - 1], Index, key);
						if ((WORD)(GetMainTree()->GetItemData(GetMainTree()->GetSelectedItem()) + 1) == NewEntry->HandleInfo.ObjectTypeIndex) {
							fi.flags = LVFI_PARAM;
							fi.lParam = (LPARAM)NewEntry;
							if ((Index = GetProcList()->FindItem(&fi)) != -1)
								GetProcList()->DeleteItem(Index);
						}
						delete [] NewEntry->Name;
						delete NewEntry;
						DeleteArray.Add(key);
					}
				}
				for (cb = 0; cb < (DWORD)DeleteArray.GetCount(); cb++) {
					ProcHandleTable->RemoveKey(DeleteArray[cb]);
				}
				DeleteArray.RemoveAll();
				if (ProcHandleTable->IsEmpty()) {
					AddTraceLog("Event=PID Deletion HandleCount=0 PID=%04X\r\n", keyback);
					DeleteProcArray.Add((WORD)keyback);
					delete ProcHandleTable;
				}
			}
			for (cb = 0; cb < (DWORD)DeleteProcArray.GetCount(); cb++) {
				m_ProcessIdDatabase->RemoveKey(DeleteProcArray[cb]);
			}
			if (pHandleInfo)
				delete [] pHandleInfo;
		}
		GetProcList()->SortItems(SortFunc, (DWORD_PTR)this);
	}
	void GetHandleTypeInformation()
	{
		/*NTSTATUS ntReturn;
		FILE_PIPE_INFORMATION fileinfo;
		IO_STATUS_BLOCK StatusBlock;
		StatusBlock.Pointer = NULL;
		StatusBlock.Information = NULL;
		StatusBlock.Status = 0;
		if (pHandleInfo->Handles[cb].ObjectTypeIndex == m_FileTypeIndex) {
			if (_NtQueryInformationFile) {
				if (ntReturn = _NtQueryInformationFile(hObject, &StatusBlock, &fileinfo, sizeof(FILE_PIPE_INFORMATION), (FILE_INFORMATION_CLASS)FilePipeInformation) != STATUS_SUCCESS) {
					Str.Format("Call=NtQueryInformationFile Error=%08X\r\n", ntReturn);
					AddTraceLog(Str);
					continue;
				} else {
					Str.Format("Call=NtQueryInformationFile Success=Found named pipe\r\n");
					continue;
				}
			}
		}*/
	}
	void GetProcessName(CString & String, DWORD pid)
	{
		HANDLE hProcess;
		char szImageName[MAX_PATH << 4];
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);
		DWORD SystemID = (osvi.dwMajorVersion >= 5) ? ((osvi.dwMinorVersion == 0) ? 2 : 4) : 8;
		// PID 0 is System Idle Process and PID 2, 4 or 8 is System
		// should test correct for all Windows versions (95, 98, Me, NT = 8, 2000 = 2, XP, 2003 = 4)
		if (!(hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)) || (hProcess == INVALID_HANDLE_VALUE)) {
			if (pid) { // PID 0 cannot be opened for query
				if (hProcess != INVALID_HANDLE_VALUE)
					AddTraceLog("APICall=OpenProcess Rights=QUERY_INFORMATION|VM_READ PID=%04X Error=%08X\r\n", pid, GetLastError());
				else
					AddTraceLog("APICall=OpenProcess Rights=QUERY_INFORMATION|VM_READ PID=%04X Error=INVALID_HANDLE_VALUE\r\n", pid);
			}
		}
		if (hProcess && (hProcess != INVALID_HANDLE_VALUE) && _GetModuleFileNameEx && _GetModuleFileNameEx(hProcess, NULL, szImageName, MAX_PATH << 4)) {
			String = szImageName;
		} else {
			if (hProcess && (hProcess != INVALID_HANDLE_VALUE) && !CloseHandle(hProcess)) {
				AddTraceLog("APICall=CloseHandle Process=%08X Error=%08X PID=%04X\r\n", hProcess, GetLastError(), pid);
			}
			if ((hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid)) && (hProcess != INVALID_HANDLE_VALUE)) {
				if (_GetProcessImageFileName && _GetProcessImageFileName(hProcess, szImageName, MAX_PATH << 4)) {
					String = szImageName;
					// convert from device path to physical path
				} else {
					if (_GetModuleFileNameEx || _GetProcessImageFileName) {
						if (pid && (pid != SystemID)) { // PID 0=System Idle Process and System and cannot be opened for query information
							AddTraceLog("APICall=GetModuleFileNameEx|GetProcessImageFileName Process=%08X Error=%08X PID=%04X\r\n", hProcess, GetLastError(), pid);
						}
					}
				}
			} else {
				if (pid && (pid != SystemID)) { // PID 0=System Idle Process and System and cannot be opened for query information
					if (hProcess != INVALID_HANDLE_VALUE)
						AddTraceLog("APICall=OpenProcess Rights=QUERY_INFORMATION PID=%04X Error=%08X\r\n", pid, GetLastError());
					else
						AddTraceLog("APICall=OpenProcess Rights=QUERY_INFORMATION PID=%04X Error=INVALID_HANDLE_VALUE\r\n", pid);
				}
			}
		}
		if (hProcess && (hProcess != INVALID_HANDLE_VALUE) && !CloseHandle(hProcess)) {
			AddTraceLog("APICall=CloseHandle Process=%08X Error=%08X PID=%04X\r\n", hProcess, GetLastError(), pid);
		}
	}
	void UpdateServiceInformation()
	{
		DWORD Counter;
		DWORD nServices = 0;
		DWORD ResumeHandle = 0;
		DWORD MoreBytes;
		LVFINDINFO fi;
		fi.psz = NULL;
		fi.vkDirection = 0;
		fi.pt.x = 0;
		fi.pt.y = 0;
		if (OpenServiceManager()) {
			if (!m_ServiceInformation)
				m_ServiceInformation = (LPENUM_SERVICE_STATUS_PROCESS)new BYTE[sizeof(ENUM_SERVICE_STATUS_PROCESS) * m_ServiceInformationBufferSize];
			while (!EnumServicesStatusEx(m_hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS, SERVICE_STATE_ALL, (LPBYTE)m_ServiceInformation, m_ServiceInformationBufferSize, &MoreBytes, &nServices, &ResumeHandle, NULL) && (GetLastError() == ERROR_MORE_DATA)) {
				m_ServiceInformationBufferSize += MoreBytes;
				delete [] m_ServiceInformation;
				m_ServiceInformation = (LPENUM_SERVICE_STATUS_PROCESS)new BYTE[sizeof(ENUM_SERVICE_STATUS_PROCESS) * m_ServiceInformationBufferSize];
				nServices = 0;
				ResumeHandle = 0;
			}
			for (Counter = 0; Counter < max(nServices, m_NumberOfServices); Counter++) {
				if (hService = OpenService(m_hSCM, m_ServiceInformation, SERVICE_QUERY_CONFIG)) {
					//QueryServiceConfig(hService, ServiceConfig, BufSize, BytesNeeded);
				}
				fi.lParam = Counter;
				fi.flags = LVFI_PARAM;
				if ((MoreBytes = GetProcList()->FindItem(&fi)) == -1) {
					if (Counter < nServices)
						GetProcList()->SetItemData(GetProcList()->InsertItem(GetProcList()->GetItemCount(), LPSTR_TEXTCALLBACK), Counter);
				} else if (Counter >= nServices) {
					GetProcList()->DeleteItem(MoreBytes);
				}
			}
			m_NumberOfServices = nServices;
			GetProcList()->SortItems(SortFunc, (DWORD_PTR)this);
		}
	}
	void UpdateDriverInformation()
	{
		DWORD Counter;
		DWORD nServices = 0;
		DWORD ResumeHandle = 0;
		DWORD MoreBytes;
		LVFINDINFO fi;
		fi.psz = NULL;
		fi.vkDirection = 0;
		fi.pt.x = 0;
		fi.pt.y = 0;
		if (OpenServiceManager()) {
			if (!m_DriverInformation)
				m_DriverInformation = (LPENUM_SERVICE_STATUS_PROCESS)new BYTE[sizeof(ENUM_SERVICE_STATUS_PROCESS) * m_DriverInformationBufferSize];
			while (!EnumServicesStatusEx(m_hSCM, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER | SERVICE_ADAPTER, SERVICE_STATE_ALL, (LPBYTE)m_DriverInformation, m_DriverInformationBufferSize, &MoreBytes, &nServices, &ResumeHandle, NULL) && (GetLastError() == ERROR_MORE_DATA)) {
				m_DriverInformationBufferSize += MoreBytes;
				delete [] m_DriverInformation;
				m_DriverInformation = (LPENUM_SERVICE_STATUS_PROCESS)new BYTE[sizeof(ENUM_SERVICE_STATUS_PROCESS) * m_DriverInformationBufferSize];
				nServices = 0;
				ResumeHandle = 0;
			}
			for (Counter = 0; Counter < max(nServices, m_NumberOfDrivers); Counter++) {
				fi.lParam = Counter;
				fi.flags = LVFI_PARAM;
				if ((MoreBytes = GetProcList()->FindItem(&fi)) == -1) {
					if (Counter < nServices)
						GetProcList()->SetItemData(GetProcList()->InsertItem(GetProcList()->GetItemCount(), LPSTR_TEXTCALLBACK), Counter);
				} else if (Counter >= nServices) {
					GetProcList()->DeleteItem(MoreBytes);
				}
			}
			m_NumberOfDrivers = nServices;
			GetProcList()->SortItems(SortFunc, (DWORD_PTR)this);
		}
	}
	CString GetProcessUserName(WORD PID)
	{
		HANDLE hProcess;
		HANDLE hToken;
		TOKEN_USER* ptu;
		DWORD dwSize;
		DWORD dwOtherSize;
		SID_NAME_USE use;
		CString Str;
		char* Name;
		char* Domain;
		//TokenSessionId
		//TokenPrivileges
		if (hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, PID)) {
			if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
				if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
					ptu = (TOKEN_USER*)new BYTE[dwSize];
					if (GetTokenInformation(hToken, TokenUser, ptu, dwSize, &dwSize)) {
						dwSize = 0;
						dwOtherSize = 0;
						if (!LookupAccountSid(NULL, ptu->User.Sid, NULL, &dwSize, NULL, &dwOtherSize, &use) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
							Name = new char[dwSize];
							Domain = new char[dwOtherSize];
							if (LookupAccountSid(NULL, ptu->User.Sid, Name, &dwSize, Domain, &dwOtherSize, &use)) {
								Str.Format("%s\\%s", Name, Domain);
							}
							delete [] Name;
							delete [] Domain;
						}
					}
					delete [] ptu;
				}
				CloseHandle(hToken);
			}
			CloseHandle(hProcess);
		}
		return Str;
	}
	struct EnumStruct {
		HWND hwnd;
		DWORD PID;
		CString* String;
	};
	static BOOL CALLBACK WindowEnumFunc(HWND hwnd, LPARAM lParam)
	{
		CString String;
		CWnd* Wnd;
		DWORD dwPid;
		if (((EnumStruct*)lParam)->hwnd == 0) {
			GetWindowThreadProcessId(hwnd, &dwPid);
			if (dwPid == ((EnumStruct*)lParam)->PID) {
				Wnd = CWnd::FromHandle(hwnd);
				if ((Wnd->GetParent() ? !Wnd->GetParent()->IsWindowVisible() : TRUE) && Wnd->IsWindowVisible()) {
					Wnd->GetWindowText(*((EnumStruct*)lParam)->String);
					if (!((EnumStruct*)lParam)->String->IsEmpty())
						((EnumStruct*)lParam)->hwnd = hwnd;
				}
			}
		}
		return TRUE;
	}
	void UpdateProcessList()
	{
		DWORD cb;
		DWORD newcb;
		DWORD* pidArray = NULL;
		HANDLE hSnapshot;
		HANDLE hProcess;
		ProcessEntry* Entry;
		ProcessEntry* OldEntry;
		CArray<WORD, WORD> DeleteArray;
		POSITION pos;
		NTSTATUS ntReturn;
		LVFINDINFO fi;
		int item;
		WORD pid;
		USES_CONVERSION;
		fi.psz = NULL;
		fi.vkDirection = 0;
		fi.pt.x = 0;
		fi.pt.y = 0;
		cb = 4;
		newcb = 0;
		if (_EnumProcesses) {
			do {
				cb = cb << 2;
				if (pidArray)
					delete [] pidArray;
				pidArray = new DWORD[cb >> 2];
				newcb = cb;
				if (_EnumProcesses(pidArray, cb, &newcb) == 0) {
					AddTraceLog("APICall=EnumProcesses Error=%08X\r\n", GetLastError());
					newcb = 0;
					break;
				}
			} while (cb == newcb);
		}
		pos = m_ProcessEntries.GetStartPosition();
		while (pos) {
			m_ProcessEntries.GetNextAssoc(pos, pid, (void*&)Entry);
			Entry->bDirty = TRUE;
			Entry->ChildProcessIds.RemoveAll();
		}
		if (hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) {
			Entry = new ProcessEntry;
			Entry->hIcon = NULL;
			Entry->bExpanded = TRUE;
			Entry->lppe.dwSize = sizeof(PROCESSENTRY32);
			Entry->bDirty = FALSE;
			Entry->ProcessThreadInformationOffset = -1;
			if (Process32First(hSnapshot, &Entry->lppe)) {
				do {
					for (cb = 0; cb < (newcb >> 2); cb++) {
						if (pidArray[cb] == Entry->lppe.th32ProcessID) {
							break;
						}
					}
					if (m_ProcessEntries.Lookup((WORD)Entry->lppe.th32ProcessID, (void*&)OldEntry)) {
						memcpy(&OldEntry->lppe, &Entry->lppe, sizeof(PROCESSENTRY32));
						OldEntry->bDirty = FALSE;
					} else {
						m_ProcessEntries.SetAt((WORD)Entry->lppe.th32ProcessID, Entry);
						Entry = new ProcessEntry;
						Entry->hIcon = NULL;
						Entry->bExpanded = TRUE;
						Entry->lppe.dwSize = sizeof(PROCESSENTRY32);
						Entry->bDirty = FALSE;
						Entry->ProcessThreadInformationOffset = -1;
					}
					if (newcb && (cb == (newcb >> 2))) {
						AddTraceLog("FailAPI=EnumProcesses SuccessAPI=Process32First/Process32Next PID=%04X Path=%s hidden\r\n", Entry->lppe.th32ProcessID, Entry->lppe.szExeFile);
					}
				} while (Process32Next(hSnapshot, &Entry->lppe));
				delete Entry;
				if (GetLastError() != ERROR_NO_MORE_FILES) {
					AddTraceLog("APICall=Process32Next Snapshot=%08X Error=%08X\r\n", hSnapshot, GetLastError());
				}
			} else {
				AddTraceLog("APICall=Process32First Snapshot=%08X Error=%08X\r\n", hSnapshot, GetLastError());
			}
			if (!CloseHandle(hSnapshot)) {
				AddTraceLog("APICall=CloseHandle Snapshot=%08X Error=%08X\r\n", hSnapshot, GetLastError());
			}
		} else {
			AddTraceLog("APICall=CreateToolhelp32Snapshot List=SNAPPROCESS Error=%08X\r\n", GetLastError());
		}
		pos = m_ProcessEntries.GetStartPosition();
		while (pos) {
			m_ProcessEntries.GetNextAssoc(pos, pid, (void*&)Entry);
			if (Entry->bDirty) {
				fi.lParam = pid;
				fi.flags = LVFI_PARAM;
				if ((item = GetProcList()->FindItem(&fi)) != -1)
					GetProcList()->DeleteItem(item);
				if ((item = m_ProcTreeList->FindItem(&fi)) != -1)
					m_ProcTreeList->DeleteItem(item);
				DeleteArray.Add(pid);
				delete Entry;
			} else if (pid && ((m_ProcessEntries.Lookup((WORD)Entry->lppe.th32ParentProcessID, (void*&)OldEntry)) || (m_ProcessEntries.Lookup(0, (void*&)OldEntry)))) {
				OldEntry->ChildProcessIds.Add(pid);
			}
		}
		for (cb = 0; cb < (DWORD)DeleteArray.GetCount(); cb++) {
			m_ProcessEntries.RemoveKey(DeleteArray[cb]);
		}
		for (cb = 0; cb < (newcb >> 2); cb++) {
			if (!m_ProcessEntries.Lookup((WORD)pidArray[cb], (void*&)OldEntry)) {
				AddTraceLog("FailAPI=Process32First/Process32Next SuccessAPI=EnumProcesses PID=%04X Path=%s hidden\r\n", OldEntry->lppe.th32ProcessID, OldEntry->lppe.szExeFile);
			}
			if (pidArray[cb] == OldEntry->lppe.th32ProcessID) {
				break;
			}
		}
		if (!m_pspi)
			m_pspi = (__PSYSTEM_PROCESS_INFORMATION)new BYTE[m_dwpspiSize = sizeof(SYSTEM_PROCESS_INFORMATION)];
		__PSYSTEM_PROCESS_INFORMATION pspienum;
		if (_NtQuerySystemInformation) {
			ntReturn = _NtQuerySystemInformation(SystemProcessesAndThreadsInformation, m_pspi, m_dwpspiSize, &m_dwpspiSize);
			while (ntReturn == STATUS_INFO_LENGTH_MISMATCH) {
				delete [] m_pspi;
				m_pspi = (__PSYSTEM_PROCESS_INFORMATION )new BYTE[m_dwpspiSize];
				ntReturn = _NtQuerySystemInformation(SystemProcessesAndThreadsInformation, m_pspi, m_dwpspiSize, &m_dwpspiSize);
			}
			if (ntReturn != STATUS_SUCCESS) {
				AddTraceLog("Call=NtQuerySystemInformation Type=ProcessThreadInformation Error=%08X\r\n", ntReturn);
			}
		}
		pspienum = m_pspi;
		while (pspienum) {
			if (m_ProcessEntries.Lookup((WORD)pspienum->Process.dUniqueProcessId, (void*&)Entry)) {
				Entry->ProcessThreadInformationOffset = (DWORD)((char*)pspienum - (char*)m_pspi);
			}
			pspienum = (pspienum->Process.dNext ? (__PSYSTEM_PROCESS_INFORMATION)(((BYTE*)pspienum) + pspienum->Process.dNext) : NULL);
		};
		pos = m_ProcessEntries.GetStartPosition();
		while (pos) {
			m_ProcessEntries.GetNextAssoc(pos, pid, (void*&)Entry);
			if (hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)) {
				PROCESS_BASIC_INFORMATION* PBI;
				DWORD ParamOffset = 0;
				DWORD read;
				WCHAR* wstr;
				RTL_USER_PROCESS_PARAMETERS params;
				DWORD size = sizeof(PROCESS_BASIC_INFORMATION);
				PBI = (PROCESS_BASIC_INFORMATION*)new BYTE[size];
				ntReturn = _NtQueryInformationProcess(hProcess, ProcessBasicInformation, PBI, size, &size);
				while (ntReturn == STATUS_INFO_LENGTH_MISMATCH) {
					delete [] PBI;
					PBI = (PROCESS_BASIC_INFORMATION*)new BYTE[size];
					ntReturn = _NtQueryInformationProcess(hProcess, ProcessBasicInformation, PBI, size, &size);
				}
				if (ntReturn != STATUS_SUCCESS) {
				} else {
					ReadProcessMemory(hProcess, (char*)PBI->PebBaseAddress + 0x10, &ParamOffset, sizeof(ParamOffset), &read);
					memset(&params, 0, sizeof(params));
					ReadProcessMemory(hProcess, (LPCVOID)ParamOffset, &params, sizeof(params), &read);
					wstr = new WCHAR[params.CommandLine.Length + 1];
					memset(wstr, 0, (params.CommandLine.Length + 1) * sizeof(WCHAR));
					ReadProcessMemory(hProcess, params.CommandLine.Buffer, wstr, params.CommandLine.Length, &read);
					Entry->CommandLine = W2A(wstr);
					delete [] wstr;
					wstr = new WCHAR[params.ImagePathName.Length + 1];
					memset(wstr, 0, (params.ImagePathName.Length + 1) * sizeof(WCHAR));
					ReadProcessMemory(hProcess, params.ImagePathName.Buffer, wstr, params.ImagePathName.Length, &read);
					Entry->ImagePathName = W2A(wstr);
					delete [] wstr;
					wstr = new WCHAR[params.WindowTitle.Length + 1];
					memset(wstr, 0, (params.WindowTitle.Length + 1) * sizeof(WCHAR));
					ReadProcessMemory(hProcess, params.WindowTitle.Buffer, wstr, params.WindowTitle.Length, &read);
					Entry->WindowTitle = W2A(wstr);
					delete [] wstr;
				}
				delete [] PBI;
				CloseHandle(hProcess);
			}
			Entry->UserName = GetProcessUserName((WORD)Entry->lppe.th32ProcessID);
			Entry->DepStatus = 0xFF;
			if (hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, Entry->lppe.th32ProcessID)) {
				DWORD Result;
				if (DriverQuery(IOCTL_FOI_QUERYDEP, &hProcess, sizeof(hProcess), &Result, sizeof(Result))) {
					Entry->DepStatus = (BYTE)Result;
				}
			}
			Entry->WindowStatus = 0xFF;
			Entry->WindowTitle.Empty();
			struct EnumStruct NewEnumStruct = { NULL, Entry->lppe.th32ProcessID, &Entry->WindowTitle };
			if (EnumWindows(WindowEnumFunc, (LPARAM)&NewEnumStruct) && NewEnumStruct.hwnd && _IsHungAppWindow) {
				Entry->WindowStatus = _IsHungAppWindow(NewEnumStruct.hwnd) ? 0 : 1;
			}
			if (!ProcessIdToSessionId(Entry->lppe.th32ProcessID, &Entry->SessionId))
				Entry->SessionId = -1;
			if (Entry->ImagePathName.IsEmpty()) {
				//try our function
			}
			if (!Entry->ImagePathName.IsEmpty()) {
				DWORD dwSize;
				BYTE* VerInfo;
				BYTE* Buffer = NULL;
				UINT BufLen = 0;
				BYTE* RealBuf = NULL;
				UINT RealBufLen = 0;
				CString QueryBuf;
				if (dwSize = GetFileVersionInfoSize(Entry->ImagePathName, &dwSize)) {
					VerInfo = new BYTE[dwSize];
					if (GetFileVersionInfo(Entry->ImagePathName, 0, dwSize, VerInfo)) {
						if (VerQueryValue(VerInfo, "\\VarFileInfo\\Translation", (LPVOID*)&Buffer, &BufLen)) {
							RealBuf = VerInfo;
							while (*((DWORD*)RealBuf) != 0xFEEF04BD) {
								RealBuf++;
							}
							Entry->Version.Format("%d.%02d.%04d.%04d", *((WORD*)(RealBuf + 10)), *((WORD*)(RealBuf + 8)), *((WORD*)(RealBuf + 14)), *((WORD*)(RealBuf + 12)));
							QueryBuf.Format("\\StringFileInfo\\%04X%04X\\%s", *((USHORT*)Buffer), *((USHORT*)Buffer + 1), "FileDescription");
							if (VerQueryValue(VerInfo, (LPSTR)(LPCSTR)QueryBuf, (LPVOID*)&RealBuf, &RealBufLen)) {
								Entry->FileDescription = RealBuf;
							} else {
								QueryBuf.Format("\\StringFileInfo\\%04X%04X\\%s", *((USHORT*)Buffer), 1252, "FileDescription");
								if (VerQueryValue(VerInfo, (LPSTR)(LPCSTR)QueryBuf, (LPVOID*)&RealBuf, &RealBufLen)) {
									Entry->FileDescription = RealBuf;
								}
							}
							QueryBuf.Format("\\StringFileInfo\\%04X%04X\\%s", *((USHORT*)Buffer), *((USHORT*)Buffer + 1), "CompanyName");
							if (VerQueryValue(VerInfo, (LPSTR)(LPCSTR)QueryBuf, (LPVOID*)&RealBuf, &RealBufLen)) {
								Entry->CompanyName = RealBuf;
							} else {
								QueryBuf.Format("\\StringFileInfo\\%04X%04X\\%s", *((USHORT*)Buffer), 1252, "CompanyName");
								if (VerQueryValue(VerInfo, (LPSTR)(LPCSTR)QueryBuf, (LPVOID*)&RealBuf, &RealBufLen)) {
									Entry->CompanyName = RealBuf;
								}
							}
						}
					}
					delete [] VerInfo;
				}
			}
		}

		fi.lParam = 0;
		fi.flags = LVFI_PARAM;
		m_ProcessEntries.Lookup(0, (void*&)Entry);
		if (GetProcList()->FindItem(&fi) == -1) {
			GetProcList()->SetItemData(GetProcList()->InsertItem(GetProcList()->GetItemCount(), LPSTR_TEXTCALLBACK), 0);
		}
		if (m_ProcTreeList->FindItem(&fi) == -1) {
			m_ProcTreeList->SetItemData(m_ProcTreeList->InsertItem(m_ProcTreeList->GetItemCount(), LPSTR_TEXTCALLBACK), 0);
		}
		ExpandCollapseTree(Entry);
		if (pidArray)
			delete [] pidArray;
		GetProcList()->SortItems(SortFunc, (DWORD_PTR)this);
		m_ProcTreeList->SortItems(SortFunc, (DWORD_PTR)this);
		m_ProcTreeList->RedrawWindow();
	}
	BOOL HasChildPID(WORD PID)
	{
		ProcessEntry* Entry;
		if (m_ProcessEntries.Lookup(PID, (void*&)Entry)) {
			return (Entry->ChildProcessIds.GetCount() != 0);
		} else {
			return FALSE;
		}
	}
	void ExpandCollapseTree(ProcessEntry* ParentEntry)
	{
		WORD curpid;
		int item;
		LVFINDINFO fi;
		ProcessEntry* Entry;
		INT_PTR Counter;
		BOOL bExpand = TRUE;
		fi.psz = NULL;
		fi.vkDirection = 0;
		fi.pt.x = 0;
		fi.pt.y = 0;
		if (GetMainTab()->GetCurSel() == 0) {
			curpid = (WORD)ParentEntry->lppe.th32ProcessID;
			while (bExpand && m_ProcessEntries.Lookup(curpid, (void*&)Entry) && curpid) {
				bExpand = Entry->bExpanded;
				curpid = (WORD)Entry->lppe.th32ParentProcessID;
			}
			if (bExpand && curpid) {
				curpid = 0;
				m_ProcessEntries.Lookup(curpid, (void*&)Entry);
			}
			bExpand = Entry->bExpanded;
		}
		for (Counter = 0; Counter < ParentEntry->ChildProcessIds.GetCount(); Counter++) {
			if (m_ProcessEntries.Lookup(ParentEntry->ChildProcessIds[Counter], (void*&)Entry)) {
				fi.flags = LVFI_PARAM;
				fi.lParam = Entry->lppe.th32ProcessID;
				if (bExpand) {
					if (GetProcList()->FindItem(&fi) == -1) {
						GetProcList()->SetItemData(GetProcList()->InsertItem(GetProcList()->GetItemCount(), LPSTR_TEXTCALLBACK), Entry->lppe.th32ProcessID);
					}
					if (m_ProcTreeList->FindItem(&fi) == -1) {
						m_ProcTreeList->SetItemData(m_ProcTreeList->InsertItem(m_ProcTreeList->GetItemCount(), LPSTR_TEXTCALLBACK), Entry->lppe.th32ProcessID);
					}
				} else {
					if ((item = GetProcList()->FindItem(&fi)) != -1) {
						GetProcList()->DeleteItem(item);
					}
					if ((item = m_ProcTreeList->FindItem(&fi)) != -1) {
						m_ProcTreeList->DeleteItem(item);
					}
				}
				ExpandCollapseTree(Entry);
			}
		}
	}
	afx_msg void OnLvnClickProcList(NMHDR *pNMHDR, LRESULT *pResult)
	{
		DWORD Count;
		ProcessEntry* Entry;
		WORD PID;
		CRect rect;
		LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)pNMHDR;
		if ((lpnmitem->iItem != -1) && (lpnmitem->iSubItem == 0)) {
			if (GetItemType(GetMainTree()->GetSelectedItem()) == PROCESS_ITEM) {
				if (HasChildPID((WORD)m_ProcTreeList->GetItemData(lpnmitem->iItem))) {
					Count = 0;
					PID = (WORD)m_ProcTreeList->GetItemData(lpnmitem->iItem);
					while (PID && m_ProcessEntries.Lookup(PID, (void*&)Entry)) {
						Count++;
						PID = (WORD)Entry->lppe.th32ParentProcessID;
					}
					m_ProcTreeList->GetItemRect((int)lpnmitem->iItem, rect, LVIR_LABEL);
					rect.left += 2 - GetSystemMetrics(SM_CXSMICON) + Count * 16;
					rect.right = rect.left + 8;
					rect.top += (rect.bottom - rect.top - 8) >> 1;
					rect.bottom = rect.top + 8;
					if (m_ProcessEntries.Lookup((WORD)m_ProcTreeList->GetItemData(lpnmitem->iItem), (void*&)Entry)) {
						if ((lpnmitem->ptAction.x >= rect.left) && (lpnmitem->ptAction.x <= rect.right)) {
							if ((lpnmitem->ptAction.y >= rect.top) && (lpnmitem->ptAction.y <= rect.bottom)) {
								Entry->bExpanded = !Entry->bExpanded;
								ExpandCollapseTree(Entry);
								if (Entry->bExpanded) {
									GetProcList()->SortItems(SortFunc, (DWORD_PTR)this);
									m_ProcTreeList->SortItems(SortFunc, (DWORD_PTR)this);
								}
								GetProcList()->RedrawItems(lpnmitem->iItem, lpnmitem->iItem);
								m_ProcTreeList->RedrawItems(lpnmitem->iItem, lpnmitem->iItem);
							}
						}
					}
				}
			}
		}
	}
	void KillProcess(WORD PID, BOOL bRecurse)
	{
		HANDLE hProcess;
		INT_PTR Counter;
		ProcessEntry* Entry;
		if (bRecurse) {
			if (m_ProcessEntries.Lookup(PID, (void*&)Entry)) {
				for (Counter = 0; Counter < Entry->ChildProcessIds.GetCount(); Counter++) {
					KillProcess(Entry->ChildProcessIds[Counter], bRecurse);
				}
			}
		}
		if (hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, PID)) {
			TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
		}
	}
	BOOL IsProcessSuspended(WORD PID)
	{
		ProcessEntry* Entry;
		DWORD Counter;
		if (m_ProcessEntries.Lookup(PID, (void*&)Entry)) {
			if (Entry->ProcessThreadInformationOffset != -1) {
				for (Counter = 0; Counter < ((__PSYSTEM_PROCESS_INFORMATION)(&((char*)m_pspi)[Entry->ProcessThreadInformationOffset]))->Process.dThreadCount; Counter++) {
					if ((((__PSYSTEM_PROCESS_INFORMATION)(&((char*)m_pspi)[Entry->ProcessThreadInformationOffset]))->Process_NT5.aThreads[Counter].dThreadState == 5) && (((__PSYSTEM_PROCESS_INFORMATION)(&((char*)m_pspi)[Entry->ProcessThreadInformationOffset]))->Process_NT5.aThreads[Counter].WaitReason == 5)) {
					} else {
						return FALSE;
					}
				}
			}			
		}
		return TRUE;
	}
	void SuspendResumeProcess(WORD PID, BOOL bRecurse, BOOL bSuspend)
	{
		HANDLE hProcess;
		INT_PTR Counter;
		ProcessEntry* Entry;
		if (bRecurse) {
			if (m_ProcessEntries.Lookup(PID, (void*&)Entry)) {
				for (Counter = 0; Counter < Entry->ChildProcessIds.GetCount(); Counter++) {
					SuspendResumeProcess(Entry->ChildProcessIds[Counter], bRecurse, bSuspend);
				}
			}
		}
		if (_NtSuspendProcess && _NtResumeProcess) {
			if (hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, PID)) {
				(bSuspend ? _NtSuspendProcess : _NtResumeProcess)(hProcess);
				CloseHandle(hProcess);
			}
		} else {
			//OpenThread/SuspendThread/ResumeThread
		}
	}
	afx_msg void OnLvnRClickProcList(NMHDR *pNMHDR, LRESULT *pResult)
	{
		LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)pNMHDR;
		CMenu Menu;
		CMenu MenuExtra1;
		CMenu MenuExtra2;
		CRect rect;
		char buf[1024];
		LVCOLUMN lvcol;
		DWORD Counter;
		if (lpnmitem->iItem != -1) {
			if (GetItemType(GetMainTree()->GetSelectedItem()) == PROCESS_ITEM) {
				//ProcessEntry* Entry;
				// put into column information
				/*if (m_ProcessEntries.Lookup((WORD)GetProcList()->GetItemData(lpnmitem->iItem), (void*&)Entry)) {
					if (Entry->ProcessThreadInformationOffset != -1) {
					// fix the wchar stuff
						MessageBoxW(NULL, ((__PSYSTEM_PROCESS_INFORMATION)&((char*)m_pspi)[Entry->ProcessThreadInformationOffset])->Process.usName.Buffer, NULL, MB_OK);
					}
				}*/
				Menu.CreatePopupMenu();
				MenuExtra1.CreatePopupMenu();
				MenuExtra1.AppendMenu(MF_STRING, 10, "&Kill");
				//MenuExtra1.AppendMenu(MF_STRING, 15, "&Graceful shutdown");
				//MenuExtra1.AppendMenu(MF_STRING, 30, "Graceful &Restart");
				//MenuExtra1.AppendMenu(MF_STRING, 35, "Kill and &Restart");
				MenuExtra1.AppendMenu(MF_STRING, 50, IsProcessSuspended((WORD)GetProcList()->GetItemData(lpnmitem->iItem)) ? "Resu&me" : "&Suspend");
				MenuExtra2.CreatePopupMenu();
				MenuExtra2.AppendMenu(MF_STRING, 20, "&Kill");
				//MenuExtra2.AppendMenu(MF_STRING, 40, "&Restart");
				MenuExtra2.AppendMenu(MF_STRING, 60, "&Suspend");
				MenuExtra2.AppendMenu(MF_STRING, 80, "Resu&me");
				Menu.AppendMenu(MF_POPUP, (UINT_PTR)MenuExtra1.GetSafeHmenu(), "&Process");
				Menu.AppendMenu(MF_POPUP, (UINT_PTR)MenuExtra2.GetSafeHmenu(), "&Process Tree");
				((pNMHDR->hwndFrom == GetProcList()->GetSafeHwnd()) ? GetProcList() : m_ProcTreeList)->GetWindowRect(rect);
				switch (Menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, rect.left + lpnmitem->ptAction.x, rect.top + lpnmitem->ptAction.y, this, NULL)) {
				case 10:
					KillProcess((WORD)GetProcList()->GetItemData(lpnmitem->iItem), FALSE);
					break;
				case 20:
					KillProcess((WORD)GetProcList()->GetItemData(lpnmitem->iItem), TRUE);
					break;
				case 30:
					//GetCommandLine information
					KillProcess((WORD)GetProcList()->GetItemData(lpnmitem->iItem), FALSE);
					//CreateProcess
					break;
				case 40:
					//GetCommandLine information recursive...
					KillProcess((WORD)GetProcList()->GetItemData(lpnmitem->iItem), TRUE);
					//CreateProcess recursive...
					break;
				case 50:
					SuspendResumeProcess((WORD)GetProcList()->GetItemData(lpnmitem->iItem), FALSE, !IsProcessSuspended((WORD)GetProcList()->GetItemData(lpnmitem->iItem)));
					break;
				case 60:
					SuspendResumeProcess((WORD)GetProcList()->GetItemData(lpnmitem->iItem), TRUE, TRUE);
					break;
				case 80:
					SuspendResumeProcess((WORD)GetProcList()->GetItemData(lpnmitem->iItem), TRUE, FALSE);
					break;
				}
				MenuExtra2.DestroyMenu();
				MenuExtra1.DestroyMenu();
				Menu.DestroyMenu();
			}
		} else {
			if (lpnmitem->iSubItem == -1) {
				Menu.CreatePopupMenu();
				lvcol.mask = LVCF_TEXT;
				lvcol.cchTextMax = 1024;
				lvcol.pszText = buf;
				((pNMHDR->hwndFrom == GetProcList()->GetSafeHwnd()) ? GetProcList() : m_ProcTreeList)->GetWindowRect(rect);
				for (Counter = 0; Counter < (DWORD)((pNMHDR->hwndFrom == GetProcList()->GetSafeHwnd()) ? GetProcList() : m_ProcTreeList)->GetHeaderCtrl()->GetItemCount(); Counter++) {
					if (((pNMHDR->hwndFrom == GetProcList()->GetSafeHwnd()) ? GetProcList() : m_ProcTreeList)->GetColumn(Counter, &lvcol))
						Menu.AppendMenu(MF_STRING, Counter + 1, lvcol.pszText);
				}
				POINT Point;
				GetCursorPos(&Point);
				if ((Counter = Menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, Point.x, Point.y, this, NULL)) != 0) {
				}
				Menu.DestroyMenu();
			}
		}
	}
	afx_msg LRESULT OnAppMouseWheel(WPARAM wParam, LPARAM lParam)
	{
		if (GetItemType(GetMainTree()->GetSelectedItem()) == PROCESS_ITEM) {
			GetProcList()->SendMessage(WM_MOUSEWHEEL, wParam, lParam);
		}
		return 0;
	}
	afx_msg LRESULT OnAppSyncScroll(WPARAM wParam, LPARAM lParam)
	{
		CRect Rect;
		CRect NewRect;
		if (GetItemType(GetMainTree()->GetSelectedItem()) == PROCESS_ITEM) {
			(((HWND)lParam == GetProcList()->GetSafeHwnd()) ? m_ProcTreeList : GetProcList())->GetItemRect((((HWND)lParam == GetProcList()->GetSafeHwnd()) ? m_ProcTreeList : GetProcList())->GetTopIndex(), NewRect, LVIR_BOUNDS);
			(((HWND)lParam == GetProcList()->GetSafeHwnd()) ? m_ProcTreeList : GetProcList())->GetItemRect((((HWND)lParam != GetProcList()->GetSafeHwnd()) ? m_ProcTreeList : GetProcList())->GetTopIndex(), Rect, LVIR_BOUNDS);
			(((HWND)lParam == GetProcList()->GetSafeHwnd()) ? m_ProcTreeList : GetProcList())->Scroll(CSize(0, Rect.top - NewRect.top));
		}
		return 0;
	}
	afx_msg LRESULT OnAppSizeTreeList(WPARAM wParam, LPARAM lParam)
	{
		static int InitialTop = -1;
		CRect Rect;
		CRect TestRect;
		DWORD Counter;
		if (m_MainTab && m_MainTab->GetSafeHwnd()) {
			m_MainTab->GetClientRect(Rect);
			Rect.bottom = Rect.top;
			for (Counter = 0; Counter < (DWORD)m_MainTab->GetItemCount(); Counter++) {
				m_MainTab->GetItemRect(Counter, TestRect);
				if (TestRect.bottom > Rect.bottom)
					Rect.bottom = TestRect.bottom;
			}
			TestRect.top = Rect.bottom - Rect.top;
			m_ProcList->GetWindowRect(Rect);
			m_ProcList->GetParent()->ScreenToClient(Rect);
			if (InitialTop == -1)
				InitialTop = Rect.top;
			Rect.top = InitialTop + TestRect.top;
			//m_ProcList->MoveWindow(Rect);
			m_ProcList->SetWindowPos(&CWnd::wndTop, Rect.left, Rect.top, Rect.Width(), Rect.Height(), 0);
		}
		return 0;
	}
	afx_msg void OnLvnCustomdrawProcList(NMHDR *pNMHDR, LRESULT *pResult)
	{
		CString Str;
		CRect rect;
		CRect TreeButtonRect;
		CDC dc;
		ProcessEntry* Entry;
		WORD PID;
		DWORD Count;
		COLORREF ref;
		COLORREF bkref;
		DWORD OldLeft;
		SHFILEINFO sfi;
		CBrush Brush;
		CPen Pen(PS_SOLID, 1, RGB(0, 0, 0));
		CPen* pOldObject;
		LPNMLVCUSTOMDRAW lpNMCustomDraw = (LPNMLVCUSTOMDRAW)pNMHDR;
		*pResult = CDRF_DODEFAULT;
		if (lpNMCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT) {
			if (GetItemType(GetMainTree()->GetSelectedItem()) == PROCESS_ITEM) {
				*pResult = CDRF_NOTIFYITEMDRAW;
			}
		} else if (lpNMCustomDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
			*pResult = CDRF_NOTIFYITEMDRAW;
		} else if (lpNMCustomDraw->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
			if (lpNMCustomDraw->iSubItem == 0) {
				if (GetItemType(GetMainTree()->GetSelectedItem()) == PROCESS_ITEM) {
					lpNMCustomDraw->nmcd.lItemlParam = m_ProcTreeList->GetItemData((int)lpNMCustomDraw->nmcd.dwItemSpec);
					dc.Attach(lpNMCustomDraw->nmcd.hdc);
					Count = 0;
					PID = (WORD)lpNMCustomDraw->nmcd.lItemlParam;
					while (PID && m_ProcessEntries.Lookup(PID, (void*&)Entry)) {
						Count++;
						PID = (WORD)Entry->lppe.th32ParentProcessID;
					}
					Entry = NULL;
					if (m_ProcessEntries.Lookup((WORD)lpNMCustomDraw->nmcd.lItemlParam, (void*&)Entry)) {
						Str.Empty();
						GetProcessName(Str, (DWORD)lpNMCustomDraw->nmcd.lItemlParam);
						if (!Entry->hIcon) { // perhaps have a timeout and reload the icon
							ZeroMemory(&sfi, sizeof(sfi));
							if (!Str.IsEmpty()) {
								SHGetFileInfo(Str, 0, &sfi, sizeof(sfi), SHGFI_SMALLICON | SHGFI_ICON);
							}
							if (!(Entry->hIcon = sfi.hIcon)) {
								Entry->hIcon = LoadIcon(NULL, IDI_APPLICATION);
							}
						}
						Str = Entry->lppe.szExeFile;
					}
					m_ProcTreeList->GetItemRect((int)lpNMCustomDraw->nmcd.dwItemSpec, rect, LVIR_LABEL);
					OldLeft = rect.left;
					rect.left += 2 - GetSystemMetrics(SM_CXSMICON);
					if (GetMainTab()->GetCurSel() == 0) {
						rect.left += Count * 16;
						if (HasChildPID((WORD)lpNMCustomDraw->nmcd.lItemlParam)) {
							TreeButtonRect = rect;
							TreeButtonRect.right = TreeButtonRect.left + 8;
							TreeButtonRect.top += (TreeButtonRect.bottom - TreeButtonRect.top - 8) >> 1;
							TreeButtonRect.bottom = TreeButtonRect.top + 8;
							pOldObject = dc.SelectObject(&Pen);
							dc.MoveTo(TreeButtonRect.left, TreeButtonRect.top);
							dc.LineTo(TreeButtonRect.right, TreeButtonRect.top);
							dc.LineTo(TreeButtonRect.right, TreeButtonRect.bottom);
							dc.LineTo(TreeButtonRect.left, TreeButtonRect.bottom);
							dc.LineTo(TreeButtonRect.left, TreeButtonRect.top);
							dc.MoveTo(TreeButtonRect.left + 2, ((TreeButtonRect.bottom - TreeButtonRect.top) >> 1) + TreeButtonRect.top);
							dc.LineTo(TreeButtonRect.right - 1, ((TreeButtonRect.bottom - TreeButtonRect.top) >> 1) + TreeButtonRect.top);
							if (Entry && (Entry->bExpanded == FALSE)) {
								dc.MoveTo(((TreeButtonRect.right - TreeButtonRect.left) >> 1) + TreeButtonRect.left, TreeButtonRect.top + 2);
								dc.LineTo(((TreeButtonRect.right - TreeButtonRect.left) >> 1) + TreeButtonRect.left, TreeButtonRect.bottom - 1);
							}
							dc.SelectObject(pOldObject);
							rect.left += 12;
						}
					}
					if (Entry && Entry->hIcon) {
						if (!DrawIconEx(dc.GetSafeHdc(), rect.left, rect.top, Entry->hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL)) {}
					}
					rect.left += GetSystemMetrics(SM_CXSMICON);
					dc.SetTextAlign(TA_LEFT);					
					Brush.CreateSysColorBrush((m_ProcTreeList->GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec, LVIS_SELECTED)) ? COLOR_HIGHLIGHT : COLOR_WINDOW);
					dc.FillRect(rect, &Brush);
					Brush.DeleteObject();
					if (m_ProcTreeList->GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec, LVIS_SELECTED)) {
						ref = dc.GetTextColor();
						bkref = dc.GetBkColor();
						dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
						dc.SetBkColor(GetSysColor(COLOR_HIGHLIGHT));
					}
					rect.left += 4;
					rect.top += 2;
					dc.DrawText(Str, rect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
					rect.top -= 2;
					if (m_ProcTreeList->GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec, LVIS_SELECTED)) {
						dc.SetTextColor(ref);
						dc.SetBkColor(bkref);
					}
					rect.left -= 4;
					if (m_ProcTreeList->GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec, LVIS_FOCUSED))
						dc.DrawFocusRect(rect);
					dc.Detach();
					rect.left = OldLeft;
					*pResult = CDRF_SKIPDEFAULT;
				}
			}
		}
	}
};