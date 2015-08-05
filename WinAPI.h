#pragma once

#include <winnt.h>
#include <winternl.h>
#define _NTDEF_
typedef LONG NTSTATUS, *PNTSTATUS;
#include <ntsecapi.h>

#include <winioctl.h>

#define _SETUPAPI_VER _WIN32_WINNT_LONGHORN //_WIN32_WINNT_WINXP
#include <setupapi.h>

#include "Utility.h"
#include "TraceLog.h"

//kernel32
HMODULE PRU_GetModuleHandle(LPCTSTR lpModuleName);
HMODULE PRU_LoadLibrary(LPCTSTR lpLibFileName);
BOOL PRU_FreeLibrary(HMODULE hLibModule);
FARPROC PRU_GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
BOOL PRU_PathFileExists(LPCTSTR pszPath);
BOOL PRU_ExpandEnvironmentStrings(LPCTSTR szPath, CString & ExpandedPath);
BOOL PRU_LaunchProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine);
BOOL PRU_DeleteFile(LPCTSTR szFileName);
BOOL PRU_RemoveDirectory(LPCTSTR lpPathName);
BOOL PRU_DeleteFileTree(LPCTSTR szPath);
BOOL PRU_GetWindowsDirectory(CString & Dir);
BOOL PRU_EnumDirectory(LPCTSTR szSearchPath, BOOL bDirectories,
					   BOOL bFiles, BOOL bTitle, CStringArray & EnumResults,
					   LPCTSTR* pszExclusions = NULL);
BOOL PRU_QueryDosDevice(LPCTSTR szTarget, CStringArray & Devices);
BOOL PRU_GetVolumePathNamesForVolumeName(LPCTSTR szVolName, CStringArray & PathNames);
BOOL PRU_CheckWindowsVersionMinumum(DWORD dwMajorVer, DWORD dwMinorVer);
LPWSTR GetFilePath(HANDLE hFile);
BOOL PRU_NtOpenDirectoryObject(HANDLE & hDirObj, LPCWSTR szName, HANDLE hParent, ACCESS_MASK dwAccess);
BOOL PRU_NtQueryDirectoryObject(HANDLE hDirObj, BOOLEAN bFirst, ULONG & ulIndex, CStringW & Name, CStringW & Type);
void PRU_GetAllTypeObjects(CStringArray & Names, LPCWSTR szType);
BOOL PRU_NtQuerySecurityObject(HANDLE hObject, SECURITY_INFORMATION si, CByteArray & Bytes);
BOOL PRU_CloseHandle(HANDLE hObject);

//comctl32
BOOL PRU_CheckCommonControlsVersionMinimum(DWORD dwMajorVer, DWORD dwMinorVer); //and kernel32

//advapi32
#include <winsvc.h>
class ServiceManager
{
public:
	ServiceManager() { m_hSCM = NULL; OpenServiceManager(); }
	virtual ~ServiceManager() { CloseServiceManager(); }
	BOOL OpenServiceManager();
	void CloseServiceManager();
	SC_HANDLE GetHandle() { return m_hSCM; }
private:
	SC_HANDLE m_hSCM;
};

BOOL PRU_SetPrivilege(PCTSTR name, BOOL bEnable);
BOOL PRU_FriendlyNameFromSid(PSID psid, CString & Name);
BOOL PRU_GetOwner(LPTSTR lpszPath, SE_OBJECT_TYPE ObjectType, PSID* outpsid);//and kernel32
BOOL PRU_TakeOwnership(LPTSTR lpszPath, SE_OBJECT_TYPE ObjectType, BOOL bRecurse, PSID psid = NULL);
BOOL PRU_RegOpenKey(HKEY hBaseKey, LPCTSTR szRegistryPath, HKEY & hKey);
BOOL PRU_RegCloseKey(HKEY & hKey);
BOOL PRU_WriteRegistryDWORD(HKEY hKey, LPCTSTR szRegistryValueName, DWORD dwValue);
BOOL PRU_WriteRegistryString(HKEY hKey, LPCTSTR szRegistryValueName,
							   CString ValueString, BOOL bExpandable);
BOOL PRU_WriteRegistryMultiString(HKEY hKey, LPCTSTR szRegistryValueName,
								  CStringArray & StringArray);
BOOL PRU_WriteRegistryBinary(HKEY hKey, LPCTSTR szRegistryValueName, CByteArray & ByteArray);
BOOL PRU_ReadRegistryBinary(HKEY hKey, LPCTSTR szRegistryValueName, CByteArray & ByteArray);
BOOL PRU_ReadRegistryMultiString(HKEY hKey, LPCTSTR szRegistryValueName,
								CStringArray & StringArray);
BOOL PRU_ReadRegistryString(HKEY hKey, LPCTSTR szRegistryValueName, CString & String);
BOOL PRU_ReadRegistryDWORD(HKEY hKey, LPCTSTR szRegistryValueName, DWORD & dwValue);
BOOL PRU_ReadRegistryBinary(HKEY hBaseKey, LPCTSTR szRegistryPath,
							LPCTSTR szRegistryValueName, CByteArray & ByteArray);
BOOL PRU_WriteRegistryDWORD(HKEY hBaseKey, LPCTSTR szRegistryPath,
							LPCTSTR szRegistryValueName, DWORD dwValue);
BOOL PRU_WriteRegistryString(HKEY hBaseKey, LPCTSTR szRegistryPath,
							 LPCTSTR szRegistryValueName,
							 CString ValueString, BOOL bExpandable);
BOOL PRU_WriteRegistryMultiString(HKEY hBaseKey, LPCTSTR szRegistryPath,
								  LPCTSTR szRegistryValueName,
								  CStringArray StringArray);
BOOL PRU_WriteRegistryBinary(HKEY hBaseKey, LPCTSTR szRegistryPath,
							LPCTSTR szRegistryValueName, CByteArray & ByteArray);
BOOL PRU_ReadRegistryMultiString(HKEY hBaseKey, LPCTSTR szRegistryPath,
									LPCTSTR szRegistryValueName,
									CStringArray & StringArray);
BOOL PRU_ReadRegistryString( HKEY hBaseKey, LPCTSTR szRegistryPath,
								LPCTSTR szRegistryValueName, CString & String);
BOOL PRU_ReadRegistryDWORD(	HKEY hBaseKey, LPCTSTR szRegistryPath,
								LPCTSTR szRegistryValueName, DWORD & dwValue);
BOOL PRU_RegistryEnumValues(HKEY hKey, CStringArray & Keys, BOOL bInitialize = TRUE);
BOOL PRU_RegistryEnumValues(HKEY hKey, CKVArray & KeyValues, BOOL bInitialize = TRUE);
BOOL PRU_RegistryEnumValues(HKEY hBaseKey, LPCTSTR szRegistryPath,
                               CKVArray & KeyValues, BOOL bInitialize = TRUE);
BOOL PRU_RegistryEnumKey(HKEY hKey, CStringArray & StringArray);
BOOL PRU_RegistryEnumKey(HKEY hBaseKey, LPCTSTR szRegistryPath, CStringArray & StringArray);
BOOL PRU_RegistryValueCount(HKEY hKey, DWORD &dwCount);
BOOL PRU_RegistryKeyCount(HKEY hKey, DWORD &dwCount);
BOOL PRU_RegistryKeyExists(HKEY hBaseKey, LPCTSTR szRegistryPath, BOOL& bExists);
BOOL PRU_RegDeleteKey(HKEY hKey, LPCTSTR lpSubKey);
BOOL PRU_RegDeleteTree(HKEY hBaseKey, LPCTSTR szRegistryPath);
BOOL PRU_RegDeleteValue(HKEY hKey, LPCTSTR lpValueName);
BOOL PRU_ComputeMemoryMD5(BYTE* pbBuffer, DWORD dwBufferLength, BYTE* pbMD5Hash);
BOOL PRU_ComputeFileMD5(CFile & File, BYTE* pbMD5Hash);

//shell32
void PRU_GetCommandLineLaunchArg(LPCTSTR szCmdLine, CString& String); //and kernel32
void PRU_GetShellIcon(LPCTSTR szIconPath, HICON& hIcon);

//setupapi
BOOL PRU_SetupDiGetDeviceRegistryProperty(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData, DWORD dwProperty, CByteArray & Bytes);
BOOL PRU_GetDeviceRegistryPropertyMultiString(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData, DWORD dwProperty, CStringArray & Strings);
BOOL PRU_GetDeviceRegistryPropertyString(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData, DWORD dwProperty, CString & String);
BOOL PRU_GetDeviceRegistryPropertyDWORD(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData, DWORD dwProperty, DWORD & dwValue);

typedef struct _CONFIGURATION_INFORMATION {

    //
    // This field indicates the total number of disks in the system.  This
    // number should be used by the driver to determine the name of new
    // disks.  This field should be updated by the driver as it finds new
    // disks.
    //

    ULONG DiskCount;                // Count of hard disks thus far
    ULONG FloppyCount;              // Count of floppy disks thus far
    ULONG CdRomCount;               // Count of CD-ROM drives thus far
    ULONG TapeCount;                // Count of tape drives thus far
    ULONG ScsiPortCount;            // Count of SCSI port adapters thus far
    ULONG SerialCount;              // Count of serial devices thus far
    ULONG ParallelCount;            // Count of parallel devices thus far

    //
    // These next two fields indicate ownership of one of the two IO address
    // spaces that are used by WD1003-compatable disk controllers.
    //

    BOOLEAN AtDiskPrimaryAddressClaimed;    // 0x1F0 - 0x1FF
    BOOLEAN AtDiskSecondaryAddressClaimed;  // 0x170 - 0x17F

    //
    // Indicates the structure version, as anything value belong this will have been added.
    // Use the structure size as the version.
    //

    ULONG Version;

    //
    // Indicates the total number of medium changer devices in the system.
    // This field will be updated by the drivers as it determines that
    // new devices have been found and will be supported.
    //

    ULONG MediumChangerCount;

} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

#define DRVO_UNLOAD_INVOKED             0x00000001
#define DRVO_LEGACY_DRIVER              0x00000002
#define DRVO_BUILTIN_DRIVER             0x00000004    // Driver objects for Hal, PnP Mgr
#define DRVO_REINIT_REGISTERED          0x00000008
#define DRVO_INITIALIZED                0x00000010
#define DRVO_BOOTREINIT_REGISTERED      0x00000020
#define DRVO_LEGACY_RESOURCES           0x00000040

#define LDRP_STATIC_LINK 0x00000002
#define LDRP_IMAGE_DLL 0x00000004
#define LDRP_LOAD_IN_PROGRESS 0x00001000
#define LDRP_UNLOAD_IN_PROGRESS 0x00002000
#define LDRP_ENTRY_PROCESSED 0x00004000
#define LDRP_ENTRY_INSERTED 0x00008000
#define LDRP_CURRENT_LOAD 0x00010000
#define LDRP_FAILED_BUILTIN_LOAD 0x00020000
#define LDRP_DONT_CALL_FOR_THREADS 0x00040000
#define LDRP_PROCESS_ATTACH_CALLED 0x00080000
#define LDRP_DEBUG_SYMBOLS_LOADED 0x00100000
#define LDRP_IMAGE_NOT_AT_BASE 0x00200000
#define LDRP_COR_IMAGE 0x00400000
#define LDRP_COR_OWNS_UNMAP 0x00800000
#define LDRP_SYSTEM_MAPPED 0x01000000
#define LDRP_IMAGE_VERIFYING 0x02000000
#define LDRP_DRIVER_DEPENDENT_DLL 0x04000000
#define LDRP_ENTRY_NATIVE 0x08000000
#define LDRP_REDIRECTED 0x10000000
#define LDRP_NON_PAGED_DEBUG_INFO 0x20000000
#define LDRP_MM_LOADED 0x40000000
#define LDRP_COMPAT_DATABASE_PROCESSED 0x80000000

//
// Define the major function codes for IRPs.
//


#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b

// allow use in driver and application build!
// 4/8 byte alignment required for all NT structures

#define MEM_EXECUTE_OPTION_DISABLE  0x1
#define MEM_EXECUTE_OPTION_ENABLE 0x2

//#define OBJ_INHERIT              Ox00000002L
#define OBJ_PERMANENT            0x00000010L
#define OBJ_EXCLUSIVE            0x00000020L
#define OBJ_CASE_INSENSITIVE     0x00000040L
#define OBJ_OPENIF               0x00000080L
//#define OBJ_VALID_ATTRIBUTES     0x000000F2L

#define DIRTYPESTRW L"Directory"
#define DIRROOTSTRW L"\\"
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

//typedef struct __RTL_USER_PROCESS_PARAMETERS { 
//        ULONG MaximumLength; // 0x0 
//        ULONG Length; 
//        ULONG Flags; 
//        ULONG DebugFlags; 
//        PVOID ConsoleHandle; //0x10
//        ULONG ConsoleFlags; 
//        HANDLE StdInputHandle; 
//        HANDLE StdOutputHandle; 
//        HANDLE StdErrorHandle; //0x20
//        UNICODE_STRING CurrentDirectoryPath; 
//        HANDLE CurrentDirectoryHandle; 
//        UNICODE_STRING DllPath; //0x30
//        UNICODE_STRING ImagePathName; 
//        UNICODE_STRING CommandLine; //0x40
//        PVOID Environment;
//        ULONG StartingPositionLeft; 
//        ULONG StartingPositionTop; //0x50
//        ULONG Width; 
//        ULONG Height; 
//        ULONG CharWidth; 
//        ULONG CharHeight; //0x60
//        ULONG ConsoleTextAttributes; 
//        ULONG WindowFlags; 
//        ULONG ShowWindowFlags; 
//        UNICODE_STRING WindowTitle; //0x70
//        UNICODE_STRING DesktopName; 
//        UNICODE_STRING ShellInfo; //0x80
//        UNICODE_STRING RuntimeData; 
//        RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20]; //0x90
//} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS; 

/*typedef enum _PROCESSINFOCLASS { 
    ProcessBasiCEnumModel = 0, 
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

typedef enum ___OBJECT_INFORMATION_CLASS {
    _ObjectBasicInformation,
    _ObjectNameInformation,
    _ObjectTypeInformation,
    _ObjectAllTypesInformation,
    _ObjectHandleInformation
} __OBJECT_INFORMATION_CLASS;

#define STATUS_BUFFER_OVERFLOW ((NTSTATUS)0x80000005)
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef enum ___SYSTEM_INFORMATION_CLASS {
	_SystemBasicInformation,				// 0
	SystemProcessorInformation,				// 1
	_SystemPerformanceInformation,			// 2
	_SystemTimeOfDayInformation,			// 3
	SystemNotImplemented1,					// 4
	SystemProcessesAndThreadsInformation,	// 5
	SystemCallCounts,						// 6
	SystemConfigurationInformation,			// 7
	SystemProcessorTimes,					// 8
	SystemGlobalFlag,						// 9
	SystemNotImplemented2,					// 10
	SystemModuleInformation,				// 11
	SystemLockInformation,					// 12
	SystemNotImplemented3,					// 13
	SystemNotImplemented4,					// 14
	SystemNotImplemented5,					// 15
	SystemHandleInformation,				// 16
	SystemObjectInformation,				// 17
	SystemPagefileInformation,				// 18
	SystemInstructionEmulationCounts,		// 19
	SystemInvalidInfoClass1,				// 20
	SystemCacheInformation,					// 21
	SystemPoolTagInformation,				// 22
	SystemProcessorStatistics,				// 23
	SystemDpCEnumModel,					// 24
	SystemNotImplemented6,					// 25
	SystemLoadImage,						// 26
	SystemUnloadImage,						// 27
	SystemTimeAdjustment,					// 28
	SystemNotImplemented7,					// 29
	SystemNotImplemented8,					// 30
	SystemNotImplemented9,					// 31
	SystemCrashDumpInformation,				// 32
	_SystemExceptionInformation,			// 33
	SystemCrashDumpStateInformation,		// 34
	SystemKernelDebuggerInformation,		// 35
	SystemContextSwitchInformation,			// 36
	_SystemRegistryQuotaInformation,		// 37
	SystemLoadAndCallImage,					// 38
	SystemPrioritySeparation,				// 39
	SystemNotImplemented10,					// 40
	SystemNotImplemented11,					// 41
	SystemInvalidInfoClass2,				// 42
	SystemInvalidInfoClass3,				// 43
	SystemTimeZoneInformation,				// 44
	_SystemLookasideInformation,			// 45
	SystemSetTimeSlipEvent,					// 46
	SystemCreateSession,					// 47
	SystemDeleteSession,					// 48
	SystemInvalidInfoClass4,				// 49
	SystemRangeStartInformation,			// 50
	SystemVerifierInformation,				// 51
	SystemAddVerifier,						// 52
	SystemSessionProcessesInformation		// 53
} __SYSTEM_INFORMATION_CLASS;

typedef struct _FILE_NAME_INFORMATION {                     
    ULONG FileNameLength;                                   
    WCHAR FileName[1];                                      
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;           

typedef enum ___FILE_INFORMATION_CLASS {
// end_wdm
    _FileDirectoryInformation       = 1,
    FileFullDirectoryInformation,   // 2
    FileBothDirectoryInformation,   // 3
    FileBasicInformation,           // 4  wdm
    FileStandardInformation,        // 5  wdm
    FileInternalInformation,        // 6
    FileEaInformation,              // 7
    FileAccessInformation,          // 8
    FileNameInformation,            // 9
    FileRenameInformation,          // 10
    FileLinkInformation,            // 11
    FileNamesInformation,           // 12
    FileDispositionInformation,     // 13
    FilePositionInformation,        // 14 wdm
    FileFullEaInformation,          // 15
    FileModeInformation,            // 16
    FileAlignmentInformation,       // 17
    FileAllInformation,             // 18
    FileAllocationInformation,      // 19
    FileEndOfFileInformation,       // 20 wdm
    FileAlternateNameInformation,   // 21
    FileStreamInformation,          // 22
    FilePipeInformation,            // 23
    FilePipeLocalInformation,       // 24
    FilePipeRemoteInformation,      // 25
    FileMailslotQueryInformation,   // 26
    FileMailslotSetInformation,     // 27
    FileCompressionInformation,     // 28
    FileObjectIdInformation,        // 29
    FileCompletionInformation,      // 30
    FileMoveClusterInformation,     // 31
    FileQuotaInformation,           // 32
    FileReparsePointInformation,    // 33
    FileNetworkOpenInformation,     // 34
    FileAttributeTagInformation,    // 35
    FileTrackingInformation,        // 36
    FileIdBothDirectoryInformation, // 37
    FileIdFullDirectoryInformation, // 38
    FileMaximumInformation
// begin_wdm
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
#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

#define QUERY_STATE 1 // query state for timer, mutex, etc

class CAPILibrary
{
public:
	//should allow version with indexes of functions used for large libraries
	CAPILibrary(LPCTSTR szLibName, LPCSTR* pszaArgs)
	{
		DWORD dwCount = 0;
		HMODULE hModule;
		m_hModule = NULL;
		//GetModuleHandle creates thread safety issues as DLL can unload in other thread
		//  can only use GetModuleHandle in DLLs loaded on startup which are not enumerated currently
		if (//(hModule = PRU_GetModuleHandle(szLibName)) != NULL ||
			(hModule = m_hModule = PRU_LoadLibrary(szLibName)) != NULL) {
		}
		while (pszaArgs[dwCount]) {
			m_Functions.Add(hModule ? PRU_GetProcAddress(hModule, pszaArgs[dwCount]) : NULL);
			dwCount++;
		}
	}
	~CAPILibrary()
	{
		if (m_hModule) PRU_FreeLibrary(m_hModule);
	}
protected:
	CArray<FARPROC> m_Functions;
	HMODULE m_hModule;
};

#define MAKEAPILIB(cname) cname() : CAPILibrary(m_szLibName, m_pszaArgs) { }\
	private:\
	static LPCTSTR m_szLibName;\
	static LPCSTR m_pszaArgs[];

#define MAKEGETFUNC(func, index) inline func Get##func() { return (func)m_Functions[index]; }

//undocumented ntdll.dll
typedef NTSTATUS (NTAPI *_NtOpenDirectoryObject)(PHANDLE, ACCESS_MASK,
												  POBJECT_ATTRIBUTES);
typedef NTSTATUS (NTAPI *_NtQueryDirectoryObject)(HANDLE, PVOID, ULONG,
												   DIRECTORYINFOCLASS,
												   BOOLEAN, PULONG, PULONG);
typedef NTSTATUS (NTAPI *_NtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK,
												   PVOID, ULONG,
												   FILE_INFORMATION_CLASS);
typedef NTSTATUS (NTAPI *_NtQuerySecurityObject)(HANDLE, SECURITY_INFORMATION,
												 PSECURITY_DESCRIPTOR, ULONG, PULONG);
typedef NTSTATUS (NTAPI *_NtResumeProcess)(HANDLE);
typedef NTSTATUS (NTAPI *_NtSuspendProcess)(HANDLE);
//ntdll.dll from Winternl.h
typedef NTSTATUS (NTAPI *_NtQueryObject)(HANDLE, OBJECT_INFORMATION_CLASS,
										  PVOID, ULONG, PULONG);
typedef NTSTATUS (NTAPI *_NtQuerySystemInformation)(ULONG, PVOID,
													 ULONG, PULONG);
typedef NTSTATUS (NTAPI *_NtQueryInformationProcess)(HANDLE,
													  PROCESSINFOCLASS, PVOID,
													  ULONG, PULONG);
typedef VOID (NTAPI *_RtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
typedef VOID (NTAPI *_RtlFreeUnicodeString)(PUNICODE_STRING);

class CAPINTDLL : public CAPILibrary
{
public:
	MAKEGETFUNC(_NtQueryObject, 0)
	MAKEGETFUNC(_NtQuerySystemInformation, 1)
	MAKEGETFUNC(_NtQueryInformationProcess, 2)
	MAKEGETFUNC(_NtQuerySecurityObject, 3)
	MAKEGETFUNC(_NtOpenDirectoryObject, 4)
	MAKEGETFUNC(_NtQueryDirectoryObject, 5)
	MAKEGETFUNC(_NtQueryInformationFile, 6)
	MAKEGETFUNC(_NtSuspendProcess, 7)
	MAKEGETFUNC(_NtResumeProcess, 8)
	MAKEGETFUNC(_RtlInitUnicodeString, 9)
	MAKEGETFUNC(_RtlFreeUnicodeString, 10)
	MAKEAPILIB(CAPINTDLL)
};

//psapi.dll from psapi.h
typedef DWORD
(WINAPI
*_GetModuleFileNameExA)(
    __in HANDLE hProcess,
    __in_opt HMODULE hModule,
    __out_ecount(nSize) LPSTR lpFilename,
    __in DWORD nSize
    );

typedef DWORD
(WINAPI
*_GetModuleFileNameExW)(
    __in HANDLE hProcess,
    __in_opt HMODULE hModule,
    __out_ecount(nSize) LPWSTR lpFilename,
    __in DWORD nSize
    );

#ifdef UNICODE
#define _GetModuleFileNameEx  _GetModuleFileNameExW
#define LOOKUP_GetModuleFileNameEx "GetModuleFileNameExW"
#else
#define _GetModuleFileNameEx  _GetModuleFileNameExA
#define LOOKUP_GetModuleFileNameEx "GetModuleFileNameExA"
#endif

typedef BOOL
(WINAPI
*_EnumProcesses)(
    __out_bcount(cb) DWORD * lpidProcess,
    __in DWORD cb,
    __out LPDWORD lpcbNeeded
    );

typedef DWORD
(WINAPI
*_GetProcessImageFileNameA)(
    __in HANDLE hProcess,
    __out_ecount(nSize) LPSTR lpImageFileName,
    __in DWORD nSize
    );

typedef DWORD
(WINAPI
*_GetProcessImageFileNameW)(
    __in HANDLE hProcess,
    __out_ecount(nSize) LPWSTR lpImageFileName,
    __in DWORD nSize
    );

#ifdef UNICODE
#define _GetProcessImageFileName  _GetProcessImageFileNameW
#define LOOKUP_GetProcessImageFileName "GetProcessImageFileNameW"
#else
#define _GetProcessImageFileName  _GetProcessImageFileNameA
#define LOOKUP_GetProcessImageFileName "GetProcessImageFileNameA"
#endif

class CAPIPSAPI : public CAPILibrary
{
public:
	MAKEGETFUNC(_GetModuleFileNameEx, 0)
	MAKEGETFUNC(_EnumProcesses, 1)
	MAKEGETFUNC(_GetProcessImageFileName, 2)
	MAKEAPILIB(CAPIPSAPI)
};

//user32.dll from WinUser.h
typedef BOOL (WINAPI *_IsHungAppWindow)(HWND);

class CAPIUser32 : public CAPILibrary
{
public:
	MAKEGETFUNC(_IsHungAppWindow, 0)
	MAKEAPILIB(CAPIUser32)
};

//secur32.dll from Ntsecapi.h
typedef NTSTATUS (NTAPI *_LsaEnumerateLogonSessions)(PULONG, PLUID*);
typedef NTSTATUS (NTAPI *_LsaGetLogonSessionData)
										(PLUID, PSECURITY_LOGON_SESSION_DATA*);
typedef NTSTATUS (NTAPI *_LsaFreeReturnBuffer)(PVOID);

class CAPISecur32 : public CAPILibrary
{
public:
	MAKEGETFUNC(_LsaEnumerateLogonSessions, 0)
	MAKEGETFUNC(_LsaGetLogonSessionData, 1)
	MAKEGETFUNC(_LsaFreeReturnBuffer, 2)
	MAKEAPILIB(CAPISecur32)
};

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiGetDeviceRegistryPropertyA)(
    __in HDEVINFO DeviceInfoSet,
    __in PSP_DEVINFO_DATA DeviceInfoData,
    __in DWORD Property,
    __out_opt PDWORD PropertyRegDataType, 
    __out_bcount_opt(PropertyBufferSize) PBYTE PropertyBuffer,
    __in DWORD PropertyBufferSize,
    __out_opt PDWORD RequiredSize 
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiGetDeviceRegistryPropertyW)(
    __in HDEVINFO DeviceInfoSet,
    __in PSP_DEVINFO_DATA DeviceInfoData,
    __in DWORD Property,
    __out_opt PDWORD PropertyRegDataType,
    __out_bcount_opt(PropertyBufferSize) PBYTE PropertyBuffer,
    __in DWORD PropertyBufferSize,
    __out_opt PDWORD RequiredSize
    );

#ifdef UNICODE
#define _SetupDiGetDeviceRegistryProperty _SetupDiGetDeviceRegistryPropertyW
#define LOOKUP_SetupDiGetDeviceRegistryProperty "SetupDiGetDeviceRegistryPropertyW"
#else
#define _SetupDiGetDeviceRegistryProperty _SetupDiGetDeviceRegistryPropertyA
#define LOOKUP_SetupDiGetDeviceRegistryProperty "SetupDiGetDeviceRegistryPropertyA"
#endif

typedef __checkReturn
WINSETUPAPI
HDEVINFO
(WINAPI
*_SetupDiGetClassDevsA)(
    __in_opt CONST GUID *ClassGuid,
    __in_opt PCSTR Enumerator,
    __in_opt HWND hwndParent,
    __in DWORD Flags
    );

typedef __checkReturn
WINSETUPAPI
HDEVINFO
(WINAPI
*_SetupDiGetClassDevsW)(
    __in_opt CONST GUID *ClassGuid,
    __in_opt PCWSTR Enumerator,
    __in_opt HWND hwndParent,
    __in DWORD Flags
    );

#ifdef UNICODE
#define _SetupDiGetClassDevs _SetupDiGetClassDevsW
#define LOOKUP_SetupDiGetClassDevs "SetupDiGetClassDevsW"
#else
#define _SetupDiGetClassDevs _SetupDiGetClassDevsA
#define LOOKUP_SetupDiGetClassDevs "SetupDiGetClassDevsA"
#endif

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiEnumDeviceInfo)(
    __in HDEVINFO DeviceInfoSet,
    __in DWORD MemberIndex,
    __out PSP_DEVINFO_DATA DeviceInfoData
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiDestroyDeviceInfoList)(
    __in HDEVINFO DeviceInfoSet
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_pSetupInfIsInbox)(
    __in         PCWSTR         wszInfFileName,
    __out        PULONG			pulIsInbox
	);

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiRemoveDevice)(
    __in HDEVINFO DeviceInfoSet,
    __inout PSP_DEVINFO_DATA DeviceInfoData
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiGetDevicePropertyW)(
    __in         HDEVINFO         DeviceInfoSet,
    __in         PSP_DEVINFO_DATA DeviceInfoData,
    __in   CONST DEVPROPKEY      *PropertyKey,
    __out        DEVPROPTYPE     *PropertyType,
    __out_bcount_opt(PropertyBufferSize) PBYTE PropertyBuffer,
    __in         DWORD            PropertyBufferSize,
    __out_opt    PDWORD           RequiredSize,
    __in         DWORD            Flags
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiGetDeviceInstallParamsA)(
    __in HDEVINFO DeviceInfoSet,
    __in_opt PSP_DEVINFO_DATA DeviceInfoData,
    __out PSP_DEVINSTALL_PARAMS_A DeviceInstallParams
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiGetDeviceInstallParamsW)(
    __in HDEVINFO DeviceInfoSet,
    __in_opt PSP_DEVINFO_DATA DeviceInfoData,
    __out PSP_DEVINSTALL_PARAMS_W DeviceInstallParams
    );

#ifdef UNICODE
#define _SetupDiGetDeviceInstallParams _SetupDiGetDeviceInstallParamsW
#define LOOKUP_SetupDiGetDeviceInstallParams "SetupDiGetDeviceInstallParamsW"
#else
#define _SetupDiGetDeviceInstallParams _SetupDiGetDeviceInstallParamsA
#define LOOKUP_SetupDiGetDeviceInstallParams "SetupDiGetDeviceInstallParamsA"
#endif

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiSetClassInstallParamsA)(
    __in HDEVINFO DeviceInfoSet,
    __in_opt PSP_DEVINFO_DATA DeviceInfoData,
    __in_bcount_opt(ClassInstallParamsSize) PSP_CLASSINSTALL_HEADER ClassInstallParams,
    __in DWORD ClassInstallParamsSize
    );
typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiSetClassInstallParamsW)(
    __in HDEVINFO DeviceInfoSet,
    __in_opt PSP_DEVINFO_DATA DeviceInfoData,
    __in_bcount_opt(ClassInstallParamsSize) PSP_CLASSINSTALL_HEADER ClassInstallParams,
    __in DWORD ClassInstallParamsSize
    );

#ifdef UNICODE
#define _SetupDiSetClassInstallParams _SetupDiSetClassInstallParamsW
#define LOOKUP_SetupDiSetClassInstallParams "SetupDiSetClassInstallParamsW"
#else
#define _SetupDiSetClassInstallParams _SetupDiSetClassInstallParamsA
#define LOOKUP_SetupDiSetClassInstallParams "SetupDiSetClassInstallParamsA"
#endif

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupDiCallClassInstaller)(
    __in DI_FUNCTION InstallFunction,
    __in HDEVINFO DeviceInfoSet,
    __in_opt PSP_DEVINFO_DATA DeviceInfoData
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupGetInfInformationA)(
    __in LPCVOID InfSpec,
    __in DWORD SearchControl,
    __out_bcount_opt(ReturnBufferSize) PSP_INF_INFORMATION ReturnBuffer, 
    __in DWORD ReturnBufferSize,
    __out_opt PDWORD RequiredSize
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupGetInfInformationW)(
    __in LPCVOID InfSpec,
    __in DWORD SearchControl,
    __out_bcount_opt(ReturnBufferSize) PSP_INF_INFORMATION ReturnBuffer,
    __in DWORD ReturnBufferSize,
    __out_opt PDWORD RequiredSize
    );

#ifdef UNICODE
#define _SetupGetInfInformation _SetupGetInfInformationW
#define LOOKUP_SetupGetInfInformation "SetupGetInfInformationW"
#else
#define _SetupGetInfInformation _SetupGetInfInformationA
#define LOOKUP_SetupGetInfInformation "SetupGetInfInformationA"
#endif

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupQueryInfVersionInformationA)(
    __in PSP_INF_INFORMATION InfInformation,
    __in UINT InfIndex,
    __in_opt PCSTR Key,
    __out_ecount_opt(ReturnBufferSize) PSTR ReturnBuffer,
    __in DWORD ReturnBufferSize,
    __out_opt PDWORD RequiredSize
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupQueryInfVersionInformationW)(
    __in PSP_INF_INFORMATION InfInformation,
    __in UINT InfIndex,
    __in_opt PCWSTR Key,
    __out_ecount_opt(ReturnBufferSize) PWSTR ReturnBuffer,
    __in DWORD ReturnBufferSize,
    __out_opt PDWORD RequiredSize
    );

#ifdef UNICODE
#define _SetupQueryInfVersionInformation _SetupQueryInfVersionInformationW
#define LOOKUP_SetupQueryInfVersionInformation "SetupQueryInfVersionInformationW"
#else
#define _SetupQueryInfVersionInformation _SetupQueryInfVersionInformationA
#define LOOKUP_SetupQueryInfVersionInformation "SetupQueryInfVersionInformationA"
#endif

typedef WINSETUPAPI
HINF
(WINAPI
*_SetupOpenInfFileW)(
    __in PCWSTR FileName,
    __in_opt PCWSTR InfClass,
    __in DWORD InfStyle,
    __out_opt PUINT ErrorLine
    );

typedef WINSETUPAPI
HINF
(WINAPI
*_SetupOpenInfFileA)(
    __in PCSTR FileName,
    __in_opt PCSTR InfClass,
    __in DWORD InfStyle,
    __out_opt PUINT ErrorLine
    );

#ifdef UNICODE
#define _SetupOpenInfFile _SetupOpenInfFileW
#define LOOKUP_SetupOpenInfFile "SetupOpenInfFileW"
#else
#define _SetupOpenInfFile _SetupOpenInfFileA
#define LOOKUP_SetupOpenInfFile "SetupOpenInfFileA"
#endif

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupFindFirstLineA)(
    __in HINF InfHandle,
    __in PCSTR Section,
    __in_opt PCSTR Key,
    __out PINFCONTEXT Context
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupFindFirstLineW)(
    __in HINF InfHandle,
    __in PCWSTR Section,
    __in_opt PCWSTR Key,
    __out PINFCONTEXT Context
    );

#ifdef UNICODE
#define _SetupFindFirstLine _SetupFindFirstLineW
#define LOOKUP_SetupFindFirstLine "SetupFindFirstLineW"
#else
#define _SetupFindFirstLine _SetupFindFirstLineA
#define LOOKUP_SetupFindFirstLine "SetupFindFirstLineA"
#endif

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupGetStringFieldA)(
    __in PINFCONTEXT Context,
    __in DWORD FieldIndex,
    __out_ecount_opt(ReturnBufferSize) PSTR ReturnBuffer,
    __in DWORD ReturnBufferSize,
    __out_opt PDWORD RequiredSize
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupGetStringFieldW)(
    __in PINFCONTEXT Context,
    __in DWORD FieldIndex,
    __out_ecount_opt(ReturnBufferSize) PWSTR ReturnBuffer,
    __in DWORD ReturnBufferSize,
    __out_opt PDWORD RequiredSize
    );

#ifdef UNICODE
#define _SetupGetStringField _SetupGetStringFieldW
#define LOOKUP_SetupGetStringField "SetupGetStringFieldW"
#else
#define _SetupGetStringField _SetupGetStringFieldA
#define LOOKUP_SetupGetStringField "SetupGetStringFieldA"
#endif

typedef WINSETUPAPI
VOID
(WINAPI
*_SetupCloseInfFile)(
    __in HINF InfHandle
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupUninstallOEMInfA)(
    __in PCSTR InfFileName,
    __in DWORD Flags,
    __reserved PVOID Reserved
    );

typedef WINSETUPAPI
BOOL
(WINAPI
*_SetupUninstallOEMInfW)(
    __in PCWSTR InfFileName,
    __in DWORD Flags,
    __reserved PVOID Reserved
    );

#ifdef UNICODE
#define _SetupUninstallOEMInf _SetupUninstallOEMInfW
#define LOOKUP_SetupUninstallOEMInf "SetupUninstallOEMInfW"
#else
#define _SetupUninstallOEMInf _SetupUninstallOEMInfA
#define LOOKUP_SetupUninstallOEMInf "SetupUninstallOEMInfA"
#endif

#include <cfgmgr32.h>

typedef CMAPI
CONFIGRET
(WINAPI
*_CM_Get_DevNode_Status)(
    __out PULONG        pulStatus,
    __out PULONG        pulProblemNumber,
    __in  DEVINST       dnDevInst,
    __in  ULONG         ulFlags
    );

class CAPISetup : public CAPILibrary
{
public:
	MAKEGETFUNC(_CM_Get_DevNode_Status, 0)
	MAKEGETFUNC(_SetupDiGetDeviceRegistryProperty, 1)
	MAKEGETFUNC(_SetupDiGetClassDevs, 2)
	MAKEGETFUNC(_SetupDiEnumDeviceInfo, 3)
	MAKEGETFUNC(_SetupDiDestroyDeviceInfoList, 4)
	MAKEGETFUNC(_pSetupInfIsInbox, 5)
	MAKEGETFUNC(_SetupDiRemoveDevice, 6)
	MAKEGETFUNC(_SetupDiGetDevicePropertyW, 7)
	MAKEGETFUNC(_SetupDiGetDeviceInstallParams, 8)
	MAKEGETFUNC(_SetupDiSetClassInstallParams, 9)
	MAKEGETFUNC(_SetupDiCallClassInstaller, 10)
	MAKEGETFUNC(_SetupGetInfInformation, 11)
	MAKEGETFUNC(_SetupQueryInfVersionInformation, 12)
	MAKEGETFUNC(_SetupOpenInfFile, 13)
	MAKEGETFUNC(_SetupFindFirstLine, 14)
	MAKEGETFUNC(_SetupGetStringField, 15)
	MAKEGETFUNC(_SetupCloseInfFile, 16)
	MAKEGETFUNC(_SetupUninstallOEMInf, 17)
	MAKEAPILIB(CAPISetup)
};

#include "DriverInterface.h"

class CDriverUse
{
public:
	CDriverUse()
	{
		//SeLoadDriverPrivilege if using NtLoadDriver
		if (!PRU_SetPrivilege(SE_BACKUP_NAME, TRUE))
			AddTraceLog(_T("MyCall=EnablePrivilege Privilege=")
						SE_BACKUP_NAME _T(" Error\r\n"));
		if (!PRU_SetPrivilege(SE_SECURITY_NAME, TRUE))
			AddTraceLog(_T("MyCall=EnablePrivilege Privilege=")
						SE_SECURITY_NAME _T(" Error\r\n"));
		if (!PRU_SetPrivilege(SE_DEBUG_NAME, TRUE))
			AddTraceLog(_T("MyCall=EnablePrivilege Privilege=")
						SE_DEBUG_NAME _T(" Error\r\n"));
		if (!PRU_SetPrivilege(SE_TAKE_OWNERSHIP_NAME, TRUE))
			AddTraceLog(_T("MyCall=EnablePrivilege Privilege=")
						SE_TAKE_OWNERSHIP_NAME _T(" Error\r\n"));
		if (!PRU_SetPrivilege(SE_TCB_NAME, TRUE)) //grant SE_TCB_NAME if necessary
			AddTraceLog(_T("MyCall=EnablePrivilege Privilege=")
						SE_TCB_NAME _T(" Error\r\n"));
		UnloadDriver(GPD_DOSDEVICE, GPD_DOSDEVICEGLOBAL, GPD_PESTRIDSERVICENAME);
	}
	~CDriverUse()
	{
		UnloadDriver(GPD_DOSDEVICE, GPD_DOSDEVICEGLOBAL, GPD_PESTRIDSERVICENAME);
		PRU_SetPrivilege(SE_BACKUP_NAME, FALSE);
		PRU_SetPrivilege(SE_SECURITY_NAME, FALSE);
		PRU_SetPrivilege(SE_DEBUG_NAME, FALSE);
		PRU_SetPrivilege(SE_TAKE_OWNERSHIP_NAME, FALSE);
		PRU_SetPrivilege(SE_TCB_NAME, FALSE);
	}
};