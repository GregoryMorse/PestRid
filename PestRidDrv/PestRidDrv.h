// PestRidDrv.h    Include file for File Object info driver
//

#pragma warning(disable : 4115 4201 4214)
#include <ntverp.h>
#include <ntddk.h>
#include <string.h>
//#include <devioctl.h>
#include <warning.h>
// PestRidDrvIoCtl.h    Include file for PestRidDrv driver
//
#include <ntdddisk.h>

#include "..\DriverInterface.h"

#ifdef _X86_
#define HARDWARE_PTE    HARDWARE_PTE_X86
#define PHARDWARE_PTE   PHARDWARE_PTE_X86

typedef struct _HARDWARE_PTE_X86 {
    ULONG Valid             : 1;
    ULONG Write             : 1;
    ULONG Owner             : 1;
    ULONG WriteThrough      : 1;
    ULONG CacheDisable      : 1;
    ULONG Accessed          : 1;
    ULONG Dirty             : 1;
    ULONG LargePage         : 1;
    ULONG Global            : 1;
    ULONG CopyOnWrite       : 1;
    ULONG Prototype         : 1;
    ULONG reserved          : 1;
    ULONG PageFrameNumber   : 20;
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

#else
typedef struct _HARDWARE_PTE
{
     union
     {
          ULONG Valid: 1;
          ULONG Write: 1;
          ULONG Owner: 1;
          ULONG WriteThrough: 1;
          ULONG CacheDisable: 1;
          ULONG Accessed: 1;
          ULONG Dirty: 1;
          ULONG LargePage: 1;
          ULONG Global: 1;
          ULONG CopyOnWrite: 1;
          ULONG Prototype: 1;
          ULONG reserved0: 1;
          ULONG PageFrameNumber: 26;
          ULONG reserved1: 26;
          ULONG LowPart;
     };
     ULONG HighPart;
} HARDWARE_PTE, *PHARDWARE_PTE;
#endif

typedef struct _KSERVICE_TABLE_DESCRIPTOR {
	PULONG ServiceTableBase;
	PVOID ServiceCounterTableBase;
	UINT_PTR NumberOfServices;
	PUCHAR ParamTableBase;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

//#define NUMBER_SERVICE_TABLES 2 //4

typedef struct _MMWSL                           *PMMWSL;
typedef struct _HANDLE_TABLE                    *PHANDLE_TABLE;
typedef struct _EPROCESS_QUOTA_BLOCK            *PEPROCESS_QUOTA_BLOCK;

/*#if (VER_PRODUCTBUILD >= 3790)

typedef struct _MMSUPPORT {
    LIST_ENTRY      WorkingSetExpansionLinks;
    LARGE_INTEGER   LastTrimTime; // 0x8
    MMSUPPORT_FLAGS Flags; // 0x10
    ULONG           PageFaultCount; // 0x14
    ULONG           PeakWorkingSetSize; // 0x18
    ULONG           GrowthSinceLastEstimate; // 0x1c
    ULONG           MinimumWorkingSetSize; // 0x20
    ULONG           MaximumWorkingSetSize; // 0x24
    PMMWSL          VmWorkingSetList; // 0x28 
    ULONG           Claim; // 0x2c
    ULONG           NextEstimationSlot; // 0x30
    ULONG           NextAgingSlot; // 0x34
    ULONG           EstimatedAvailable; // 0x38
    ULONG           WorkingSetSize;  //0x3c
    KGUARDED_MUTEX  Mutex; // 0x40
} MMSUPPORT, *PMMSUPPORT;

#elif (VER_PRODUCTBUILD >= 2600)

typedef struct _MMSUPPORT {
    LARGE_INTEGER   LastTrimTime;
    MMSUPPORT_FLAGS Flags;
    ULONG           PageFaultCount;
    ULONG           PeakWorkingSetSize;
    ULONG           WorkingSetSize;
    ULONG           MinimumWorkingSetSize;
    ULONG           MaximumWorkingSetSize;
    PMMWSL          VmWorkingSetList;
    LIST_ENTRY      WorkingSetExpansionLinks;
    ULONG           Claim;
    ULONG           NextEstimationSlot;
    ULONG           NextAgingSlot;
    ULONG           EstimatedAvailable;
    ULONG           GrowthSinceLastEstimate;
} MMSUPPORT, *PMMSUPPORT;

#else*/

typedef struct _MMSUPPORT {
    LARGE_INTEGER   LastTrimTime;
    ULONG           LastTrimFaultCount;
    ULONG           PageFaultCount;
    ULONG           PeakWorkingSetSize;
    ULONG           WorkingSetSize;
    ULONG           MinimumWorkingSetSize;
    ULONG           MaximumWorkingSetSize;
    PMMWSL          VmWorkingSetList;
    LIST_ENTRY      WorkingSetExpansionLinks;
    BOOLEAN         AllowWorkingSetAdjustment;
    BOOLEAN         AddressSpaceBeingDeleted;
    UCHAR           ForegroundSwitchCount;
    UCHAR           MemoryPriority;
/*#if (VER_PRODUCTBUILD >= 2195)
    union {
        ULONG           LongFlags;
        MMSUPPORT_FLAGS Flags;
    } u;
    ULONG           Claim;
    ULONG           NextEstimationSlot;
    ULONG           NextAgingSlot;
    ULONG           EstimatedAvailable;
    ULONG           GrowthSinceLastEstimate;
#endif // (VER_PRODUCTBUILD >= 2195)*/
} MMSUPPORT, *PMMSUPPORT;

//#endif

#if (VER_PRODUCTBUILD >= 2600)

typedef struct _KPROCESS {
    DISPATCHER_HEADER   Header;
    LIST_ENTRY          ProfileListHead;
    ULONG               DirectoryTableBase[2];
    KGDTENTRY           LdtDescriptor;
    KIDTENTRY           Int21Descriptor;
    USHORT              IopmOffset;
    UCHAR               Iopl;
    UCHAR               Unused;
    ULONG               ActiveProcessors;
    ULONG               KernelTime;
    ULONG               UserTime;
    LIST_ENTRY          ReadyListHead;  
    SINGLE_LIST_ENTRY   SwapListEntry;
    PVOID               VdmTrapcHandler;
    LIST_ENTRY          ThreadListHead;
    KSPIN_LOCK          ProcessLock;
    KAFFINITY           Affinity;
    USHORT              StackCount;
    CHAR                BasePriority;
    CHAR                ThreadQuantum;
    BOOLEAN             AutoAlignment;
    UCHAR               State;
    UCHAR               ThreadSeed;
    BOOLEAN             DisableBoost;
    UCHAR               PowerState;
    BOOLEAN             DisableQuantum;
    UCHAR               IdealNode;
    UCHAR               Spare;
} KPROCESS, *PKPROCESS;

#else

typedef struct _KPROCESS {
    DISPATCHER_HEADER   Header;
    LIST_ENTRY          ProfileListHead;
    ULONG               DirectoryTableBase[2];
    KGDTENTRY           LdtDescriptor;
    KIDTENTRY           Int21Descriptor;
    USHORT              IopmOffset;
    UCHAR               Iopl;
    UCHAR               VdmFlag;
    ULONG               ActiveProcessors;
    ULONG               KernelTime;
    ULONG               UserTime;
    LIST_ENTRY          ReadyListHead;  
    SINGLE_LIST_ENTRY   SwapListEntry;
    PVOID               Reserved1;
    LIST_ENTRY          ThreadListHead;
    KSPIN_LOCK          ProcessLock;
    KAFFINITY           Affinity;
    USHORT              StackCount;
    UCHAR               BasePriority;
    UCHAR               ThreadQuantum;
    BOOLEAN             AutoAlignment;
    UCHAR               State;
    UCHAR               ThreadSeed;
    BOOLEAN             DisableBoost;
#if (VER_PRODUCTBUILD >= 2195)
    UCHAR               PowerState;
    BOOLEAN             DisableQuantum;
    UCHAR               IdealNode;
    UCHAR               Spare;
#endif // (VER_PRODUCTBUILD >= 2195)
} KPROCESS, *PKPROCESS;

#endif

/*#if (VER_PRODUCTBUILD >= 3790)

typedef struct _MM_ADDRESS_NODE {
    union {
        ULONG                   Balance : 2;
        struct _MM_ADDRESS_NODE *Parent; // lower 2 bits of Parent are Balance and must be zeroed to obtain Parent
    };
    struct _MM_ADDRESS_NODE     *LeftChild;
    struct _MM_ADDRESS_NODE     *RightChild;
    ULONG_PTR                   StartingVpn;
    ULONG_PTR                   EndingVpn;
} MMADDRESS_NODE, *PMMADDRESS_NODE;

typedef struct _MM_AVL_TABLE {
    MMADDRESS_NODE  BalancedRoot; // Vadroot; incorrectly represents the NULL pages (EndingVpn should be 0xf, etc.)
    ULONG           DepthOfTree : 5; // 0x14
    ULONG           Unused : 3;
    ULONG           NumberGenericTableElements : 24; // total number of nodes
    PVOID           NodeHint; // 0x18 (0x270 in _EPROCESS)
    PVOID           NodeFreeHint; // 0x1c
} MM_AVL_TABLE, *PMM_AVL_TABLE;

typedef struct _EPROCESS {
    KPROCESS                        Pcb; // +0x000
    EX_PUSH_LOCK                    ProcessLock; // +0x06c
    LARGE_INTEGER                   CreateTime; // +0x070
    LARGE_INTEGER                   ExitTime; // +0x078
    EX_RUNDOWN_REF                  RundownProtect; // +0x080
    ULONG                           UniqueProcessId; // +0x084
    LIST_ENTRY                      ActiveProcessLinks; // +0x088
    ULONG                           QuotaUsage[3]; // +0x090
    ULONG                           QuotaPeak[3]; // +0x09c
    ULONG                           CommitCharge; // +0x0a8
    ULONG                           PeakVirtualSize; // +0x0ac
    ULONG                           VirtualSize; // +0x0b0
    LIST_ENTRY                      SessionProcessLinks; // +0x0b4
    PVOID                           DebugPort; // +0x0bc
    PVOID                           ExceptionPort; // +0x0c0
    PHANDLE_TABLE                   ObjectTable; // +0x0c4
    EX_FAST_REF                     Token; // +0x0c8
    ULONG                           WorkingSetPage; // +0x0cc
    KGUARDED_MUTEX                  AddressCreationLock; // +0x0d0
    ULONG                           HyperSpaceLock; // +0x0f0
    PETHREAD                        ForkInProgress; // +0x0f4
    ULONG                           HardwareTrigger; // +0x0f8
    PMM_AVL_TABLE                   PhysicalVadRoot; // +0x0fc
    PVOID                           CloneRoot; // +0x100
    ULONG                           NumberOfPrivatePages; // +0x104
    ULONG                           NumberOfLockedPages; // +0x108
    PVOID                           Win32Process; // +0x10c
    PEJOB                           Job; // +0x110
    PVOID                           SectionObject; // +0x114
    PVOID                           SectionBaseAddress; // +0x118
    PEPROCESS_QUOTA_BLOCK           QuotaBlock; // +0x11c
    PPAGEFAULT_HISTORY              WorkingSetWatch; // +0x120
    PVOID                           Win32WindowStation; // +0x124
    ULONG                           InheritedFromUniqueProcessId; // +0x128
    PVOID                           LdtInformation; // +0x12c
    PVOID                           VadFreeHint; // +0x130
    PVOID                           VdmObjects; // +0x134
    PVOID                           DeviceMap; // +0x138
    PVOID                           Spare0[3]; // +0x13c
    union {
        HARDWARE_PTE                PageDirectoryPte; // +0x148
        UINT64                      Filler; // +0x148
    };
    PVOID                           Session; // +0x150
    UCHAR                           ImageFileName[16]; // +0x154
    LIST_ENTRY                      JobLinks; // +0x164
    PVOID                           LockedPagesList; // +0x16c
    LIST_ENTRY                      ThreadListHead; // +0x170
    PVOID                           SecurityPort; // +0x178
    PVOID                           PaeTop; // +0x17c
    ULONG                           ActiveThreads; // +0x180
    ULONG                           GrantedAccess; // +0x184
    ULONG                           DefaultHardErrorProcessing; // +0x188
    SHORT                           LastThreadExitStatus; // +0x18c
    PPEB                            Peb; // +0x190
    EX_FAST_REF                     PrefetchTrace; // +0x194
    LARGE_INTEGER                   ReadOperationCount; // +0x198
    LARGE_INTEGER                   WriteOperationCount; // +0x1a0
    LARGE_INTEGER                   OtherOperationCount; // +0x1a8
    LARGE_INTEGER                   ReadTransferCount; // +0x1b0
    LARGE_INTEGER                   WriteTransferCount; // +0x1b8
    LARGE_INTEGER                   OtherTransferCount; // +0x1c0
    ULONG                           CommitChargeLimit; // +0x1c8
    ULONG                           CommitChargePeak; // +0x1cc
    PVOID                           AweInfo; // +0x1d0
    SE_AUDIT_PROCESS_CREATION_INFO  SeAuditProcessCreationInfo; // +0x1d4
    MMSUPPORT                       Vm; // +0x1d8
    LIST_ENTRY                      MmProcessLinks; // +0x238
    ULONG                           ModifiedPageCount; // +0x240
    ULONG                           JobStatus; // +0x244
    union {
        ULONG                       Flags; // 0x248
        struct {
            ULONG                   CreateReported              : 1;
            ULONG                   NoDebugInherit              : 1;
            ULONG                   ProcessExiting              : 1;
            ULONG                   ProcessDelete               : 1;
            ULONG                   Wow64SplitPages             : 1;
            ULONG                   VmDeleted                   : 1;
            ULONG                   OutswapEnabled              : 1;
            ULONG                   Outswapped                  : 1;
            ULONG                   ForkFailed                  : 1;
            ULONG                   Wow64VaSpace4Gb             : 1;
            ULONG                   AddressSpaceInitialized     : 2;
            ULONG                   SetTimerResolution          : 1;
            ULONG                   BreakOnTermination          : 1;
            ULONG                   SessionCreationUnderway     : 1;
            ULONG                   WriteWatch                  : 1;
            ULONG                   ProcessInSession            : 1;
            ULONG                   OverrideAddressSpace        : 1;
            ULONG                   HasAddressSpace             : 1;
            ULONG                   LaunchPrefetched            : 1;
            ULONG                   InjectInpageErrors          : 1;
            ULONG                   VmTopDown                   : 1;
            ULONG                   ImageNotifyDone             : 1;
            ULONG                   PdeUpdateNeeded             : 1;
            ULONG                   VdmAllowed                  : 1;
            ULONG                   Unused                      : 7;
        };
    };
    NTSTATUS                        ExitStatus; // +0x24c
    USHORT                          NextPageColor; // +0x250
    union {
        struct {
            UCHAR                   SubSystemMinorVersion; // +0x252
            UCHAR                   SubSystemMajorVersion; // +0x253
        };
        USHORT                      SubSystemVersion; // +0x252
    };
    UCHAR                           PriorityClass; // +0x254
    MM_AVL_TABLE                    VadRoot; // +0x258
} EPROCESS, *PEPROCESS; // 0x278 in total

#elif (VER_PRODUCTBUILD >= 2600)

typedef struct _EPROCESS {
    KPROCESS                        Pcb;
    EX_PUSH_LOCK                    ProcessLock;
    LARGE_INTEGER                   CreateTime;
    LARGE_INTEGER                   ExitTime;
    EX_RUNDOWN_REF                  RundownProtect;
    ULONG                           UniqueProcessId;
    LIST_ENTRY                      ActiveProcessLinks;
    ULONG                           QuotaUsage[3];
    ULONG                           QuotaPeak[3];
    ULONG                           CommitCharge;
    ULONG                           PeakVirtualSize;
    ULONG                           VirtualSize;
    LIST_ENTRY                      SessionProcessLinks;
    PVOID                           DebugPort;
    PVOID                           ExceptionPort;
    PHANDLE_TABLE                   ObjectTable;
    EX_FAST_REF                     Token;
    FAST_MUTEX                      WorkingSetLock;
    ULONG                           WorkingSetPage;
    FAST_MUTEX                      AddressCreationLock;
    KSPIN_LOCK                      HyperSpaceLock;
    PETHREAD                        ForkInProgress;
    ULONG                           HardwareTrigger;
    PVOID                           VadRoot;
    PVOID                           VadHint;
    PVOID                           CloneRoot;
    ULONG                           NumberOfPrivatePages;
    ULONG                           NumberOfLockedPages;
    PVOID                           Win32Process;
    PEJOB                           Job;
    PSECTION_OBJECT                 SectionObject;
    PVOID                           SectionBaseAddress;
    PEPROCESS_QUOTA_BLOCK           QuotaBlock;
    PPAGEFAULT_HISTORY              WorkingSetWatch;
    PVOID                           Win32WindowStation;
    PVOID                           InheritedFromUniqueProcessId;
    PVOID                           LdtInformation;
    PVOID                           VadFreeHint;
    PVOID                           VdmObjects;
    PDEVICE_MAP                     DeviceMap;
    LIST_ENTRY                      PhysicalVadList;
    union {
        HARDWARE_PTE                PageDirectoryPte;
        ULONGLONG                   Filler;
    };
    PVOID                           Session;
    UCHAR                           ImageFileName[16];
    LIST_ENTRY                      JobLinks;
    PVOID                           LockedPageList;
    LIST_ENTRY                      ThreadListHead;
    PVOID                           SecurityPort;
    PVOID                           PaeTop;
    ULONG                           ActiveThreads;
    ULONG                           GrantedAccess;
    ULONG                           DefaultHardErrorProcessing;
    NTSTATUS                        LastThreadExitStatus;
    PPEB                            Peb;
    EX_FAST_REF                     PrefetchTrace;
    LARGE_INTEGER                   ReadOperationCount;
    LARGE_INTEGER                   WriteOperationCount;
    LARGE_INTEGER                   OtherOperationCount;
    LARGE_INTEGER                   ReadTransferCount;
    LARGE_INTEGER                   WriteTransferCount;
    LARGE_INTEGER                   OtherTransferCount;
    ULONG                           CommitChargeLimit;
    ULONG                           CommitChargePeek;
    PVOID                           AweInfo;
    SE_AUDIT_PROCESS_CREATION_INFO  SeAuditProcessCreationInfo;
    MMSUPPORT                       Vm;
    ULONG                           LastFaultCount;
    ULONG                           ModifiedPageCount;
    ULONG                           NumberOfVads;
    ULONG                           JobStatus;
    union {
        ULONG                       Flags;
        struct {
            ULONG                   CreateReported              : 1;
            ULONG                   NoDebugInherit              : 1;
            ULONG                   ProcessExiting              : 1;
            ULONG                   ProcessDelete               : 1;
            ULONG                   Wow64SplitPages             : 1;
            ULONG                   VmDeleted                   : 1;
            ULONG                   OutswapEnabled              : 1;
            ULONG                   Outswapped                  : 1;
            ULONG                   ForkFailed                  : 1;
            ULONG                   HasPhysicalVad              : 1;
            ULONG                   AddressSpaceInitialized     : 2;
            ULONG                   SetTimerResolution          : 1;
            ULONG                   BreakOnTermination          : 1;
            ULONG                   SessionCreationUnderway     : 1;
            ULONG                   WriteWatch                  : 1;
            ULONG                   ProcessInSession            : 1;
            ULONG                   OverrideAddressSpace        : 1;
            ULONG                   HasAddressSpace             : 1;
            ULONG                   LaunchPrefetched            : 1;
            ULONG                   InjectInpageErrors          : 1;
            ULONG                   Unused                      : 11;
        };
    };
    NTSTATUS                        ExitStatus;
    USHORT                          NextPageColor;
    union {
        struct {
            UCHAR                   SubSystemMinorVersion;
            UCHAR                   SubSystemMajorVersion;
        };
        USHORT                      SubSystemVersion;
    };
    UCHAR                           PriorityClass;
    BOOLEAN                         WorkingSetAcquiredUnsafe;
} EPROCESS, *PEPROCESS;

#else*/

typedef struct _EPROCESS {
    KPROCESS                        Pcb;
    NTSTATUS                        ExitStatus;
    KEVENT                          LockEvent;
    ULONG                           LockCount;
    LARGE_INTEGER                   CreateTime;
    LARGE_INTEGER                   ExitTime;
    PKTHREAD                        LockOwner;
    ULONG                           UniqueProcessId;
    LIST_ENTRY                      ActiveProcessLinks;
    ULONGLONG                       QuotaPeakPoolUsage;
    ULONGLONG                       QuotaPoolUsage;
    ULONG                           PagefileUsage;
    ULONG                           CommitCharge;
    ULONG                           PeakPagefileUsage;
    ULONG                           PeakVirtualSize;
    ULONGLONG                       VirtualSize;
    MMSUPPORT                       Vm;
#if (VER_PRODUCTBUILD < 2195)
    ULONG                           LastProtoPteFault;
#else // (VER_PRODUCTBUILD >= 2195)
    LIST_ENTRY                      SessionProcessLinks;
#endif // (VER_PRODUCTBUILD >= 2195)
    ULONG                           DebugPort;
    ULONG                           ExceptionPort;
    PHANDLE_TABLE                   ObjectTable;
    PACCESS_TOKEN                   Token;
    FAST_MUTEX                      WorkingSetLock;
    ULONG                           WorkingSetPage;
    BOOLEAN                         ProcessOutswapEnabled;
    BOOLEAN                         ProcessOutswapped;
    BOOLEAN                         AddressSpaceInitialized;
    BOOLEAN                         AddressSpaceDeleted;
    FAST_MUTEX                      AddressCreationLock;
    KSPIN_LOCK                      HyperSpaceLock;
    PETHREAD                        ForkInProgress;
    USHORT                          VmOperation;
    BOOLEAN                         ForkWasSuccessful;
    UCHAR                           MmAgressiveWsTrimMask;
    PKEVENT                         VmOperationEvent;
#if (VER_PRODUCTBUILD < 2195)
    HARDWARE_PTE                    PageDirectoryPte;
#else // (VER_PRODUCTBUILD >= 2195)
    PVOID                           PaeTop;
#endif // (VER_PRODUCTBUILD >= 2195)
    ULONG                           LastFaultCount;
    ULONG                           ModifiedPageCount;
    PVOID                           VadRoot;
    PVOID                           VadHint;
    ULONG                           CloneRoot;
    ULONG                           NumberOfPrivatePages;
    ULONG                           NumberOfLockedPages;
    USHORT                          NextPageColor;
    BOOLEAN                         ExitProcessCalled;
    BOOLEAN                         CreateProcessReported;
    HANDLE                          SectionHandle;
    PPEB                            Peb;
    PVOID                           SectionBaseAddress;
    PEPROCESS_QUOTA_BLOCK           QuotaBlock;
    NTSTATUS                        LastThreadExitStatus;
    PPROCESS_WS_WATCH_INFORMATION   WorkingSetWatch;
    HANDLE                          Win32WindowStation;
    HANDLE                          InheritedFromUniqueProcessId;
    ACCESS_MASK                     GrantedAccess;
    ULONG                           DefaultHardErrorProcessing;
    PVOID                           LdtInformation;
    PVOID                           VadFreeHint;
    PVOID                           VdmObjects;
//#if (VER_PRODUCTBUILD < 2195)
    KMUTANT                         ProcessMutant;
/*#else // (VER_PRODUCTBUILD >= 2195)
    PDEVICE_MAP                     DeviceMap;
    ULONG                           SessionId;
    LIST_ENTRY                      PhysicalVadList;
    HARDWARE_PTE                    PageDirectoryPte;
    ULONG                           Filler;
    ULONG                           PaePageDirectoryPage;
#endif // (VER_PRODUCTBUILD >= 2195)*/
    UCHAR                           ImageFileName[16];
    ULONG                           VmTrimFaultValue;
    UCHAR                           SetTimerResolution;
    UCHAR                           PriorityClass;
    union {
        struct {
            UCHAR                   SubSystemMinorVersion;
            UCHAR                   SubSystemMajorVersion;
        };
        USHORT                      SubSystemVersion;
    };
    PVOID                           Win32Process;
/*#if (VER_PRODUCTBUILD >= 2195)
    PEJOB                           Job;
    ULONG                           JobStatus;
    LIST_ENTRY                      JobLinks;
    PVOID                           LockedPageList;
    PVOID                           SecurityPort;
    PWOW64_PROCESS                  Wow64Process;
    LARGE_INTEGER                   ReadOperationCount;
    LARGE_INTEGER                   WriteOperationCount;
    LARGE_INTEGER                   OtherOperationCount;
    LARGE_INTEGER                   ReadTransferCount;
    LARGE_INTEGER                   WriteTransferCount;
    LARGE_INTEGER                   OtherTransferCount;
    ULONG                           CommitChargeLimit;
    ULONG                           CommitChargePeek;
    LIST_ENTRY                      ThreadListHead;
    PRTL_BITMAP                     VadPhysicalPagesBitMap;
    ULONG                           VadPhysicalPages;
    ULONG                           AweLock;
#endif // (VER_PRODUCTBUILD >= 2195)*/
} EPROCESS, *PEPROCESS;

//#endif

typedef struct _LDR_DATA_TABLE_ENTRY
{
     LIST_ENTRY InLoadOrderLinks;
     LIST_ENTRY InMemoryOrderLinks;
     LIST_ENTRY InInitializationOrderLinks;
     PVOID DllBase;
     PVOID EntryPoint;
     ULONG SizeOfImage;
     UNICODE_STRING FullDllName;
     UNICODE_STRING BaseDllName;
     ULONG Flags;
     USHORT LoadCount;
     USHORT TlsIndex;
     union
     {
          LIST_ENTRY HashLinks;
          struct
          {
               PVOID SectionPointer;
               ULONG CheckSum;
          };
     };
     union
     {
          ULONG TimeDateStamp;
          PVOID LoadedImports;
     };
     /*_ACTIVATION_CONTEXT * EntryPointActivationContext;
     PVOID PatchInformation;
     LIST_ENTRY ForwarderLinks;
     LIST_ENTRY ServiceTagLinks;
     LIST_ENTRY StaticLinks;*/
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

/*typedef struct _ObjectInfo
{
	ULONG PID;
	PVOID Object;
	HANDLE ObjectHandle;
} ObjectInfo, *PObjectInfo;*/

#if (VER_PRODUCTBUILD == 2195) && (_WIN32_WINNT >= 0x0500)

#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX    CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef unsigned short      WORD;
typedef unsigned long       DWORD;

//
// Support for GUID Partition Table (GPT) disks.
//

//
// There are currently two ways a disk can be partitioned. With a traditional
// AT-style master boot record (PARTITION_STYLE_MBR) and with a new, GPT
// partition table (PARTITION_STYLE_GPT). RAW is for an unrecognizable
// partition style. There are a very limited number of things you can
// do with a RAW partititon.
//

typedef enum _PARTITION_STYLE {
    PARTITION_STYLE_MBR,
    PARTITION_STYLE_GPT,
    PARTITION_STYLE_RAW
} PARTITION_STYLE;

//
// The DISK_GEOMETRY_EX structure is returned on issuing an
// IOCTL_DISK_GET_DRIVE_GEOMETRY_EX ioctl.
//

typedef enum _DETECTION_TYPE {
        DetectNone,
        DetectInt13,
        DetectExInt13
} DETECTION_TYPE;

typedef struct _DISK_INT13_INFO {
        WORD   DriveSelect;
        DWORD MaxCylinders;
        WORD   SectorsPerTrack;
        WORD   MaxHeads;
        WORD   NumberDrives;
} DISK_INT13_INFO, *PDISK_INT13_INFO;

typedef struct _DISK_EX_INT13_INFO {
        WORD   ExBufferSize;
        WORD   ExFlags;
        DWORD ExCylinders;
        DWORD ExHeads;
        DWORD ExSectorsPerTrack;
        DWORD64 ExSectorsPerDrive;
        WORD   ExSectorSize;
        WORD   ExReserved;
} DISK_EX_INT13_INFO, *PDISK_EX_INT13_INFO;

typedef struct _DISK_DETECTION_INFO {
        DWORD SizeOfDetectInfo;
        DETECTION_TYPE DetectionType;
        union {
                struct {

                        //
                        // If DetectionType == DETECTION_INT13 then we have just the Int13
                        // information.
                        //

                        DISK_INT13_INFO Int13;

                        //
                        // If DetectionType == DETECTION_EX_INT13, then we have the
                        // extended int 13 information.
                        //

                        DISK_EX_INT13_INFO ExInt13;     // If DetectionType == DetectExInt13
                };
        };
} DISK_DETECTION_INFO, *PDISK_DETECTION_INFO;


typedef struct _DISK_PARTITION_INFO {
        DWORD SizeOfPartitionInfo;
        PARTITION_STYLE PartitionStyle;                 // PartitionStyle = RAW, GPT or MBR
        union {
                struct {                                                        // If PartitionStyle == MBR
                        DWORD Signature;                                // MBR Signature
                        DWORD CheckSum;                                 // MBR CheckSum
                } Mbr;
                struct {                                                        // If PartitionStyle == GPT
                        GUID DiskId;
                } Gpt;
        };
} DISK_PARTITION_INFO, *PDISK_PARTITION_INFO;


//
// The Geometry structure is a variable length structure composed of a
// DISK_GEOMETRY_EX structure followed by a DISK_PARTITION_INFO structure
// followed by a DISK_DETECTION_DATA structure.
//

#if (NTDDI_VERSION < NTDDI_WIN2003)
#define DiskGeometryGetPartition(Geometry)\
                        ((PDISK_PARTITION_INFO)((Geometry)+1))

#define DiskGeometryGetDetect(Geometry)\
                        ((PDISK_DETECTION_INFO)(((PBYTE)DiskGeometryGetPartition(Geometry)+\
                                        DiskGeometryGetPartition(Geometry)->SizeOfPartitionInfo)))
#else
#define DiskGeometryGetPartition(Geometry)\
                        ((PDISK_PARTITION_INFO)((Geometry)->Data))

#define DiskGeometryGetDetect(Geometry)\
                        ((PDISK_DETECTION_INFO)(((DWORD_PTR)DiskGeometryGetPartition(Geometry)+\
                                        DiskGeometryGetPartition(Geometry)->SizeOfPartitionInfo)))
#endif

#endif

// driver local data structure specific to each device object
typedef struct _LOCAL_DEVICE_INFO {
    ULONG               DeviceType;     // Our private Device Type
    PDEVICE_OBJECT      DeviceObject;   // The Gpd device object.
} LOCAL_DEVICE_INFO, *PLOCAL_DEVICE_INFO;

extern POBJECT_TYPE *IoDriverObjectType;
extern POBJECT_TYPE *IoDeviceObjectType;

/********************* function prototypes ***********************************/
//
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

NTSTATUS GpdDispatch(IN PDEVICE_OBJECT pDO, IN PIRP pIrp);

VOID GpdUnload(IN  PDRIVER_OBJECT DriverObject);