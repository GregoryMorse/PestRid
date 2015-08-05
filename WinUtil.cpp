#include "stdafx.h"
#include "WinAPI.h"

//kernel32

HMODULE PRU_GetModuleHandle(LPCTSTR lpModuleName)
{
	HMODULE hModule;
	if ((hModule = GetModuleHandle(lpModuleName)) == NULL && GetLastError() != ERROR_MOD_NOT_FOUND) {
		AddTraceLog(_T("APICall=GetModuleHandle ModuleName=%s Error=%08X\r\n"),
					lpModuleName, GetLastError());
	}
	return hModule;
}

HMODULE PRU_LoadLibrary(LPCTSTR lpLibFileName)
{
	HMODULE hModule;
	if ((hModule = LoadLibrary(lpLibFileName)) == NULL) {
		AddTraceLog(_T("APICall=LoadLibrary LibraryName=%s Error=%08X\r\n"),
					lpLibFileName, GetLastError());
	}
	return hModule;
}

BOOL PRU_FreeLibrary(HMODULE hLibModule)
{
	BOOL bRet;
	if ((bRet = FreeLibrary(hLibModule)) == FALSE) {
		AddTraceLog(_T("APICall=FreeLibrary Module=%08X Error=%08X\r\n"),
					hLibModule, GetLastError());
	}
	return bRet;
}

FARPROC PRU_GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	FARPROC fpResult;
	if ((fpResult = GetProcAddress(hModule, lpProcName)) == NULL) {
		AddTraceLog(_T("APICall=GetProcAddress FunctionName=%hs Error=%08X\r\n"),
					lpProcName, GetLastError());
	}
	return fpResult;
}

BOOL PRU_PathFileExists(LPCTSTR pszPath)
{
	if (GetFileAttributes(pszPath) != INVALID_FILE_ATTRIBUTES) return TRUE;
	if (GetLastError() == ERROR_FILE_NOT_FOUND) return FALSE;
	if (GetLastError() == ERROR_BAD_NETPATH) return FALSE;
	if (GetLastError() == ERROR_ACCESS_DENIED) return TRUE;
	AddTraceLog(_T("APICall=GetFileAttributes FunctionName=%s Error=%08X\r\n"),
				pszPath, GetLastError());
	return FALSE;
	//could use shlwapi function if present
	//return PathFileExists(pszPath);
}

BOOL PRU_ExpandEnvironmentStrings(LPCTSTR szPath, CString & ExpandedPath)
{
	DWORD dwSize = 0;
	DWORD dwNewSize;
	while (	(dwNewSize = ExpandEnvironmentStrings(szPath,
							dwSize ? ExpandedPath.GetBuffer(dwSize) : NULL, dwSize)) != 0 &&
			(dwSize != dwNewSize + (sizeof(TCHAR) == sizeof(char) ? 1 : 0))) {
		dwSize = dwNewSize;
	}
	if (!dwNewSize) {
		AddTraceLog(_T("APICall=ExpandEnvironmentStrings Path=%s Error=%08X\r\n"),
					szPath, GetLastError());
	}
	if (dwSize) ExpandedPath.ReleaseBuffer();
	return dwNewSize != 0;
}

BOOL PRU_LaunchProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine)
{
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;
	if ((lpApplicationName || lpCommandLine) && CreateProcess(lpApplicationName, lpCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return TRUE;
	} else return FALSE;
}

BOOL PRU_DeleteFile(LPCTSTR lpFileName)
{
	if (DeleteFile(lpFileName)) return TRUE;
	AddTraceLog(_T("APICall=DeleteFile FileName=%s Error=%08X\r\n"), lpFileName, GetLastError());
	return FALSE;
}

BOOL PRU_RemoveDirectory(LPCTSTR lpPathName)
{
	if (RemoveDirectory(lpPathName)) return TRUE;
	AddTraceLog(_T("APICall=RemoveDirectory FileName=%s Error=%08X\r\n"), lpPathName, GetLastError());
	return FALSE;
}

BOOL PRU_DeleteFileTree(LPCTSTR szPath)
{
	CString Path = szPath;
	BOOL bFind;
	CFileFind FileFind;
	bFind = FileFind.FindFile(Path + _T("\\*"));
	while (bFind) {
		bFind = FileFind.FindNextFile();
		if (FileFind.IsDirectory()) {
			if (!FileFind.IsDots()) {
				if (!PRU_DeleteFileTree(FileFind.GetFilePath())) return FALSE;
			}
		} else {
			if (!PRU_DeleteFile(FileFind.GetFilePath())) return FALSE;
		}
	}
	return PRU_RemoveDirectory(szPath);
}

BOOL PRU_GetWindowsDirectory(CString & Dir)
{
	DWORD dwSize = 0;
	DWORD dwNewSize;
	while (	(dwNewSize = GetWindowsDirectory(Dir.GetBuffer(dwSize), dwSize)) != 0 &&
			(dwSize <= dwNewSize)) {
		dwSize = dwNewSize;
	}
	if (!dwNewSize) {
		AddTraceLog(_T("APICall=GetWindowsDirectory Error=%08X\r\n"), GetLastError());
	}
	Dir.ReleaseBuffer();
	return dwNewSize != 0;
}

BOOL PRU_EnumDirectory(LPCTSTR szSearchPath, BOOL bDirectories,
					   BOOL bFiles, BOOL bTitle, CStringArray & EnumResults,
					   LPCTSTR* pszExclusions)
{
	CFileFind FileFind;
	DWORD dwCount;
	BOOL bWorking = FileFind.FindFile(szSearchPath);
	while (bWorking) {
		bWorking = FileFind.FindNextFile();
		if (bDirectories && FileFind.IsDirectory() && !FileFind.IsDots()) {
			if (pszExclusions) {
				dwCount = 0;
				while (pszExclusions[dwCount]) {
					if (_tcsicmp(pszExclusions[dwCount],
							bTitle ? FileFind.GetFileName() : FileFind.GetFilePath()) == 0) break;
					dwCount++;
				}
				if (pszExclusions[dwCount]) continue;
			}
			EnumResults.Add(bTitle ? FileFind.GetFileName() : FileFind.GetFilePath());
		} else if (bFiles && !FileFind.IsDirectory()) {
			if (pszExclusions) {
				dwCount = 0;
				while (pszExclusions[dwCount]) {
					if (_tcsicmp(pszExclusions[dwCount],
							bTitle ? FileFind.GetFileTitle() : FileFind.GetFilePath()) == 0) break;
					dwCount++;
				}
				if (pszExclusions[dwCount]) continue;
			}
			EnumResults.Add(bTitle ? FileFind.GetFileTitle() : FileFind.GetFilePath());
		}
	}
	FileFind.Close();
	return TRUE;
}

BOOL PRU_QueryDosDevice(LPCTSTR szTarget, CStringArray & Devices)
{
	CString String;
	DWORD dwSize;
	DWORD dwStrSize;
	dwSize = 2048;
	do {
		dwStrSize = dwSize;
		dwSize = QueryDosDevice(szTarget, String.GetBuffer(dwStrSize), dwStrSize);
	} while ((dwSize && dwSize > dwStrSize) ||
			 ((dwSize == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) ? (dwSize = dwStrSize * 2, TRUE) : FALSE));
	if (dwSize == 0) {
		AddTraceLog(_T("APICall=QueryDosDevice Target=%s Error=%08X\r\n"), szTarget, GetLastError());
		return FALSE;
	} else {
		String.ReleaseBuffer(dwSize);
		StringArrayFromLPTSTRS((TCHAR*)(LPCTSTR)String, Devices);
		return TRUE;
	}
}

BOOL PRU_GetVolumePathNamesForVolumeName(LPCTSTR szVolName, CStringArray & PathNames)
{
	CString String;
	DWORD dwStrSize;
	dwStrSize = 2048;
	do {
	} while (!GetVolumePathNamesForVolumeName(szVolName, String.GetBuffer(dwStrSize), dwStrSize, &dwStrSize) &&
			 (GetLastError() == ERROR_MORE_DATA ? TRUE : (dwStrSize = 0, FALSE)));
	if (dwStrSize == 0) {
		AddTraceLog(_T("APICall=GetVolumePathNamesForVolumeName VolumeName=%s Error=%08X\r\n"), szVolName, GetLastError());
		return FALSE;
	} else {
		String.ReleaseBuffer(dwStrSize);
		StringArrayFromLPTSTRS((TCHAR*)(LPCTSTR)String, PathNames);
		return TRUE;
	}
}

#define PACKVERSION(major, minor) MAKELONG(minor, major)

BOOL PRU_CheckWindowsVersionMinumum(DWORD dwMajorVer, DWORD dwMinorVer)
{
	static OSVERSIONINFO osvi;
	if (!osvi.dwOSVersionInfoSize) {
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		//VerifyVersionInfo();
		GetVersionEx(&osvi);
		if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT ||
			osvi.dwMajorVersion == 4) {
			//9x/Me platforms are supported through emulation
			//AddTraceLog(_T("APICall=GetVersionEx Error=Windows NT 4.0, 2000, XP, 2003 platform not detected will not run with all features\r\n"));
		}
	}
	return (PACKVERSION(osvi.dwMajorVersion, osvi.dwMinorVersion) >=
			PACKVERSION(dwMajorVer, dwMinorVer));
}

BOOL PRU_NtOpenDirectoryObject(HANDLE & hDirObj, LPCWSTR szName, HANDLE hParent, ACCESS_MASK dwAccess)
{
	CAPINTDLL NTDLLAPI;
	UNICODE_STRING ustr;
	OBJECT_ATTRIBUTES obj;
	NTSTATUS lStatus;
	if (!NTDLLAPI.Get_RtlInitUnicodeString() || !NTDLLAPI.Get_NtOpenDirectoryObject()) return FALSE;
	NTDLLAPI.Get_RtlInitUnicodeString()(&ustr, szName);
	obj.Length = sizeof(OBJECT_ATTRIBUTES);
	obj.RootDirectory = hParent;
	obj.Attributes = OBJ_CASE_INSENSITIVE;
	obj.ObjectName = &ustr;
	obj.SecurityDescriptor = NULL;
	obj.SecurityQualityOfService = NULL;
	if ((lStatus = NTDLLAPI.Get_NtOpenDirectoryObject()(&hDirObj, dwAccess, &obj)) == STATUS_SUCCESS)
		return TRUE;
	//if cannot access then use the kernel driver to open it
	if (lStatus == STATUS_ACCESS_DENIED) {
		if ((lStatus = NTDLLAPI.Get_NtOpenDirectoryObject()(&hDirObj, READ_CONTROL, &obj)) == STATUS_SUCCESS) {
			CDriverUse DriverUse;
			HANDLE hObj;
			if (DriverQuery(IOCTL_FOI_OPENOBJECTBYHANDLE, &hDirObj, sizeof(HANDLE), &hObj, sizeof(HANDLE))) {
				PRU_CloseHandle(hDirObj);
				hDirObj = hObj;
				return TRUE;
			}
			PRU_CloseHandle(hDirObj);
			hDirObj = NULL;
			return FALSE;
		}
	}
	AddTraceLog(_T("APICall=NtOpenDirectoryObject Name=%lS Rights=0x%08X Error=%08X\r\n"),
				szName, dwAccess, lStatus);
	return FALSE;
}

BOOL PRU_NtQueryDirectoryObject(HANDLE hDirObj, BOOLEAN bFirst, ULONG & ulIndex, CStringW & Name, CStringW & Type)
{
	CAPINTDLL NTDLLAPI;
	if (!NTDLLAPI.Get_NtQueryDirectoryObject()) return FALSE;
	CByteArray Bytes;
	ULONG ulRetLen = 0;
	long lStatus;
	Bytes.SetSize(QUERY_DIRECTORY_BUF_SIZE);
	do {
		if ((lStatus = NTDLLAPI.Get_NtQueryDirectoryObject()(hDirObj, Bytes.GetData(), (ULONG)(ULONG_PTR)Bytes.GetSize(), ObjectByOne, bFirst, &ulIndex, &ulRetLen)) == STATUS_SUCCESS) {
			Bytes.SetSize(ulRetLen);
			UniStringToCStringW(&((POBJECT_NAMETYPE_INFO)Bytes.GetData())->ObjectName, Name);
			UniStringToCStringW(&((POBJECT_NAMETYPE_INFO)Bytes.GetData())->ObjectType, Type);
			return TRUE;
		}
		if (lStatus == STATUS_BUFFER_TOO_SMALL) {
			Bytes.SetSize(ulRetLen);
		} else break;
	} while (TRUE);
	if (lStatus != STATUS_NO_MORE_ENTRIES) {
		AddTraceLog(_T("APICall=NtQueryDirectoryObject ObjectTypesDirectory=%08X Index=%08X Error=%08X\r\n"), hDirObj, ulIndex, lStatus);
	}
	Name.Empty();
	Type.Empty();
	return FALSE;
}

void PRU_GetAllTypeObjects(CStringArray & Names, LPCWSTR szType, HANDLE hParent, LPCWSTR szName, LPCWSTR szBaseName)
{
	HANDLE hDirObj;
	BOOLEAN bFirst = TRUE;
	ULONG ulIndex = 0;
	CStringW Name;
	CStringW Type;
	if (PRU_NtOpenDirectoryObject(hDirObj, szName, hParent, DIRECTORY_QUERY | DIRECTORY_TRAVERSE)) {
		while (PRU_NtQueryDirectoryObject(hDirObj, bFirst, ulIndex, Name, Type)) {
			if ((Type.GetLength() == wcslen(DIRTYPESTRW)) &&
				Type.CompareNoCase(DIRTYPESTRW) == 0)
				PRU_GetAllTypeObjects(Names, szType, hDirObj, Name, hParent ? CStringW(szBaseName) + OBJ_NAME_PATH_SEPARATOR + szName : L"");
			if ((Type.GetLength() == wcslen(szType)) &&
				Type.CompareNoCase(szType) == 0) {
				Names.Add(CString(hParent ? CStringW(szBaseName) + OBJ_NAME_PATH_SEPARATOR + szName + OBJ_NAME_PATH_SEPARATOR + Name : szName + Name));
			}
			bFirst = FALSE;
		}
		PRU_CloseHandle(hDirObj);
	}
}

void PRU_GetAllTypeObjects(CStringArray & Names, LPCWSTR szType)
{
	PRU_GetAllTypeObjects(Names, szType, NULL, DIRROOTSTRW, NULL);
}

BOOL PRU_NtQuerySecurityObject(HANDLE hObject, SECURITY_INFORMATION si, CByteArray & Bytes)
{
	CAPINTDLL NTDLLAPI;
	ULONG ulRetLen;
	long lStatus;
	Bytes.SetSize(0x200);
	do {
		if ((lStatus = NTDLLAPI.Get_NtQuerySecurityObject()(hObject, si, Bytes.GetData(), (ULONG)(ULONG_PTR)Bytes.GetSize(), &ulRetLen)) == STATUS_SUCCESS) {
			Bytes.SetSize(ulRetLen);
			return TRUE;
		}
		if (lStatus == STATUS_BUFFER_TOO_SMALL) {
			Bytes.SetSize(ulRetLen);
		} else break;
	} while (TRUE);
	Bytes.RemoveAll();
	return FALSE;
}

BOOL PRU_CloseHandle(HANDLE hObject)
{
	if (!CloseHandle(hObject)) {
		AddTraceLog(_T("APICall=CloseHandle Error=%08X\r\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}

#include <shlwapi.h>

//comctl32
BOOL PRU_CheckCommonControlsVersionMinimum(	DWORD dwMajorVer, DWORD dwMinorVer)//and kernel32
{
	static DLLVERSIONINFO dvi;
	HINSTANCE hCommCtrl;
	DLLGETVERSIONPROC pDllGetVersion;
	if (!dvi.cbSize) {
		if ((hCommCtrl = GetModuleHandle(_T("comctl32.dll"))) != NULL) {
			if ((pDllGetVersion =
				 (DLLGETVERSIONPROC)GetProcAddress(	hCommCtrl, 
													"DllGetVersion")) != NULL) {
				dvi.cbSize = sizeof(DLLVERSIONINFO);
				(*pDllGetVersion)(&dvi);
			} else return FALSE;
		} else return FALSE;
	}
	return (PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion) >=
			PACKVERSION(dwMajorVer, dwMinorVer));
}

#include <winsvc.h>
BOOL ServiceManager::OpenServiceManager()
{
	if (m_hSCM ||
		(m_hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) != NULL) {
		return TRUE;
	} else {
		AddTraceLog(_T("APICall=OpenSCManager DesiredAccess=SC_MANAGER_ALL_ACCESS Error=%08X\r\n"),
					GetLastError());
		return FALSE;
	}
}

void ServiceManager::CloseServiceManager()
{
	if (m_hSCM) {
		if (!CloseServiceHandle(m_hSCM)) {
			AddTraceLog(_T("APICall=CloseServiceHandle SCManagerHandle=%08X Error=%08X\r\n"),
						m_hSCM, GetLastError());
		}
	}
	m_hSCM = NULL;
}

BOOL PRU_SetPrivilege(PCTSTR name, BOOL bEnable)
{
	HANDLE hToken;
	BOOL rv = FALSE;
	TOKEN_PRIVILEGES priv = {1, {0, 0, bEnable ? SE_PRIVILEGE_ENABLED : 0}};
	if (LookupPrivilegeValue(0, name, &priv.Privileges[0].Luid)) {
		if (OpenProcessToken(	GetCurrentProcess(),
								TOKEN_ADJUST_PRIVILEGES, &hToken)) {
			if (AdjustTokenPrivileges(	hToken, FALSE, &priv,
										sizeof priv, 0, 0)) {
				rv = TRUE;
			} else {
				AddTraceLog(_T("APICall=AdjustTokenPrivileges Token=%08X PrivilegeName=%s Error=%08X\r\n"),
							hToken, name, GetLastError());
			}
			if (!CloseHandle(hToken)) {
				AddTraceLog(_T("APICall=CloseHandle Token=%08X Error=%08X\r\n"),
							hToken, GetLastError());
			}
		} else {
			AddTraceLog(_T("APICall=OpenProcessToken Rights=ADJUST_PRIVILEGES Process=Self Error=%08X\r\n"),
						GetLastError());
		}
	} else {
		AddTraceLog(_T("APICall=LookupPrivilegeValue PrivilegeName=%s Error=%08X\r\n"),
					name, GetLastError());
	}
	return rv;
}

#include <aclapi.h>

BOOL PRU_FriendlyNameFromSid(PSID psid, CString & Name)
{
	DWORD dwNameSize = 0;
	DWORD dwReferencedDomainSize = 0;
	SID_NAME_USE eSidNameUse;
	CString Domain;
	if (!LookupAccountSid(NULL, psid, NULL, &dwNameSize, NULL, &dwReferencedDomainSize, &eSidNameUse) && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		if (LookupAccountSid(NULL, psid, Name.GetBuffer(dwNameSize + 1), &dwNameSize, Domain.GetBuffer(dwReferencedDomainSize + 1), &dwReferencedDomainSize, &eSidNameUse)) {
			Name.ReleaseBuffer();
			Domain.ReleaseBuffer();
			Name = Domain + _T("\\") + Name;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL PRU_GetOwner(LPTSTR lpszPath, SE_OBJECT_TYPE ObjectType, PSID* outpsid)//and kernel32
{
	BOOL bRet = FALSE;
	PSID psid = NULL;
	PSECURITY_DESCRIPTOR psd = NULL;
	DWORD dwErr;
	if (lpszPath && (dwErr = GetNamedSecurityInfo(lpszPath, ObjectType, OWNER_SECURITY_INFORMATION,
								&psid, NULL, NULL, NULL, &psd)) == ERROR_SUCCESS &&
		IsValidSid(psid)) {
		*outpsid = new BYTE[GetLengthSid(psid)]; 
		if (*outpsid && (bRet = CopySid(GetLengthSid(psid), *outpsid, psid)) != FALSE) {
			delete [] *outpsid;
			*outpsid = NULL;
		}
	} else if (lpszPath) {
		if (dwErr == ERROR_SUCCESS) {
			AddTraceLog(_T("APICall=GetNamedSecurityInfo Path=%s Action=Returned bad SID\r\n"), lpszPath);
		} else {
			AddTraceLog(_T("APICall=GetNamedSecurityInfo Path=%s Error=%08X\r\n"), lpszPath, dwErr);
		}
	}
	if (psd) LocalFree(psd);
	return bRet;
}

//forced take ownership, for deletion only
BOOL PRU_TakeOwnership(LPTSTR lpszPath, SE_OBJECT_TYPE ObjectType, BOOL bRecurse, PSID psid)
{
    BOOL bRetval = FALSE;
    PSID pSIDAdmin = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    DWORD dwRes;
    // Create a SID for the BUILTIN\Administrators group.
    if (!psid && !AllocateAndInitializeSid(&SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &pSIDAdmin)) {
        AddTraceLog(_T("AllocateAndInitializeSid (Admin) error %u\r\n"),
                GetLastError());
        goto Cleanup;
    }
    // Enable the SE_TAKE_OWNERSHIP_NAME privilege.
    if (!PRU_SetPrivilege(SE_TAKE_OWNERSHIP_NAME, TRUE)) {
        AddTraceLog(_T("You must be logged on as Administrator.\r\n"));
        goto Cleanup; 
    }
    // Set the owner in the object's security descriptor.
	if (bRecurse) {
		dwRes = TreeSetNamedSecurityInfo(
			lpszPath,                 // name of the object
			ObjectType,              // type of object
			OWNER_SECURITY_INFORMATION,  // change only the object's owner
			psid ? psid : pSIDAdmin,                   // SID of Administrator group
			NULL,
			NULL,
			NULL, TREE_SEC_INFO_RESET, NULL, ProgressInvokeNever, NULL);
	} else {
		dwRes = SetNamedSecurityInfo(
			lpszPath,                 // name of the object
			ObjectType,              // type of object
			OWNER_SECURITY_INFORMATION,  // change only the object's owner
			psid ? psid : pSIDAdmin,                   // SID of Administrator group
			NULL,
			NULL,
			NULL);
	}
    if (dwRes != ERROR_SUCCESS) {
		AddTraceLog(_T("APICall=%s File=%s Error=%08X\r\n"), bRecurse ? _T("TreeSetNamedSecurityInfo") : _T("SetNamedSecurityInfo"), lpszPath, dwRes);
        goto Cleanup;
    }
    // Disable the SE_TAKE_OWNERSHIP_NAME privilege.
    if (!PRU_SetPrivilege(SE_TAKE_OWNERSHIP_NAME, FALSE)) {
        AddTraceLog(_T("Failed SetPrivilege call unexpectedly.\r\n"));
        goto Cleanup;
    }
Cleanup:
    if (pSIDAdmin)
        FreeSid(pSIDAdmin); 
    return bRetval;
}

BOOL PRU_RegOpenKey(HKEY hBaseKey, LPCTSTR szRegistryPath, HKEY & hKey)
{
	LONG lErr;
	if ((lErr = RegOpenKey(	hBaseKey,
						szRegistryPath, &hKey)) == ERROR_SUCCESS) return TRUE;
	if (lErr != ERROR_FILE_NOT_FOUND)
		AddTraceLog(_T("APICall=RegOpenKey SubKey=%s Error=%08X\r\n"), szRegistryPath, lErr);
	return FALSE;
}

BOOL PRU_RegCloseKey(HKEY & hKey)
{
	LONG lErr;
	if ((lErr = RegCloseKey(hKey)) == ERROR_SUCCESS) return TRUE;
	AddTraceLog(_T("APICall=RegCloseKey Error=%08X\r\n"), lErr);
	return FALSE;
}

BOOL PRU_WriteRegistryDWORD(HKEY hKey, LPCTSTR szRegistryValueName, DWORD dwValue)
{
	if (!RegSetValueEx(	hKey, szRegistryValueName, NULL, REG_DWORD, 
						(BYTE*)&dwValue, sizeof(DWORD))) {
		//trace log
		return FALSE;
	}
	return TRUE;
}

BOOL PRU_WriteRegistryString(HKEY hKey, LPCTSTR szRegistryValueName,
							   CString ValueString, BOOL bExpandable)
{
	if (!RegSetValueEx(	hKey, szRegistryValueName, NULL,
						bExpandable ? REG_EXPAND_SZ : REG_SZ, 
						(BYTE*)ValueString.GetBuffer(),
						ValueString.GetLength() + sizeof(TCHAR))) {
		//trace log
		return FALSE;
	}
	return TRUE;
}

BOOL PRU_WriteRegistryMultiString(HKEY hKey, LPCTSTR szRegistryValueName,
								  CStringArray & StringArray)
{
	CByteArray Bytes;
	LPTSTRSFromStringArray(StringArray, Bytes);
	DWORD dwType = REG_MULTI_SZ;
	DWORD dwSize = (DWORD)Bytes.GetSize();
	if (!RegSetValueEx(	hKey, szRegistryValueName,
						NULL, dwType,
						Bytes.GetData(), dwSize) == ERROR_SUCCESS) {
		//trace log
		return FALSE;
	}
	return TRUE;
}

BOOL PRU_WriteRegistryBinary(HKEY hKey, LPCTSTR szRegistryValueName, CByteArray & ByteArray)
{
	DWORD dwType = REG_BINARY;
	DWORD dwSize = (DWORD)ByteArray.GetSize();
	if (!RegSetValueEx(	hKey, szRegistryValueName,
						NULL, dwType,
						ByteArray.GetData(), dwSize) == ERROR_SUCCESS) {
		//trace log
		return FALSE;
	}
	return TRUE;
}

BOOL PRU_ReadRegistryBinary(HKEY hKey, LPCTSTR szRegistryValueName, CByteArray & ByteArray)
{
	DWORD dwType;
	DWORD dwSize;
	LONG lErr;
	ByteArray.RemoveAll();
	if ((lErr = RegQueryValueEx(hKey, szRegistryValueName,
						NULL, &dwType, NULL, &dwSize)) == ERROR_SUCCESS &&
		(dwType == REG_BINARY || dwType == REG_NONE)) {
		ByteArray.SetSize(dwSize);
		if ((lErr = RegQueryValueEx(hKey, szRegistryValueName,
							NULL, &dwType, ByteArray.GetData(),
							&dwSize)) == ERROR_SUCCESS &&
			(dwType == REG_BINARY || dwType == REG_NONE)) {
			return TRUE;
		} else if (lErr != ERROR_FILE_NOT_FOUND) {
			AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s Error=%08X\r\n"), szRegistryValueName, lErr);
		}
		ByteArray.RemoveAll();
	} else if (lErr != ERROR_FILE_NOT_FOUND) {
		AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s Type=%lu REG_BINARY Error=%08X\r\n"), szRegistryValueName, dwType, lErr);
	}
	return FALSE;
}

BOOL PRU_ReadRegistryMultiString(HKEY hKey, LPCTSTR szRegistryValueName, CStringArray & StringArray)
{
	DWORD dwType;
	DWORD dwSize;
	TCHAR* pcBuff;
	LONG lErr;
	StringArray.RemoveAll();
	if ((lErr = RegQueryValueEx(hKey, szRegistryValueName,
						NULL, &dwType, NULL, &dwSize)) == ERROR_SUCCESS &&
		dwType == REG_MULTI_SZ) {
		if (dwSize == 0) return TRUE;
		if ((pcBuff = (TCHAR*)malloc(sizeof(TCHAR) * (dwSize + 2))) == NULL) return FALSE;
		if ((lErr = RegQueryValueEx(hKey, szRegistryValueName,
							NULL, &dwType, (LPBYTE)pcBuff,
							&dwSize)) == ERROR_SUCCESS &&
			dwType == REG_MULTI_SZ) {
			((TCHAR*)pcBuff)[dwSize] = 0; //must properly NULL terminate
			((TCHAR*)pcBuff)[dwSize + 1] = 0; //must properly double NULL terminate
			StringArrayFromLPTSTRS(pcBuff, StringArray);
			free(pcBuff);
			return TRUE;
		} else if (lErr != ERROR_FILE_NOT_FOUND) {
			AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s Error=%08X\r\n"), szRegistryValueName, lErr);
		}
		free(pcBuff);
	} else if (lErr != ERROR_FILE_NOT_FOUND) {
		AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s  Type=%lu REG_MULTI_SZ Error=%08X\r\n"), szRegistryValueName, dwType, lErr);
	}
	return FALSE;
}

BOOL PRU_ReadRegistryString(HKEY hKey, LPCTSTR szRegistryValueName, CString & String)
{
	DWORD dwType;
	DWORD dwSize;
	LONG lErr;
	String.Empty();
	if ((lErr = RegQueryValueEx(hKey, szRegistryValueName, 
						NULL, &dwType, NULL, &dwSize)) == ERROR_SUCCESS &&
		(dwType == REG_SZ || dwType == REG_EXPAND_SZ)) {
		if (dwSize == 0) return TRUE;
		if ((lErr = RegQueryValueEx(hKey, szRegistryValueName, 
							NULL, &dwType, (LPBYTE)String.GetBuffer(dwSize + 1), 
							&dwSize)) == ERROR_SUCCESS &&
			(dwType == REG_SZ || dwType == REG_EXPAND_SZ)) {
			String.GetBuffer()[dwSize] = 0; //must properly NULL terminate
			String.ReleaseBuffer();
			return TRUE;
		} else if (lErr != ERROR_FILE_NOT_FOUND) {
			AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s Error=%08X\r\n"), szRegistryValueName, lErr);
		}
		String.ReleaseBuffer();
		String.Empty();
	} else if (lErr != ERROR_FILE_NOT_FOUND) {
		AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s Type=%lu REG_SZ or REG_EXPAND_SZ Error=%08X\r\n"), szRegistryValueName, dwType, lErr);
	}
	return FALSE;
}

BOOL PRU_ReadRegistryDWORD(HKEY hKey, LPCTSTR szRegistryValueName, DWORD & dwValue)
{
	DWORD dwType;
	DWORD dwSize;
	LONG lErr;
	if ((lErr = RegQueryValueEx(hKey, szRegistryValueName, 
						NULL, &dwType, NULL, &dwSize)) == ERROR_SUCCESS &&
		dwType == REG_DWORD) {
		if (dwSize == sizeof(DWORD) &&
			(lErr = RegQueryValueEx(hKey, szRegistryValueName, 
							NULL, &dwType, (LPBYTE)&dwValue, 
							&dwSize)) == ERROR_SUCCESS &&
			dwType == REG_DWORD) {
			return TRUE;
		} else if (dwSize != sizeof(DWORD)) {
			AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s REG_DWORD wrong size\r\n"), szRegistryValueName);
		} else if (lErr != ERROR_FILE_NOT_FOUND) {
			AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s Error=%08X\r\n"), szRegistryValueName, lErr);
		}
	} else if (lErr != ERROR_FILE_NOT_FOUND) {
		AddTraceLog(_T("APICall=RegQueryValueEx ValueName=%s Type=%lu REG_DWORD Error=%08X\r\n"), szRegistryValueName, dwType, lErr);
	}
	dwValue = ~0UL;
	return FALSE;
}

BOOL PRU_ReadRegistryBinary(HKEY hBaseKey, LPCTSTR szRegistryPath,
							LPCTSTR szRegistryValueName,
							CByteArray & ByteArray)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_ReadRegistryBinary(hKey, szRegistryValueName, ByteArray);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_WriteRegistryDWORD(HKEY hBaseKey, LPCTSTR szRegistryPath,
								LPCTSTR szRegistryValueName, DWORD dwValue)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_WriteRegistryDWORD(hKey, szRegistryValueName, dwValue);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_WriteRegistryString(HKEY hBaseKey, LPCTSTR szRegistryPath,
							LPCTSTR szRegistryValueName,
							CString ValueString, BOOL bExpandable)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_WriteRegistryString(	hKey, szRegistryValueName,
									ValueString, bExpandable);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_WriteRegistryMultiString(HKEY hBaseKey, LPCTSTR szRegistryPath,
								  LPCTSTR szRegistryValueName,
								  CStringArray StringArray)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_WriteRegistryMultiString(hKey, szRegistryValueName, StringArray);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_WriteRegistryBinary(HKEY hBaseKey, LPCTSTR szRegistryPath,
							LPCTSTR szRegistryValueName, CByteArray & ByteArray)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_WriteRegistryBinary(hKey, szRegistryValueName, ByteArray);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_ReadRegistryMultiString(HKEY hBaseKey, LPCTSTR szRegistryPath,
									LPCTSTR szRegistryValueName,
									CStringArray & StringArray)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_ReadRegistryMultiString(hKey, szRegistryValueName, StringArray);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_ReadRegistryString( HKEY hBaseKey, LPCTSTR szRegistryPath,
								LPCTSTR szRegistryValueName, CString & String)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_ReadRegistryString(hKey, szRegistryValueName, String);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_ReadRegistryDWORD(	HKEY hBaseKey, LPCTSTR szRegistryPath,
								LPCTSTR szRegistryValueName, DWORD & dwValue)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_ReadRegistryDWORD(hKey, szRegistryValueName, dwValue);
		PRU_RegCloseKey(hKey);
	} else {
		dwValue = ~0UL;
	}
	return bRet;
}

BOOL PRU_RegistryEnumValues(HKEY hKey, CStringArray & Keys, BOOL bInitialize)
{
	BOOL bRet = FALSE;
	DWORD dwIndex;
	DWORD dwTotal;
	DWORD dwMaxNameSize;
	DWORD dwNameSize;
	CString Key;
	LONG lErr;
	if (bInitialize) Keys.RemoveAll();
	if ((lErr = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL,
						NULL, &dwTotal, &dwMaxNameSize, NULL, NULL,
						NULL)) == ERROR_SUCCESS) {
		for (dwIndex = 0; dwIndex < dwTotal; dwIndex++) {
			dwNameSize = dwMaxNameSize + 1;
			if ((lErr = RegEnumValue(	hKey, dwIndex,
								Key.GetBuffer(dwMaxNameSize + 1),
								&dwNameSize, NULL, NULL,
								NULL, NULL)) == ERROR_SUCCESS) {
				Key.ReleaseBuffer();
				//do not add guaranteed and empty default name
				if (Key.GetLength()) Keys.Add(Key);
			} else if (lErr == ERROR_NO_MORE_ITEMS) {
				break;
			} else {
				AddTraceLog(_T("APICall=RegEnumValue Error=%08X\r\n"), lErr);
			}
		}
		bRet = (DWORD)Keys.GetCount() == dwTotal;
	} else {
		AddTraceLog(_T("APICall=RegQueryInfoKey Error=%08X\r\n"), lErr);
	}
	return bRet;
}

BOOL PRU_RegistryEnumValues(HKEY hKey, CKVArray & KeyValues, BOOL bInitialize)
{
	BOOL bRet = FALSE;
	DWORD dwIndex;
	DWORD dwTotal;
	DWORD dwMaxSize;
	DWORD dwMaxNameSize;
	DWORD dwSize;
	DWORD dwNameSize;
	KeyValue KV;
	LONG lErr;
	if (bInitialize) KeyValues.RemoveAll();
	if ((lErr = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL,
						NULL, &dwTotal, &dwMaxNameSize, &dwMaxSize,
						NULL, NULL)) == ERROR_SUCCESS) {
		for (dwIndex = 0; dwIndex < dwTotal; dwIndex++) {
			dwNameSize = dwMaxNameSize + 1;
			dwSize = dwMaxSize;
			KV.Value.SetSize(dwMaxSize);
			if ((lErr = RegEnumValue(	hKey, dwIndex,
								KV.Key.GetBuffer(dwMaxNameSize + 1),
								&dwNameSize, NULL, &KV.dwType,
								(LPBYTE)KV.Value.GetData(),
								&dwSize)) == ERROR_SUCCESS) {
				KV.Key.ReleaseBuffer();
				KV.Value.SetSize(dwSize);
				//strings must be checked for null termination and multi strings for double null termination
				if ((KV.dwType == REG_SZ || KV.dwType == REG_EXPAND_SZ || KV.dwType == REG_MULTI_SZ) &&
					(dwSize == 0 || ((TCHAR*)KV.Value.GetData())[dwSize / sizeof(TCHAR) - 1] != 0)) {
					AddTraceLog(_T("APICall=RegistryEnumValues Key=%s Improperly null-terminated string\r\n"), KV.Key);
					KV.Value.SetSize(dwSize + sizeof(TCHAR));
					KV.Value[dwSize] = 0;
#ifdef UNICODE
					KV.Value[dwSize + 1] = 0;
#endif
					dwSize += sizeof(TCHAR);
				}
				if (KV.dwType == REG_MULTI_SZ && (dwSize == sizeof(TCHAR) || ((TCHAR*)KV.Value.GetData())[dwSize / sizeof(TCHAR) - 2] != 0)) {
					AddTraceLog(_T("APICall=RegistryEnumValues Key=%s Improperly null-terminated multi-string\r\n"), KV.Key);
					KV.Value.SetSize(dwSize + sizeof(TCHAR));
					KV.Value[dwSize] = 0;
#ifdef UNICODE
					KV.Value[dwSize + 1] = 0;
#endif
				}
				//must add copies or pointer reference lives on...
				KeyValues.Add(KV);
			} else if (lErr == ERROR_NO_MORE_ITEMS) {
				break;
			} else {
				AddTraceLog(_T("APICall=RegEnumValue Error=%08X\r\n"), lErr);
			}
		}
		bRet = (DWORD)KeyValues.GetCount() == dwTotal;
	} else {
		AddTraceLog(_T("APICall=RegQueryInfoKey Error=%08X\r\n"), lErr);
	}
	return bRet;
}

BOOL PRU_RegistryEnumValues(HKEY hBaseKey, LPCTSTR szRegistryPath,
                               CKVArray & KeyValues, BOOL bInitialize)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_RegistryEnumValues(hKey, KeyValues, bInitialize);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_RegistryEnumKey(HKEY hKey, CStringArray & StringArray)
{
	BOOL bRet = FALSE;
	DWORD dwIndex;
	DWORD dwTotal;
	DWORD dwMaxSize;
	CString String;
	LONG lErr;
	StringArray.RemoveAll();
	if ((lErr = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwTotal, &dwMaxSize,
						NULL, NULL, NULL, NULL, NULL,
						NULL)) == ERROR_SUCCESS) {
		for (dwIndex = 0; dwIndex < dwTotal; dwIndex++) {
			if ((lErr = RegEnumKey(	hKey, dwIndex, String.GetBuffer(dwMaxSize + 1), 
							dwMaxSize + 1)) == ERROR_SUCCESS) {
				String.ReleaseBuffer();
				StringArray.Add(String);
			} else if (lErr == ERROR_NO_MORE_ITEMS) {
				break;
			} else {
				AddTraceLog(_T("APICall=RegEnumKey Error=%08X\r\n"), lErr);
			}
		}
		bRet = (DWORD)StringArray.GetCount() == dwTotal;
	} else {
		AddTraceLog(_T("APICall=RegQueryInfoKey Error=%08X\r\n"), lErr);
	}
	//call should allow partial results as trace logs here suffice
	return bRet;
}

BOOL PRU_RegistryEnumKey(HKEY hBaseKey, LPCTSTR szRegistryPath, CStringArray & StringArray)
{
	HKEY hKey;
	BOOL bRet = FALSE;
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		bRet = PRU_RegistryEnumKey(hKey, StringArray);
		PRU_RegCloseKey(hKey);
	}
	return bRet;
}

BOOL PRU_RegistryValueCount(HKEY hKey, DWORD &dwCount)
{
	BOOL bRet = FALSE;
	LONG lErr;
	if ((lErr = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL,
						NULL, &dwCount, NULL, NULL, NULL,
						NULL)) == ERROR_SUCCESS) {
		bRet = TRUE;
	} else {
		AddTraceLog(_T("APICall=RegQueryInfoKey Error=%08X\r\n"), lErr);
	}
	return bRet;
}

BOOL PRU_RegistryKeyCount(HKEY hKey, DWORD &dwCount)
{
	BOOL bRet = FALSE;
	LONG lErr;
	if ((lErr = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwCount, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL)) == ERROR_SUCCESS) {
		bRet = TRUE;
	} else {
		AddTraceLog(_T("APICall=RegQueryInfoKey Error=%08X\r\n"), lErr);
	}
	return bRet;
}

BOOL PRU_RegistryKeyExists(HKEY hBaseKey, LPCTSTR szRegistryPath, BOOL& bExists)
{
	LONG lErr;
	HKEY hKey;
	if ((lErr = RegOpenKey(hBaseKey, szRegistryPath, &hKey)) == ERROR_SUCCESS) {
		RegCloseKey(hKey);
		bExists = TRUE;
		return TRUE;
	} else if (lErr == ERROR_FILE_NOT_FOUND) {
		bExists = FALSE;
		return TRUE;
	}
	AddTraceLog(_T("APICall=RegOpenKey SubKey=%s Error=%08X\r\n"), szRegistryPath, lErr);
	return FALSE;
}

BOOL PRU_RegDeleteKey(HKEY hKey, LPCTSTR lpSubKey)
{
	LONG lRet;
	if ((lRet = RegDeleteKey(hKey, lpSubKey)) == ERROR_SUCCESS) return TRUE;
	AddTraceLog(_T("APICall=RegDeleteKey SubKey=%s Error=%08X\r\n"), lpSubKey, lRet);
	return FALSE;
}

BOOL PRU_RegDeleteTree(HKEY hBaseKey, LPCTSTR szRegistryPath)
{
	//should any failure attempt rollback which may then also fail?
	HKEY hKey;
	CStringArray Keys;
	DWORD dwCount;
	//delete child keys
	if (PRU_RegOpenKey(hBaseKey, szRegistryPath, hKey)) {
		if (PRU_RegistryEnumKey(hKey, Keys)) {
			for (dwCount = 0; dwCount < (DWORD)Keys.GetCount(); dwCount++) {
				//stop on failure
				if (!PRU_RegDeleteTree(hKey, Keys[dwCount])) return FALSE;
			}
		}
		PRU_RegCloseKey(hKey);
	}
	//delete main key
	return PRU_RegDeleteKey(hBaseKey, szRegistryPath);
}

BOOL PRU_RegDeleteValue(HKEY hKey, LPCTSTR lpValueName)
{
	LONG lRet;
	if ((lRet = RegDeleteValue(hKey, lpValueName)) == ERROR_SUCCESS) return TRUE;
	AddTraceLog(_T("APICall=RegDeleteKey ValueName=%s Error=%08X\r\n"), lpValueName, lRet);
	return FALSE;
}

#include <wincrypt.h>

//memory only, file MD5 must hash in chunks to reduce memory usage
BOOL PRU_ComputeMemoryMD5(BYTE* pbBuffer, DWORD dwBufferLength, BYTE* pbMD5Hash)
{
	HCRYPTPROV hCryptProv;
	HCRYPTHASH hHash;
	DWORD dwHashLen = 16; // The MD5 algorithm always returns 16 bytes
	if (CryptAcquireContext(&hCryptProv, 
		NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET)) {
		if (CryptCreateHash(hCryptProv, 
			CALG_MD5,	// algorithm identifier definitions see: wincrypt.h
			0, 0, &hHash)) {
			if (CryptHashData(hHash, pbBuffer, dwBufferLength, 0)) {
				if (CryptGetHashParam(hHash, HP_HASHVAL, pbMD5Hash, &dwHashLen, 0)) {
				} else {
					AddTraceLog(_T("Error getting hash param"));
					return FALSE;
				}
			} else {
				AddTraceLog(_T("Error hashing data"));
				return FALSE;
			}
			CryptDestroyHash(hHash); 
		} else {
			AddTraceLog(_T("Error creating hash"));
			return FALSE;
		}
		CryptReleaseContext(hCryptProv, 0); 
	} else {
		AddTraceLog(_T("Error acquiring context"));
		return FALSE;
	}
	return TRUE;
}

//hash in intervals for memory efficiency
BOOL PRU_ComputeFileMD5(CFile & File, BYTE* pbMD5Hash)
{
	HCRYPTPROV hCryptProv;
	HCRYPTHASH hHash;
	CByteArray Bytes;
	DWORD dwHashLen = 16; // The MD5 algorithm always returns 16 bytes
	if (CryptAcquireContext(&hCryptProv, 
		NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET)) {
		if (CryptCreateHash(hCryptProv, 
			CALG_MD5,	// algorithm identifier definitions see: wincrypt.h
			0, 0, &hHash)) {
			do {
				Bytes.SetSize(min(File.GetLength() - File.GetPosition(), 0x200000));
				File.Read(Bytes.GetData(), (UINT)Bytes.GetSize());
				if (CryptHashData(hHash, Bytes.GetData(), (DWORD)Bytes.GetSize(), 0)) {
					if (CryptGetHashParam(hHash, HP_HASHVAL, pbMD5Hash, &dwHashLen, 0)) {
					} else {
						AddTraceLog(_T("Error getting hash param"));
						return FALSE;
					}
				} else {
					AddTraceLog(_T("Error hashing data"));
					return FALSE;
				}
			} while (File.GetPosition() != File.GetLength());
			CryptDestroyHash(hHash); 
		} else {
			AddTraceLog(_T("Error creating hash"));
			return FALSE;
		}
		CryptReleaseContext(hCryptProv, 0); 
	} else {
		AddTraceLog(_T("Error acquiring context"));
		return FALSE;
	}
	return TRUE;
}

//shell32
void PRU_GetCommandLineLaunchArg(LPCTSTR szCmdLine, CString& String) //and kernel32
{
	int argc = 0;
	LPWSTR* lpArgv = NULL;
	lpArgv = CommandLineToArgvW(CT2W(szCmdLine), &argc);
	if (lpArgv && argc) String = lpArgv[0];
	if (lpArgv) LocalFree(lpArgv);
	if (String.GetLength() > 1 && String[0] == '\"' && String[String.GetLength() - 1] == '\"') {
		String.Delete(0);
		String.Delete(String.GetLength() - 1);
	}
}

void PRU_GetShellIcon(LPCTSTR szIconPath, HICON& hIcon)
{
	SHFILEINFO sfi;
	if (szIconPath) {
		ZeroMemory(&sfi, sizeof(sfi));
		SHGetFileInfo(	szIconPath, 0, &sfi, sizeof(sfi),
						SHGFI_SMALLICON | SHGFI_ICON);
		hIcon = sfi.hIcon;
	}
}

//setupapi

BOOL PRU_SetupDiGetDeviceRegistryProperty(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData, DWORD dwProperty, CByteArray & Bytes)
{
	DWORD dwReqSize;
	CAPISetup SetupAPI;
	//ERROR_INVALID_DATA if property does not exist or data is not valid
	//SPDRP_INSTALL_STATE will return ERROR_NO_SUCH_DEVINST if device is not installed
	if (SetupAPI.Get_SetupDiGetDeviceRegistryProperty()(hDevInfo, pspDevInfoData, dwProperty, NULL, NULL, 0, &dwReqSize) ||
		GetLastError() == ERROR_INVALID_DATA ||
		GetLastError() == ERROR_NO_SUCH_DEVINST && dwProperty == SPDRP_INSTALL_STATE) {
		Bytes.RemoveAll();
		return TRUE;
	}
	while (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		Bytes.SetSize(dwReqSize);
		if (SetupAPI.Get_SetupDiGetDeviceRegistryProperty()(hDevInfo, pspDevInfoData, dwProperty, NULL, Bytes.GetData(), dwReqSize, &dwReqSize)) {
			Bytes.SetSize(dwReqSize);
			return TRUE;
		}
	}
	AddTraceLog(_T("APICall=SetupDiGetDeviceRegistryProperty Property=%08X Error=%08X\r\n"), dwProperty, GetLastError());
	return FALSE;
}

BOOL PRU_GetDeviceRegistryPropertyMultiString(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData, DWORD dwProperty, CStringArray & Strings)
{
	CByteArray Bytes;
	if (PRU_SetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, dwProperty, Bytes)) {
		if (Bytes.GetSize())
			StringArrayFromLPTSTRS((TCHAR*)Bytes.GetData(), Strings);
		return TRUE;
	}
	return FALSE;
}

BOOL PRU_GetDeviceRegistryPropertyString(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData, DWORD dwProperty, CString & String)
{
	CByteArray Bytes;
	if (PRU_SetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, dwProperty, Bytes)) {
		if (Bytes.GetSize()) {
			memcpy(String.GetBuffer((int)Bytes.GetSize() / sizeof(TCHAR)), Bytes.GetData(), Bytes.GetSize());
			String.ReleaseBuffer();
		}
		return TRUE;
	}
	return FALSE;
}

BOOL PRU_GetDeviceRegistryPropertyDWORD(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData, DWORD dwProperty, DWORD & dwValue)
{
	CByteArray Bytes;
	if (PRU_SetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData, dwProperty, Bytes)) {
		if (Bytes.GetSize() == sizeof(DWORD)) {
			dwValue = *((DWORD*)Bytes.GetData());
			return TRUE;
		}
	}
	return FALSE;
}

struct MOUNTMGR_TARGET_NAME { USHORT DeviceNameLength; WCHAR DeviceName[1]; }; 
struct MOUNTMGR_VOLUME_PATHS { ULONG MultiSzLength; WCHAR MultiSz[1]; }; 
 
#define MOUNTMGRCONTROLTYPE ((ULONG)'m')
#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)  

LPWSTR GetFilePath(HANDLE hFile) 
{
	CAPINTDLL NTDLL;
	MOUNTMGR_TARGET_NAME nameMnt;
    MOUNTMGR_VOLUME_PATHS nameOutMnt;
    FILE_NAME_INFORMATION nameRel;
    UNICODE_STRING nameFull;
    ULONG returnedLength;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
    if ((status = NTDLL.Get_NtQueryObject()(hFile, (OBJECT_INFORMATION_CLASS)_ObjectNameInformation,
		&nameFull, USHRT_MAX, &returnedLength)) == STATUS_SUCCESS) {
		if ((status = NTDLL.Get_NtQueryInformationFile()(hFile, &iosb, &nameRel,
			sizeof(nameRel), (FILE_INFORMATION_CLASS)FileNameInformation)) == STATUS_SUCCESS) {
		    //unsure how this works with network paths...
			if (nameFull.Length >= nameRel.FileNameLength) {
				nameMnt.DeviceNameLength = (USHORT)( 
					nameFull.Length - nameRel.FileNameLength); 
				wcsncpy(nameMnt.DeviceName, nameFull.Buffer, 
					nameMnt.DeviceNameLength / sizeof(WCHAR)); 
				HANDLE hMountPointMgr = CreateFile(_T("\\\\.\\MountPointManager"), 
					0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
					NULL, OPEN_EXISTING, 0, NULL); 
				DWORD bytesReturned; 
				BOOL success = DeviceIoControl(hMountPointMgr, 
					IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH, &nameMnt, 
					sizeof(nameMnt), &nameOutMnt, sizeof(nameOutMnt), 
					&bytesReturned, NULL); 
				CloseHandle(hMountPointMgr);
				if (success && nameOutMnt.MultiSzLength > 0) {
					wcsncat(nameOutMnt.MultiSz, nameRel.FileName, 
						nameRel.FileNameLength / sizeof(WCHAR)); 
					return nameOutMnt.MultiSz;
				}
			}
		}
	}
	return NULL;
}

LPCTSTR CAPINTDLL::m_szLibName = { _T("ntdll.dll") };
LPCSTR CAPINTDLL::m_pszaArgs[] = {
	"NtQueryObject", "NtQuerySystemInformation", "NtQueryInformationProcess", "NtQuerySecurityObject",
	"NtOpenDirectoryObject", "NtQueryDirectoryObject", "NtQueryInformationFile", "NtSuspendProcess",
	"NtResumeProcess", "RtlInitUnicodeString", "RtlFreeUnicodeString", NULL };

LPCTSTR CAPIPSAPI::m_szLibName = { _T("psapi.dll") };
LPCSTR CAPIPSAPI::m_pszaArgs[] = {
	LOOKUP_GetModuleFileNameEx, "EnumProcesses", LOOKUP_GetProcessImageFileName, NULL };

LPCTSTR CAPIUser32::m_szLibName = { _T("user32.dll") };
LPCSTR CAPIUser32::m_pszaArgs[] = { "IsHungAppWindow", NULL };

LPCTSTR CAPISecur32::m_szLibName = { _T("secur32.dll") };
LPCSTR CAPISecur32::m_pszaArgs[] = {
	"LsaEnumerateLogonSessions", "LsaGetLogonSessionData", "LsaFreeReturnBuffer", NULL };

LPCTSTR CAPISetup::m_szLibName = { _T("setupapi.dll") };
LPCSTR CAPISetup::m_pszaArgs[] = {
	"CM_Get_DevNode_Status", LOOKUP_SetupDiGetDeviceRegistryProperty, LOOKUP_SetupDiGetClassDevs,
	"SetupDiEnumDeviceInfo", "SetupDiDestroyDeviceInfoList", "pSetupInfIsInbox", 
	"SetupDiRemoveDevice", "SetupDiGetDevicePropertyW", LOOKUP_SetupDiGetDeviceInstallParams,
	LOOKUP_SetupDiSetClassInstallParams, "SetupDiCallClassInstaller", LOOKUP_SetupGetInfInformation,
	LOOKUP_SetupQueryInfVersionInformation, LOOKUP_SetupOpenInfFile, LOOKUP_SetupFindFirstLine,
	LOOKUP_SetupGetStringField, "SetupCloseInfFile", LOOKUP_SetupUninstallOEMInf, NULL };