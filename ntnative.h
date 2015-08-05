/*	
	NT Native API header file
	(c) 2004, 2005 EP_X0FF
	Ring0 - the source of inspiration...
*/
#ifndef _ntnative_
#define _ntnative_

#define ANSI_STRING STRING
#define PANSI_STRING PSTRING
#define DUPLICATE_SAME_ATTRIBUTES 0x00000003L 

#define NTAPI	__stdcall
#define CCALL   __cdecl
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

//object attributes
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L

//RtlDebugBuffer consts
#define PDI_MODULES 0x01 // The loaded modules of the process
#define PDI_BACKTRACE 0x02 // The heap stack back traces
#define PDI_HEAPS 0x04 // The heaps of the process
#define PDI_HEAP_TAGS 0x08 // The heap tags
#define PDI_HEAP_BLOCKS 0x10 // The heap blocks
#define PDI_LOCKS 0x20

//privilegies
#define SE_CREATE_TOKEN_PRIVILEGE         (2L)
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   (3L)
#define SE_LOCK_MEMORY_PRIVILEGE          (4L)
#define SE_INCREASE_QUOTA_PRIVILEGE       (5L)
#define SE_MACHINE_ACCOUNT_PRIVILEGE      (6L)
#define SE_UNSOLICITED_INPUT_PRIVILEGE    (6L)
#define SE_TCB_PRIVILEGE                  (7L)
#define SE_SECURITY_PRIVILEGE             (8L)
#define SE_TAKE_OWNERSHIP_PRIVILEGE       (9L)
#define SE_LOAD_DRIVER_PRIVILEGE          (10L)
#define SE_SYSTEM_PROFILE_PRIVILEGE       (11L)
#define SE_SYSTEMTIME_PRIVILEGE           (12L)
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE  (13L)
#define SE_INC_BASE_PRIORITY_PRIVILEGE    (14L)
#define SE_CREATE_PAGEFILE_PRIVILEGE      (15L)
#define SE_CREATE_PERMANENT_PRIVILEGE     (16L)
#define SE_BACKUP_PRIVILEGE               (17L)
#define SE_RESTORE_PRIVILEGE              (18L)
#define SE_SHUTDOWN_PRIVILEGE             (19L)
#define SE_DEBUG_PRIVILEGE                (20L)
#define SE_AUDIT_PRIVILEGE                (21L)
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE   (22L)
#define SE_CHANGE_NOTIFY_PRIVILEGE        (23L)
#define SE_REMOTE_SHUTDOWN_PRIVILEGE      (24L)
#define SE_UNDOCK_PRIVILEGE               (25L)
#define SE_SYNC_AGENT_PRIVILEGE           (26L)
#define SE_ENABLE_DELEGATION_PRIVILEGE    (27L)
#define SE_MANAGE_VOLUME                  (28L)

typedef LONG	NTSTATUS;
typedef LONG	KPRIORITY;
typedef ULONG	KAFFINITY, *PKAFFINITY; 

//ZwQuerySystemInformation enum type
typedef enum _SYSTEM_INFORMATION_CLASS {           //Query Set
	SystemBasicInformation,                        //  0 Y N
	SystemProcessorInformation,                    //  1 Y N
	SystemPerformanceInformation,                  //  2 Y N
	SystemTimeOfDayInformation,                    //  3 Y N
	SystemPathInformation,                         //  4 Y N
	SystemProcessesAndThreadsInformation,          //  5 Y N
	SystemCallCounts,                              //  6 Y N
	SystemConfigurationInformation,                //  7 Y N
	SystemProcessorTimes,                          //  8 Y N
	SystemGlobalFlag,                              //  9 Y Y
	SystemCallTimeInformation,                     // 10 Y N
	SystemModuleInformation,                       // 11 Y N
	SystemLockInformation,                         // 12 Y N
	SystemStackTraceInformation,                   // 13 Y N
	SystemPagedPoolInformation,                    // 14 Y N
	SystemNonPagedPoolInformation,                 // 15 Y N
	SystemHandleInformation,                       // 16 Y N
	SystemObjectInformation,                       // 17 Y N
	SystemPagefileInformation,                     // 18 Y N
	SystemInstructionEmulationCounts,              // 19 Y N
	SystemVdmBopInformation,                       // 20
	SystemCacheInformation,                        // 21 Y Y
	SystemPoolTagInformation,                      // 22 Y N
	SystemProcessorStatistics,                     // 23 Y N
	SystemDpcInformation,                          // 24 Y Y
	SystemMemoryUsageInformation,                  // 25 Y N
	SystemLoadImage,                               // 26 N Y
	SystemUnloadImage,                             // 27 N Y
	SystemTimeAdjustment,                          // 28 Y Y
	SystemPoolBlocksInformation,                   // 29 Y N
	SystemNextEventIdInformation,                  // 30 Y N
	SystemEventIdsInformation,                     // 31 Y N
	SystemCrashDumpInformation,                    // 32 Y N
	SystemExceptionInformation,                    // 33 Y N
	SystemCrashDumpStateInformation,               // 34 Y Y/N
	SystemKernelDebuggerInformation,               // 35 Y N
	SystemContextSwitchInformation,                // 36 Y N
	SystemRegistryQuotaInformation,                // 37 Y Y
	SystemLoadAndCallImage,                        // 38 N Y
	SystemPrioritySeparation,                      // 39 N Y
	SystemPlugPlayBusInformation,                  // 40 Y N
	SystemDockInformation,                         // 41 Y N
	SystemPowerInformation2,                       // 42
	SystemProcessorSpeedInformation,               // 43
	SystemTimeZoneInformation,                     // 44 Y N
	SystemLookasideInformation,                    // 45 Y N
	SystemSetTimeSlipEvent,                        // 46 N Y
	SystemCreateSession,                           // 47 N Y
	SystemDeleteSession,                           // 48 N Y
	SystemInvalidInfoClass4,                       // 49
	SystemRangeStartInformation,                   // 50 Y N //sizeof = 4
	SystemVerifierInformation,                     // 51 Y Y
	SystemAddVerifier,                             // 52 N Y
	SystemSessionProcessesInformation,             // 53 Y N
	SystemInformation54,                           // 54
	SystemNumaNodeInformation,                     // 55 Y   //GetNumaProcessorNode,...
	SystemInformation56,                           // 56
	SystemUnknownInformation57,                    // 57 Y
	SystemInformation58,                           // 58
	SystemComPlusPackageInstallStatusInformation,  // 59 Y	Y  //sizeof = 4, GetComPlusPackageInstallStatus
	SystemNumaMemoryInformation,                   // 60 Y     //sizeof = 0x88, GetNumaAvailableMemoryNode
	SystemInformation61,                           // 61
	SystemInformation62,                           // 62
	SystemInformation63,                           // 63
	SystemInformation64,                           // 64
	SystemInformation65,                           // 65
	SystemInformation66,                           // 66
	SystemInformation67,                           // 67
	SystemInformation68,                           // 68
	SystemInformation69,                           // 69   Y
	SystemInformation70,                           // 70
	SystemInformation71,                           // 71
	SystemInformation72,                           // 72
	SystemLogicalProcessorInformation              // 73 Y
} SYSTEM_INFORMATION_CLASS;

typedef enum _THREADINFOCLASS {          //Query Set
	ThreadBasicInformation,          // 0  Y N
	ThreadTimes,                     // 1  Y N
	ThreadPriority,                  // 2  N Y
	ThreadBasePriority,              // 3  N Y
	ThreadAffinityMask,              // 4  N Y
	ThreadImpersonationToken,        // 5  N Y
	ThreadDescriptorTableEntry,      // 6  Y N
	ThreadEnableAlignmentFaultFixup, // 7  N Y
	ThreadEventPair,                 // 8  N Y
	ThreadQuerySetWin32StartAddress, // 9  Y Y
	ThreadZeroTlsCell,               // 10 N Y
	ThreadPerformanceCount,          // 11 Y N
	ThreadAmILastThread,             // 12 Y N
	ThreadIdealProcessor,            // 13 N Y
	ThreadPriorityBoost,             // 14 Y Y
	ThreadSetTlsArrayAddress,        // 15 N Y
	ThreadIsIoPending,               // 16 Y N
	ThreadHideFromDebugger,          // 17 N Y
	ThreadIsCriticalInformation      // 18 Y Y //RtlSetThreadIsCritical, sizeof = 4, XP
} THREADINFOCLASS;

typedef enum _FILE_INFORMATION_CLASS 
{ 
	FileDirectoryInformation = 1, 
	FileFullDirectoryInformation, // 2 
	FileBothDirectoryInformation, // 3 
	FileBasicInformation, // 4 
	FileStandardInformation, // 5 
	FileInternalInformation, // 6 
	FileEaInformation, // 7 
	FileAccessInformation, // 8 
	FileNameInformation, // 9 
	FileRenameInformation, // 10 
	FileLinkInformation, // 11 
	FileNamesInformation, // 12 
	FileDispositionInformation, // 13 
	FilePositionInformation, // 14 
	FileFullEaInformation, // 15 
	FileModeInformation, // 16 
	FileAlignmentInformation, // 17 
	FileAllInformation, // 18 
	FileAllocationInformation, // 19 
	FileEndOfFileInformation, // 20 
	FileAlternateNameInformation, // 21 
	FileStreamInformation, // 22 
	FilePipeInformation, // 23 
	FilePipeLocalInformation, // 24 
	FilePipeRemoteInformation, // 25 
	FileMailslotQueryInformation, // 26 
	FileMailslotSetInformation, // 27 
	FileCompressionInformation, // 28 
	FileObjectIdInformation, // 29 
	FileCompletionInformation, // 30 
	FileMoveClusterInformation, // 31 
	FileInformationReserved32, // 32 
	FileInformationReserved33, // 33 
	FileNetworkOpenInformation, // 34 
	FileMaximumInformation 
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS; 

typedef enum {
	AdjustCurrentProcess,
	AdjustCurrentThread
} ADJUST_PRIVILEGE_TYPE;

typedef struct _SERIAL_COMMPROP {
    USHORT PacketLength;
    USHORT PacketVersion;
    ULONG ServiceMask;
    ULONG Reserved1;
    ULONG MaxTxQueue;
    ULONG MaxRxQueue;
    ULONG MaxBaud;
    ULONG ProvSubType;
    ULONG ProvCapabilities;
    ULONG SettableParams;
    ULONG SettableBaud;
    USHORT SettableData;
    USHORT SettableStopParity;
    ULONG CurrentTxQueue;
    ULONG CurrentRxQueue;
    ULONG ProvSpec1;
    ULONG ProvSpec2;
    WCHAR ProvChar[1];
} SERIAL_COMMPROP,*PSERIAL_COMMPROP;

typedef struct _UNICODE_STRING {
	WORD  Length;
	WORD  MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _CLIENT_ID {
    DWORD	    UniqueProcess;
    DWORD	    UniqueThread;
} CLIENT_ID, * PCLIENT_ID;
/*
typedef struct _IO_COUNTERS {
	ULONGLONG ReadOperationCount;  
	ULONGLONG WriteOperationCount;  
	ULONGLONG OtherOperationCount;  
	ULONGLONG ReadTransferCount;  
	ULONGLONG WriteTransferCount;  
	ULONGLONG OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;*/

typedef struct _VM_COUNTERS { // Information Class 3
	ULONG PeakVirtualSize;
	ULONG VirtualSize;
	ULONG PageFaultCount;
	ULONG PeakWorkingSetSize;
	ULONG WorkingSetSize;
	ULONG QuotaPeakPagedPoolUsage;
	ULONG QuotaPagedPoolUsage;
	ULONG QuotaPeakNonPagedPoolUsage;
	ULONG QuotaNonPagedPoolUsage;
	ULONG PagefileUsage;
	ULONG PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER   KernelTime;
    LARGE_INTEGER   UserTime;
    LARGE_INTEGER   CreateTime;
    ULONG			WaitTime;
    PVOID			StartAddress;
    CLIENT_ID	    ClientId;
    KPRIORITY	    Priority;
    KPRIORITY	    BasePriority;
    ULONG			ContextSwitchCount;
    LONG			State;
    LONG			WaitReason;
} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;

// Note, that the size of the SYSTEM_PROCESS_INFORMATION structure is 
// different on NT 4 and Win2K.

typedef struct _SYSTEM_PROCESS_INFORMATION_NT4 {
    ULONG			NextEntryDelta;
    ULONG			ThreadCount;
    ULONG			Reserved1[6];
    LARGE_INTEGER   CreateTime;
    LARGE_INTEGER   UserTime;
    LARGE_INTEGER   KernelTime;
    UNICODE_STRING  ProcessName;
    KPRIORITY	    BasePriority;
    ULONG			ProcessId;
    ULONG			InheritedFromProcessId;
    ULONG			HandleCount;
    ULONG			Reserved2[2];
    VM_COUNTERS	    VmCounters;
    SYSTEM_THREAD_INFORMATION  Threads[1];
} SYSTEM_PROCESS_INFORMATION_NT4, * PSYSTEM_PROCESS_INFORMATION_NT4;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG			NextEntryDelta;
    ULONG			ThreadCount;
    ULONG			Reserved1[6];
    LARGE_INTEGER   CreateTime;
    LARGE_INTEGER   UserTime;
    LARGE_INTEGER   KernelTime;
    UNICODE_STRING  ProcessName;
    KPRIORITY	    BasePriority;
    ULONG			ProcessId;
    ULONG			InheritedFromProcessId;
    ULONG			HandleCount;
    ULONG			Reserved2[2];
    VM_COUNTERS	    VmCounters;
    IO_COUNTERS	    IoCounters;
    SYSTEM_THREAD_INFORMATION  Threads[1];
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

typedef struct _SYSTEM_PAGEFILE_INFORMATION {
    ULONG            NextEntryOffset;   // offset to the next entry
    ULONG            CurrentSize;       // current file size
    ULONG            TotalUsed;         // current file usage
    ULONG            PeakUsed;          // peak file usage
    UNICODE_STRING   FileName;          // file name in native format
} SYSTEM_PAGEFILE_INFORMATION, * PSYSTEM_PAGEFILE_INFORMATION;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef struct _SYSTEM_HANDLE_INFORMATION
{ 
	ULONG ProcessId;
	UCHAR ObjectTypeNumber;
	UCHAR Flags;
	USHORT Handle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION; 

typedef enum _OBJECT_INFORMATION_CLASS 
{ 
	ObjectBasicInformation, // 0 Y N 
	ObjectNameInformation, // 1 Y N 
	ObjectTypeInformation, // 2 Y N 
	ObjectAllTypesInformation, // 3 Y N 
	ObjectHandleInformation // 4 Y Y 
} OBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_NAME_INFORMATION 
{ 
	UNICODE_STRING Name; 
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION; 

typedef struct _OBJECT_BASIC_INFORMATION 
{ 
	ULONG Attributes; 
	ACCESS_MASK GrantedAccess; 
	ULONG HandleCount; 
	ULONG PointerCount; 
	ULONG PagedPoolUsage; 
	ULONG NonPagedPoolUsage; 
	ULONG Reserved[3]; 
	ULONG NameInformationLength; 
	ULONG TypeInformationLength; 
	ULONG SecurityDescriptorLength; 
	LARGE_INTEGER CreateTime; 
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION; 

typedef VOID *POBJECT; 

typedef enum _POOL_TYPE 
{ 
	NonPagedPool, 
	PagedPool, 
	NonPagedPoolMustSucceed, 
	DontUseThisType, 
	NonPagedPoolCacheAligned, 
	PagedPoolCacheAligned, 
	NonPagedPoolCacheAlignedMustS, 
	MaxPoolType 
} POOL_TYPE, *PPOOL_TYPE; 

typedef struct _SYSTEM_OBJECT_TYPE_INFORMATION 
{ 
	ULONG NextEntryOffset; // absolute offset 
	ULONG ObjectCount; 
	ULONG HandleCount; 
	ULONG TypeIndex; // OB_TYPE_* (OB_TYPE_TYPE, etc.) 
	ULONG InvalidAttributes; // OBJ_* (OBJ_INHERIT, etc.) 
	GENERIC_MAPPING GenericMapping; 
	ACCESS_MASK ValidAccessMask; 
	POOL_TYPE PoolType; 
	BOOLEAN SecurityRequired; 
	BOOLEAN WaitableObject; 
	UNICODE_STRING TypeName; 
} SYSTEM_OBJECT_TYPE_INFORMATION, *PSYSTEM_OBJECT_TYPE_INFORMATION; 

typedef struct _SYSTEM_OBJECT_INFORMATION 
{ 
	ULONG NextEntryOffset; // absolute offset 
	POBJECT Object; 
	ULONG CreatorProcessId; 
	USHORT CreatorBackTraceIndex; 
	USHORT Flags; // see "Native API Reference" page 24 
	LONG PointerCount; 
	LONG HandleCount; 
	ULONG PagedPoolCharge; 
	ULONG NonPagedPoolCharge; 
	ULONG ExclusiveProcessId; 
	PSECURITY_DESCRIPTOR SecurityDescriptor; 
	UNICODE_STRING ObjectName; 
} SYSTEM_OBJECT_INFORMATION, *PSYSTEM_OBJECT_INFORMATION; 

typedef struct _OBJECT_TYPE_INFORMATION 
{ 
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
	UCHAR Reserved3[2]; 
	POOL_TYPE PoolType; 
	ULONG PagedPoolUsage; 
	ULONG NonPagedPoolUsage; 
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION; 

typedef struct _SYSTEM_PERFORMANCE_INFORMATION {//Information Class 2
	LARGE_INTEGER IdleTime;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
	ULONG ReadOperationCount;
	ULONG WriteOperationCount;
	ULONG OtherOperationCount;
	ULONG AvailablePages;
	ULONG TotalCommittedPages;
	ULONG TotalCommitLimit;
	ULONG PeakCommitment;
	ULONG PageFaults;
	ULONG WriteCopyFaults;
	ULONG TransitionFaults;
	ULONG Reserved1;
	ULONG DemandZeroFaults;
	ULONG PagesRead;
	ULONG PageReadIos;
	ULONG Reserved2 [2];
	ULONG PagefilePagesWritten;
	ULONG PagefilePageWriteIos;
	ULONG MappedFilePagesWritten;
	ULONG MappedFilePageWriteIos;
	ULONG PagedPoolUsage;
	ULONG NonPagedPoolUsage;
	ULONG PagedPoolAllocs;
	ULONG PagedPoolFrees;
	ULONG NonPagedPoolAllocs;
	ULONG NonPagedPoolFrees;
	ULONG TotalFreeSystemPtes;
	ULONG SystemCodePage;
	ULONG TotalSystemDriverPages;
	ULONG TotalSystemCodePages;
	ULONG SmallNonPagedLookasideListAllocateHits;
	ULONG SmallPagedLookasideListAllocateHits;
	ULONG Reserved3;
	ULONG MmSystemCachePage;
	ULONG PagedPoolPage;
	ULONG SystemDriverPage;
	ULONG FastReadNoWait;
	ULONG FastReadWait;
	ULONG FastReadResourceMiss;
	ULONG FastReadNotPossible;
	ULONG FastMdlReadNoWait;
	ULONG FastMdlReadWait;
	ULONG FastMdlReadResourceMiss;
	ULONG FastMdlReadNotPossible;
	ULONG MapDataNoWait;
	ULONG MapDataWait;
	ULONG MapDataNoWaitMiss;
	ULONG MapDataWaitMiss;
	ULONG PinMappedDataCount;
	ULONG PinReadNoWait;
	ULONG PinReadWait;
	ULONG PinReadNoWaitMiss;
	ULONG PinReadWaitMiss;
	ULONG CopyReadNoWait;
	ULONG CopyReadWait;
	ULONG CopyReadNoWaitMiss;
	ULONG CopyReadWaitMiss;
	ULONG MdlReadNoWait;
	ULONG MdlReadWait;
	ULONG MdlReadNoWaitMiss;
	ULONG MdlReadWaitMiss;
	ULONG ReadAheadIos;
	ULONG LazyWriteIos;
	ULONG LazyWritePages;
	ULONG DataFlushes;
	ULONG DataPages;
	ULONG ContextSwitches;
	ULONG FirstLevelTbFills;
	ULONG SecondLevelTbFills;
	ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION,*PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_MODULE_INFORMATION {
    ULONG  Reserved[2];
    PVOID  Base;
    ULONG  Size;
    ULONG  Flags;
    USHORT Index;
    USHORT Unknown;
    USHORT LoadCount;
    USHORT ModuleNameOffset;
    CHAR   ImageName[256];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef struct _IO_STATUS_BLOCK 
{ 
	NTSTATUS Status; 
	ULONG uInformation; 
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK; 

typedef struct _SYSTEM_STRINGS 
{ 
	UNICODE_STRING SystemRoot; // C:WINNT 
	UNICODE_STRING System32Root; // C:WINNTSystem32 
	UNICODE_STRING BaseNamedObjects; // BaseNamedObjects 
}SYSTEM_STRINGS,*PSYSTEM_STRINGS; 

typedef struct _TEXT_INFO 
{ 
	PVOID Reserved; 
	PSYSTEM_STRINGS SystemStrings; 
}TEXT_INFO, *PTEXT_INFO; 

typedef struct _PEB_LDR_DATA 
{ 
	ULONG Length; 
	BOOLEAN Initialized; 
	PVOID SsHandle; 
	LIST_ENTRY InLoadOrderModuleList; // ref. to PLDR_DATA_TABLE_ENTRY->InLoadOrderModuleList 
	LIST_ENTRY InMemoryOrderModuleList; // ref. to PLDR_DATA_TABLE_ENTRY->InMemoryOrderModuleList 
	LIST_ENTRY InInitializationOrderModuleList; // ref. to PLDR_DATA_TABLE_ENTRY->InInitializationOrderModuleList 
} PEB_LDR_DATA, *PPEB_LDR_DATA; 

typedef struct _RTL_BITMAP 
{ 
	DWORD SizeOfBitMap; 
	PDWORD Buffer; 
} RTL_BITMAP, *PRTL_BITMAP, **PPRTL_BITMAP; 

typedef struct _PEB_FREE_BLOCK 
{ 
	struct _PEB_FREE_BLOCK *Next; 
	ULONG Size; 
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK; 

typedef VOID NTSYSAPI (*PPEBLOCKROUTINE)(PVOID); 

typedef struct _CURDIR 
{ 
	UNICODE_STRING DosPath; 
	HANDLE Handle; 
} CURDIR, *PCURDIR; 

typedef struct _RTL_DRIVE_LETTER_CURDIR 
{ 
	WORD Flags; 
	WORD Length; 
	DWORD TimeStamp; 
	UNICODE_STRING DosPath; 
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR; 

typedef struct _PROCESS_PARAMETERS 
{ 
	ULONG MaximumLength; 
	ULONG Length; 
	ULONG Flags; // PROCESS_PARAMETERS_NORMALIZED 
	ULONG DebugFlags; 
	HANDLE ConsoleHandle; 
	ULONG ConsoleFlags; 
	HANDLE StandardInput; 
	HANDLE StandardOutput; 
	HANDLE StandardError; 
	CURDIR CurrentDirectory; 
	UNICODE_STRING DllPath; 
	UNICODE_STRING ImagePathName; 
	UNICODE_STRING CommandLine; 
	PWSTR Environment; 
	ULONG StartingX; 
	ULONG StartingY; 
	ULONG CountX; 
	ULONG CountY; 
	ULONG CountCharsX; 
	ULONG CountCharsY; 
	ULONG FillAttribute; 
	ULONG WindowFlags; 
	ULONG ShowWindowFlags; 
	UNICODE_STRING WindowTitle; 
	UNICODE_STRING Desktop; 
	UNICODE_STRING ShellInfo; 
	UNICODE_STRING RuntimeInfo; 
	RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32]; 
} PROCESS_PARAMETERS, *PPROCESS_PARAMETERS; 

typedef struct _LDR_DATA_TABLE_ENTRY 
{ 
	LIST_ENTRY InLoadOrderModuleList; 
	LIST_ENTRY InMemoryOrderModuleList; 
	LIST_ENTRY InInitializationOrderModuleList; 
	PVOID DllBase; 
	PVOID EntryPoint; 
	ULONG SizeOfImage; // in bytes 
	UNICODE_STRING FullDllName; 
	UNICODE_STRING BaseDllName; 
	ULONG Flags; // LDR_* 
	USHORT LoadCount; 
	USHORT TlsIndex; 
	LIST_ENTRY HashLinks; 
	PVOID SectionPointer; 
	ULONG CheckSum; 
	ULONG TimeDateStamp; 
	// PVOID LoadedImports; // seems they are exist only on XP !!! 
	// PVOID EntryPointActivationContext; // -same- 
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY; 

typedef struct _PEB 
{ 
	UCHAR InheritedAddressSpace; // 0 
	UCHAR ReadImageFileExecOptions; // 1 
	UCHAR BeingDebugged; // 2 
	BYTE SpareBool; // 3 
	PVOID Mutant; // 4 
	PVOID ImageBaseAddress; // 8 
	PPEB_LDR_DATA Ldr; // C 
	PPROCESS_PARAMETERS ProcessParameters; // 10 
	PVOID SubSystemData; // 14 
	PVOID ProcessHeap; // 18 
	KSPIN_LOCK FastPebLock; // 1C 
	PPEBLOCKROUTINE FastPebLockRoutine; // 20 
	PPEBLOCKROUTINE FastPebUnlockRoutine; // 24 
	ULONG EnvironmentUpdateCount; // 28 
	PVOID *KernelCallbackTable; // 2C 
	PVOID EventLogSection; // 30 
	PVOID EventLog; // 34 
	PPEB_FREE_BLOCK FreeList; // 38 
	ULONG TlsExpansionCounter; // 3C 
	PRTL_BITMAP TlsBitmap; // 40 
	ULONG TlsBitmapData[0x2]; // 44 
	PVOID ReadOnlySharedMemoryBase; // 4C 
	PVOID ReadOnlySharedMemoryHeap; // 50 
	PTEXT_INFO ReadOnlyStaticServerData; // 54 
	PVOID InitAnsiCodePageData; // 58 
	PVOID InitOemCodePageData; // 5C 
	PVOID InitUnicodeCaseTableData; // 60 
	ULONG KeNumberProcessors; // 64 
	ULONG NtGlobalFlag; // 68 
	DWORD d6C; // 6C 
	LARGE_INTEGER MmCriticalSectionTimeout; // 70 
	ULONG MmHeapSegmentReserve; // 78 
	ULONG MmHeapSegmentCommit; // 7C 
	ULONG MmHeapDeCommitTotalFreeThreshold; // 80 
	ULONG MmHeapDeCommitFreeBlockThreshold; // 84 
	ULONG NumberOfHeaps; // 88 
	ULONG AvailableHeaps; // 8C 
	PHANDLE ProcessHeapsListBuffer; // 90 
	PVOID GdiSharedHandleTable; // 94 
	PVOID ProcessStarterHelper; // 98 
	PVOID GdiDCAttributeList; // 9C 
	KSPIN_LOCK LoaderLock; // A0 
	ULONG NtMajorVersion; // A4 
	ULONG NtMinorVersion; // A8 
	USHORT NtBuildNumber; // AC 
	USHORT NtCSDVersion; // AE 
	ULONG PlatformId; // B0 
	ULONG Subsystem; // B4 
	ULONG MajorSubsystemVersion; // B8 
	ULONG MinorSubsystemVersion; // BC 
	KAFFINITY AffinityMask; // C0 
	ULONG GdiHandleBuffer[0x22]; // C4 
	ULONG PostProcessInitRoutine; // 14C 
	ULONG TlsExpansionBitmap; // 150 
	UCHAR TlsExpansionBitmapBits[0x80]; // 154 
	ULONG SessionId; // 1D4 
	ULARGE_INTEGER AppCompatFlags; // 1D8 
	PWORD CSDVersion; // 1E0 
/*  PVOID AppCompatInfo; // 1E4 
	UNICODE_STRING usCSDVersion; 
	PVOID ActivationContextData; 
	PVOID ProcessAssemblyStorageMap; 
	PVOID SystemDefaultActivationContextData; 
	PVOID SystemAssemblyStorageMap; 
	ULONG MinimumStackCommit;*/ 
} PEB, *PPEB; 

typedef struct _TEB 
{ 
	NT_TIB Tib; 
	PVOID EnvironmentPointer; 
	CLIENT_ID Cid; 
	PVOID ActiveRpcInfo; 
	PVOID ThreadLocalStoragePointer; 
	PPEB Peb; 
	ULONG LastErrorValue; 
	ULONG CountOfOwnedCriticalSections; 
	PVOID CsrClientThread; 
	PVOID Win32ThreadInfo; 
	ULONG Win32ClientInfo[0x1F]; 
	PVOID WOW32Reserved; 
	ULONG CurrentLocale; 
	ULONG FpSoftwareStatusRegister; 
	PVOID SystemReserved1[0x36]; 
	PVOID Spare1; 
	LONG ExceptionCode; 
	ULONG SpareBytes1[0x28]; 
	PVOID SystemReserved2[0xA]; 
	ULONG gdiRgn; 
	ULONG gdiPen; 
	ULONG gdiBrush; 
	CLIENT_ID RealClientId; 
	PVOID GdiCachedProcessHandle; 
	ULONG GdiClientPID; 
	ULONG GdiClientTID; 
	PVOID GdiThreadLocaleInfo; 
	PVOID UserReserved[5]; 
	PVOID glDispatchTable[0x118]; 
	ULONG glReserved1[0x1A]; 
	PVOID glReserved2; 
	PVOID glSectionInfo; 
	PVOID glSection; 
	PVOID glTable; 
	PVOID glCurrentRC; 
	PVOID glContext; 
	NTSTATUS LastStatusValue; 
	UNICODE_STRING StaticUnicodeString; 
	WCHAR StaticUnicodeBuffer[0x105]; 
	PVOID DeallocationStack; 
	PVOID TlsSlots[0x40]; 
	LIST_ENTRY TlsLinks; 
	PVOID Vdm; 
	PVOID ReservedForNtRpc; 
	PVOID DbgSsReserved[0x2]; 
	ULONG HardErrorDisabled; 
	PVOID Instrumentation[0x10]; 
	PVOID WinSockData; 
	ULONG GdiBatchCount; 
	ULONG Spare2; 
	ULONG Spare3; 
	ULONG Spare4; 
	PVOID ReservedForOle; 
	ULONG WaitingOnLoaderLock; 
	PVOID StackCommit; 
	PVOID StackCommitMax; 
	PVOID StackReserve; 
} TEB, *PTEB; 

typedef struct _DEBUG_BUFFER {
	HANDLE SectionHandle;
	PVOID SectionBase;
	PVOID RemoteSectionBase;
	ULONG SectionBaseDelta;
	HANDLE EventPairHandle;
	ULONG Unknown[2];
	HANDLE RemoteThreadHandle;
	ULONG InfoClassMask;
	ULONG SizeOfInfo;
	ULONG AllocatedSize;
	ULONG SectionSize;
	PVOID ModuleInformation;
	PVOID BackTraceInformation;
	PVOID HeapInformation;
	PVOID LockInformation;
	PVOID Reserved[8];
} DEBUG_BUFFER, *PDEBUG_BUFFER;

typedef struct _DEBUG_MODULE_INFORMATION { // c.f. SYSTEM_MODULE_INFORMATION
	ULONG Reserved[2];
	ULONG Base;
	ULONG Size;
	ULONG Flags;
	USHORT Index;
	USHORT Unknown;
	USHORT LoadCount;
	USHORT ModuleNameOffset;
	CHAR ImageName[256];
} DEBUG_MODULE_INFORMATION, *PDEBUG_MODULE_INFORMATION;

typedef struct _DEBUG_HEAP_INFORMATION {
	ULONG Base;
	ULONG Flags;
	USHORT Granularity;
	USHORT Unknown;
	ULONG Allocated;
	ULONG Committed;
	ULONG TagCount;
	ULONG BlockCount;
	ULONG Reserved[7];
	PVOID Tags;
	PVOID Blocks;
} DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;

typedef enum _PROCESSINFOCLASS {
	ProcessBasicInformation, // 0 Y N
	ProcessQuotaLimits, // 1 Y Y
	ProcessIoCounters, // 2 Y N
	ProcessVmCounters, // 3 Y N
	ProcessTimes, // 4 Y N
	ProcessBasePriority, // 5 N Y
	ProcessRaisePriority, // 6 N Y
	ProcessDebugPort, // 7 Y Y
	ProcessExceptionPort, // 8 N Y
	ProcessAccessToken, // 9 N Y
	ProcessLdtInformation, // 10 Y Y
	ProcessLdtSize, // 11 N Y
	ProcessDefaultHardErrorMode, // 12 Y Y
	ProcessIoPortHandlers, // 13 N Y
	ProcessPooledUsageAndLimits, // 14 Y N
	ProcessWorkingSetWatch, // 15 Y Y
	ProcessUserModeIOPL, // 16 N Y
	ProcessEnableAlignmentFaultFixup, // 17 N Y
	ProcessPriorityClass, // 18 N Y
	ProcessWx86Information, // 19 Y N
	ProcessHandleCount, // 20 Y N
	ProcessAffinityMask, // 21 N Y
	ProcessPriorityBoost, // 22 Y Y
	ProcessDeviceMap, // 23 Y Y
	ProcessSessionInformation, // 24 Y Y
	ProcessForegroundInformation, // 25 N Y
	ProcessWow64Information // 26 Y N
} PROCESSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION { // Information Class 0
	NTSTATUS ExitStatus;
	PPEB PebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueProcessId;
	ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _KERNEL_USER_TIMES { // Information Class 4	
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

typedef struct _THREAD_BASIC_INFORMATION { // Information Class 0
	NTSTATUS ExitStatus;
	PTEB TebBaseAddress;
	CLIENT_ID ClientId;
	KAFFINITY AffinityMask;
	KPRIORITY Priority;
	KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_TIMES {//Information Class 8
	LARGE_INTEGER IdleTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER DpcTime;
	LARGE_INTEGER InterruptTime;
	ULONG InterruptCount;
}SYSTEM_PROCESSOR_TIMES, *PSYSTEM_PROCESSOR_TIMES;

typedef enum _MEMORY_INFORMATION_CLASS {
	MemoryBasicInformation,
	MemoryWorkingSetList,
	MemorySectionName,
	MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;

typedef struct _MEMORY_WORKING_SET_LIST {
	ULONG NumberOfPages;
	ULONG WorkingSetList [1];
} MEMORY_WORKING_SET_LIST, *PMEMORY_WORKING_SET_LIST;

typedef struct _MEMORY_SECTION_NAME {
	UNICODE_STRING SectionFileName;
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

typedef struct _BACKEDUP_SECTION_FILENAME_INFO {
	UNICODE_STRING BakedupSectionFileName;
	WCHAR FileName[1];
} BACKEDUP_SECTION_FILENAME_INFO, *PBACKEDUP_SECTION_FILENAME_INFO;

typedef struct _SYSTEM_BASIC_INFORMATION {
	ULONG Unknown;
	ULONG MaximumIncrement;
	ULONG PhysicalPageSize;
	ULONG NumberOfPhysicalPages;
	ULONG LowestPhysicalPage;
	ULONG HighestPhysicalPage;
	ULONG AllocationGranularity;
	ULONG LowestUserAddress;
	ULONG HighestUserAddress;
	ULONG ActiveProcessors;
	UCHAR NumberProcessors;
}SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_CACHE_INFORMATION {
	ULONG SystemCacheWsSize;
	ULONG SystemCacheWsPeakSize;
	ULONG SystemCacheWsFaults;
	ULONG SystemCacheWsMinimum;
	ULONG SystemCacheWsMaximum;
	ULONG TransitionSharedPages;
	ULONG TransitionSharedPagesPeak;
	ULONG Reserved [2];
}SYSTEM_CACHE_INFORMATION, *PSYSTEM_CACHE_INFORMATION;
//EOF
#endif /*_ntnative_*/