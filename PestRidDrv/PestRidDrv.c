// File Object Info driver - peek protected kernel memory to get some
// information about file objects
//

#include "PestRidDrv.h"
#include <stdlib.h>
#include <stddef.h>

// Definition for ObQueryNameString call
//
NTKERNELAPI NTSTATUS NTAPI ObQueryNameString(IN PVOID Object, OUT POBJECT_NAME_INFORMATION ObjectNameInfo, IN ULONG Length, OUT PULONG ReturnLength);
NTKERNELAPI NTSTATUS NTAPI PsLookupProcessByProcessId(IN PVOID ProcessId, OUT PEPROCESS *Process);
NTKERNELAPI VOID NTAPI KeAttachProcess(IN PEPROCESS pEProcess);
NTKERNELAPI VOID NTAPI KeDetachProcess(); 
NTSYSAPI NTSTATUS NTAPI ZwOpenProcess(OUT PHANDLE ProcessHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, IN PCLIENT_ID ClientId OPTIONAL);
NTSYSAPI NTSTATUS NTAPI ZwOpenProcessToken(IN HANDLE ProcessHandle, IN ACCESS_MASK DesiredAccess, OUT PHANDLE TokenHandle);
NTSYSAPI NTSTATUS NTAPI ZwDuplicateObject(IN HANDLE SourceProcessHandle, IN HANDLE SourceHandle, IN HANDLE TargetProcessHandle OPTIONAL, OUT PHANDLE TargetHandle OPTIONAL, IN ACCESS_MASK DesiredAccess, IN ULONG HandleAttributes, IN ULONG Options);
NTSYSAPI NTSTATUS NTAPI ZwQueryInformationProcess(IN HANDLE ProcessHandle, IN PROCESSINFOCLASS ProcessInformationClass, OUT PVOID ProcessInformation, IN ULONG ProcessInformationLength, OUT PULONG ReturnLength OPTIONAL);
NTKERNELAPI VOID NTAPI SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext);
NTKERNELAPI BOOLEAN NTAPI SePrivilegeCheck(IN OUT PPRIVILEGE_SET RequiredPrivileges, IN PSECURITY_SUBJECT_CONTEXT SubjectContext, IN KPROCESSOR_MODE AccessMode);
NTKERNELAPI VOID NTAPI SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext);
NTKERNELAPI NTSTATUS NTAPI ObOpenObjectByPointer(IN PVOID Object, IN ULONG HandleAttributes, IN PACCESS_STATE PassedAccessState OPTIONAL, IN ACCESS_MASK DesiredAccess OPTIONAL, IN POBJECT_TYPE ObjectType OPTIONAL, IN KPROCESSOR_MODE AccessMode, OUT PHANDLE Handle);
NTKERNELAPI NTSTATUS NTAPI ObReferenceObjectByName(
IN PUNICODE_STRING ObjectPath, 
IN ULONG Attributes, 
IN PACCESS_STATE PassedAccessState OPTIONAL, 
IN ACCESS_MASK DesiredAccess OPTIONAL, 
IN POBJECT_TYPE ObjectType, 
IN KPROCESSOR_MODE AccessMode, 
IN OUT PVOID ParseContext OPTIONAL, 
OUT PVOID *ObjectPtr);
NTKERNELAPI NTSTATUS NTAPI ObOpenObjectByName(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PACCESS_STATE AccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle);
NTKERNELAPI NTSTATUS NTAPI IoEnumerateDeviceObjectList(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  *DeviceObjectList,
    IN ULONG  DeviceObjectListSize,
    OUT PULONG  ActualNumberDeviceObjects);

NTSYSAPI ULONG NtBuildNumber;

#define FileTypeIndex 5
#define ProcessExecuteFlags 0x22
#define PROCESS_VM_OPERATION      (0x0008)
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

//no longer exported so must calculate
PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable = NULL;
PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTableShadow = NULL;

extern USHORT _idtlimit;
extern ULONGLONG _idtbase;

extern void _getidt(PDTSTRUCT);
extern void _getgdt(PDTSTRUCT);
extern void _getldt(PUSHORT);
extern void _gettr(PUSHORT);
extern void _getmsw(PUSHORT);
extern void _getcr0(PUINT_PTR);
extern void _getcr2(PUINT_PTR);
extern void _getcr3(PUINT_PTR);
extern void _getcr4(PUINT_PTR);
#ifdef _AMD64_
extern void _getcr8(PUINT_PTR);
#endif
extern void _getdr0(PUINT_PTR);
extern void _getdr1(PUINT_PTR);
extern void _getdr2(PUINT_PTR);
extern void _getdr3(PUINT_PTR);
extern void _getdr6(PUINT_PTR);
extern void _getdr7(PUINT_PTR);

void GetKeServiceDescriptorTables64()
{
	//KeServiceDescriptorTable, KeServiceDescriptorTableShadow
	//  are in between KdDebuggerNotPresent and &_strnicmp
	DWORD_PTR dwCount;
	UCHAR* pSearch = (UCHAR*)(ULONGLONG)&_strnicmp;
	//search for 13 byte code signature
	//KiSystemServiceStart mov edi, eax  shr edi, 7  and edi, 0x20  and eax, 0xFFF
	UCHAR bSig[] = { 0x8B, 0xF8, 0xC1, 0xEF, 0x07, 0x83, 0xE7, 0x20, 0x25, 0xFF, 0x0F, 0x00, 0x00 };
	while ((ULONGLONG)KdDebuggerNotPresent - (ULONGLONG)pSearch != 0) {
		//search for first byte and skip by that amount to greatly reduce search time
		if (RtlCompareMemory(pSearch, bSig, sizeof(bSig)) == 0) {
			for (dwCount = 0; dwCount < 50; dwCount++) {
				//find the following lea r10, KeServiceDescriptorTable lea r11, KeServiceDescriptorTableShadow opcodes within 50 bytes
				//which is start of function KiSystemServiceRepeat which loads addresses of SSDT and Shadow SSDT
				if (*((USHORT*)(pSearch + dwCount)) == 0x8D4C &&
					*((UCHAR*)(pSearch + dwCount + 2)) == 0x15 &&
					*((USHORT*)(pSearch + dwCount + 7)) == 0x8D4C &&
					*((UCHAR*)(pSearch + dwCount + 7 + 2)) == 0x1D) {
					//offset is this plus 7 bytes for instruction length
					//	and the 4 byte relative offset which is 3 bytes after the signature
					//Shadow SSDT is identical immediately after
					KeServiceDescriptorTable = (PKSERVICE_TABLE_DESCRIPTOR)(pSearch + dwCount + 7 + *(ULONG*)(pSearch + dwCount + 3));
					KeServiceDescriptorTableShadow = (PKSERVICE_TABLE_DESCRIPTOR)(pSearch + dwCount + 7 + 7 + *(ULONG*)(pSearch + dwCount + 7 + 3));
					return;
				}
			}
		}
		pSearch++;
	}
	return;
}

NTSTATUS getidt(DTSTRUCT* pDT)
{
	PDTSTRUCT pBuf;
	pBuf = ExAllocatePool(NonPagedPool, sizeof(DTSTRUCT));
	if (pBuf) {
		_getidt(pBuf);
		RtlCopyMemory(pDT, pBuf, sizeof(DTSTRUCT));
		ExFreePool(pBuf);
		return STATUS_SUCCESS;
	} else return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS getgdt(DTSTRUCT* pDT)
{
	PDTSTRUCT pBuf;
	pBuf = ExAllocatePool(NonPagedPool, sizeof(DTSTRUCT));
	if (pBuf) {
		_getgdt(pBuf);
		RtlCopyMemory(pDT, pBuf, sizeof(DTSTRUCT));
		ExFreePool(pBuf);
		return STATUS_SUCCESS;
	} else return STATUS_INSUFFICIENT_RESOURCES;
}

void ReturnUnicodeString(PUNICODE_STRING pusz, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	if (pusz->Buffer && MmIsAddressValid(pusz->Buffer)) {
		((PUNICODE_STRING)OutputBuffer)->Length = pusz->Length;
		if ((((PUNICODE_STRING)OutputBuffer)->MaximumLength = (USHORT)min((OutputBufferLength - sizeof(UNICODE_STRING)), 0xFFFF)) != 0 &&
			pusz->Length != 0) {
			RtlCopyMemory((PWSTR)((UCHAR*)OutputBuffer + sizeof(UNICODE_STRING)), pusz->Buffer, min(pusz->Length, ((PUNICODE_STRING)OutputBuffer)->MaximumLength));
		}
		//cannot use buffer pointer as the output buffer is copied into user mode space
		//  user mode must repair this pointer to be immediately after the UNICODE_STRING
	} else {
		((PUNICODE_STRING)OutputBuffer)->Length = 0;
		((PUNICODE_STRING)OutputBuffer)->MaximumLength = 0;
	}
	((PUNICODE_STRING)OutputBuffer)->Buffer = NULL;
}

void DoCopyPage(PVOID pvAddress, PVOID OutputBuffer, ULONG OutputBufferLength, BOOLEAN bIsPaged, BOOLEAN bIsCode)
{
	PVOID pvHandle = NULL;
	//ntoskrnl.exe and perhaps others require very special handling for the paged sections...
	//PVOID pvSecure = MmSecureVirtualMemory(pvAddress, OutputBufferLength, PAGE_READONLY);
	//if (pvSecure) {
		if (bIsPaged) {
			__try {
			if (bIsCode) {
				pvHandle = MmLockPagableCodeSection(pvAddress);
			} else {
				pvHandle = MmLockPagableDataSection(pvAddress);
			}
			//must continue or risk APC_INDEX_MISMATCH
			} __except (EXCEPTION_CONTINUE_EXECUTION) {
			}
		}
		if (MmIsAddressValid(pvAddress) && MmIsAddressValid(pvAddress)) {
			__try {
				RtlCopyMemory(OutputBuffer, pvAddress, OutputBufferLength);
			//must continue or risk APC_INDEX_MISMATCH
			} __except (EXCEPTION_CONTINUE_EXECUTION) {
			}
		}
		if (bIsPaged) {
			MmUnlockPagableImageSection(pvHandle);
		}
	//	MmUnsecureVirtualMemory(pvSecure);
	//}
	return;
	/*long Status = STATUS_UNSUCCESSFUL;
	PMDL pMdl;
	if (MmIsAddressValid(pvAddress)) {
		if ((pMdl = IoAllocateMdl(pvAddress, OutputBufferLength, FALSE, FALSE, NULL)) != NULL) {
			__try {
				MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
				Status = STATUS_SUCCESS;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				Status = STATUS_ACCESS_VIOLATION;
			}
			if (Status == STATUS_SUCCESS) {
				PVOID MappedBuffer = MmMapLockedPagesSpecifyCache(pMdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
				if (MappedBuffer) {
					RtlCopyMemory(OutputBuffer, MappedBuffer, OutputBufferLength);
					MmUnmapLockedPages(MappedBuffer, pMdl);
				} else Status = STATUS_UNSUCCESSFUL;
				MmUnlockPages(pMdl);
			}
			IoFreeMdl(pMdl);
		}
	}
	if (Status != STATUS_SUCCESS) {
		RtlZeroMemory(OutputBuffer, OutputBufferLength);
	}*/
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	PLOCAL_DEVICE_INFO pLocalInfo;
	NTSTATUS Status;
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING NtDeviceName;
	UNICODE_STRING Win32DeviceName;
	char* Pool;
	ULONG ulCount;
	if (RegistryPath == NULL) {
		Pool = ExAllocatePoolWithTag(NonPagedPool, sizeof(DRIVER_OBJECT) * 2, 0x206B6444);
		RtlZeroMemory(Pool, sizeof(DRIVER_OBJECT) * 2);
		DriverObject = (PDRIVER_OBJECT)(Pool + sizeof(DRIVER_OBJECT));
		for (ulCount = 0; ulCount <= IRP_MJ_MAXIMUM_FUNCTION; ulCount++) {
			DriverObject->MajorFunction[ulCount] = GpdDispatch;
		}
    }
	DriverObject->MajorFunction[IRP_MJ_CREATE] = GpdDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = GpdDispatch;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = GpdDispatch;
	DriverObject->DriverUnload = GpdUnload;
	RtlInitUnicodeString(&NtDeviceName, GPD_DEVICE_NAME);
	Status = IoCreateDevice(DriverObject, sizeof(LOCAL_DEVICE_INFO), &NtDeviceName, GPD_TYPE, 0, FALSE, &DeviceObject);
	if (NT_SUCCESS(Status)) {
		RtlZeroMemory(DeviceObject->DeviceExtension, sizeof(LOCAL_DEVICE_INFO));
		Status = STATUS_INVALID_PARAMETER;
		if (RegistryPath == NULL) {
			RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME_GLOBAL);
			Status = IoCreateSymbolicLink(&Win32DeviceName, &NtDeviceName);
		}
		if (!NT_SUCCESS(Status)) {
			RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);
			Status = IoCreateSymbolicLink(&Win32DeviceName, &NtDeviceName);
		}
		if (!NT_SUCCESS(Status)) {
			IoDeleteDevice(DeviceObject);
		}
	}
    if (NT_SUCCESS(Status)) {
        pLocalInfo = (PLOCAL_DEVICE_INFO)DeviceObject->DeviceExtension;
        pLocalInfo->DeviceObject = DeviceObject;
        pLocalInfo->DeviceType = GPD_TYPE;
    }
	GetKeServiceDescriptorTables64();
    return Status;
}

NTSTATUS GpdDeviceControl(PFILE_OBJECT pFileObject, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK pIoStatus, PDEVICE_OBJECT pDO)
{
	PVOID Obj;
	HANDLE ProcessHandle;
	HANDLE TargetHandle;
	ULONG ReturnLength;
	ULONG Counter;
	ULONG UniStringsLength;
	char* Buf;
	PFILE_OBJECT Ptr;
	PKSERVICE_TABLE_DESCRIPTOR pKeServiceDescriptorTable;
	PCONFIGURATION_INFORMATION pConfigInfo;
	PWSTR pwstr;
	NTSTATUS Status;
	ANSI_STRING AnsiString;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES ObjectAttributes;
	char OldIrql;
	ULONG ulMajorVersion;
	UNICODE_STRING uszCore;
	PDRIVER_OBJECT pDriverObj;
	PDEVICE_OBJECT pDeviceObj;
	PLDR_DATA_TABLE_ENTRY pdte;
	DTSTRUCT dt;
	pFileObject;//unused
	pDO;//unused
	Status = STATUS_SUCCESS;
	pIoStatus->Information = 0;
	__try {
		switch (IoControlCode) {
		case IOCTL_FOI_GETPROCESSOBJECTNAME:
			// IN [ULONG PID, PVOID Object, HANDLE ObjectHandle]
			// OUT [LPTSTR Name]
			Obj = 0;
			if (*((ULONG*)InputBuffer) < 8) {
				if (NT_SUCCESS(PsLookupProcessByProcessId(*((PVOID*)InputBuffer), (PEPROCESS*)&ProcessHandle))) {
					KeAttachProcess(ProcessHandle);
					if (NT_SUCCESS(ObReferenceObjectByHandle(*((HANDLE*)InputBuffer + 2), GENERIC_READ, 0, 0, &Obj, 0))) {
						if (*((PVOID*)InputBuffer + 1) != Obj) {
							ObfDereferenceObject(Obj);
							Obj = 0;
						}
					} else {
						Obj = 0;
					}
					KeDetachProcess();
					ObfDereferenceObject(ProcessHandle);
				}
			} else {
				ClientId.UniqueProcess = *((HANDLE*)InputBuffer);
				ClientId.UniqueThread = 0;
				ObjectAttributes.Length = sizeof(ObjectAttributes);
				ObjectAttributes.RootDirectory = 0;
				ObjectAttributes.Attributes = 0;
				ObjectAttributes.ObjectName = 0;
				ObjectAttributes.SecurityDescriptor = 0;
				ObjectAttributes.SecurityQualityOfService = 0;
				if (NT_SUCCESS(ZwOpenProcess(&ProcessHandle, PROCESS_DUP_HANDLE, &ObjectAttributes, &ClientId))) {
					if (NT_SUCCESS(ZwDuplicateObject(ProcessHandle, *((HANDLE*)InputBuffer + 2), INVALID_HANDLE_VALUE, &TargetHandle, 0, 0, DUPLICATE_SAME_ACCESS))) {
						ZwClose(ProcessHandle);
						if (NT_SUCCESS(ObReferenceObjectByHandle(TargetHandle, GENERIC_READ, 0, 0, &Obj, 0))) {
							if (*((PVOID*)InputBuffer + 1) != Obj) {
								ObfDereferenceObject(Obj);
								Obj = 0;
							}
						} else {
							Obj = 0;
						}
						ZwClose(TargetHandle);
					} else
						ZwClose(ProcessHandle);
				}
			}
			if (Obj) {
				*((char*)OutputBuffer) = 0;
				if ((((PFILE_OBJECT)Obj)->Type != FileTypeIndex) || (((PFILE_OBJECT)Obj)->Size != sizeof(FILE_OBJECT))) {
					if (NT_SUCCESS(Status = ObQueryNameString(Obj, (POBJECT_NAME_INFORMATION)OutputBuffer, (ULONG)OutputBufferLength, &ReturnLength))) {
						if (((POBJECT_NAME_INFORMATION)OutputBuffer)->Name.Length != 0) {
							if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString, &((POBJECT_NAME_INFORMATION)OutputBuffer)->Name, TRUE))) {
								strncpy(OutputBuffer, AnsiString.Buffer, (AnsiString.Length >= OutputBufferLength) ? (OutputBufferLength - 1) : AnsiString.Length);
								((char*)OutputBuffer)[(AnsiString.Length >= OutputBufferLength) ? (OutputBufferLength - 1) : AnsiString.Length] = 0;
								RtlFreeAnsiString(&AnsiString);
							} else
								*((char*)OutputBuffer) = 0;
						} else
							*((char*)OutputBuffer) = 0;
					} else
						*((char*)OutputBuffer) = 0;
				} else {
					Buf = OutputBuffer;
					if (((PFILE_OBJECT)Obj)->DeviceObject) {
						if (NT_SUCCESS(ObQueryNameString(((PFILE_OBJECT)Obj)->DeviceObject, (POBJECT_NAME_INFORMATION)OutputBuffer, (ULONG)OutputBufferLength, &ReturnLength))) {
							if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString, &((POBJECT_NAME_INFORMATION)OutputBuffer)->Name, TRUE))) {
								strncpy(Buf, AnsiString.Buffer, (AnsiString.Length >= OutputBufferLength) ? (OutputBufferLength - 1) : AnsiString.Length);
								((char*)Buf)[(AnsiString.Length >= OutputBufferLength) ? (OutputBufferLength - 1) : AnsiString.Length] = 0;
								Buf += ((AnsiString.Length >= OutputBufferLength) ? (OutputBufferLength - 1) : AnsiString.Length);
								RtlFreeAnsiString(&AnsiString);
							} else
								*((char*)OutputBuffer) = 0;
						} else
							*((char*)OutputBuffer) = 0;
					} else
						*((char*)OutputBuffer) = 0;
					Counter = (((PFILE_OBJECT)Obj)->FileName.Length >> 1);
					Ptr = ((PFILE_OBJECT)Obj)->RelatedFileObject;
					if ((Counter != 0) || Ptr) {
						if (((PFILE_OBJECT)Obj)->FileName.Buffer[0] != 0x5C) {
							while (Ptr) {
								Counter += (Ptr->FileName.Length >> 1) + 1;
								Ptr = Ptr->RelatedFileObject;
							}
						}
						Counter -= (((PFILE_OBJECT)Obj)->FileName.Length >> 1);
						RtlUnicodeStringToAnsiString(&AnsiString, &((PFILE_OBJECT)Obj)->FileName, TRUE);
						RtlCopyMemory(&((char*)Buf)[Counter], AnsiString.Buffer, AnsiString.Length);
						((char*)Buf)[AnsiString.Length + Counter] = 0;
						RtlFreeAnsiString(&AnsiString);
						if (((PFILE_OBJECT)Obj)->FileName.Buffer[0] != 0x5C) {
							Ptr = ((PFILE_OBJECT)Obj)->RelatedFileObject;
							while (Ptr) {
								((char*)Buf)[Counter - 1] = 0x5C;
								Counter -= (1 + (Ptr->FileName.Length >> 1));
								RtlUnicodeStringToAnsiString(&AnsiString, &Ptr->FileName, TRUE);
								RtlCopyMemory(&((char*)Buf)[Counter], AnsiString.Buffer, AnsiString.Length);
								RtlFreeAnsiString(&AnsiString);
								Ptr = Ptr->RelatedFileObject;
							}
							if ((((PFILE_OBJECT)Obj)->FileName.Length >> 1) > 3) {
								if ((((char*)OutputBuffer)[2] == 0x5C) && (((char*)OutputBuffer)[3] == 0x5C)) {
									memmove(&((char*)OutputBuffer)[2], &((char*)OutputBuffer)[3], strlen(&((char*)OutputBuffer)[3]) + 1);
								}
							}
						}
					}
				}
				pIoStatus->Information = strlen(OutputBuffer) + 1;
				ObfDereferenceObject(Obj);
			} else {
				Status = STATUS_ACCESS_DENIED;
			}
			break;
		case IOCTL_FOI_CLOSEHANDLE:
			//IN [ULONG PID, PVOID Object, HANDLE ObjectHandle]
			//OUT []
			if (NT_SUCCESS(Status = PsLookupProcessByProcessId(*((PVOID*)InputBuffer), (PEPROCESS*)&ProcessHandle))) {
				KeAttachProcess(ProcessHandle);
				if (NT_SUCCESS(Status = ObReferenceObjectByHandle(*((HANDLE*)InputBuffer + 2), GENERIC_READ, 0, 0, &Obj, 0))) {
					if (*((PVOID*)InputBuffer + 1) == Obj) {
						ZwClose(*((HANDLE*)InputBuffer + 2));
					}
					ObfDereferenceObject(Obj);
				}
				KeDetachProcess();
				ObfDereferenceObject(ProcessHandle);
			}
			break;
		case IOCTL_FOI_COMPAREVERSION:
			//IN [ULONG TestVersion]
			//OUT [ULONG FixedVersion]
			if (*((ULONG*)InputBuffer) > 0x35D)
				Status = STATUS_REVISION_MISMATCH;
			else {
				*((ULONG*)OutputBuffer) = 0x35D;
				pIoStatus->Information = sizeof(ULONG);
			}
			break;
		case IOCTL_FOI_OPENPROCESSTOKEN:
			//IN [HANDLE ProcessHandle]
			//OUT [HANDLE ProcessTokenHandle]
			if (NT_SUCCESS(Status = ZwOpenProcessToken(*((HANDLE*)InputBuffer), PROCESS_VM_OPERATION, (HANDLE*)OutputBuffer)))
				pIoStatus->Information = sizeof(ULONG);
			break;
		case IOCTL_FOI_DUPLICATEOBJECT:
			//IN [HANDLE ProcessHandle, HANDLE ObjectHandle]
			//OUT [HANDLE DuplicateHandle]
			if (NT_SUCCESS(Status = ZwDuplicateObject(*((HANDLE*)InputBuffer), *((HANDLE*)InputBuffer + 1), INVALID_HANDLE_VALUE, (HANDLE*)OutputBuffer, GENERIC_ALL, 0, 0)))
				pIoStatus->Information = sizeof(ULONG);
			break;
		case IOCTL_FOI_GETFILEDEVICEOBJECT:
			//IN [PFILE_OBJECT FileObject]
			//OUT [PDEVICE_OBJECT DeviceObject]
			if ((InputBufferLength == sizeof(PFILE_OBJECT)) && (OutputBufferLength == sizeof(PDEVICE_OBJECT)) && (((PFILE_OBJECT)InputBuffer)->Type == FileTypeIndex) && (((PFILE_OBJECT)InputBuffer)->Size == sizeof(FILE_OBJECT))) {
				*((PDEVICE_OBJECT*)OutputBuffer) = ((PFILE_OBJECT)InputBuffer)->DeviceObject;
				pIoStatus->Information = sizeof(ULONG);
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READKSTACK:
			//IN [HANDLE ObjectHandle, PVOID Object]
			//OUT [VOID MemoryData]
			// needs to be cleaned up
			if NT_SUCCESS(ObReferenceObjectByHandle(*((HANDLE*)InputBuffer), 0, 0, 0, &Obj, 0)) {
				KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
				if ((((ULONG*)Obj + 6) <= (ULONG*)((char*)*((PVOID*)InputBuffer + 1) + OutputBufferLength)) && (((ULONG*)Obj + 7) >= (ULONG*)*((PVOID*)InputBuffer + 1))) {
					if ((Counter = OutputBufferLength) > 0) {
						do {
							*((char*)OutputBuffer + Counter) = (MmIsAddressValid((char*)*((PVOID*)InputBuffer + 1) + Counter)) ? *((char*)*((PVOID*)InputBuffer + 1) + Counter) : 0;
						} while (--Counter);
					}
				} else {
					RtlCopyMemory(OutputBuffer, *((PVOID*)InputBuffer + 1), OutputBufferLength);
				}
				pIoStatus->Information = OutputBufferLength;
				KeLowerIrql(OldIrql);
				ObfDereferenceObject(Obj);
			}
			break;
		case IOCTL_FOI_GETKCONTEXT:
			//IN [HANDLE ContextHandle]
			//OUT [PVOID PVOID* PVOID]
			// needs to be cleaned up
			if (NT_SUCCESS(ObReferenceObjectByHandle(*((HANDLE*)InputBuffer), 0, 0, 0, &Obj, 0))) {
				PVOID ObjPtr;
				ObjPtr = (PVOID)((char*)Obj + (((NtBuildNumber && 0xFFFFFFF) >= 0xECE) ? 0x20 : 0x28));
				if (MmIsAddressValid((PVOID)((char*)ObjPtr + 12))) {
					*((PVOID**)OutputBuffer + 1) = ((PVOID*)ObjPtr + 3);
					*((PVOID*)OutputBuffer + 2) = *((PVOID*)ObjPtr + 3);
					*((PVOID*)OutputBuffer) = *((PVOID*)ObjPtr + 2);
					pIoStatus->Information = 12;
				} else
					Status = STATUS_ACCESS_VIOLATION;
				ObfDereferenceObject(Obj);
			}
			break;
		case IOCTL_FOI_GETMUTANTOWNER:
			//IN [HANDLE MutantHandle]
			//OUT [HANDLE MutantOwnerHandle]
			if (NT_SUCCESS(Status = ObReferenceObjectByHandle(*((HANDLE*)InputBuffer), 0, 0, 0, &Obj, 0))) {
				if (((KMUTANT*)Obj)->OwnerThread) {
					ObOpenObjectByPointer(((KMUTANT*)Obj)->OwnerThread, 0, 0, GENERIC_ALL, 0, 0, (HANDLE*)OutputBuffer);
				} else
					*((HANDLE*)OutputBuffer) = 0;
				pIoStatus->Information = sizeof(ULONG);
				ObfDereferenceObject(Obj);
			}
			break;
		case IOCTL_FOI_COPYMEMORY://data
		case IOCTL_FOI_COPYPAGEDMEMORY://code
		case IOCTL_FOI_COPYNONPAGEDMEMORY://code
			//IN [PVOID SourceAddress]
			//OUT [VOID MemoryData]
			//if (MmIsAddressValid(*((PVOID*)InputBuffer)) && MmIsAddressValid((PVOID)((char*)*((PVOID*)InputBuffer) + OutputBufferLength))) {
				//must scan page by page in case parts of the driver memory have been freed or reused as paged to avoid page fault in non paged area
				MmResetDriverPaging(*((PVOID*)InputBuffer));
				Counter = (ULONG)(ULONG_PTR)*((PVOID*)InputBuffer) & PAGE_SIZE;
				if (Counter) {
					Counter = PAGE_SIZE - Counter;
					DoCopyPage(*((PVOID*)InputBuffer), OutputBuffer, OutputBufferLength < Counter ? OutputBufferLength : Counter, IoControlCode != IOCTL_FOI_COPYNONPAGEDMEMORY, IoControlCode != IOCTL_FOI_COPYMEMORY);
				}
				for (; Counter < OutputBufferLength; Counter += PAGE_SIZE) {
					DoCopyPage((UCHAR*)*((PVOID*)InputBuffer) + Counter, (UCHAR*)OutputBuffer + Counter, (OutputBufferLength - Counter) < PAGE_SIZE ? (OutputBufferLength - Counter) : PAGE_SIZE, IoControlCode != IOCTL_FOI_COPYNONPAGEDMEMORY, IoControlCode != IOCTL_FOI_COPYMEMORY);
				}
				pIoStatus->Information = OutputBufferLength;
				Status = STATUS_SUCCESS;
			//} else
				//Status = STATUS_INVALID_PARAMETER;
			break;
		case IOCTL_FOI_QUERYDEP:
			//IN [HANDLE ProcessHandle]
			//OUT [ULONG DepStatus]
			Obj = INVALID_HANDLE_VALUE;
			if (NT_SUCCESS(Status = ObReferenceObjectByHandle(*((HANDLE*)InputBuffer), GENERIC_READ, 0, 0, &Obj, 0))) {
				KeAttachProcess(Obj);
				if (NT_SUCCESS(Status = ZwQueryInformationProcess(INVALID_HANDLE_VALUE, ProcessExecuteFlags, OutputBuffer, 4, &ReturnLength))) {
					pIoStatus->Information = ReturnLength;
				}
				ObfDereferenceObject(Obj);			
				KeDetachProcess();
			}
			break;
		case IOCTL_FOI_GETFILESHAREACCESS:
			//IN [PFILE_OBJECT Object]
			//OUT [ULONG Result]
			if ((InputBufferLength == sizeof(PFILE_OBJECT)) && (OutputBufferLength == sizeof(ULONG)) && (((PFILE_OBJECT)InputBuffer)->Type == FileTypeIndex) && (((PFILE_OBJECT)InputBuffer)->Size == sizeof(FILE_OBJECT))) {
				*((ULONG*)OutputBuffer) = ((((PFILE_OBJECT)InputBuffer)->SharedRead) ? 1 : 0) | ((((PFILE_OBJECT)InputBuffer)->SharedWrite) ? 2 : 0) | ((((PFILE_OBJECT)InputBuffer)->SharedDelete) ? 4 : 0);
				pIoStatus->Information = sizeof(ULONG);
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_OPENPROCESS:
			//IN [ULONG PID]
			//OUT [HANDLE ProcessHandle]
			ClientId.UniqueProcess = *((HANDLE*)InputBuffer);
			ClientId.UniqueThread = 0;
			ObjectAttributes.Length = sizeof(ObjectAttributes);
			ObjectAttributes.RootDirectory = 0;
			ObjectAttributes.Attributes = 0;
			ObjectAttributes.ObjectName = 0;
			ObjectAttributes.SecurityDescriptor = 0;
			ObjectAttributes.SecurityQualityOfService = 0;
			if (NT_SUCCESS(Status = ZwOpenProcess((PHANDLE)OutputBuffer, GENERIC_ALL, &ObjectAttributes, &ClientId)))
				pIoStatus->Information = sizeof(ULONG);
			break;
		case IOCTL_FOI_GETSSDT:
		case IOCTL_FOI_GETSHADOWSSDT:
			pKeServiceDescriptorTable = (IoControlCode == IOCTL_FOI_GETSHADOWSSDT) ? KeServiceDescriptorTableShadow : KeServiceDescriptorTable;
			if (MmIsAddressValid(pKeServiceDescriptorTable) &&
				OutputBufferLength == sizeof(KSERVICE_TABLE_DESCRIPTOR_RECEIVE)) {
				((PKSERVICE_TABLE_DESCRIPTOR_RECEIVE)OutputBuffer)->pKeServiceTable = pKeServiceDescriptorTable;
				((PKSERVICE_TABLE_DESCRIPTOR_RECEIVE)OutputBuffer)->ServiceTableBase = pKeServiceDescriptorTable->ServiceTableBase;
				((PKSERVICE_TABLE_DESCRIPTOR_RECEIVE)OutputBuffer)->ServiceCounterTableBase = pKeServiceDescriptorTable->ServiceCounterTableBase;
				((PKSERVICE_TABLE_DESCRIPTOR_RECEIVE)OutputBuffer)->NumberOfServices = pKeServiceDescriptorTable->NumberOfServices;
				((PKSERVICE_TABLE_DESCRIPTOR_RECEIVE)OutputBuffer)->ParamTableBase = pKeServiceDescriptorTable->ParamTableBase;
				pIoStatus->Information = sizeof(KSERVICE_TABLE_DESCRIPTOR_RECEIVE);
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READSSDT:
		case IOCTL_FOI_READSHADOWSSDT:
			//IN [UINT_PTR dwIndex]
			//OUT [PVOID pFunctionAddress]
			pKeServiceDescriptorTable = (IoControlCode == IOCTL_FOI_READSHADOWSSDT) ? KeServiceDescriptorTableShadow : KeServiceDescriptorTable;
			if (InputBufferLength == sizeof(UINT_PTR) &&
				MmIsAddressValid(pKeServiceDescriptorTable) &&
				OutputBufferLength == sizeof(UINT_PTR)) {
				if (*((UINT_PTR*)InputBuffer) < pKeServiceDescriptorTable->NumberOfServices) {
					//must sign extend it as offset can be negative if entry before ServiceTableBase
					*((INT_PTR*)OutputBuffer) = (int)pKeServiceDescriptorTable->ServiceTableBase[*((UINT_PTR*)InputBuffer)];
					PsGetVersion(&ulMajorVersion, NULL, NULL, NULL);
					if (ulMajorVersion >= 6) {
						*((INT_PTR*)OutputBuffer) >>= 4;
					} else {
						//why not 0xFFFFFFFFFFFFFFF0?
						*((UINT_PTR*)OutputBuffer) &= 0xFFFFFFF0;
					}
					*((UINT_PTR*)OutputBuffer) += (UINT_PTR)pKeServiceDescriptorTable->ServiceTableBase;
				} else {
					*((UINT_PTR*)OutputBuffer) = 0;
				}
				pIoStatus->Information = sizeof(UINT_PTR);
				Status = STATUS_SUCCESS;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READDEBUGGERNOTPRESENT:
			//OUT [UINT_PTR]
#ifdef KD_DEBUGGER_NOT_PRESENT
			if (OutputBufferLength == sizeof(UINT_PTR)) {
				*((UINT_PTR*)OutputBuffer) = KD_DEBUGGER_NOT_PRESENT;
				pIoStatus->Information = sizeof(UINT_PTR);
				Status = STATUS_SUCCESS;
#else
			if (FALSE) {
#endif
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READCONFIGURATIONINFORMATION:
			//OUT [CONFIGURATION_INFORMATION]
			//must be running at default of PASSIVE_LEVEL
			pConfigInfo = IoGetConfigurationInformation();
			if (OutputBufferLength == sizeof(CONFIGURATION_INFORMATION) &&
				MmIsAddressValid(pConfigInfo)) {
				RtlCopyMemory(OutputBuffer, pConfigInfo, sizeof(CONFIGURATION_INFORMATION));
				pIoStatus->Information = sizeof(CONFIGURATION_INFORMATION);
				Status = STATUS_SUCCESS;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READDEVICELIST:
			//IN [GUID]
			//OUT [ULONG LPAWSTRS]
			if (InputBufferLength == sizeof(GUID) &&
				OutputBufferLength >= sizeof(ULONG) &&
				IoGetDeviceInterfaces(	InputBuffer, NULL,
										DEVICE_INTERFACE_INCLUDE_NONACTIVE,
										&pwstr) == STATUS_SUCCESS) {
				Counter = 0;
				while (wcslen(pwstr + Counter)) {
					Counter += (ULONG)wcslen(pwstr + Counter) + 1;
				}
				*(ULONG*)OutputBuffer = (Counter + 1) * sizeof(WCHAR);
				if (OutputBufferLength > sizeof(ULONG)) {
					RtlCopyMemory((UCHAR*)OutputBuffer + sizeof(ULONG), pwstr, (Counter + 1) * sizeof(WCHAR));
					pIoStatus->Information = sizeof(ULONG) + (Counter + 1) * sizeof(WCHAR);
				} else {
					pIoStatus->Information = sizeof(ULONG);
				}
				ExFreePool(pwstr);
				Status = STATUS_SUCCESS;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READMODULELIST:
			//OUT [ULONG PLDR_DATA_TABLE_ENTRYs]
			//extremely dangerous as module load/unload table modifications race conditions are possible
			//  need to see if we can lock the table or the processor
			//must search code for reference to PsLoadedModuleResource which is not exported
			//ExAcquireResourceShared(&PsLoadedModuleResource, TRUE);
            //ExReleaseResource(&PsLoadedModuleResource);
			Counter = 0;
			if (MmIsAddressValid(pDO) && MmIsAddressValid(pDO->DriverObject)) {
				PLDR_DATA_TABLE_ENTRY pdte = pDO->DriverObject->DriverSection;
				do {
					if (MmIsAddressValid(pdte)) {
						if (OutputBufferLength >= (sizeof(ULONG) + (Counter + 1) * sizeof(PLDR_DATA_TABLE_ENTRY)))
							*((PLDR_DATA_TABLE_ENTRY*)((UCHAR*)OutputBuffer + sizeof(ULONG) + sizeof(PLDR_DATA_TABLE_ENTRY) * Counter)) = pdte;
						Counter++;
						pdte = (PLDR_DATA_TABLE_ENTRY)pdte->InLoadOrderLinks.Flink;
					} else
						break;
				} while (pdte != pDO->DriverObject->DriverSection);
				if (OutputBufferLength >= sizeof(ULONG))
					*(ULONG*)OutputBuffer = Counter;
				pIoStatus->Information = min(OutputBufferLength, (sizeof(ULONG) + Counter * sizeof(PLDR_DATA_TABLE_ENTRY)));
				Status = STATUS_SUCCESS;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READMODULESECTION:
			//IN [PLDR_DATA_TABLE_ENTRY]
			//OUT [LDR_DATA_TABLE_RECEIVE_ENTRY]
			//should also pass driver handle and reference it for pointer safety
			//  extremely dangerous as module may be cleaned up or removed
			Counter = 0;
			if (InputBufferLength == sizeof(PLDR_DATA_TABLE_ENTRY) &&
				MmIsAddressValid(pdte = *((PLDR_DATA_TABLE_ENTRY*)InputBuffer)) &&
				OutputBufferLength == sizeof(LDR_DATA_TABLE_RECEIVE_ENTRY)) {
				((PLDR_DATA_TABLE_RECEIVE_ENTRY)OutputBuffer)->DllBase = pdte->DllBase;
				((PLDR_DATA_TABLE_RECEIVE_ENTRY)OutputBuffer)->EntryPoint = pdte->EntryPoint;
				((PLDR_DATA_TABLE_RECEIVE_ENTRY)OutputBuffer)->SizeOfImage = pdte->SizeOfImage;
				((PLDR_DATA_TABLE_RECEIVE_ENTRY)OutputBuffer)->Flags = pdte->Flags;
				((PLDR_DATA_TABLE_RECEIVE_ENTRY)OutputBuffer)->LoadCount = pdte->LoadCount;
				((PLDR_DATA_TABLE_RECEIVE_ENTRY)OutputBuffer)->SectionPointer = pdte->SectionPointer;
				((PLDR_DATA_TABLE_RECEIVE_ENTRY)OutputBuffer)->CheckSum = pdte->CheckSum;
				((PLDR_DATA_TABLE_RECEIVE_ENTRY)OutputBuffer)->LoadedImports = pdte->LoadedImports;
				pIoStatus->Information = sizeof(LDR_DATA_TABLE_RECEIVE_ENTRY);
				Status = STATUS_SUCCESS;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READMODULEFULLNAME:
			//IN [PLDR_DATA_TABLE_ENTRY]
			//OUT [UNICODE_STRING]
			//should also pass driver handle and reference it for pointer safety
			//  extremely dangerous as module may be cleaned up or removed
			if (InputBufferLength == sizeof(PLDR_DATA_TABLE_ENTRY) &&
				MmIsAddressValid(pdte = *((PLDR_DATA_TABLE_ENTRY*)InputBuffer)) &&
				OutputBufferLength >= sizeof(UNICODE_STRING)) {
				ReturnUnicodeString(&pdte->FullDllName, OutputBuffer, OutputBufferLength);
				pIoStatus->Information = OutputBufferLength;
				Status = STATUS_SUCCESS;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READMODULEBASENAME:
			//IN [PLDR_DATA_TABLE_ENTRY]
			//OUT [UNICODE_STRING]
			//should also pass driver handle and reference it for pointer safety
			//  extremely dangerous as module may be cleaned up or removed
			if (InputBufferLength == sizeof(PLDR_DATA_TABLE_ENTRY) &&
				MmIsAddressValid(pdte = *((PLDR_DATA_TABLE_ENTRY*)InputBuffer)) &&
				OutputBufferLength >= sizeof(UNICODE_STRING)) {
				ReturnUnicodeString(&pdte->BaseDllName, OutputBuffer, OutputBufferLength);
				pIoStatus->Information = OutputBufferLength;
				Status = STATUS_SUCCESS;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READDRIVERNAME:
			//IN [LPWSTR]
			//OUT [UNICODE_STRING]
			RtlInitUnicodeString(&uszCore, InputBuffer);
			if (OutputBufferLength >= sizeof(UNICODE_STRING)) {
				if ((Status = ObReferenceObjectByName(&uszCore, OBJ_CASE_INSENSITIVE,
							NULL, 0, *IoDriverObjectType, KernelMode,
							NULL, &pDriverObj)) == STATUS_SUCCESS) {
					ReturnUnicodeString(&pDriverObj->DriverName, OutputBuffer, OutputBufferLength);
					pIoStatus->Information = OutputBufferLength;
					ObfDereferenceObject(pDriverObj);
				}
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READDRIVERHARDWAREDATABASE:
			//IN [LPWSTR]
			//OUT [UNICODE_STRING]
			RtlInitUnicodeString(&uszCore, InputBuffer);
			if (OutputBufferLength >= sizeof(UNICODE_STRING)) {
				if ((Status = ObReferenceObjectByName(&uszCore, OBJ_CASE_INSENSITIVE,
							NULL, 0, *IoDriverObjectType, KernelMode,
							NULL, &pDriverObj)) == STATUS_SUCCESS) {
					if (MmIsAddressValid(pDriverObj->HardwareDatabase)) {
						ReturnUnicodeString(pDriverObj->HardwareDatabase, OutputBuffer, OutputBufferLength);
					} else {
						((PUNICODE_STRING)OutputBuffer)->Length = 0;
						((PUNICODE_STRING)OutputBuffer)->MaximumLength = 0;
						((PUNICODE_STRING)OutputBuffer)->Buffer = NULL;
					}
					pIoStatus->Information = OutputBufferLength;
					ObfDereferenceObject(pDriverObj);
				}
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READDRIVEROBJECT:
			//IN [LPWSTR]
			//OUT [DRIVER_RECEIVE_OBJECT]
			RtlInitUnicodeString(&uszCore, InputBuffer);
			if (OutputBufferLength == sizeof(DRIVER_RECEIVE_OBJECT)) {
				if ((Status = ObReferenceObjectByName(&uszCore, OBJ_CASE_INSENSITIVE,
							NULL, 0, *IoDriverObjectType, KernelMode,
							NULL, &pDriverObj)) == STATUS_SUCCESS) {
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->pdo = pDriverObj;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->Type = pDriverObj->Type;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->Size = pDriverObj->Size;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->DeviceObject = pDriverObj->DeviceObject;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->Flags = pDriverObj->Flags;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->DriverStart = pDriverObj->DriverStart;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->DriverSize = pDriverObj->DriverSize;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->DriverSection = pDriverObj->DriverSection;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->DriverExtension = pDriverObj->DriverExtension;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->FastIoDispatch = pDriverObj->FastIoDispatch;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->DriverInit = pDriverObj->DriverInit;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->DriverStartIo = pDriverObj->DriverStartIo;
					((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->DriverUnload = pDriverObj->DriverUnload;
					RtlCopyMemory(((PDRIVER_RECEIVE_OBJECT)OutputBuffer)->MajorFunction, pDriverObj->MajorFunction, min((pDriverObj->Size - offsetof(DRIVER_OBJECT, MajorFunction)), sizeof(PDRIVER_DISPATCH) * (IRP_MJ_MAXIMUM_FUNCTION + 1)));
					ObfDereferenceObject(pDriverObj);
					pIoStatus->Information = sizeof(DRIVER_RECEIVE_OBJECT);
				}
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READDEVICEOBJECT:
			//IN [LPWSTR]
			//OUT [DEVICE_RECEIVE_OBJECT]
			RtlInitUnicodeString(&uszCore, InputBuffer);
			if (OutputBufferLength == sizeof(DEVICE_RECEIVE_OBJECT)) {
				if ((Status = IoGetDeviceObjectPointer(&uszCore, 0, &Ptr, &pDeviceObj)) == STATUS_SUCCESS) {
					if ((Status = ObReferenceObjectByPointer(pDeviceObj, 0, *IoDeviceObjectType, KernelMode)) == STATUS_SUCCESS) {
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->pdo = pDeviceObj;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->Type = pDeviceObj->Type;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->Size = pDeviceObj->Size;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->ReferenceCount = pDeviceObj->ReferenceCount;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->DriverObject = pDeviceObj->DriverObject;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->NextDevice = pDeviceObj->NextDevice;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->AttachedDevice = pDeviceObj->AttachedDevice;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->CurrentIrp = pDeviceObj->CurrentIrp;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->Timer = pDeviceObj->Timer;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->Flags = pDeviceObj->Flags;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->Characteristics = pDeviceObj->Characteristics;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->Vpb = pDeviceObj->Vpb;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->DeviceExtension = pDeviceObj->DeviceExtension;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->DeviceType = pDeviceObj->DeviceType;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->StackSize = pDeviceObj->StackSize;

						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->AlignmentRequirement = pDeviceObj->AlignmentRequirement;
						
						
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->ActiveThreadCount = pDeviceObj->ActiveThreadCount;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->SecurityDescriptor = pDeviceObj->SecurityDescriptor;

						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->SectorSize = pDeviceObj->SectorSize;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->Spare1 = pDeviceObj->Spare1;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->DeviceObjectExtension = pDeviceObj->DeviceObjectExtension;
						((PDEVICE_RECEIVE_OBJECT)OutputBuffer)->Reserved = pDeviceObj->Reserved;
						ObfDereferenceObject(pDeviceObj);
						pIoStatus->Information = sizeof(DEVICE_RECEIVE_OBJECT);
					}
					ObfDereferenceObject(Ptr); //file object is referenced
				}
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_ENUMDRIVERDEVICES:
			//IN [LPWSTR]
			//OUT [ULONG PDEVICE_OBJECTs ULONG UNICODE_STRINGs]
			RtlInitUnicodeString(&uszCore, InputBuffer);
			if (OutputBufferLength >= sizeof(ULONG)) {
				if ((Status = ObReferenceObjectByName(&uszCore, OBJ_CASE_INSENSITIVE,
							NULL, 0, *IoDriverObjectType, KernelMode,
							NULL, &pDriverObj)) == STATUS_SUCCESS) {
					Status = IoEnumerateDeviceObjectList(pDriverObj, NULL, 0, (PULONG)OutputBuffer);
					if ((OutputBufferLength >= sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT)) + sizeof(ULONG) &&
						Status == STATUS_BUFFER_TOO_SMALL && *((PULONG)OutputBuffer) != 0) {
						UniStringsLength = 0;
						//should instead pass in the full OutputBufferLength to avoid buffer too small
						if ((Status = IoEnumerateDeviceObjectList(pDriverObj, (PDEVICE_OBJECT*)((UCHAR*)OutputBuffer + sizeof(ULONG)), *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT), (PULONG)&ReturnLength)) == STATUS_SUCCESS ||
							 Status == STATUS_BUFFER_TOO_SMALL) {
							//it looks as if the ReturnLength is 0 if the buffer is exactly matching and STATUS_SUCCESS is returned?
							//if (ReturnLength < *((PULONG)OutputBuffer)) *((PULONG)OutputBuffer) = ReturnLength;
							for (Counter = 0; Counter < *((PULONG)OutputBuffer); Counter++) {
								Status = ObQueryNameString(*((PDEVICE_OBJECT*)((UCHAR*)OutputBuffer + sizeof(ULONG) + sizeof(PDEVICE_OBJECT) * Counter)), NULL, 0, (PULONG)&ReturnLength);
								Buf = NULL;
								if (Status == STATUS_INFO_LENGTH_MISMATCH && ReturnLength != 0 &&
									(Buf = ExAllocatePool(NonPagedPool, ReturnLength)) != NULL &&
									(Status = ObQueryNameString(*((PDEVICE_OBJECT*)((UCHAR*)OutputBuffer + sizeof(ULONG) + sizeof(PDEVICE_OBJECT) * Counter)), (POBJECT_NAME_INFORMATION)Buf, ReturnLength, (PULONG)&ReturnLength)) == STATUS_SUCCESS) {
									if (OutputBufferLength > (sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength + sizeof(UNICODE_STRING)))
										ReturnUnicodeString(&((POBJECT_NAME_INFORMATION)Buf)->Name, ((UCHAR*)OutputBuffer + sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength), OutputBufferLength - (sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength));
									UniStringsLength += sizeof(UNICODE_STRING) + min(((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength))->Length, ((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength))->MaximumLength);
								} else {
									if (OutputBufferLength > (sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength + sizeof(UNICODE_STRING))) {
										((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength))->Length = 0;
										((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength))->MaximumLength = 0;
										((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength))->Buffer = NULL;
									}
									UniStringsLength += sizeof(UNICODE_STRING);
								}
								if (Buf) ExFreePool(Buf);
								else Status = STATUS_INSUFFICIENT_RESOURCES;
								ObfDereferenceObject(*((PDEVICE_OBJECT*)((UCHAR*)OutputBuffer + sizeof(ULONG) + sizeof(PDEVICE_OBJECT) * Counter)));
							}
						}
						*((PULONG)((UCHAR*)OutputBuffer + sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT))) = UniStringsLength;
						pIoStatus->Information = min(sizeof(ULONG) + *((PULONG)OutputBuffer) * sizeof(PDEVICE_OBJECT) + sizeof(ULONG) + UniStringsLength, OutputBufferLength);
					} else {
						pIoStatus->Information = sizeof(ULONG);
					}
					ObfDereferenceObject(pDriverObj);
				}
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_OPENOBJECTBYHANDLE:
			//IN [HANDLE]
			//OUT [HANDLE]
			if (InputBufferLength == sizeof(HANDLE) &&
				OutputBufferLength == sizeof(HANDLE)) {
				if ((Status = ObReferenceObjectByHandle(*((HANDLE*)InputBuffer), 0, NULL, KernelMode, &Obj, NULL)) == STATUS_SUCCESS) {
					if ((Status = ObOpenObjectByPointer(Obj, 0, 0, GENERIC_ALL, 0, 0, (HANDLE*)OutputBuffer)) == STATUS_SUCCESS) {
	             		pIoStatus->Information = sizeof(HANDLE);
					}
					ObfDereferenceObject(Obj);
				}
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READIDTVALUES:
			//out [KIDTENTRYs]
			if (((Status = getidt(&dt)) == STATUS_SUCCESS) &&
				(OutputBufferLength == dt.wLimit + 1)) {
				RtlCopyMemory(OutputBuffer, dt.pvAddress, dt.wLimit + 1);
				pIoStatus->Information = dt.wLimit + 1;
			} else if (Status != STATUS_SUCCESS) {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READGDTVALUES:
			//out [KIDTENTRYs]
			if (((Status = getgdt(&dt)) == STATUS_SUCCESS) &&
				(OutputBufferLength == dt.wLimit + 1)) {
				RtlCopyMemory(OutputBuffer, dt.pvAddress, dt.wLimit + 1);
				pIoStatus->Information = dt.wLimit + 1;
			} else if (Status != STATUS_SUCCESS) {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READIDT:
			//out [USHORT PVOID]
			if (OutputBufferLength == sizeof(USHORT) + sizeof(PVOID)) {
				if ((Status = getidt(OutputBuffer)) == STATUS_SUCCESS)
					pIoStatus->Information = sizeof(DTSTRUCT);
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}
			break;
		case IOCTL_FOI_READGDT:
			//out [USHORT PVOID]
			if (OutputBufferLength == sizeof(USHORT) + sizeof(PVOID)) {
				if ((Status = getgdt(OutputBuffer)) == STATUS_SUCCESS)
					pIoStatus->Information = sizeof(DTSTRUCT);
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}			
			break;
		case IOCTL_FOI_READLDT:
			//out [USHORT]
			if (OutputBufferLength == sizeof(USHORT)) {
				Buf = ExAllocatePool(NonPagedPool, sizeof(USHORT));
				if (Buf) {
					_getldt((PUSHORT)Buf);
					*((USHORT*)OutputBuffer) = *((USHORT*)Buf);
					pIoStatus->Information = sizeof(USHORT);
					ExFreePool(Buf);
				} else Status = STATUS_INSUFFICIENT_RESOURCES;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}			
			break;
		case IOCTL_FOI_READTR:
			//out [USHORT]
			if (OutputBufferLength == sizeof(USHORT)) {
				Buf = ExAllocatePool(NonPagedPool, sizeof(USHORT));
				if (Buf) {
					_gettr((PUSHORT)Buf);
					*((USHORT*)OutputBuffer) = *((USHORT*)Buf);
					pIoStatus->Information = sizeof(USHORT);
					ExFreePool(Buf);
				} else Status = STATUS_INSUFFICIENT_RESOURCES;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}			
			break;
		case IOCTL_FOI_READMSW:
			//out [USHORT]
			if (OutputBufferLength == sizeof(USHORT)) {
				Buf = ExAllocatePool(NonPagedPool, sizeof(USHORT));
				if (Buf) {
					_getmsw((PUSHORT)Buf);
					*((USHORT*)OutputBuffer) = *((USHORT*)Buf);
					pIoStatus->Information = sizeof(USHORT);
					ExFreePool(Buf);
				} else Status = STATUS_INSUFFICIENT_RESOURCES;
			} else {
				Status = STATUS_INVALID_PARAMETER;
			}			
			break;
		case IOCTL_FOI_READCRDR:
			//out [UINT_PTRs xCOUNTCRDR]
			if (OutputBufferLength == sizeof(UINT_PTR) * COUNTCRDR) {
				UINT_PTR* pBuf;
				pBuf = ExAllocatePool(NonPagedPool, sizeof(UINT_PTR) * COUNTCRDR);
				if (pBuf) {
					Counter = 0;
					_getcr0(&pBuf[Counter++]);
					_getcr2(&pBuf[Counter++]);
					_getcr3(&pBuf[Counter++]);
					_getcr4(&pBuf[Counter++]);
#ifdef _AMD64_
					_getcr8(&pBuf[Counter++]);
#endif
					_getdr0(&pBuf[Counter++]);
					_getdr1(&pBuf[Counter++]);
					_getdr2(&pBuf[Counter++]);
					_getdr3(&pBuf[Counter++]);
					_getdr6(&pBuf[Counter++]);
					_getdr7(&pBuf[Counter++]);
					RtlCopyMemory(OutputBuffer, pBuf, sizeof(UINT_PTR) * COUNTCRDR);
					pIoStatus->Information = sizeof(UINT_PTR) * COUNTCRDR;
					ExFreePool(pBuf);
				} else STATUS_INSUFFICIENT_RESOURCES;
			} else Status = STATUS_INVALID_PARAMETER;
			break;
		case IOCTL_FOI_GETDISKS:
			//out [ULONG UNICODE_STRINGs]
			RtlInitUnicodeString(&uszCore, L"\\Driver\\Disk");
			Status = STATUS_INVALID_PARAMETER;
			if (InputBufferLength == 0 &&
				(Status = ObReferenceObjectByName(&uszCore, OBJ_CASE_INSENSITIVE,
						NULL, 0, *IoDriverObjectType, KernelMode,
						NULL, &pDriverObj)) == STATUS_SUCCESS) {
				KEVENT Event;
				PDISK_GEOMETRY pDiskGeometry;
				PIRP pIrp;
				IO_STATUS_BLOCK ioStatus;
				pDeviceObj = pDriverObj->DeviceObject;
				UniStringsLength = 0;
				do {
					if ((ObQueryNameString(pDeviceObj, NULL, 0, &ReturnLength) == STATUS_INFO_LENGTH_MISMATCH) && ReturnLength != 0) {
						POBJECT_NAME_INFORMATION pNameBuffer;
						pNameBuffer = (POBJECT_NAME_INFORMATION)
										ExAllocatePoolWithTag(PagedPool, ReturnLength, ' sFI');
						if (pNameBuffer) {
							if (ObQueryNameString(pDeviceObj, pNameBuffer, ReturnLength, &ReturnLength) == STATUS_SUCCESS && pNameBuffer->Name.Buffer) {
								if (OutputBufferLength > (sizeof(ULONG) + UniStringsLength + sizeof(UNICODE_STRING)))
									ReturnUnicodeString(&pNameBuffer->Name, ((UCHAR*)OutputBuffer + sizeof(ULONG) + UniStringsLength), OutputBufferLength - (sizeof(ULONG) + UniStringsLength));
								UniStringsLength += sizeof(UNICODE_STRING) + min(((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + UniStringsLength))->Length, ((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + UniStringsLength))->MaximumLength);
							} else {
								if (OutputBufferLength > (sizeof(ULONG) + UniStringsLength + sizeof(UNICODE_STRING))) {
									((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + UniStringsLength))->Length = 0;
									((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + UniStringsLength))->MaximumLength = 0;
									((PUNICODE_STRING)((UCHAR*)OutputBuffer + sizeof(ULONG) + UniStringsLength))->Buffer = NULL;
								}
								UniStringsLength += sizeof(UNICODE_STRING);
							}
							ExFreePool(pNameBuffer);
						} else Status = STATUS_INSUFFICIENT_RESOURCES;
					}
					pDeviceObj = pDeviceObj->NextDevice;
				} while (pDeviceObj);
				*((ULONG*)OutputBuffer) = UniStringsLength;
				pIoStatus->Information = min(sizeof(ULONG) + UniStringsLength, OutputBufferLength);
				ObfDereferenceObject(pDriverObj);
			}
			break;
		case IOCTL_FOI_GETDISKGEOMETRY:
		case IOCTL_FOI_GETDISKGEOMETRYEX:
			//in [LPWSTR]
			//out [DISK_GEOMETRY or DISK_GEOMETRY_EX]
			RtlInitUnicodeString(&uszCore, L"\\Driver\\Disk");
			Status = STATUS_INVALID_PARAMETER;
			if (InputBufferLength > 0 && OutputBufferLength == (IoControlCode == IOCTL_FOI_GETDISKGEOMETRYEX ? MAX_DISK_GEOMETRY_EX_SIZE : sizeof(DISK_GEOMETRY)) &&
				(Status = ObReferenceObjectByName(&uszCore, OBJ_CASE_INSENSITIVE,
						NULL, 0, *IoDriverObjectType, KernelMode,
						NULL, &pDriverObj)) == STATUS_SUCCESS) {
				KEVENT Event;
				PDISK_GEOMETRY pDiskGeometry;
				PIRP pIrp;
				IO_STATUS_BLOCK ioStatus;
				pDeviceObj = pDriverObj->DeviceObject;
				do {
					if ((ObQueryNameString(pDeviceObj, NULL, 0, &ReturnLength) == STATUS_INFO_LENGTH_MISMATCH) && ReturnLength != 0) {
						POBJECT_NAME_INFORMATION pNameBuffer;
						pNameBuffer = (POBJECT_NAME_INFORMATION)
										ExAllocatePoolWithTag(PagedPool, ReturnLength, ' sFI');
						if (pNameBuffer) {
							if (ObQueryNameString(pDeviceObj, pNameBuffer, ReturnLength, &ReturnLength) == STATUS_SUCCESS && pNameBuffer->Name.Buffer) {
								if (_wcsnicmp(pNameBuffer->Name.Buffer, InputBuffer, pNameBuffer->Name.Length) == 0) {
									ExFreePool(pNameBuffer);
									break;
								}
							}
							ExFreePool(pNameBuffer);
						} else Status = STATUS_INSUFFICIENT_RESOURCES;
					}
					pDeviceObj = pDeviceObj->NextDevice;
				} while (pDeviceObj);
				if (pDeviceObj) {
					pDiskGeometry = ExAllocatePool(NonPagedPool, OutputBufferLength);
					if (pDiskGeometry) {
						KeInitializeEvent(&Event, NotificationEvent, FALSE);
						pIrp = IoBuildDeviceIoControlRequest(IoControlCode == IOCTL_FOI_GETDISKGEOMETRYEX ? IOCTL_DISK_GET_DRIVE_GEOMETRY_EX : IOCTL_DISK_GET_DRIVE_GEOMETRY,
													  pDeviceObj, NULL, 0, pDiskGeometry,
													  OutputBufferLength, FALSE, &Event,
													  &ioStatus);
						if (pIrp) {
							Status = IoCallDriver(pDeviceObj, pIrp);
							if (Status == STATUS_PENDING) {
								KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE,	NULL);
								Status = ioStatus.Status;
							}
							if (Status == STATUS_SUCCESS) {
								RtlCopyMemory(OutputBuffer, pDiskGeometry, OutputBufferLength);
								pIoStatus->Information = OutputBufferLength;
							}
						} else Status = STATUS_INSUFFICIENT_RESOURCES;
						ExFreePool(pDiskGeometry);
					} else Status = STATUS_INSUFFICIENT_RESOURCES;
				}
				ObfDereferenceObject(pDriverObj);
			}
			break;
		case IOCTL_FOI_RAWDISKREAD:
			//in [ULONGLONG LPWSTR]
			//out [LPACHARS]
		case IOCTL_FOI_RAWDISKWRITE:
			//in [ULONGLONG LPWSTR LPACHARS]
			RtlInitUnicodeString(&uszCore, L"\\Driver\\Disk");
			Status = STATUS_INVALID_PARAMETER;
			if (InputBufferLength > sizeof(ULONGLONG) &&
				(Status = ObReferenceObjectByName(&uszCore, OBJ_CASE_INSENSITIVE,
						NULL, 0, *IoDriverObjectType, KernelMode,
						NULL, &pDriverObj)) == STATUS_SUCCESS) {
				ULONG ulBytesPerSector;
				KEVENT Event;
				PDISK_GEOMETRY pDiskGeometry;
				PIRP pIrp;
				IO_STATUS_BLOCK ioStatus;
				//NOT SAFE, must change enumeration to IoEnumerateDeviceObjectList
				pDeviceObj = pDriverObj->DeviceObject;
				do {
					if ((ObQueryNameString(pDeviceObj, NULL, 0, &ReturnLength) == STATUS_INFO_LENGTH_MISMATCH) && ReturnLength != 0) {
						POBJECT_NAME_INFORMATION pNameBuffer;
						pNameBuffer = (POBJECT_NAME_INFORMATION)
										ExAllocatePoolWithTag(PagedPool, ReturnLength, ' sFI');
						if (pNameBuffer) {
							if (ObQueryNameString(pDeviceObj, pNameBuffer, ReturnLength, &ReturnLength) == STATUS_SUCCESS && pNameBuffer->Name.Buffer) {
								if (_wcsnicmp(pNameBuffer->Name.Buffer, (const wchar_t *)((UCHAR*)InputBuffer + sizeof(ULONGLONG)), pNameBuffer->Name.Length) == 0) {
									ExFreePool(pNameBuffer);
									break;
								}
							}
							ExFreePool(pNameBuffer);
						} else Status = STATUS_INSUFFICIENT_RESOURCES;
					}
					pDeviceObj = pDeviceObj->NextDevice;
				} while (pDeviceObj);
				if (pDeviceObj) {
					pDiskGeometry = ExAllocatePool(NonPagedPool, sizeof(DISK_GEOMETRY));
					if (pDiskGeometry) {
						KeInitializeEvent(&Event, NotificationEvent, FALSE);
						pIrp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
													  pDeviceObj, NULL, 0, pDiskGeometry,
													  sizeof(DISK_GEOMETRY), FALSE, &Event,
													  &ioStatus);
						if (pIrp) {
							Status = IoCallDriver(pDeviceObj, pIrp);
							if (Status == STATUS_PENDING) {
								KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE,	NULL);
								Status = ioStatus.Status;
							}
							if (Status == STATUS_SUCCESS) {
								ulBytesPerSector = pDiskGeometry->BytesPerSector;
							}
						} else Status = STATUS_INSUFFICIENT_RESOURCES;
						ExFreePool(pDiskGeometry);
					} else Status = STATUS_INSUFFICIENT_RESOURCES;
					if (Status == STATUS_SUCCESS) {
						UCHAR* sBuf;
						sBuf = ExAllocatePool(NonPagedPool, IoControlCode == IOCTL_FOI_RAWDISKWRITE ? InputBufferLength - sizeof(ULONGLONG) - wcslen((const wchar_t *)((UCHAR*)InputBuffer + sizeof(ULONGLONG))) * sizeof(WCHAR) - sizeof(WCHAR) : OutputBufferLength);
						if (sBuf) {
							LARGE_INTEGER lDiskOffset;
							lDiskOffset.QuadPart = ulBytesPerSector * *((ULONGLONG*)InputBuffer);
							KeInitializeEvent(&Event, NotificationEvent, FALSE);
							if (IoControlCode == IOCTL_FOI_RAWDISKWRITE) {
								RtlCopyMemory(sBuf, (PUCHAR)InputBuffer + sizeof(ULONGLONG) + wcslen((const wchar_t *)((UCHAR*)InputBuffer + sizeof(ULONGLONG))) * sizeof(WCHAR) + sizeof(WCHAR), InputBufferLength - sizeof(ULONGLONG) - wcslen((const wchar_t *)((UCHAR*)InputBuffer + sizeof(ULONGLONG))) * sizeof(WCHAR) - sizeof(WCHAR));
							} else {
								RtlZeroMemory(sBuf, OutputBufferLength);
							}
							pIrp = IoBuildSynchronousFsdRequest(
								IoControlCode == IOCTL_FOI_RAWDISKWRITE ? IRP_MJ_WRITE : IRP_MJ_READ,
								pDeviceObj, sBuf, IoControlCode == IOCTL_FOI_RAWDISKWRITE ? InputBufferLength - sizeof(ULONGLONG) - wcslen((const wchar_t *)((UCHAR*)InputBuffer + sizeof(ULONGLONG))) * sizeof(WCHAR) - sizeof(WCHAR) : OutputBufferLength,
								&lDiskOffset, &Event, &ioStatus);			        
							if (pIrp) {
								Status = IoCallDriver(pDeviceObj, pIrp);
								if (Status == STATUS_PENDING) {
									KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
									Status = ioStatus.Status;
								}
								if (Status == STATUS_SUCCESS) {
									if (IoControlCode != IOCTL_FOI_RAWDISKWRITE) {
										RtlCopyMemory(OutputBuffer, sBuf, OutputBufferLength);
										pIoStatus->Information = OutputBufferLength;
									}
								}
							} else Status = STATUS_INSUFFICIENT_RESOURCES;
							ExFreePool(sBuf);
						} else Status = STATUS_INSUFFICIENT_RESOURCES;
					}
				}
				ObfDereferenceObject(pDriverObj);
			}
			break;
 		default:
			Status = STATUS_INVALID_PARAMETER;
			break;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		pIoStatus->Information = 0;
		Status = STATUS_ACCESS_VIOLATION;
	}
	return Status;
}

NTSTATUS GpdDispatch(IN PDEVICE_OBJECT pDO, IN PIRP pIrp)
{
    PLOCAL_DEVICE_INFO pLDI;
    PIO_STACK_LOCATION pIrpStack;
    NTSTATUS Status;
	PRIVILEGE_SET PrivilegeSet;
	SECURITY_SUBJECT_CONTEXT CaptureSubjectContext;
	pIrp->IoStatus.Information = 0;
    pLDI = (PLOCAL_DEVICE_INFO)pDO->DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    Status = STATUS_INVALID_DEVICE_REQUEST;
    switch (pIrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
			PrivilegeSet.PrivilegeCount = 1;
			PrivilegeSet.Control = 1;
			PrivilegeSet.Privilege->Luid.LowPart = 0x14;
			PrivilegeSet.Privilege->Luid.HighPart = 0;
			PrivilegeSet.Privilege->Attributes = 0;
			SeCaptureSubjectContext(&CaptureSubjectContext);
			if (SePrivilegeCheck(&PrivilegeSet, &CaptureSubjectContext, ExGetPreviousMode())) {
				SeReleaseSubjectContext(&CaptureSubjectContext);
	            Status = STATUS_SUCCESS;
			} else
				Status = STATUS_ACCESS_DENIED;
            break;
        case IRP_MJ_CLOSE:
            Status = STATUS_SUCCESS;
            break;
        case IRP_MJ_DEVICE_CONTROL:
			Status = GpdDeviceControl(pIrpStack->FileObject, pIrp->AssociatedIrp.SystemBuffer, pIrpStack->Parameters.DeviceIoControl.InputBufferLength, pIrp->AssociatedIrp.SystemBuffer, pIrpStack->Parameters.DeviceIoControl.OutputBufferLength, pIrpStack->Parameters.DeviceIoControl.IoControlCode, &pIrp->IoStatus, pDO);
            break;
    }
    pIrp->IoStatus.Status = Status;
    IofCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}

VOID GpdUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING Win32DeviceName;
    RtlInitUnicodeString(&Win32DeviceName, DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&Win32DeviceName);
    IoDeleteDevice(((PLOCAL_DEVICE_INFO)DriverObject->DeviceObject->DeviceExtension)->DeviceObject);
}
