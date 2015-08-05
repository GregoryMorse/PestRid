#pragma once

typedef struct _KSERVICE_TABLE_DESCRIPTOR_RECEIVE {
	PVOID pKeServiceTable;
	PULONG ServiceTableBase;
	PVOID ServiceCounterTableBase;
	UINT_PTR NumberOfServices;
	PUCHAR ParamTableBase;
} KSERVICE_TABLE_DESCRIPTOR_RECEIVE, *PKSERVICE_TABLE_DESCRIPTOR_RECEIVE;

typedef struct _LDR_DATA_TABLE_RECEIVE_ENTRY
{
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union
    {
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
} LDR_DATA_TABLE_RECEIVE_ENTRY, *PLDR_DATA_TABLE_RECEIVE_ENTRY;

#define IRP_MJ_MAXIMUM_FUNCTION         0x1b

typedef struct _DRIVER_RECEIVE_OBJECT {
	PVOID pdo;
    USHORT Type;
    USHORT Size;

    //
    // The following links all of the devices created by a single driver
    // together on a list, and the Flags word provides an extensible flag
    // location for driver objects.
    //

    PVOID DeviceObject;
    ULONG Flags;

    //
    // The following section describes where the driver is loaded.  The count
    // field is used to count the number of times the driver has had its
    // registered reinitialization routine invoked.
    //

    PVOID DriverStart;
    ULONG DriverSize;
    PVOID DriverSection;
    PVOID DriverExtension;

    //
    // The following section contains the optional pointer to an array of
    // alternate entry points to a driver for "fast I/O" support.  Fast I/O
    // is performed by invoking the driver routine directly with separate
    // parameters, rather than using the standard IRP call mechanism.  Note
    // that these functions may only be used for synchronous I/O, and when
    // the file is cached.
    //

    PVOID FastIoDispatch;

    //
    // The following section describes the entry points to this particular
    // driver.  Note that the major function dispatch table must be the last
    // field in the object so that it remains extensible.
    //

    PVOID DriverInit;
    PVOID DriverStartIo;
    PVOID DriverUnload;
    PVOID MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

} DRIVER_RECEIVE_OBJECT;
typedef struct _DRIVER_RECEIVE_OBJECT *PDRIVER_RECEIVE_OBJECT; 

typedef struct _DEVICE_RECEIVE_OBJECT {
	PVOID pdo;
    USHORT Type;
    USHORT Size;
    LONG ReferenceCount;
    PVOID DriverObject;
    PVOID NextDevice;
    PVOID AttachedDevice;
    PVOID CurrentIrp;
    PVOID Timer;
    ULONG Flags;                                // See above:  DO_...
    ULONG Characteristics;                      // See ntioapi:  FILE_...
    PVOID Vpb;
    PVOID DeviceExtension;
    DEVICE_TYPE DeviceType;
    CCHAR StackSize;
    //union {
    //    LIST_ENTRY ListEntry;
    //    WAIT_CONTEXT_BLOCK Wcb;
    //} Queue;
    ULONG AlignmentRequirement;
    //KDEVICE_QUEUE DeviceQueue;
    //KDPC Dpc;

    //
    //  The following field is for exclusive use by the filesystem to keep
    //  track of the number of Fsp threads currently using the device
    //

    ULONG ActiveThreadCount;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    //KEVENT DeviceLock;

    USHORT SectorSize;
    USHORT Spare1;

    PVOID DeviceObjectExtension;
    PVOID  Reserved;
} DEVICE_RECEIVE_OBJECT;

typedef struct _DEVICE_RECEIVE_OBJECT *PDEVICE_RECEIVE_OBJECT; 

//GDT and IDT

#include <pshpack2.h>
typedef struct _DTSTRUCT {
	USHORT wLimit;
	PVOID pvAddress;
} DTSTRUCT, *PDTSTRUCT;

typedef struct _KGDTENTRY {
    USHORT LimitLow;
    USHORT BaseLow;
    union {
        struct {
            UCHAR BaseMid;
            UCHAR Flags1;
            UCHAR Flags2;
            UCHAR BaseHi;
        } Bytes;
        struct {
            ULONG BaseMid       : 8;
            ULONG Type          : 5;
            ULONG Dpl           : 2;
            ULONG Pres          : 1;
            ULONG LimitHi       : 4;
            ULONG Sys           : 1;
            ULONG Reserved_0    : 1;
            ULONG Default_Big   : 1;
            ULONG Granularity   : 1;
            ULONG BaseHi        : 8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;

typedef struct _KIDTENTRY {
    USHORT Offset;
    USHORT Selector;
    USHORT Access;
    USHORT ExtendedOffset;
#if defined(_AMD64_) || defined(WIN64)
	ULONG Offset64;
	ULONG Reserved;
#endif
} KIDTENTRY, *PKIDTENTRY;
#include <poppack.h>

#if defined(_AMD64_) || defined(WIN64)
#define COUNTCRDR	11
#else
#define COUNTCRDR	10
#endif

#define MAX_DISK_GEOMETRY_EX_SIZE sizeof(DISK_GEOMETRY) + sizeof(LARGE_INTEGER) + sizeof(DISK_PARTITION_INFO) + sizeof(DISK_DETECTION_INFO)

// Defines the IOCTL codes used.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.
//

// Device type           -- in the "User Defined" range."
#define GPD_TYPE    40000

// Dos-Device name
#define GPD_DOSDEVICE		_T("\\\\.\\APestRidDrv")
#define GPD_DOSDEVICEGLOBAL	_T("\\\\.\\Global\\APestRidDrv")

#define GPD_PESTRIDSYSFILENAME _T("APestRidDrv.Sys")
#define GPD_PESTRIDSERVICENAME _T("APestRidDrv")

// NT device name for driver only
#define GPD_DEVICE_NAME L"\\Device\\APestRidDrv0"

// File system device name.   When you execute a CreateFile call to open the
// device, use "\\.\GpdDev", or, given C's conversion of \\ to \, use
// "\\\\.\\GpdDev"
//For driver only
#define DOS_DEVICE_NAME L"\\DosDevices\\APestRidDrv"
#define DOS_DEVICE_NAME_GLOBAL L"\\DosDevices\\APestRidDrv"


// The IOCTL function codes from 0x800 to 0xFFF are for customer use.

#define IOCTL_FOI_GETPROCESSOBJECTNAME\
			CTL_CODE( GPD_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_CLOSEHANDLE\
			CTL_CODE( GPD_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_COMPAREVERSION\
			CTL_CODE( GPD_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_OPENPROCESSTOKEN\
			CTL_CODE( GPD_TYPE, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_DUPLICATEOBJECT\
			CTL_CODE( GPD_TYPE, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETFILEDEVICEOBJECT\
			CTL_CODE( GPD_TYPE, 0x908, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READKSTACK\
			CTL_CODE( GPD_TYPE, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETKCONTEXT\
			CTL_CODE( GPD_TYPE, 0x90A, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETMUTANTOWNER\
			CTL_CODE( GPD_TYPE, 0x90B, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_COPYMEMORY\
			CTL_CODE( GPD_TYPE, 0x90C, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_QUERYDEP\
			CTL_CODE( GPD_TYPE, 0x90D, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETFILESHAREACCESS\
			CTL_CODE( GPD_TYPE, 0x90E, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_OPENPROCESS\
			CTL_CODE( GPD_TYPE, 0x90F, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READSSDT\
			CTL_CODE( GPD_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READSHADOWSSDT\
			CTL_CODE( GPD_TYPE, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READDEBUGGERNOTPRESENT\
			CTL_CODE( GPD_TYPE, 0x912, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READCONFIGURATIONINFORMATION\
			CTL_CODE( GPD_TYPE, 0x913, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READDEVICELIST\
			CTL_CODE( GPD_TYPE, 0x914, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READMODULELIST\
			CTL_CODE( GPD_TYPE, 0x915, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READMODULESECTION\
			CTL_CODE( GPD_TYPE, 0x916, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READMODULEFULLNAME\
			CTL_CODE( GPD_TYPE, 0x917, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READMODULEBASENAME\
			CTL_CODE( GPD_TYPE, 0x918, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READDRIVEROBJECT\
			CTL_CODE( GPD_TYPE, 0x919, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_OPENOBJECTBYHANDLE\
			CTL_CODE( GPD_TYPE, 0x920, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READDEVICEOBJECT\
			CTL_CODE( GPD_TYPE, 0x921, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READDRIVERNAME\
			CTL_CODE( GPD_TYPE, 0x922, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READDRIVERHARDWAREDATABASE\
			CTL_CODE( GPD_TYPE, 0x923, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_ENUMDRIVERDEVICES\
			CTL_CODE( GPD_TYPE, 0x924, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_COPYPAGEDMEMORY\
			CTL_CODE( GPD_TYPE, 0x925, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_COPYNONPAGEDMEMORY\
			CTL_CODE( GPD_TYPE, 0x926, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_RAWDISKREAD\
			CTL_CODE( GPD_TYPE, 0x927, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_RAWDISKWRITE\
			CTL_CODE( GPD_TYPE, 0x928, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETSSDT\
			CTL_CODE( GPD_TYPE, 0x929, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETSHADOWSSDT\
			CTL_CODE( GPD_TYPE, 0x92A, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READIDT\
			CTL_CODE( GPD_TYPE, 0x92B, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READGDT\
			CTL_CODE( GPD_TYPE, 0x92C, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READLDT\
			CTL_CODE( GPD_TYPE, 0x92D, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READTR\
			CTL_CODE( GPD_TYPE, 0x92E, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READMSW\
			CTL_CODE( GPD_TYPE, 0x92F, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READIDTVALUES\
			CTL_CODE( GPD_TYPE, 0x930, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READGDTVALUES\
			CTL_CODE( GPD_TYPE, 0x931, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_READCRDR\
			CTL_CODE( GPD_TYPE, 0x932, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETDISKGEOMETRY\
			CTL_CODE( GPD_TYPE, 0x933, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETDISKS\
			CTL_CODE( GPD_TYPE, 0x934, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FOI_GETDISKGEOMETRYEX\
			CTL_CODE( GPD_TYPE, 0x935, METHOD_BUFFERED, FILE_ANY_ACCESS )