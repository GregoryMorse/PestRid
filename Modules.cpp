#include "stdafx.h"
#include <atlbase.h>
#include <atlconv.h>

#include <afxtempl.h>
#include <winsvc.h>
#define REMOTE_BOOT
#include <regstr.h>

#include "WinAPI.h"
#include "DisplayTypes.h"
#include "Modules.h"

void CGatherer::AddItem(CEnumModel* pModel, CRuntimeClass* pRuntimeClass, CCopyDWordArray & NewValues)
{
	DWORD dwIndex = (DWORD)m_ValueIndexes.Add(NewValues);
	pModel->OnItemNew(pRuntimeClass, (void*)(DWORD_PTR)dwIndex);
}

CString ResolveNTPath(LPCTSTR szPath)
{
	CString Path = szPath;
	CString String;
	//must convert %SystemRoot%\system32 into %SystemRoot%\SysWOW64 for WOW64 entries
	//should look for API or generalization of this
	if (_tcsnicmp(szPath, _T("\\SystemRoot\\"), sizeof(_T("\\SystemRoot\\")) / sizeof(TCHAR) - 1) == 0) {
		Path = Path.Mid(sizeof(_T("\\SystemRoot")) / sizeof(TCHAR) - 1);
		Path = _T("%SystemRoot%") + Path;
	}
	PRU_ExpandEnvironmentStrings(Path, String);
	String.ReleaseBuffer();
	Path = String.TrimLeft();
	return Path;
}

#define REG_STR_SERVICEORDERLIST\
					REGSTR_PATH_CURRENT_CONTROL_SET _T("\\ServiceGroupOrder")
#define REG_STR_GROUPORDERLIST\
						REGSTR_PATH_CURRENT_CONTROL_SET _T("\\GroupOrderList")

#define REGSTR_PATH_MOUNTEDDEVICES _T("SYSTEM\\MountedDevices")

#define REGSTR_WINDOWS_COMPONENT_PACKAGES _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\Packages")
#define REGSTR_WINDOWS_COMPONENT_PACKAGEINDEX _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\PackageIndex")
#define REGSTR_WINDOWS_COMPONENT_SESSIONS _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\SessionsPending")
#define REGSTR_WINDOWS_SIDEBYSIDE_COMPONENT_WINNERS _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SideBySide\\Winners")
#define REGSTR_WINDOWS_COMPONENTS _T("COMPONENTS\\DerivedData\\Components")
#define REGSTR_WINDOWS_VERSIONEDINDEX _T("COMPONENTS\\DerivedData\\VersionedIndex")
#define REGSTR_WINDOWS_INSTALLER_PRODUCTS _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData")
#define REGSTR_WINDOWS_INSTALLER_FOLDERS _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders")

#define REGSTR_WINDOWS_WPAD _T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Wpad")

#define REGSTR_WINDOWS_NETWORK_PROFILES _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Profiles")
#define REGSTR_WINDOWS_NETWORK_SIGNATURES_UNMANAGED _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Signatures\\Unmanaged")
#define REGSTR_WINDOWS_NETWORK_NLA_CACHE_INTRANET _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Nla\\Cache\\Intranet")
#define REGSTR_WINDOWS_NETWORK_NLA_WIRELESS _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Nla\\Wireless")

//REGSTR_PATH_EXPLORER _T("\\AutoplayHandlers\\KnownDevices")
//HKEY_LOCAL_MACHINE\COMPONENTS\DerivedData
//PendingXmlIdentifier, NextQueueEntryIndex, AdvancedInstallersNeedResolving
//InstallMapMissingComponentKey?

//All HKEY_LOCAL_MACHINE\SOFTWARE need Wow6432Node

struct RegEnumArgValues
{
	TCHAR* szValueName;
	DWORD dwType;
	TCHAR* szSubPath;
	TCHAR* szFilterPath;
};

struct RegEnumArgs
{
	HKEY hBaseKey;
	TCHAR* szPath;
	int iPrimaryItemIndex;
	RegEnumArgValues ArgValues[40];
};

#define MAKEREGENUM(cname) class cname : public CRegEnumerator\
{\
public:\
	DECLARE_DYNCREATE(cname)\
	cname() : CRegEnumerator(&Args) { }\
	private:\
	static RegEnumArgs Args;\
};

#define REG_PATH		0x80
#define REG_KEYS		0x81
#define REG_VALUES		0x82
#define REG_KEYCOUNT	0x83
#define REG_VALUECOUNT	0x84
#define REG_KEYVALUES	0x85
#define REG_PRIORTYPES  0x86
#define REG_PRIORVALUES 0x87
#define REG_TYPES		0xFF
#define REG_DESCENT		0x100
#define REG_DESCENTNEXT 0x200
#define REG_DESCENTPREV 0x400

#define SERVICE_SERVICENAME 0
#define SERVICE_DISPLAYNAME 1
#define SERVICE_CURRENTSTATE 2
#define SERVICE_SERVICETYPE 3

class CRegEnumerator : public CGatherer
{
public:
	CRegEnumerator(RegEnumArgs* pArgs) { m_pArgs = pArgs; }
	void* GetItemData(void *pvKey, int iIndex)
	{
		DWORD dwType = m_pArgs->ArgValues[iIndex].dwType;
		if (m_pArgs->szPath && m_Values[(DWORD_PTR)pvKey].GetCount() <= iIndex) return NULL;
		DWORD dwIndex = m_pArgs->szPath ? m_Values[(DWORD_PTR)pvKey][iIndex] : 0;
		switch (dwType & REG_TYPES) { 
		case REG_PATH:
			return (void*)m_PathKeys[dwIndex][m_PathValues[(DWORD_PTR)pvKey][iIndex]].GetString();
		case REG_KEYS:
			return (dwIndex == ~0UL) ? NULL : m_KeysArrays.GetData() + dwIndex;
		case REG_VALUES:
			return (dwIndex == ~0UL) ? NULL : m_ValuesArrays.GetData() + dwIndex;
		case REG_SZ:
			return (dwIndex == ~0UL) ? NULL : (void*)m_StrValues[dwIndex].GetString();
		case REG_MULTI_SZ:
			return (dwIndex == ~0UL) ? NULL : (m_pArgs->szPath ? (void*)&m_MultiStrValues[dwIndex] : (void*)&m_MultiStrValues[dwIndex][(DWORD_PTR)pvKey]);
		case REG_DWORD:
			return (dwIndex == ~0UL) ? NULL : &m_DWordValues[dwIndex];
		case REG_BINARY:
			return (dwIndex == ~0UL) ? NULL : &m_BinaryValues[dwIndex];
		case REG_KEYCOUNT:
			return (dwIndex == ~0UL) ? NULL : &m_DWordValues[dwIndex];
		case REG_VALUECOUNT:
			return (dwIndex == ~0UL) ? NULL : &m_DWordValues[dwIndex];
		case REG_KEYVALUES:
			return (dwIndex == ~0UL) ? NULL : (m_pArgs->szPath ? (void*)&m_KeyValues[dwIndex] : (void*)m_KeyValues[dwIndex][(DWORD_PTR)pvKey].Key.GetString());
		case REG_PRIORTYPES:
			return (dwIndex == ~0UL) ? NULL : (m_pArgs->szPath ? (void*)&m_KeyValues[dwIndex] : (void*)&m_KeyValues[dwIndex][(DWORD_PTR)pvKey].dwType);
		case REG_PRIORVALUES:
			return (dwIndex == ~0UL) ? NULL : (m_pArgs->szPath ? (void*)&m_KeyValues[dwIndex] : (void*)&m_KeyValues[dwIndex][(DWORD_PTR)pvKey].Value);
		default:
			return NULL;
		}
	}
	void RecurseKeys(HKEY hKey, DWORD & dwArgCount, CCopyDWordArray & NewValues, BOOL bStart, CCopyStringArray* PathKeys, CEnumModel* pModel, CCopyDWordArray& PathValues)
	{
		CString String;
		DWORD dwValue;
		DWORD dwSubCount;
		DWORD dwInitArg = dwArgCount;
		DWORD dwPrevArg = dwArgCount;
		for (; m_pArgs->ArgValues[dwArgCount].dwType; dwArgCount++) {
			if (!bStart && (dwArgCount != ((m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENTPREV) ? dwPrevArg : dwInitArg) || !(m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENT)) && !(m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENTNEXT)) { dwArgCount--; return; }
			if (dwArgCount != dwInitArg && (m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENT)) {
				CCopyStringArray SubEntries;
				if (m_pArgs->ArgValues[dwArgCount].szSubPath) SubEntries.Add(m_pArgs->ArgValues[dwArgCount].szSubPath);
				if (!m_pArgs->ArgValues[dwArgCount].szSubPath && !hKey && ((DWORD)m_pArgs->iPrimaryItemIndex < dwArgCount)) SubEntries.Add(_T(""));
				if (!m_pArgs->ArgValues[dwArgCount].szSubPath && hKey && !PRU_RegistryEnumKey(hKey, SubEntries) && ((DWORD)m_pArgs->iPrimaryItemIndex < dwArgCount)) SubEntries.Add(_T(""));
				if (SubEntries.GetCount() == 0) {
					while (m_pArgs->ArgValues[dwArgCount].dwType & (REG_DESCENTNEXT | REG_DESCENTPREV)) dwArgCount++;
				}
				DWORD dwOldArgCount = dwArgCount;
				INT_PTR iOldIndex = NewValues.GetCount();
				INT_PTR iBaseCount = m_Values.GetCount();
				for (dwSubCount = 0; dwSubCount < (DWORD)SubEntries.GetCount(); dwSubCount++) {
					HKEY hSubKey;
					if (m_pArgs->ArgValues[dwOldArgCount].szFilterPath && SubEntries[dwSubCount].CompareNoCase(m_pArgs->ArgValues[dwOldArgCount].szFilterPath) == 0) {
						SubEntries.RemoveAt(dwSubCount);
						dwSubCount--;
						continue;
					}
					PathValues.Add(dwSubCount);
					if (hKey && PRU_RegOpenKey(hKey, SubEntries[dwSubCount], hSubKey)) {
						dwArgCount = dwOldArgCount;
						RecurseKeys(hSubKey, dwArgCount, NewValues, FALSE, m_Values.GetCount() == iBaseCount ? &SubEntries : NULL, pModel, PathValues);
						PRU_RegCloseKey(hSubKey);
					} else {
						dwArgCount = dwOldArgCount;
						RecurseKeys(NULL, dwArgCount, NewValues, FALSE, m_Values.GetCount() == iBaseCount ? &SubEntries : NULL, pModel, PathValues);
					}
					if ((DWORD)m_pArgs->iPrimaryItemIndex == dwOldArgCount) {
						//the values and path values are still needed
						//  so must make a copy or transfer will destroy them
						CCopyDWordArray CopyValues;
						CopyValues.Copy(NewValues);
						m_Values.Add(CopyValues);
						CopyValues.Copy(PathValues);
						m_PathValues.Add(CopyValues);
						pModel->OnItemNew(GetRuntimeClass(), (void*)(DWORD_PTR)(m_Values.GetCount() - 1));
					}
					//if on the same descent path as primary item then delete
					if ((DWORD)m_pArgs->iPrimaryItemIndex >= dwOldArgCount) {
						if ((DWORD)m_pArgs->iPrimaryItemIndex < dwArgCount || (m_pArgs->ArgValues[dwArgCount + 1].dwType & REG_DESCENTNEXT)) {
							NewValues.RemoveAt(iOldIndex, NewValues.GetCount() - iOldIndex);
							PathValues.RemoveAt(iOldIndex, PathValues.GetCount() - iOldIndex);
						}
					}
					if (m_pArgs->ArgValues[dwArgCount + 1].dwType & REG_DESCENTPREV) {
						dwPrevArg = dwArgCount + 1;
					}
				}
			} else {
				CCopyStringArray StringEntries;
				CCopyByteArray Bytes;
				CCopyKVArray KVArray;
				switch (m_pArgs->ArgValues[dwArgCount].dwType & REG_TYPES) {
				case REG_PATH:
					if (PathKeys) StringEntries.Copy(*PathKeys); //still needed in parent
					//use previous index if already sequenced which the NULL reference represents
					NewValues.Add(PathKeys ? (DWORD)m_PathKeys.Add(StringEntries) : m_Values[m_Values.GetCount() - 1][NewValues.GetCount()]);
					break;
				case REG_KEYS:
					NewValues.Add(hKey != NULL && PRU_RegistryEnumKey(hKey, StringEntries) ? (DWORD)m_KeysArrays.Add(StringEntries) : ~0UL);
					break;
				case REG_VALUES:
					NewValues.Add(hKey != NULL && PRU_RegistryEnumValues(hKey, StringEntries) ? (DWORD)m_ValuesArrays.Add(StringEntries) : ~0UL);
					break;
				case REG_SZ:
					NewValues.Add(hKey != NULL && PRU_ReadRegistryString(hKey, m_pArgs->ArgValues[dwArgCount].szValueName, String) ?
									(DWORD)m_StrValues.Add(String) : ~0UL);
					break;
				case REG_MULTI_SZ:
					NewValues.Add(hKey != NULL && PRU_ReadRegistryMultiString(hKey, m_pArgs->ArgValues[dwArgCount].szValueName, StringEntries) ?
									(DWORD)m_MultiStrValues.Add(StringEntries) : ~0UL);
					break;
				case REG_DWORD:
					NewValues.Add(hKey != NULL && PRU_ReadRegistryDWORD(hKey, m_pArgs->ArgValues[dwArgCount].szValueName, dwValue) ?
									(DWORD)m_DWordValues.Add(dwValue) : ~0UL);
					break;
				case REG_BINARY:
					NewValues.Add(hKey != NULL && PRU_ReadRegistryBinary(hKey, m_pArgs->ArgValues[dwArgCount].szValueName, Bytes) ?
									(DWORD)m_BinaryValues.Add(Bytes) : ~0UL);
					break;
				case REG_KEYCOUNT:
					BOOL bExists;
					NewValues.Add(hKey != NULL && PRU_RegistryKeyCount(hKey, dwValue) ?
						(DWORD)m_DWordValues.Add(dwValue - ((m_pArgs->ArgValues[dwArgCount].szFilterPath && PRU_RegistryKeyExists(hKey, m_pArgs->ArgValues[dwArgCount].szFilterPath, bExists) && bExists) ? 1 : 0)) : ~0UL);
					break;
				case REG_VALUECOUNT:
					NewValues.Add(hKey != NULL && PRU_RegistryValueCount(hKey, dwValue) ?
									(DWORD)m_DWordValues.Add(dwValue) : ~0UL);
					break;
				case REG_KEYVALUES:
					NewValues.Add(hKey != NULL && PRU_RegistryEnumValues(hKey, KVArray) ?
									(DWORD)m_KeyValues.Add(KVArray) : ~0UL);
					break;
				case REG_PRIORTYPES:
					NewValues.Add(NewValues[NewValues.GetCount() - 1]);
					break;
				case REG_PRIORVALUES:
					NewValues.Add(NewValues[NewValues.GetCount() - 1]);
					break;
				default:
					if (m_pArgs->ArgValues[dwArgCount].dwType) NewValues.Add(~0UL);
					break;
				}
				//this should be linked closer to REG_PATH
				if (m_pArgs->ArgValues[dwArgCount].dwType &&
					!(m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENT) &&
					(!bStart || dwArgCount != 0)) {
					PathValues.Add(~0UL);
				}
			}
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		HKEY hKey;
		DWORD_PTR dwCount;
		DWORD dwArgCount;
		CCopyStringArray Keys;
		CCopyDWordArray PathValues;
		if (!m_pArgs->szPath) {
			if (PRU_RegOpenKey(m_pArgs->hBaseKey, m_pArgs->ArgValues[0].szSubPath, hKey)) {
				if (hKey && (m_pArgs->ArgValues[0].dwType & REG_TYPES) == REG_MULTI_SZ) {
					CCopyStringArray StringArray;
					if (PRU_ReadRegistryMultiString(hKey, m_pArgs->ArgValues[0].szValueName, StringArray)) {
						m_MultiStrValues.Add(StringArray);
						for (dwCount = 0; dwCount < (DWORD)m_MultiStrValues[0].GetCount(); dwCount++) {
							pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
						}
					}
				} else if (hKey && (m_pArgs->ArgValues[0].dwType & REG_TYPES) == REG_KEYVALUES) {
					CCopyKVArray KVArray;
					if (PRU_RegistryEnumValues(hKey, KVArray)) {
						m_KeyValues.Add(KVArray);
						for (dwCount = 0; dwCount < (DWORD)m_KeyValues[0].GetCount(); dwCount++) {
							pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
						}
					}
				}
				PRU_RegCloseKey(hKey);
			}
		} else if (PRU_RegistryEnumKey(m_pArgs->hBaseKey, m_pArgs->szPath, Keys)) {
			for (dwCount = 0; dwCount < (DWORD)Keys.GetCount(); dwCount++) {
				CCopyDWordArray NewValues;
				dwArgCount = 0;
				if (m_pArgs->ArgValues[dwArgCount].szFilterPath && Keys[dwCount].CompareNoCase(m_pArgs->ArgValues[dwArgCount].szFilterPath) == 0) {
					Keys.RemoveAt(dwCount);
					dwCount--;
					continue;
				}
				PathValues.Add((DWORD)dwCount);
				if (PRU_RegOpenKey(m_pArgs->hBaseKey, CString(m_pArgs->szPath) + _T("\\") + Keys[dwCount], hKey)) {
					RecurseKeys(hKey, dwArgCount, NewValues, TRUE, m_Values.GetCount() == 0 ? &Keys : NULL, pModel, PathValues);
					PRU_RegCloseKey(hKey);
				} else {
					RecurseKeys(NULL, dwArgCount, NewValues, TRUE, m_Values.GetCount() == 0 ? &Keys : NULL, pModel, PathValues);
				}
				if (m_pArgs->iPrimaryItemIndex == 0) {
					m_Values.Add(NewValues);
					m_PathValues.Add(PathValues);
					pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
				}
				PathValues.RemoveAll();
			}
		}
	}
	void DeleteRecurseKeys(HKEY hKey, DWORD & dwArgCount, BOOL bStart, CString & Path, BOOL & bCheck, DWORD & dwValCount, BOOL & bFound, void* pvKey)
	{
		CString String;
		DWORD dwValue;
		DWORD dwSubCount;
		DWORD dwInitArg = dwArgCount;
		DWORD dwPrevArg = dwArgCount;
		for (; m_pArgs->ArgValues[dwArgCount].dwType; dwArgCount++) {
			if (!bStart && (dwArgCount != ((m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENTPREV) ? dwPrevArg : dwInitArg) || !(m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENT)) && !(m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENTNEXT)) { dwArgCount--; return; }
			if (dwArgCount != dwInitArg && (m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENT)) {
				CCopyStringArray SubEntries;
				if (m_pArgs->ArgValues[dwArgCount].szSubPath) SubEntries.Add(m_pArgs->ArgValues[dwArgCount].szSubPath);
				if (!m_pArgs->ArgValues[dwArgCount].szSubPath && !hKey && ((DWORD)m_pArgs->iPrimaryItemIndex < dwArgCount)) SubEntries.Add(_T(""));
				if (!m_pArgs->ArgValues[dwArgCount].szSubPath && hKey && !PRU_RegistryEnumKey(hKey, SubEntries) && ((DWORD)m_pArgs->iPrimaryItemIndex < dwArgCount)) SubEntries.Add(_T(""));
				if (SubEntries.GetCount() == 0) {
					while (m_pArgs->ArgValues[dwArgCount].dwType & (REG_DESCENTNEXT | REG_DESCENTPREV)) dwArgCount++;
				}
				DWORD dwOldArgCount = dwArgCount;
				INT_PTR iOldIndex = dwValCount;
				for (dwSubCount = 0; dwSubCount < (DWORD)SubEntries.GetCount(); dwSubCount++) {
					HKEY hSubKey;
					if (m_pArgs->ArgValues[dwOldArgCount].szFilterPath && SubEntries[dwSubCount].CompareNoCase(m_pArgs->ArgValues[dwOldArgCount].szFilterPath) == 0) {
						SubEntries.RemoveAt(dwSubCount);
						dwSubCount--;
						continue;
					}
					if (hKey && PRU_RegOpenKey(hKey, SubEntries[dwSubCount], hSubKey)) {
						dwArgCount = dwOldArgCount;
						DeleteRecurseKeys(hSubKey, dwArgCount, FALSE, SubEntries[dwSubCount], bCheck, dwValCount, bFound, pvKey);
						PRU_RegCloseKey(hSubKey);
					} else {
						dwArgCount = dwOldArgCount;
						DeleteRecurseKeys(NULL, dwArgCount, FALSE, SubEntries[dwSubCount], bCheck, dwValCount, bFound, pvKey);
					}
					if (bFound) return;
					if ((DWORD)m_pArgs->iPrimaryItemIndex == dwOldArgCount && bCheck) {
						bFound = TRUE;
						PRU_RegDeleteTree(hKey, SubEntries[dwSubCount]);
						return;
					}
					bCheck = TRUE;
					//if on the same descent path as primary item then delete
					if ((DWORD)m_pArgs->iPrimaryItemIndex >= dwOldArgCount) {
						if ((DWORD)m_pArgs->iPrimaryItemIndex < dwArgCount || (m_pArgs->ArgValues[dwArgCount + 1].dwType & REG_DESCENTNEXT)) {
							dwValCount = (DWORD)iOldIndex;
						}
					}
					if (m_pArgs->ArgValues[dwArgCount + 1].dwType & REG_DESCENTPREV) {
						dwPrevArg = dwArgCount + 1;
					}
				}
			} else {
				CCopyStringArray StringEntries;
				CCopyByteArray Bytes;
				CCopyKVArray KVArray;
				switch (m_pArgs->ArgValues[dwArgCount].dwType & REG_TYPES) {
				case REG_PATH:
					//use previous index if already sequenced which the NULL reference represents
					bCheck = Path.Compare((LPCTSTR)GetItemData(pvKey, dwValCount)) == 0;
					dwValCount++;
					break;
				case REG_KEYS:
					if (hKey) {
						bCheck = PRU_RegistryEnumKey(hKey, StringEntries) ?
										StringEntries == *(CStringArray*)GetItemData(pvKey, dwValCount) :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_VALUES:
					if (hKey) {
						bCheck = PRU_RegistryEnumValues(hKey, StringEntries) ?
										StringEntries == *(CStringArray*)GetItemData(pvKey, dwValCount) :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_SZ:
					if (hKey) {
						bCheck = PRU_ReadRegistryString(hKey, m_pArgs->ArgValues[dwArgCount].szValueName, String) ?
										String.Compare((LPCTSTR)GetItemData(pvKey, dwValCount)) == 0 :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_MULTI_SZ:
					if (hKey) {
						bCheck = PRU_ReadRegistryMultiString(hKey, m_pArgs->ArgValues[dwArgCount].szValueName, StringEntries) ?
										StringEntries == *(CStringArray*)GetItemData(pvKey, dwValCount) :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_DWORD:
					if (hKey) {
						bCheck = PRU_ReadRegistryDWORD(hKey, m_pArgs->ArgValues[dwArgCount].szValueName, dwValue) ?
										dwValue == *(DWORD*)GetItemData(pvKey, dwValCount) :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_BINARY:
					if (hKey) {
						bCheck = PRU_ReadRegistryBinary(hKey, m_pArgs->ArgValues[dwArgCount].szValueName, Bytes) ?
										Bytes.GetSize() == ((CByteArray*)GetItemData(pvKey, dwValCount))->GetSize() &&
										memcmp(Bytes.GetData(), ((CByteArray*)GetItemData(pvKey, dwValCount))->GetData(), Bytes.GetSize()) == 0 :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_KEYCOUNT:
					BOOL bExists;
					if (hKey) {
						bCheck = PRU_RegistryKeyCount(hKey, dwValue) ?
										dwValue - ((m_pArgs->ArgValues[dwArgCount].szFilterPath && PRU_RegistryKeyExists(hKey, m_pArgs->ArgValues[dwArgCount].szFilterPath, bExists) && bExists) ? 1 : 0) == *(DWORD*)GetItemData(pvKey, dwValCount) :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_VALUECOUNT:
					if (hKey) {
						bCheck = PRU_RegistryValueCount(hKey, dwValue) ?
										dwValue == *(DWORD*)GetItemData(pvKey, dwValCount) :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_KEYVALUES:
					if (hKey) {
						bCheck = PRU_RegistryEnumValues(hKey, KVArray) ?
										KVArray == *(CKVArray*)GetItemData(pvKey, dwValCount) :
										NULL == GetItemData(pvKey, dwValCount);
						dwValCount++;
					}
					break;
				case REG_PRIORTYPES:
					break;
				case REG_PRIORVALUES:
					break;
				default:
					if (m_pArgs->ArgValues[dwArgCount].dwType) dwValCount++;
					break;
				}
				if (!bCheck) return;
				//this should be linked closer to REG_PATH
				if (m_pArgs->ArgValues[dwArgCount].dwType &&
					!(m_pArgs->ArgValues[dwArgCount].dwType & REG_DESCENT) &&
					(!bStart || dwArgCount != 0)) {
				}
			}
		}
	}
	void DeleteAction(void* pvKey)
	{
		HKEY hKey;
		DWORD dwCount;
		DWORD dwArgCount;
		CCopyStringArray Keys;
		if (!m_pArgs->szPath) {
			if (PRU_RegOpenKey(m_pArgs->hBaseKey, m_pArgs->ArgValues[0].szSubPath, hKey)) {
				if (hKey && (m_pArgs->ArgValues[0].dwType & REG_TYPES) == REG_MULTI_SZ) {
					CCopyStringArray StringArray;
					if (PRU_ReadRegistryMultiString(hKey, m_pArgs->ArgValues[0].szValueName, StringArray)) {
						for (dwCount = 0; dwCount < (DWORD)StringArray.GetCount(); dwCount++) {
							if (StringArray[dwCount].Compare((LPCTSTR)GetItemData(pvKey, 0)) == 0) {
								StringArray.RemoveAt(dwCount);
								break;
							}
						}
						PRU_WriteRegistryMultiString(hKey, m_pArgs->ArgValues[0].szValueName, StringArray);
					}
				} else if (hKey && (m_pArgs->ArgValues[0].dwType & REG_TYPES) == REG_KEYVALUES) {
					CCopyKVArray KVArray;
					if (PRU_RegistryEnumValues(hKey, KVArray)) {
						for (dwCount = 0; dwCount < (DWORD)KVArray.GetCount(); dwCount++) {
							if (KVArray[dwCount].Key.Compare((LPCTSTR)GetItemData(pvKey, 0)) == 0) {
								PRU_RegDeleteValue(hKey, KVArray[dwCount].Key);
								break;
							}
						}
					}
				}
				PRU_RegCloseKey(hKey);
			}
		} else if (PRU_RegistryEnumKey(m_pArgs->hBaseKey, m_pArgs->szPath, Keys)) {
			for (dwCount = 0; dwCount < (DWORD)Keys.GetCount(); dwCount++) {
				dwArgCount = 0;
				if (m_pArgs->ArgValues[dwArgCount].szFilterPath && Keys[dwCount].CompareNoCase(m_pArgs->ArgValues[dwArgCount].szFilterPath) == 0) {
					Keys.RemoveAt(dwCount);
					dwCount--;
					continue;
				}
				DWORD dwValCount = 0;
				BOOL bCheck = TRUE;
				BOOL bFound = FALSE;
				if (PRU_RegOpenKey(m_pArgs->hBaseKey, CString(m_pArgs->szPath) + _T("\\") + Keys[dwCount], hKey)) {
					DeleteRecurseKeys(hKey, dwArgCount, TRUE, Keys[dwCount], bCheck, dwValCount, bFound, pvKey);
					PRU_RegCloseKey(hKey);
				} else {
					DeleteRecurseKeys(NULL, dwArgCount, TRUE, Keys[dwCount], bCheck, dwValCount, bFound, pvKey);
				}
				if (bFound) break;
				if (m_pArgs->iPrimaryItemIndex == 0 && bCheck) {
					if (PRU_RegOpenKey(m_pArgs->hBaseKey, m_pArgs->szPath, hKey)) {
						PRU_RegDeleteTree(hKey, Keys[dwCount]);
						PRU_RegCloseKey(hKey);
						break;
					}
				}
			}
		}
	}
	static void DeleteAction(CGatherer* pGatherer, void* pvKey)
	{
		DWORD dwCount;
		CArray<void*, void*&>* Keys = (CArray<void*, void*&>*)pvKey;
		for (dwCount = 0; dwCount < (DWORD)Keys->GetCount(); dwCount++) {
			((CRegEnumerator*)pGatherer)->DeleteAction(Keys->GetAt(dwCount));
		}
	}
private:
	CArray<CCopyStringArray, CCopyStringArray&>  m_ValuesArrays;
	CArray<CCopyStringArray, CCopyStringArray&>  m_KeysArrays;
	CArray<CCopyStringArray, CCopyStringArray&>  m_PathKeys;
	CArray<CCopyDWordArray, CCopyDWordArray&> m_Values;
	CArray<CCopyDWordArray, CCopyDWordArray&> m_PathValues;
	CArray<CCopyStringArray, CCopyStringArray&> m_MultiStrValues;
	CArray<CCopyKVArray, CCopyKVArray&> m_KeyValues;
	CStringArray m_StrValues;
	CDWordArray m_DWordValues;
	CArray<CCopyByteArray, CCopyByteArray&> m_BinaryValues;
	RegEnumArgs* m_pArgs;
};

MAKEREGENUM(CWindowsSharedDLLEntries)
MAKEREGENUM(CWindowsAppPathsEntries)
MAKEREGENUM(CWindowsUninstallEntries)
MAKEREGENUM(CWindowsInstallerAssemblies)
MAKEREGENUM(CWindowsInstallerWin32Assemblies)
MAKEREGENUM(CWindowsClassesTypelibEntries)
MAKEREGENUM(CWindowsClassesTypelibs)
MAKEREGENUM(CWindowsClassesAppIDs)
MAKEREGENUM(CWindowsClassesInterfaces)
MAKEREGENUM(CWindowsClassesCLSIDs)
MAKEREGENUM(CWindowsNetworkSignaturesUnmanaged)
MAKEREGENUM(CWindowsNetworkProfiles)
MAKEREGENUM(CWindowsNetworkNlaWireless)
MAKEREGENUM(CWindowsNetworkNlaCacheIntranet)
MAKEREGENUM(CWindowsNetworkConnections)
MAKEREGENUM(CWindowsNetworkDevices)
MAKEREGENUM(CWindowsServiceDrivers)
MAKEREGENUM(CWindowsComponentPackages)
MAKEREGENUM(CWindowsComponentPackageIndexes)
MAKEREGENUM(CWindowsComponentPackageSessions)
MAKEREGENUM(CWindowsSideBySideComponentWinners)
//RegLoadKey(HKEY_LOCAL_MACHINE, _T("COMPONENTS"), _T("%windir%\\system32\\config\\COMPONENTS"));
//RegUnLoadKey(HKEY_LOCAL_MACHINE, _T("COMPONENTS"));
MAKEREGENUM(CWindowsComponents)
MAKEREGENUM(CWindowsComponentFamilies)
MAKEREGENUM(CWindowsInstallerProducts)
MAKEREGENUM(CWindowsInstallerFolders)
MAKEREGENUM(CWindowsInstallerProductPatches)
MAKEREGENUM(CWindowsInstallerPatches)
MAKEREGENUM(CWindowsInstallerComponents)
MAKEREGENUM(CEnumEntries)
MAKEREGENUM(CHardwareClasses)
MAKEREGENUM(CHardwareClassList)
MAKEREGENUM(CCriticalDevices)
MAKEREGENUM(CMountedDevices)
MAKEREGENUM(CDeviceClasses)
MAKEREGENUM(CServiceOrderGroups)

RegEnumArgs CWindowsSharedDLLEntries::Args = {
	HKEY_LOCAL_MACHINE, NULL, 0, {
		{ NULL, REG_KEYVALUES, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs") },
		{ NULL, REG_PRIORVALUES },
	} };

RegEnumArgs CWindowsAppPathsEntries::Args = {
	HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths"), 0, {
		{ NULL, REG_PATH },
		{ _T(""), REG_SZ },
		{ _T("Path"), REG_SZ },
		{ _T("SaveURL"), REG_SZ },
		{ _T("UseURL"), REG_SZ },
		{ _T("BlockOnTSNonInstallMode"), REG_DWORD },
	} };

//TEXT("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
RegEnumArgs CWindowsUninstallEntries::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_UNINSTALL, 0, {
		{ NULL, REG_PATH },
		{ _T("AuthorizedCDFPrefix"), REG_SZ },
		{ _T("Comments"), REG_SZ },
		{ _T("Contact"), REG_SZ },
		{ _T("DisplayName"), REG_SZ },
		{ _T("DisplayVersion"), REG_SZ },
		{ _T("EstimatedSize"), REG_DWORD },
		{ _T("HelpLink"), REG_SZ },
		{ _T("HelpTelephone"), REG_SZ },
		{ _T("InstallDate"), REG_SZ },
		{ _T("InstallLocation"), REG_SZ },
		{ _T("InstallSource"), REG_SZ },
		{ _T("Language"), REG_DWORD },
		{ _T("ModifyPath"), REG_EXPAND_SZ },
		{ _T("NoModify"), REG_DWORD },
		{ _T("NoRepair"), REG_DWORD },
		{ _T("Publisher"), REG_SZ },
		{ _T("Readme"), REG_EXPAND_SZ },
		{ _T("Size"), REG_SZ },
		{ _T("UninstallString"), REG_EXPAND_SZ },
		{ _T("URLInfoAbout"), REG_SZ },
		{ _T("URLUpdateInfo"), REG_SZ },
		{ _T("Version"), REG_DWORD },
		{ _T("VersionMajor"), REG_DWORD },
		{ _T("VersionMinor"), REG_DWORD },
		{ _T("WindowsInstaller"), REG_DWORD },
	} };

RegEnumArgs CWindowsInstallerAssemblies::Args = {
	HKEY_CLASSES_ROOT, _T("Installer\\Assemblies"), 0, {
		{ NULL, REG_PATH },
		{ NULL, REG_KEYVALUES },
	} };

RegEnumArgs CWindowsInstallerWin32Assemblies::Args = {
	HKEY_CLASSES_ROOT, _T("Installer\\Win32Assemblies"), 0, {
		{ NULL, REG_PATH },
		{ NULL, REG_KEYVALUES },
	} };
//Components
//Dependencies
//Features
//Patches
//Products
//UpgradeCodes

//HKCR\AppID + Wow6432Node
//HKCR\CLSID + Wow6432Node
//HKCR\Interface + Wow6432Node
//HKCR\TypeLib + Wow6432Node
//HKCR

RegEnumArgs CWindowsClassesTypelibEntries::Args = {
	HKEY_CLASSES_ROOT, _T("TypeLib"), 0, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH },
		{ _T(""), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, _T("HELPDIR") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("FLAGS") },
		{ NULL, REG_DESCENTPREV | REG_KEYCOUNT, NULL, _T("HELPDIR") }, //_T("FLAGS")
	} };

RegEnumArgs CWindowsClassesTypelibs::Args = {
	HKEY_CLASSES_ROOT, _T("TypeLib"), 3, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH, NULL, _T("HELPDIR") }, //_T("FLAGS")
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH },
		{ _T(""), REG_DESCENTNEXT | REG_SZ },
	} };

RegEnumArgs CWindowsClassesAppIDs::Args = {
	HKEY_CLASSES_ROOT, _T("AppID"), 0, {
		{ NULL, REG_PATH },
		{ _T(""), REG_DESCENTNEXT | REG_SZ },
		{ _T("AppID"), REG_DESCENTNEXT | REG_SZ },
		{ _T("ActivateAtStorage"), REG_DESCENTNEXT | REG_SZ },
		{ _T("DllSurrogate"), REG_DESCENTNEXT | REG_SZ },
		{ _T("DllSurrogateExecutable"), REG_DESCENTNEXT | REG_SZ },
		{ _T("LocalService"), REG_DESCENTNEXT | REG_SZ },
		{ _T("ServiceParameters"), REG_DESCENTNEXT | REG_SZ },
		{ _T("RemoteServerName"), REG_DESCENTNEXT | REG_SZ },
		{ _T("RunAs"), REG_DESCENTNEXT | REG_SZ },
		{ _T("EndPoints"), REG_DESCENTNEXT | REG_MULTI_SZ },
		{ _T("AccessPermission"), REG_DESCENTNEXT | REG_BINARY },
		{ _T("LaunchPermission"), REG_DESCENTNEXT | REG_BINARY },
		{ _T("AppIDFlags"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("AuthenticationLevel"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("LoadUserSettings"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("PreferredServerBitness"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("ROTFlags"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("SRPTrustLevel"), REG_DESCENTNEXT | REG_DWORD },
	} };

RegEnumArgs CWindowsClassesInterfaces::Args = {
	HKEY_CLASSES_ROOT, _T("Interface"), 0, {
		{ NULL, REG_PATH },
		{ _T(""), REG_DESCENTNEXT | REG_SZ },
		{ _T(""), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, _T("BaseInterface") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("NumMethods") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("ProxyStubClsid") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("ProxyStubClsid32") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("TypeLib") },
		{ _T("Version"), REG_DESCENTNEXT | REG_SZ },
	} };

RegEnumArgs CWindowsClassesCLSIDs::Args = {
	HKEY_CLASSES_ROOT, _T("CLSID"), 0, {
		{ NULL, REG_PATH },
		{ _T(""), REG_DESCENTNEXT | REG_SZ },
		{ _T("AppID"), REG_DESCENTNEXT | REG_SZ },
		{ _T(""), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, _T("InProcServer32") },
		{ _T("ThreadingModel"), REG_DESCENTNEXT | REG_SZ },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("LocalServer32") },
		{ _T("ServerExecutable"), REG_DESCENTNEXT | REG_SZ },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("ProgID") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("VersionIndependentProgID") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV | REG_KEYVALUES, _T("MiscStatus") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("ToolBoxBitmap32") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("InProcServer") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("LocalServer") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("InProcHandler32") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("InProcHandler") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("AutoConvertTo") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("AutoTreatAs") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("TreatAs") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("DefaultIcon") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("Version") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV | REG_KEYCOUNT, _T("Control") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV | REG_KEYCOUNT, _T("Insertable") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV | REG_KEYS, _T("Verb") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV, _T("AuxUserType") },
		{ _T(""), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, _T("2") },
		{ _T(""), REG_DESCENT | REG_DESCENTPREV | REG_SZ, _T("3") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV, NULL },
		{ NULL, REG_DESCENT | REG_DESCENTPREV, _T("Conversion") },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT, _T("Readable") },
		{ _T(""), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, _T("Main") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV, _T("ReadWritable") },
		{ _T(""), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, _T("Main") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV, NULL },
		{ NULL, REG_DESCENT | REG_DESCENTPREV, NULL },
		{ NULL, REG_DESCENT | REG_DESCENTPREV, _T("DataFormats") },
		{ _T(""), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, _T("DefaultFile") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV | REG_KEYS, _T("GetSet") },
		{ NULL, REG_DESCENT | REG_DESCENTPREV, NULL },
	} };

RegEnumArgs CWindowsNetworkSignaturesUnmanaged::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_NETWORK_SIGNATURES_UNMANAGED, 0, {
		{ NULL, REG_PATH },
		{ _T("DefaultGatewayMac"), REG_BINARY },
		{ _T("Description"), REG_SZ },
		{ _T("DnsSuffix"), REG_SZ },
		{ _T("FirstNetwork"), REG_SZ },
		{ _T("ProfileGuid"), REG_SZ },
		{ _T("Source"), REG_DWORD },
	} };

RegEnumArgs CWindowsNetworkProfiles::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_NETWORK_PROFILES, 0, {
		{ NULL, REG_PATH },
		{ _T("Category"), REG_DWORD },
		{ _T("CategoryType"), REG_DWORD },
		{ _T("DateCreated"), REG_BINARY },
		{ _T("DateLastConnected"), REG_BINARY },
		{ _T("Description"), REG_SZ },
		{ _T("IconType"), REG_DWORD },
		{ _T("Managed"), REG_DWORD },
		{ _T("NameType"), REG_DWORD },
		{ _T("ProfileName"), REG_SZ },
	} };

RegEnumArgs CWindowsNetworkNlaWireless::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_NETWORK_NLA_WIRELESS, 0, {
		{ NULL, REG_PATH },
		{ NULL, REG_KEYVALUES },
	} };

RegEnumArgs CWindowsNetworkNlaCacheIntranet::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_NETWORK_NLA_CACHE_INTRANET, 0, {
		{ NULL, REG_PATH },
		{ _T("Failures"), REG_DWORD },
		{ _T("Successes"), REG_DWORD },
		{ NULL, REG_KEYVALUES },
	} };

RegEnumArgs CWindowsNetworkConnections::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_CURRENT_CONTROL_SET TEXT("\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"), 0, {
		{ NULL, REG_PATH, NULL, _T("Descriptions") },
		{ _T("Name"), REG_DESCENTNEXT | REG_DESCENT | REG_SZ, _T("Connection") },
		{ _T("MediaSubType"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("PnpInstanceID"), REG_DESCENTNEXT | REG_SZ },
		{ _T("DefaultNameIndex"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("DefaultNameResourceId"), REG_DESCENTNEXT | REG_DWORD },
	} };

RegEnumArgs CWindowsNetworkDevices::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_CURRENT_CONTROL_SET TEXT("\\Network"), 1, {
		{ NULL, REG_PATH, NULL, _T("{4D36E972-E325-11CE-BFC1-08002BE10318}") },
		{ NULL, REG_DESCENTNEXT | REG_DESCENT | REG_PATH },
		{ _T("Characteristics"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("ComponentId"), REG_DESCENTNEXT | REG_SZ },
		{ _T("Description"), REG_DESCENTNEXT | REG_SZ },
		{ _T("InfPath"), REG_DESCENTNEXT | REG_SZ },
		{ _T("InfSection"), REG_DESCENTNEXT | REG_SZ },
		{ _T("InstallTimeStamp"), REG_DESCENTNEXT | REG_BINARY },
		{ _T("LocDescription"), REG_DESCENTNEXT | REG_SZ },
		{ _T("ClsId"), REG_DESCENTNEXT | REG_DESCENT | REG_SZ, _T("Ndi") },
		{ _T("ComponentDll"), REG_DESCENTNEXT | REG_SZ },
		{ _T("CoServices"), REG_DESCENTNEXT | REG_SZ },
		{ _T("ExcludeSetupStartServices"), REG_DESCENTNEXT | REG_SZ },
		{ _T("FilterClass"), REG_DESCENTNEXT | REG_SZ },
		{ _T("FilterDeviceInfId"), REG_DESCENTNEXT | REG_SZ },
		{ _T("FilterRunType"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("FilterType"), REG_DESCENTNEXT | REG_DWORD },
		{ _T("HelpText"), REG_DESCENTNEXT | REG_SZ },
		{ _T("Service"), REG_DESCENTNEXT | REG_SZ },
		{ _T("TimeStamp"), REG_DESCENTNEXT | REG_BINARY },
		{ _T("FilterMediaTypes"), REG_DESCENTNEXT | REG_DESCENT | REG_SZ, _T("Interfaces") },
		{ _T("LowerRange"), REG_DESCENTNEXT | REG_SZ },
		{ _T("UpperRange"), REG_DESCENTNEXT | REG_SZ },
	} };
//Linkage and Parameters

RegEnumArgs CWindowsServiceDrivers::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_SERVICES, 0, {
		{ NULL, REG_PATH },
		{ _T("BootFlags"), REG_DWORD },
		{ _T("DependOnService"), REG_MULTI_SZ },
		{ _T("Description"), REG_SZ },
		{ _T("DisplayName"), REG_SZ },
		{ _T("DriverPackageId"), REG_SZ },
		{ _T("ErrorControl"), REG_DWORD },
		{ _T("Group"), REG_SZ },
		{ _T("ImagePath"), REG_SZ }, //REG_EXPAND_SZ
		{ _T("ObjectName"), REG_SZ },
		{ _T("RequiredPrivileges"), REG_MULTI_SZ },
		{ _T("ServiceSidType"), REG_DWORD },
		{ _T("Start"), REG_DWORD },
		{ _T("Tag"), REG_DWORD },
		{ _T("Type"), REG_DWORD },
		{ _T("WOW64"), REG_DWORD },
	} };

RegEnumArgs CWindowsComponentPackages::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_COMPONENT_PACKAGES, 0, {
		{ NULL, REG_PATH },
		{ _T("InstallClient"), REG_SZ },
		{ _T("InstallLocation"), REG_SZ },
		{ _T("InstallName"), REG_SZ },
		{ _T("InstallUser"), REG_SZ },
		{ _T("CurrentState"), REG_DWORD },
		{ _T("InstallTimeHigh"), REG_DWORD },
		{ _T("InstallTimeLow"), REG_DWORD },
		{ _T("LastError"), REG_DWORD },
		{ _T("LastProgressState"), REG_DWORD },
		{ _T("SelfUpdate"), REG_DWORD },
		{ _T("Trusted"), REG_DWORD },
		{ _T("Visibility"), REG_DWORD },
		{ NULL, REG_DESCENT | REG_VALUES, _T("Owners") },
	} };

RegEnumArgs CWindowsComponentPackageIndexes::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_COMPONENT_PACKAGEINDEX, 0, {
		{ NULL, REG_PATH },
		{ NULL, REG_VALUES },
	} };

RegEnumArgs CWindowsComponentPackageSessions::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_COMPONENT_SESSIONS, 0, {
		{ NULL, REG_PATH },
		{ _T("Complete"), REG_DWORD },
	} };

RegEnumArgs CWindowsSideBySideComponentWinners::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_SIDEBYSIDE_COMPONENT_WINNERS, 2, {
		{ NULL, REG_PATH },
		{ _T(""), REG_SZ, NULL },
		{ NULL, REG_DESCENTNEXT | REG_DESCENT | REG_PATH, NULL },
		{ NULL, REG_DESCENTNEXT | REG_KEYVALUES, NULL },
	} };

RegEnumArgs CWindowsComponents::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_COMPONENTS, 0, {
		{ NULL, REG_PATH },
		{ _T("ClosureFlags"), REG_DWORD, NULL },
		{ _T("identity"), REG_BINARY, NULL },
		{ _T("S256H"), REG_BINARY, NULL },
	} };

RegEnumArgs CWindowsComponentFamilies::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_VERSIONEDINDEX, 2, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT, _T("ComponentFamilies") },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH, NULL },
		{ _T("SomeUnparsedVersionsExist"), REG_DESCENTNEXT | REG_BINARY, NULL },
		{ NULL, REG_DESCENTNEXT | REG_KEYS, NULL },
	} };

RegEnumArgs CWindowsInstallerProducts::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_INSTALLER_PRODUCTS, 2, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT, _T("Products") },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH, NULL },
		{ _T("DisplayName"), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, _T("InstallProperties") },
		{ _T("DisplayVersion"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("InstallDate"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("InstallLocation"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("InstallSource"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("LocalPackage"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("ModifyPath"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("Publisher"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("UninstallString"), REG_DESCENTNEXT | REG_SZ, NULL },
	} };

RegEnumArgs CWindowsInstallerFolders::Args = {
	HKEY_LOCAL_MACHINE, NULL, 0, {
	{ NULL, REG_KEYVALUES, REGSTR_WINDOWS_INSTALLER_FOLDERS },
	{ NULL, REG_PRIORTYPES },
	{ NULL, REG_PRIORVALUES },
	} };

RegEnumArgs CWindowsInstallerProductPatches::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_INSTALLER_PRODUCTS, 4, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT, _T("Products") },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH, NULL },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT, _T("Patches") },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH, NULL },
		{ _T("DisplayName"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("Installed"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("LUAEnabled"), REG_DESCENTNEXT | REG_DWORD, NULL },
		{ _T("MoreInfoURL"), REG_DESCENTNEXT | REG_SZ, NULL },
		{ _T("MSI3"), REG_DESCENTNEXT | REG_DWORD, NULL },
		{ _T("PatchType"), REG_DESCENTNEXT | REG_DWORD, NULL },
		{ _T("State"), REG_DESCENTNEXT | REG_DWORD, NULL },
		{ _T("Uninstallable"), REG_DESCENTNEXT | REG_DWORD, NULL },
	} };

RegEnumArgs CWindowsInstallerPatches::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_INSTALLER_PRODUCTS, 2, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT, _T("Patches") },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH, NULL },
		{ _T("LocalPackage"), REG_DESCENTNEXT | REG_SZ, NULL },
	} };

RegEnumArgs CWindowsInstallerComponents::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_INSTALLER_PRODUCTS, 2, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT, _T("Components") },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH, NULL },
		{ NULL, REG_DESCENTNEXT | REG_KEYVALUES },
		{ NULL, REG_DESCENTNEXT | REG_KEYS },
	} };

RegEnumArgs CEnumEntries::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_SYSTEMENUM, 2, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH },
		{ REGSTR_VAL_CLASS, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_CLASSGUID, REG_DESCENTNEXT | REG_SZ },
		{ _T("ContainerID"), REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_DEVDESC, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_DRIVER, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_FRIENDLYNAME, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_MFG, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_SERVICE, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_COMPATIBLEIDS, REG_DESCENTNEXT | REG_MULTI_SZ },
		{ REGSTR_VAL_HARDWAREID, REG_DESCENTNEXT | REG_MULTI_SZ },
		{ REGSTR_VAL_CONFIGFLAGS, REG_DESCENTNEXT | REG_DWORD },
		{ REGSTR_VAL_CAPABILITIES, REG_DESCENTNEXT | REG_DWORD },
	} };

RegEnumArgs CHardwareClassList::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_CLASS_NT, 1, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT | REG_DESCENTNEXT | REG_PATH, NULL, REGSTR_KEY_DEVICE_PROPERTIES },
		{ REGSTR_VAL_COINSTALLERS_32, REG_DESCENTNEXT | REG_MULTI_SZ },
		{ REGSTR_VAL_DRIVERDATE, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_DRVDESC, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_DRIVERVERSION, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_ENUMPROPPAGES_32, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_INFPATH, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_INFSECTION, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_MATCHINGDEVID, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_PROVIDER_NAME, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_RESOURCE_PICKER_TAGS, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_DRIVERDATEDATA, REG_DESCENTNEXT | REG_BINARY },
		{ _T("Migrated"), REG_DESCENTNEXT | REG_DWORD },
	} };

RegEnumArgs CCriticalDevices::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_CRITICALDEVICEDATABASE, 0, {
		{ NULL, REG_PATH },
		{ REGSTR_VAL_CLASSGUID, REG_DESCENTNEXT | REG_SZ },
		{ _T("DriverPackageId"), REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_SERVICE, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_LOWERFILTERS, REG_DESCENTNEXT | REG_MULTI_SZ },
		{ REGSTR_VAL_UPPERFILTERS, REG_DESCENTNEXT | REG_MULTI_SZ },
	} };

RegEnumArgs CMountedDevices::Args = {
	HKEY_LOCAL_MACHINE, NULL, 0, {
		{ NULL, REG_KEYVALUES, REGSTR_PATH_MOUNTEDDEVICES },
		{ NULL, REG_PRIORVALUES },
	} };

RegEnumArgs CHardwareClasses::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_CLASS_NT, 0, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_CLASS, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_ENUMPROPPAGES_32, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_INSICON, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_INSTALLER_32, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_LOWERFILTERS, REG_DESCENTNEXT | REG_MULTI_SZ },
		{ REGSTR_VAL_NODISPLAYCLASS, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_NOINSTALLCLASS, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_NOUSECLASS, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_SILENTINSTALL, REG_DESCENTNEXT | REG_SZ },
		{ _T("TroubleShooter-0"), REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_UPPERFILTERS, REG_DESCENTNEXT | REG_MULTI_SZ },
		{ REGSTR_VAL_CLASSDESC, REG_DESCENTNEXT | REG_SZ },
		{ _T("Default Service"), REG_DESCENTNEXT | REG_SZ },
		{ _T("LegacyInfOption"), REG_DESCENTNEXT | REG_SZ },
		{ _T("WmiConfigClasses"), REG_DESCENTNEXT | REG_SZ },
		{ _T("IconPath"), REG_DESCENTNEXT | REG_MULTI_SZ },
		{ _T("LegacyAdapterDetection"), REG_DESCENTNEXT | REG_DWORD },
		{ NULL, REG_DESCENTNEXT | REG_KEYCOUNT, NULL, REGSTR_KEY_DEVICE_PROPERTIES },
	} };

RegEnumArgs CDeviceClasses::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_DEVICE_CLASSES, 4, {
		{ NULL, REG_PATH },
		{ NULL, REG_DESCENT | REG_PATH },
		{ REGSTR_VAL_DEVICE_INSTANCE, REG_DESCENTNEXT | REG_SZ },
		{ _T("ReferenceCount"), REG_DESCENT | REG_DESCENTNEXT | REG_DWORD, REGSTR_KEY_CONTROL },
		{ NULL, REG_DESCENT | REG_DESCENTPREV | REG_PATH, NULL, REGSTR_KEY_CONTROL },
		{ REGSTR_VAL_SYMBOLIC_LINK, REG_DESCENTNEXT | REG_SZ },
		{ REGSTR_VAL_LINKED, REG_DESCENTNEXT | REG_DWORD },
		{ _T("CLSID"), REG_DESCENT | REG_DESCENTNEXT | REG_SZ, REGSTR_KEY_DEVICEPARAMETERS },
		{ _T("FilterData"), REG_DESCENTNEXT | REG_BINARY },
		{ REGSTR_VAL_FRIENDLYNAME, REG_DESCENTNEXT | REG_SZ },
	} };

RegEnumArgs CServiceOrderGroups::Args = {
	HKEY_LOCAL_MACHINE, NULL, 0, {
		{ _T("List"), REG_MULTI_SZ, REG_STR_SERVICEORDERLIST },
	} };


/*RegEnumArgs CGlobalAutoruns::Args = {
	HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN, {
		{ NULL, REG_KEYVALUES },
		{ NULL, REG_PRIORKEYS }
	},
	HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, {
		{ NULL, REG_KEYVALUES },
		{ NULL, REG_PRIORKEYS }
	},
	HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCEEX, {
		{ NULL, REG_KEYVALUES },
		{ NULL, REG_PRIORKEYS }
	}
#ifdef WIN64
	,
	HKEY_LOCAL_MACHINE, REGSTR_PATH_WOW64_RUN, {
		{ NULL, REG_KEYVALUES },
		{ NULL, REG_PRIORKEYS }
	},
	HKEY_LOCAL_MACHINE, REGSTR_PATH_WOW64_RUNONCE, {
		{ NULL, REG_KEYVALUES },
		{ NULL, REG_PRIORKEYS }
	},
	HKEY_LOCAL_MACHINE, REGSTR_PATH_WOW64_RUNONCEEX, {
		{ NULL, REG_KEYVALUES },
		{ NULL, REG_PRIORKEYS }
	}
#endif
};*/

class CVolumeGatherer : public CGatherer
{
	DECLARE_DYNCREATE(CVolumeGatherer)
public:
	void OnEnumerate(CEnumModel *pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		CString String;
		HANDLE hFind;
		hFind = FindFirstVolume(String.GetBuffer(MAX_PATH), MAX_PATH);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				CCopyStringArray DeviceNames;
				CCopyStringArray VolumePathNames;
				DWORD dwVolumeSerialNumber;
				DWORD dwMaximumComponentLength;
				DWORD dwFileSystemFlags;
				CString FileSystemName;
				CCopyDWordArray NewValues;
				String.ReleaseBuffer();
				SetItemData(0, (void*)String.GetString(), NewValues);
				dwVolumeSerialNumber = 0;
				dwMaximumComponentLength = 0;
				dwFileSystemFlags = 0;
				GetVolumeInformation(String, NULL, 0, &dwVolumeSerialNumber, &dwMaximumComponentLength, &dwFileSystemFlags, FileSystemName.GetBuffer(MAX_PATH + 1), MAX_PATH + 1);
				FileSystemName.ReleaseBuffer();
				PRU_GetVolumePathNamesForVolumeName(String, VolumePathNames);
				RemoveTrailingBackslash(String);
				PRU_QueryDosDevice((String[0] == '\\' && String[1] == '\\' && String[2] == '?' && String[3] == '\\') ? String.Mid(4) : String, DeviceNames);
				SetItemData(1, (void*)&DeviceNames, NewValues);
				SetItemData(2, (void*)&VolumePathNames, NewValues);
				SetItemData(3, (void*)&dwVolumeSerialNumber, NewValues);
				SetItemData(4, (void*)&dwMaximumComponentLength, NewValues);
				SetItemData(5, (void*)&dwFileSystemFlags, NewValues);
				SetItemData(6, (void*)FileSystemName.GetString(), NewValues);
				AddItem(pModel, GetRuntimeClass(), NewValues);
				if (!FindNextVolume(hFind, String.GetBuffer(MAX_PATH), MAX_PATH)) {
					if (GetLastError() != ERROR_NO_MORE_FILES) {}
					break;
				}
			} while (TRUE);
			FindVolumeClose(hFind);
		}
	}
	MAKEGATHERER(CVolumeGatherer)
};

eType CVolumeGatherer::m_aTypes[] = { eString, eMultiString, eMultiString, eDWord, eDWord, eDWord, eString, eEnd };
CRuntimeClass* CVolumeGatherer::m_apDependencies[] = { NULL };

class CDriveGatherer : public CGatherer
{
	DECLARE_DYNCREATE(CDriveGatherer)
public:
	void OnEnumerate(CEnumModel *pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		CString String;
		CStringArray Strings;
		DWORD dwStrSize;
		DWORD dwSize = 2048;
		DWORD dwCount;
		do {
			dwStrSize = dwSize;
			dwSize = GetLogicalDriveStrings(dwStrSize, String.GetBuffer(dwStrSize));
		} while (dwSize && dwSize > dwStrSize);
		String.ReleaseBuffer(dwSize);
		StringArrayFromLPTSTRS((TCHAR*)(LPCTSTR)String, Strings);
		for (dwCount = 0; dwCount < (DWORD)Strings.GetCount(); dwCount++) {
			CCopyStringArray DeviceNames;
			UINT uiDriveType;
			DWORD dwSectorsPerCluster;
			DWORD dwBytesPerSector;
			DWORD dwNumberOfFreeClusters;
			DWORD dwTotalNumberOfClusters;
			CCopyDWordArray NewValues;
			SetItemData(0, (void*)Strings[dwCount].GetString(), NewValues);
			uiDriveType = GetDriveType(Strings[dwCount]);
			dwSectorsPerCluster = 0;
			dwBytesPerSector = 0;
			dwNumberOfFreeClusters = 0;
			dwTotalNumberOfClusters = 0;
			GetDiskFreeSpace(Strings[dwCount], &dwSectorsPerCluster,
								&dwBytesPerSector,
								&dwNumberOfFreeClusters,
								&dwTotalNumberOfClusters);
			RemoveTrailingBackslash(Strings[dwCount]);
			PRU_QueryDosDevice(Strings[dwCount], DeviceNames);
			SetItemData(1, (void*)&DeviceNames, NewValues);
			SetItemData(2, (void*)&uiDriveType, NewValues);
			SetItemData(3, (void*)&dwSectorsPerCluster, NewValues);
			SetItemData(4, (void*)&dwBytesPerSector, NewValues);
			SetItemData(5, (void*)&dwNumberOfFreeClusters, NewValues);
			SetItemData(6, (void*)&dwTotalNumberOfClusters, NewValues);
			AddItem(pModel, GetRuntimeClass(), NewValues);
		}
	}
	MAKEGATHERER(CDriveGatherer)
};

eType CDriveGatherer::m_aTypes[] = { eString, eMultiString, eDWord, eDWord, eDWord, eDWord, eDWord, eEnd };
CRuntimeClass* CDriveGatherer::m_apDependencies[] = { NULL };

class CFileFolderNavigator : public CGatherer
{
	DECLARE_DYNCREATE(CFileFolderNavigator)
public:
	void* GetItemData(void *pvKey, int iIndex)
	{
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		DWORD dwCount;
		CStringArray Strings;
		for (dwCount = 0; dwCount < (DWORD)pDepKeys->GetCount(); dwCount++) {			
			PRU_EnumDirectory((LPCTSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 0), TRUE, TRUE, FALSE, Strings);
		}
	}
};

class CFileFolderGatherer : public CGatherer
{
	DECLARE_DYNCREATE(CFileFolderGatherer)
public:
	void* GetItemData(void *pvKey, int iIndex)
	{
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		DWORD dwCount;
		for (dwCount = 0; dwCount < (DWORD)pDepKeys->GetCount(); dwCount++) {			
			CFileStatus Status;
			FileProps NewFileProps;
			NewFileProps.ullSize = 0;
			NewFileProps.bAttributes = 0;
			if (CFile::GetStatus((LPCTSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 0), Status)) {
				NewFileProps.FullName = Status.m_szFullName;
				NewFileProps.ullSize = Status.m_size;
				NewFileProps.ModifiedDate = Status.m_mtime;
				NewFileProps.CreationDate = Status.m_ctime;
				NewFileProps.ModifiedDate = Status.m_atime;
				NewFileProps.bAttributes = (BYTE)Status.m_attribute;
			}
			m_FileProps.Add(NewFileProps);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_FileProps.GetCount() - 1));
		}
	}
private:
	struct FileProps
	{
		CString FullName;
		ULONGLONG ullSize;
		CTime ModifiedDate;
		CTime CreationDate;
		CTime LastAccessDate;
		BYTE bAttributes;
	};
	CArray<FileProps, FileProps&> m_FileProps;
};

class CMD5FileHash : public CGatherer
{
	DECLARE_DYNCREATE(CFileHash)
public:
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return &m_MD5Hashes[(DWORD_PTR)pvKey];
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		DWORD dwCount;
		for (dwCount = 0; dwCount < (DWORD)pDepKeys->GetCount(); dwCount++) {			
			CFile File;
			if (File.Open((LPCTSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 0), CFile::modeRead)) {
				CCopyByteArray MD5;
				MD5.SetSize(16);
				PRU_ComputeFileMD5(File, MD5.GetData());
				File.Close();
				m_MD5Hashes.Add(MD5);
				pModel->OnItemNew(GetRuntimeClass(), (void*)(m_MD5Hashes.GetCount() - 1));
			}
		}
	}
private:
	CArray<CCopyByteArray, CCopyByteArray&> m_MD5Hashes;
};

class CKernelObjectDirectoryTree : public CGatherer
{
	DECLARE_DYNCREATE(CKernelObjectDirectoryTree)
public:
	CKernelObjectDirectoryTree() {}
	~CKernelObjectDirectoryTree() {}
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_ObjDirs.PLookup((DWORD_PTR&)pvKey)->value.Name.GetString();		
		default:
			return NULL;
		}
	}
	CString GetTreePath(CEnumModel *pModel, void* pvKey)
	{
		if (pModel->GetRootParam() == pvKey)
			return _T("\\");
		else
			return ((pModel->GetRootParam() != pModel->GetEntryParentParam(pvKey)) ?
					GetTreePath(pModel, pModel->GetEntryParentParam(pvKey)) :
					CString(_T(""))) +
				_T("\\") + (LPCTSTR)GetItemData(pvKey, 0);
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		QueryDirectoryTree(pModel, NULL, DIRROOTSTRW, ~0ULL);
	}
protected:
	void QueryDirectoryTree(CEnumModel *pModel, HANDLE hParent, LPCWSTR szName, DWORD_PTR dwParentIndex)
	{
		HANDLE hDirObj;
		BOOLEAN bFirst = TRUE;
		ULONG ulIndex = 0;
		DWORD_PTR dwIndex;
		CStringW Name;
		CStringW Type;
		DirInfo NewDirInfo;
		NewDirInfo.Name = szName;
		m_ObjDirs.SetAt(dwIndex = (DWORD)m_ObjDirs.GetCount(), NewDirInfo);
		if (dwParentIndex != (DWORD_PTR)~0ULL) m_ObjDirs.PLookup(dwParentIndex)->value.Children.Add((void*&)dwIndex);
		if (PRU_NtOpenDirectoryObject(hDirObj, szName, hParent, DIRECTORY_QUERY | DIRECTORY_TRAVERSE)) {
			while (PRU_NtQueryDirectoryObject(hDirObj, bFirst, ulIndex, Name, Type)) {
				if ((Type.GetLength() == wcslen(DIRTYPESTRW)) &&
					Type.CompareNoCase(DIRTYPESTRW) == 0)
					QueryDirectoryTree(pModel, hDirObj, Name, dwIndex);
				bFirst = FALSE;
			}
			PRU_CloseHandle(hDirObj);
		}
		pModel->OnItemNew(GetRuntimeClass(), (void*)dwIndex, (CArray<void*, void*&>*)&m_ObjDirs.PLookup(dwIndex)->value.Children);
	}
	struct DirInfo
	{
		CCopyArray<void*, void*&> Children;
		CString Name;
	};
	CMap<DWORD_PTR, DWORD_PTR&, DirInfo, DirInfo&> m_ObjDirs;
};

/*class CObjectTypeListTree : public CEnumModel
{
public:
	DECLARE_DYNCREATE(CObjectTypeListTree)
	CObjectTypeListTree() : m_dwFileTypeIndex(~0UL), m_dwNewFileTypeIndex(~0UL)
	{
	}
	~CObjectTypeListTree()
	{
	}
	void* GetItemData(	DWORD & dwElementSize, BOOL & bDereference,
						size_t & sOffset, int iColIndex)
	{
		bDereference = FALSE;
		if (iColIndex == 0) {
			sOffset = (size_t)&((KernelObjectName*)0)->Name;
			dwElementSize = sizeof(CStringW);
			return	(m_KernelObjectNames.GetData() == NULL) ?
					(void*)-1 : m_KernelObjectNames.GetData();
		} else {
			sOffset = 0;
			dwElementSize = 0;
			return NULL;
		}
	}
	LPCTSTR ItemLookup(LPARAM lParam)
	{
		return (LPCTSTR)(LPSTR)CW2A(((KernelObjectName*)lParam)->Name);
	}
	int ImageLookup(LPARAM *//*lParam*//*) { return 0; }
	int SelectedImageLookup(LPARAM *//*lParam*//*) { return 1; }
	BOOL OnUpdateDatabase()
	{
		INT_PTR iCount;
		TreeViewChange* NextViewChange;
		HANDLE h;
		NTSTATUS st;
		//this is probably wrong so fix also move comment to modules.h:
		//calling RtlFreeUnicodeString causes heap corruption for some reason as
		//  it uses local stack in RtlInitUnicodeString for Buffer allocation
		//so do not call it as it is unneccessary
		//  the memory is automatically freed
		m_dwFileTypeIndex = ~0UL;
		if (!m_TreeViewChange) {
			m_TreeViewChange = new TreeViewChange;
			m_TreeViewChange->PtrChildren = new CPtrMap;
			m_TreeViewChange->lParam = (m_KernelObjectNames.GetData() ? 0 : -1);
			m_TreeViewChange->pvNewKey = 0;
			for (	iCount = 0;
					iCount < m_KernelObjectNames.GetCount(); iCount++) {
				NextViewChange = new TreeViewChange;
				NextViewChange->PtrChildren = NULL;
				NextViewChange->lParam = (LPARAM)m_KernelObjectNames[iCount];
				NextViewChange->pvNewKey = NULL;
				m_TreeViewChange->PtrChildren->SetAt(&m_KernelObjectNames[iCount], (void*&)NextViewChange);
			}
		}
		if (m_NTDLLAPI.GetRtlInitUnicodeString() &&
			m_NTDLLAPI.GetNtOpenDirectoryObject() &&
			m_NTDLLAPI.GetNtQueryDirectoryObject()) {
			if (PRU_NtOpenDirectoryObject(&h, L"\\ObjectTypes", NULL, DIRECTORY_QUERY)) {
				if (m_NTDLLAPI.GetNtQueryObject()) {
					DWORD dwSize = sizeof(POBJECT_ALL_TYPES_INFORMATION);
					POBJECT_ALL_TYPES_INFORMATION pObjectInfo = (POBJECT_ALL_TYPES_INFORMATION)_malloca(dwSize);
					st = m_NTDLLAPI.GetNtQueryObject()(h, ObjectAllTypesInformation, pObjectInfo, dwSize, &dwSize); 
					while (st == STATUS_INFO_LENGTH_MISMATCH) { 
						_freea(pObjectInfo);
						pObjectInfo = (POBJECT_ALL_TYPES_INFORMATION)_malloca(dwSize);
						st = m_NTDLLAPI.GetNtQueryObject()(h, ObjectAllTypesInformation, pObjectInfo, dwSize, &dwSize); 
					}
					if (st == STATUS_SUCCESS) {
						DWORD Counter;
						POBJECT_TYPE_INFORMATION TypePtr = &pObjectInfo->TypeInformation;
						m_dwNewFileTypeIndex = ~0UL;
						for (Counter = 0; Counter < pObjectInfo->NumberOfTypes; Counter++) {
							WCHAR* SafePath = (WCHAR*)_malloca(TypePtr->Name.Length + sizeof(WCHAR));
							CopyMemory(SafePath, TypePtr->Name.Buffer, TypePtr->Name.Length);
							*((WCHAR*)&(((CHAR*)SafePath)[TypePtr->Name.Length])) = (L"")[0];
							if (_wcsicmp(SafePath, L"File") == 0)
								m_dwNewFileTypeIndex = Counter + 1;
							KernelObjectName* ObjName = new KernelObjectName;
							ObjName->bIsTopLevel = FALSE;
							ObjName->Name = SafePath;
							m_NewKernelObjectNames.InsertAt(Counter, ObjName);
							if ((DWORD)m_KernelObjectNames.GetCount() <= Counter ||
								!m_TreeViewChange->PtrChildren->Lookup(
								&m_KernelObjectNames[Counter], (void*&)NextViewChange)) {
								NextViewChange = new TreeViewChange;
								NextViewChange->PtrChildren = NULL;
								NextViewChange->lParam = NULL;
								NextViewChange->pvNewKey = (LPARAM)m_NewKernelObjectNames[Counter];
								m_TreeViewChange->PtrChildren->SetAt(&m_NewKernelObjectNames[Counter], (void*&)NextViewChange);
							} else if (m_KernelObjectNames[Counter]->Name.
													Compare(SafePath) != 0) {
								NextViewChange->pvNewKey = (LPARAM)m_NewKernelObjectNames[Counter];
							} else {
								NextViewChange->pvNewKey = (LPARAM)m_KernelObjectNames[Counter];
							}
							TypePtr = (POBJECT_TYPE_INFORMATION)((char*)TypePtr + sizeof(OBJECT_TYPE_INFORMATION) + (wcslen(TypePtr->Name.Buffer) + 1) * sizeof(WCHAR));
							if ((*((DWORD*)&TypePtr) & 3) != 0) {
								TypePtr = (POBJECT_TYPE_INFORMATION)((char*)TypePtr + (4 - (*((DWORD*)&TypePtr) & 3)));
							}
							_freea(SafePath);
						}
					} else {
						AddTraceLog(NULL, _T("APICall=NtQueryObject Info=ObjectAllTypesInformation ObjectTypesDirectory=%08X Error=%08X\r\n"), h, st);
					}
					_freea(pObjectInfo);
				}
				PRU_CloseHandle(h);
			}
		}
		return TRUE;
	}
	void OnUpdateUI()
	{
		CEnumModel::OnUpdateUI();
		m_KernelObjectNames.Copy(m_NewKernelObjectNames);
		m_NewKernelObjectNames.RemoveAll();
		m_dwFileTypeIndex = m_dwNewFileTypeIndex;
		m_dwNewFileTypeIndex = ~0UL;
	}
	DWORD GetFileTypeIndex() { return m_dwFileTypeIndex; }
	CStringW GetKernelObjectName(DWORD dwIndex)
	{
		if (dwIndex >= 0 && dwIndex < (DWORD)m_KernelObjectNames.GetCount()) {
			return m_KernelObjectNames[dwIndex]->Name; 
		} else {
			return CStringW();
		}
	}
protected:
	struct KernelObjectName
	{
		BOOL bIsTopLevel;
		CStringW Name;
	};
	//must provide hash table lookup!! this is bad!!
	DWORD m_dwFileTypeIndex;
	DWORD m_dwNewFileTypeIndex;

	CArray<KernelObjectName*> m_KernelObjectNames;
	CArray<KernelObjectName*> m_NewKernelObjectNames;
	CNTDLLAPI m_NTDLLAPI;
};

class CProcessList : public CTreeModel<DWORD, DWORD>
{
public:
	//adjust more literal 0 entries to ROOT_PID...
#define ROOT_PID 0
	DECLARE_DYNCREATE(CProcessList)
	//PARENT PID ONLY TRUSTED IF PROCESS START TIME BEFORE!!!!!!!!!
	CProcessList()
	{
		m_pspi = NULL;
		m_dwpspiSize = 0;
	}
	~CProcessList()
	{
		Cleanup();
	}
	void Cleanup()
	{
		POSITION pos;
		WORD key;
		ProcessEntry* Entry;
		pos = m_ProcessEntries.GetStartPosition();
		while (pos) {
			m_ProcessEntries.GetNextAssoc(pos, key, (void*&)Entry);
			DestroyIcon(Entry->Entry.Entry.hIcon);
			delete Entry;
		}
		m_ProcessEntries.RemoveAll();
		if (m_pspi) {
			delete [] m_pspi;
			m_pspi = NULL;
		}
	}
	static BOOL CALLBACK WindowEnumFunc(HWND hwnd, LPARAM lParam)
	{
		CString String;
		CWnd* Wnd;
		DWORD dwPid;
		if (((EnumStruct*)lParam)->hwnd == 0) {
			GetWindowThreadProcessId(hwnd, &dwPid);
			if (dwPid == ((EnumStruct*)lParam)->PID) {
				Wnd = CWnd::FromHandle(hwnd);
				if ((Wnd->GetParent() ?
					 !Wnd->GetParent()->IsWindowVisible() :
					 TRUE) && Wnd->IsWindowVisible()) {
					Wnd->GetWindowText(*((EnumStruct*)lParam)->String);
					if (!((EnumStruct*)lParam)->String->IsEmpty())
						((EnumStruct*)lParam)->hwnd = hwnd;
				}
			}
		}
		return TRUE;
	}
	static BOOL SuspendResumeCaptionFunc(CString & String, LPARAM lParam, void* Ptr)
	{
		BOOL bRet;
		bRet = ((CProcessList*)Ptr)->IsProcessSuspended((WORD)lParam);
		String = bRet ? _T("Resu&me") : _T("&Suspend");
		
		return bRet;
	}
	virtual void OnColumnAction(int iColIndex, LPARAM lParam)
	{
		//ProcessEntry* Entry;
		// put into column information
		*//*if (m_ProcessEntries.Lookup(
				(WORD)GetItemData(lpnmitem->iItem),
				(void*&)Entry)) {
			if (Entry->ProcessThreadInformationOffset != -1) {
			// fix the wchar stuff
				MessageBoxW(NULL,
							((__PSYSTEM_PROCESS_INFORMATION)
								&((char*)m_pspi)[
								Entry->ProcessThreadInformationOffset])->
								Process.usName.Buffer,
							NULL, MB_OK);
			}
		}*//*
		switch (iColIndex) {
		case 0:
			KillProcess((WORD)lParam, FALSE);
			break;
		case 1:
			SuspendResumeProcess(
				(WORD)lParam, FALSE,
				!IsProcessSuspended(
					(WORD)lParam));
			break;
		case 2:
			//graceful shutdown
			break;
		case 3:
			//graceful restart
			break;
		case 4:
			//GetCommandLine information
			KillProcess((WORD)lParam, FALSE);
			//CreateProcess to restart it
			break;
		case 5:
			KillProcess((WORD)lParam, TRUE);
			break;
		case 6:
			SuspendResumeProcess((WORD)lParam, TRUE, TRUE);
			break;
		case 7:
			SuspendResumeProcess((WORD)lParam, TRUE, FALSE);
			break;
		case 8:
			//GetCommandLine information recursive...
			KillProcess((WORD)lParam, TRUE);
			//CreateProcess recursive special handling start with parent...
			break;
		}
	}
	void* GetItemData(DWORD & dwElementSize, BOOL & bDereference, size_t & sOffset, int iColIndex)
	{
		bDereference = FALSE;
		dwElementSize = 0;
		switch (iColIndex) {
		case 0:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->lppe.szExeFile;
			break;
		case 1:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->lppe.th32ProcessID;
			break;
		case 2:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->
													lppe.th32ParentProcessID;
			break;
		case 3:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->UserName;
			break;
		case 4:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->CommandLine;
			break;
		case 5:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->FileDescription;
			break;
		case 6:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->CompanyName;
			break;
		case 7:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->Version;
			break;
		case 8:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->DepStatus;
			break;
		case 9:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->WindowTitle;
			break;
		case 10:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->WindowStatus;
			break;
		case 11:
			sOffset = (size_t)&((CProcessList::ProcessEntry*)0)->SessionId;
			break;
		default:
			sOffset = 0;
			return NULL;
			break;
		}
		return &m_ProcessEntries;
	}
	DWORD GetDepth(LPARAM lParam)
	{
		ProcessEntry* Entry;
		WORD PID;
		DWORD dwCount = 0;
		PID = (WORD)lParam;
		while (PID && m_ProcessEntries.Lookup(PID, (void*&)Entry)) {
			dwCount++;
			PID = (WORD)Entry->lppe.th32ParentProcessID;
		}
		return dwCount;
	}
	TreeViewEntry* GetEntry(LPARAM lParam)
	{
		CString String;
		ProcessEntry* Entry = NULL;
		SHFILEINFO sfi;
		if (m_ProcessEntries.Lookup((WORD)lParam, (void*&)Entry)) {
			if (!Entry->Entry.Entry.hIcon) { // perhaps have a timeout and reload the icon
				String.Empty();
				GetProcessName(String, (DWORD)lParam);
				ZeroMemory(&sfi, sizeof(sfi));
				if (!String.IsEmpty()) {
					SHGetFileInfo(	String, 0, &sfi, sizeof(sfi),
									SHGFI_SMALLICON | SHGFI_ICON);
				}
				if ((Entry->Entry.Entry.hIcon = sfi.hIcon) == NULL) {
					Entry->Entry.Entry.hIcon = LoadIcon(NULL, IDI_APPLICATION);
				}
			}
		}
		return &Entry->Entry;
	}
	LPARAM GetEntryParam(TreeViewEntry* Entry) { return ((ProcessEntry*)Entry)->lppe.th32ProcessID; }
	LPARAM GetEntryParentParam(TreeViewEntry* Entry) { return ((ProcessEntry*)Entry)->lppe.th32ParentProcessID; }
	LPARAM GetRootParam() { return 0; }
	INT_PTR GetChildCount(TreeViewEntry* Entry) { return ((ProcessEntry*)Entry)->ChildProcessIds.GetCount(); }
	LPARAM GetChildParam(TreeViewEntry* Entry, INT_PTR iIndex) { return ((ProcessEntry*)Entry)->ChildProcessIds[iIndex]; }

	BOOL OnUpdateDatabase()
	{
		return TRUE;
	}
	void OnUpdateUI()
	{
		*//*DWORD cb;
		DWORD newcb;
		DWORD* pidArray = NULL;
		HANDLE hSnapshot;
		HANDLE hProcess;
		ProcessEntry* Entry;
		ProcessEntry* OldEntry;
		CArray<WORD> DeleteArray;
		POSITION pos;
		NTSTATUS ntReturn;
		LVFINDINFO fi;
		int item;
		WORD pid;
		fi.psz = NULL;
		fi.vkDirection = 0;
		fi.pt.x = 0;
		fi.pt.y = 0;
		cb = 4;
		newcb = 0;
		if (m_PSAPIAPI.GetEnumProcesses()) {
			do {
				cb = cb << 2;
				if (pidArray)
					delete [] pidArray;
				pidArray = new DWORD[cb >> 2];
				newcb = cb;
				if (m_PSAPIAPI.GetEnumProcesses()(pidArray, cb, &newcb) == 0) {
					AddTraceLog(NULL, _T("APICall=EnumProcesses Error=%08X\r\n"), GetLastError());
					newcb = 0;
					break;
				}
			} while (cb == newcb);
		}
		pos = m_ProcessEntries.GetStartPosition();
		while (pos) {
			m_ProcessEntries.GetNextAssoc(pos, pid, (void*&)Entry);
			Entry->Entry.bDirty = TRUE;
			Entry->ChildProcessIds.RemoveAll();
		}
		if (hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) {
			Entry = new ProcessEntry;
			Entry->Entry.hIcon = NULL;
			Entry->Entry.bExpanded = TRUE;
			Entry->lppe.dwSize = sizeof(PROCESSENTRY32);
			Entry->Entry.bDirty = FALSE;
			Entry->ProcessThreadInformationOffset = -1;
			if (Process32First(hSnapshot, &Entry->lppe)) {
				do {
					for (cb = 0; cb < (newcb >> 2); cb++) {
					-	if (pidArray[cb] == Entry->lppe.th32ProcessID) {
							break;
						}
					}
					if (m_ProcessEntries.Lookup((WORD)Entry->lppe.th32ProcessID, (void*&)OldEntry)) {
						memcpy(&OldEntry->lppe, &Entry->lppe, sizeof(PROCESSENTRY32));
						OldEntry->Entry.bDirty = FALSE;
					} else {
						m_ProcessEntries.SetAt((WORD)Entry->lppe.th32ProcessID, Entry);
						Entry = new ProcessEntry;
						Entry->Entry.hIcon = NULL;
						Entry->Entry.bExpanded = TRUE;
						Entry->lppe.dwSize = sizeof(PROCESSENTRY32);
						Entry->Entry.bDirty = FALSE;
						Entry->ProcessThreadInformationOffset = -1;
					}
					if (newcb && (cb == (newcb >> 2))) {
						AddTraceLog(NULL, _T("FailAPI=EnumProcesses SuccessAPI=Process32First/Process32Next PID=%04X Path=%hs hidden\r\n"), Entry->lppe.th32ProcessID, Entry->lppe.szExeFile);
					}
				} while (Process32Next(hSnapshot, &Entry->lppe));
				delete Entry;
				if (GetLastError() != ERROR_NO_MORE_FILES) {
					AddTraceLog(NULL, _T("APICall=Process32Next Snapshot=%08X Error=%08X\r\n"), hSnapshot, GetLastError());
				}
			} else {
				AddTraceLog(NULL, _T("APICall=Process32First Snapshot=%08X Error=%08X\r\n"), hSnapshot, GetLastError());
			}
			if (!CloseHandle(hSnapshot)) {
				AddTraceLog(NULL, _T("APICall=CloseHandle Snapshot=%08X Error=%08X\r\n"), hSnapshot, GetLastError());
			}
		} else {
			AddTraceLog(NULL, _T("APICall=CreateToolhelp32Snapshot List=SNAPPROCESS Error=%08X\r\n"), GetLastError());
		}
		pos = m_ProcessEntries.GetStartPosition();
		while (pos) {
			m_ProcessEntries.GetNextAssoc(pos, pid, (void*&)Entry);
			if (Entry->Entry.bDirty) {
				fi.lParam = pid;
				fi.flags = LVFI_PARAM;
				if ((item = ProcList->FindItem(&fi)) != -1)
					ProcList->DeleteItem(item);
				if ((item = ProcTreeList->FindItem(&fi)) != -1)
					ProcTreeList->DeleteItem(item);
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
				AddTraceLog(NULL, _T("FailAPI=Process32First/Process32Next SuccessAPI=EnumProcesses PID=%04X Path=%hs hidden\r\n"), OldEntry->lppe.th32ProcessID, OldEntry->lppe.szExeFile);
			}
			if (pidArray[cb] == OldEntry->lppe.th32ProcessID) {
				break;
			}
		}
		if (!m_pspi)
			m_pspi = (__PSYSTEM_PROCESS_INFORMATION)new BYTE[m_dwpspiSize = sizeof(SYSTEM_PROCESS_INFORMATION)];
		__PSYSTEM_PROCESS_INFORMATION pspienum;
		if (m_NTDLLAPI.GetNtQuerySystemInformation()) {
			ntReturn = m_NTDLLAPI.GetNtQuerySystemInformation()(SystemProcessesAndThreadsInformation, m_pspi, m_dwpspiSize, &m_dwpspiSize);
			while (ntReturn == STATUS_INFO_LENGTH_MISMATCH) {
				delete [] m_pspi;
				m_pspi = (__PSYSTEM_PROCESS_INFORMATION )new BYTE[m_dwpspiSize];
				ntReturn = m_NTDLLAPI.GetNtQuerySystemInformation()(SystemProcessesAndThreadsInformation, m_pspi, m_dwpspiSize, &m_dwpspiSize);
			}
			if (ntReturn != STATUS_SUCCESS) {
				AddTraceLog(NULL, _T("Call=NtQuerySystemInformation Type=ProcessThreadInformation Error=%08X\r\n"), ntReturn);
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
				LPCVOID ParamOffset = 0;
				DWORD read;
				WCHAR* wstr;
				RTL_USER_PROCESS_PARAMETERS params;
				DWORD size = sizeof(PROCESS_BASIC_INFORMATION);
				PBI = (PROCESS_BASIC_INFORMATION*)new BYTE[size];
				ntReturn = m_NTDLLAPI.GetNtQueryInformationProcess()(hProcess, ProcessBasiCEnumModel, PBI, size, &size);
				while (ntReturn == STATUS_INFO_LENGTH_MISMATCH) {
					delete [] PBI;
					PBI = (PROCESS_BASIC_INFORMATION*)new BYTE[size];
					ntReturn = m_NTDLLAPI.GetNtQueryInformationProcess()(hProcess, ProcessBasiCEnumModel, PBI, size, &size);
				}
				if (ntReturn != STATUS_SUCCESS) {
				} else {
					ReadProcessMemory(hProcess, (char*)PBI->PebBaseAddress + 0x10, &ParamOffset, sizeof(ParamOffset), &read);
					memset(&params, 0, sizeof(params));
					ReadProcessMemory(hProcess, (LPCVOID)ParamOffset, &params, sizeof(params), &read);
					wstr = new WCHAR[params.CommandLine.Length + 1];
					memset(wstr, 0, (params.CommandLine.Length + 1) * sizeof(WCHAR));
					ReadProcessMemory(hProcess, params.CommandLine.Buffer, wstr, params.CommandLine.Length, &read);
					Entry->CommandLine = CW2A(wstr);
					delete [] wstr;
					wstr = new WCHAR[params.ImagePathName.Length + 1];
					memset(wstr, 0, (params.ImagePathName.Length + 1) * sizeof(WCHAR));
					ReadProcessMemory(hProcess, params.ImagePathName.Buffer, wstr, params.ImagePathName.Length, &read);
					Entry->ImagePathName = CW2A(wstr);
					delete [] wstr;
					wstr = new WCHAR[params.WindowTitle.Length + 1];
					memset(wstr, 0, (params.WindowTitle.Length + 1) * sizeof(WCHAR));
					ReadProcessMemory(hProcess, params.WindowTitle.Buffer, wstr, params.WindowTitle.Length, &read);
					Entry->WindowTitle = CW2A(wstr);
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
			if (EnumWindows(WindowEnumFunc, (LPARAM)&NewEnumStruct) && NewEnumStruct.hwnd && m_HungAppWindowAPI.GetIsHungAppWindow()) {
				Entry->WindowStatus = m_HungAppWindowAPI.GetIsHungAppWindow()(NewEnumStruct.hwnd) ? 0 : 1;
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
						if (VerQueryValue(VerInfo, _T("\\VarFileInfo\\Translation"), (LPVOID*)&Buffer, &BufLen)) {
							RealBuf = VerInfo;
							while (*((DWORD*)RealBuf) != 0xFEEF04BD) {
								RealBuf++;
							}
							Entry->Version.Format(_T("%d.%02d.%04d.%04d"), *((WORD*)(RealBuf + 10)), *((WORD*)(RealBuf + 8)), *((WORD*)(RealBuf + 14)), *((WORD*)(RealBuf + 12)));
							QueryBuf.Format(_T("\\StringFileInfo\\%04X%04X\\%s"), *((USHORT*)Buffer), *((USHORT*)Buffer + 1), _T("FileDescription"));
							if (VerQueryValue(VerInfo, (LPTSTR)(LPCTSTR)QueryBuf, (LPVOID*)&RealBuf, &RealBufLen)) {
								Entry->FileDescription = RealBuf;
							} else {
								QueryBuf.Format(_T("\\StringFileInfo\\%04X%04X\\%s"), *((USHORT*)Buffer), 1252, _T("FileDescription"));
								if (VerQueryValue(VerInfo, (LPTSTR)(LPCTSTR)QueryBuf, (LPVOID*)&RealBuf, &RealBufLen)) {
									Entry->FileDescription = RealBuf;
								}
							}
							QueryBuf.Format(_T("\\StringFileInfo\\%04X%04X\\%s"), *((USHORT*)Buffer), *((USHORT*)Buffer + 1), _T("CompanyName"));
							if (VerQueryValue(VerInfo, (LPTSTR)(LPCTSTR)QueryBuf, (LPVOID*)&RealBuf, &RealBufLen)) {
								Entry->CompanyName = RealBuf;
							} else {
								QueryBuf.Format(_T("\\StringFileInfo\\%04X%04X\\%s"), *((USHORT*)Buffer), 1252, _T("CompanyName"));
								if (VerQueryValue(VerInfo, (LPTSTR)(LPCTSTR)QueryBuf, (LPVOID*)&RealBuf, &RealBufLen)) {
									Entry->CompanyName = RealBuf;
								}
							}
						}
					}
					delete [] VerInfo;
				}
			}
		}
		fi.lParam = ROOT_PID;
		fi.flags = LVFI_PARAM;
		m_ProcessEntries.Lookup(ROOT_PID, (void*&)Entry);
		if (ProcList->FindItem(&fi) == -1) {
			if (ProcList->InsertItem(	LVIF_TEXT | LVIF_PARAM, 
										ProcList->GetItemCount(),
										LPSTR_TEXTCALLBACK, 0, 0, 0,
										ROOT_PID) == -1) {
			}
		}
		if (ProcTreeList->FindItem(&fi) == -1) {
			if (ProcTreeList->InsertItem(	LVIF_TEXT | LVIF_PARAM, 
											ProcTreeList->GetItemCount(),
											LPSTR_TEXTCALLBACK, 0, 0, 0,
											ROOT_PID) == -1) {
			}
		}
		if (pidArray)
			delete [] pidArray;*//*
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
		if ((hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, PID)) != NULL) {
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
		} else {
			return FALSE;
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
		if (m_NTDLLAPI.GetNtSuspendProcess() &&
			m_NTDLLAPI.GetNtResumeProcess()) {
			if ((hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, PID)) != NULL) {
				(bSuspend ? m_NTDLLAPI.GetNtSuspendProcess() : m_NTDLLAPI.GetNtResumeProcess())(hProcess);
				CloseHandle(hProcess);
			}
		} else {
			//OpenThread/SuspendThread/ResumeThread
		}
	}
	BOOL HasChild(LPARAM lParam) { return HasChildPID((WORD)lParam); }
	BOOL HasChildPID(WORD PID)
	{
		ProcessEntry* Entry;
		if (m_ProcessEntries.Lookup(PID, (void*&)Entry)) {
			return (Entry->ChildProcessIds.GetCount() != 0);
		} else {
			return FALSE;
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
		TCHAR* Name;
		TCHAR* Domain;
		//TokenSessionId
		//TokenPrivileges
		if (hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, PID)) {
			if (DriverQuery((DWORD)IOCTL_FOI_OPENPROCESSTOKEN, &hProcess, sizeof(hProcess), &hToken, sizeof(hToken)) ||
				OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
				if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
					ptu = (TOKEN_USER*)new BYTE[dwSize];
					if (GetTokenInformation(hToken, TokenUser, ptu, dwSize, &dwSize)) {
						dwSize = 0;
						dwOtherSize = 0;
						if (!LookupAccountSid(NULL, ptu->User.Sid, NULL, &dwSize, NULL, &dwOtherSize, &use) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
							Name = new TCHAR[dwSize];
							Domain = new TCHAR[dwOtherSize];
							if (LookupAccountSid(NULL, ptu->User.Sid, Name, &dwSize, Domain, &dwOtherSize, &use)) {
								Str.Format(_T("%s\\%s"), Domain, Name);
							} else
								Str = _T("<Unknown Owner>");
							delete [] Name;
							delete [] Domain;
						} else
							Str = _T("<Unknown Owner>");
					} else
						Str = _T("<Unable to Query Owner>");
					delete [] ptu;
				}
				CloseHandle(hToken);
			}
			CloseHandle(hProcess);
		} else if (PID)
			Str = _T("<Access Denied>");
		else
			Str = _T("NT AUTHORITY\\SYSTEM");
		return Str;
	}
	void GetProcessName(CString & String, DWORD pid)
	{
		HANDLE hProcess;
		TCHAR szImageName[MAX_PATH << 4];
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);
		DWORD SystemID = (osvi.dwMajorVersion >= 5) ? ((osvi.dwMinorVersion == 0) ? 2 : 4) : 8;
		// PID 0 is System Idle Process and PID 2, 4 or 8 is System
		// should test correct for all Windows versions (95, 98, Me, NT = 8, 2000 = 2, XP, 2003 = 4)
		if ((hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)) != NULL || (hProcess == INVALID_HANDLE_VALUE)) {
			if (pid) { // PID 0 cannot be opened for query
				if (hProcess != INVALID_HANDLE_VALUE)
					AddTraceLog(NULL, _T("APICall=OpenProcess Rights=QUERY_INFORMATION|VM_READ PID=%04X Error=%08X\r\n"), pid, GetLastError());
				else
					AddTraceLog(NULL, _T("APICall=OpenProcess Rights=QUERY_INFORMATION|VM_READ PID=%04X Error=INVALID_HANDLE_VALUE\r\n"), pid);
			}
		}
		if (hProcess && (hProcess != INVALID_HANDLE_VALUE) && m_PSAPIAPI.GetGetModuleFileNameEx() && m_PSAPIAPI.GetGetModuleFileNameEx()(hProcess, NULL, szImageName, MAX_PATH << 4)) {
			String = szImageName;
		} else {
			if (hProcess && (hProcess != INVALID_HANDLE_VALUE) && !CloseHandle(hProcess)) {
				AddTraceLog(NULL, _T("APICall=CloseHandle Process=%08X Error=%08X PID=%04X\r\n"), hProcess, GetLastError(), pid);
			}
			if ((hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid)) != NULL && (hProcess != INVALID_HANDLE_VALUE)) {
				if (m_PSAPIAPI.GetGetProcessImageFileName() && m_PSAPIAPI.GetGetProcessImageFileName()(hProcess, szImageName, MAX_PATH << 4)) {
					String = szImageName;
					// convert from device path to physical path
				} else {
					if (m_PSAPIAPI.GetGetModuleFileNameEx() || m_PSAPIAPI.GetGetProcessImageFileName()) {
						if (pid && (pid != SystemID)) { // PID 0=System Idle Process and System and cannot be opened for query information
							AddTraceLog(NULL, _T("APICall=GetModuleFileNameEx|GetProcessImageFileName Process=%08X Error=%08X PID=%04X\r\n"), hProcess, GetLastError(), pid);
						}
					}
				}
			} else {
				if (pid && (pid != SystemID)) { // PID 0=System Idle Process and System and cannot be opened for query information
					if (hProcess != INVALID_HANDLE_VALUE)
						AddTraceLog(NULL, _T("APICall=OpenProcess Rights=QUERY_INFORMATION PID=%04X Error=%08X\r\n"), pid, GetLastError());
					else
						AddTraceLog(NULL, _T("APICall=OpenProcess Rights=QUERY_INFORMATION PID=%04X Error=INVALID_HANDLE_VALUE\r\n"), pid);
				}
			}
		}
		if (hProcess && (hProcess != INVALID_HANDLE_VALUE) && !CloseHandle(hProcess)) {
			AddTraceLog(NULL, _T("APICall=CloseHandle Process=%08X Error=%08X PID=%04X\r\n"), hProcess, GetLastError(), pid);
		}
	}
protected:
	struct EnumStruct {
		HWND hwnd;
		DWORD PID;
		CString* String;
	};
	struct ProcessEntry
	{
		TreeViewEntry Entry;
		DWORD ProcessThreadInformationOffset;
		PROCESSENTRY32 lppe;
		BYTE DepStatus;
		BYTE WindowStatus;
		DWORD SessionId;
		CArray<WORD> ChildProcessIds;
		CString CommandLine;
		CString ImagePathName;
		CString FileDescription;
		CString CompanyName;
		CString WindowTitle;
		CString Version;
		CString UserName;
	};
	DWORD m_dwpspiSize;
	__PSYSTEM_PROCESS_INFORMATION m_pspi;
	CMapWordToPtr m_ProcessEntries;
	CPSAPIAPI m_PSAPIAPI;
	CNTDLLAPI m_NTDLLAPI;
	CHungAppWindowAPI m_HungAppWindowAPI;
};*/

/*class CHandleList : public CEnumModel
{
public:
	DECLARE_DYNCREATE(CHandleList)
	CHandleList()
	{
		m_ObjInf.hEventStart = m_ObjInf.hEventDone = m_hObjThread = NULL;
		m_ProcessIdDatabase = new CMapWordToPtr(2048);
		//http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
		m_ProcessIdDatabase->InitHashTable(49157);
		m_FileTypeIndex = ~0UL;
	}
	~CHandleList()
	{
		POSITION posin;
		POSITION pos;
		WORD key;
		HandleEntry* val;
		CMapWordToPtr* map;
		Cleanup();
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
	}
	void Cleanup()
	{
		if (m_hObjThread && (m_hObjThread != INVALID_HANDLE_VALUE)) {
			if (!m_ObjInf.hEventStart || m_ObjInf.hEventStart == INVALID_HANDLE_VALUE) {
				if ((m_ObjInf.hEventStart = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
					AddTraceLog(NULL, _T("APICall=CreateEvent Use=Start Error=%08X\r\n"), GetLastError());
				}
			}
			if (!m_ObjInf.hEventDone || m_ObjInf.hEventDone == INVALID_HANDLE_VALUE) {
				if ((m_ObjInf.hEventDone = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
					AddTraceLog(NULL, _T("APICall=CreateEvent Use=Done Error=%08X\r\n"), GetLastError());
				}
			}
			m_ObjInf.hObject = INVALID_HANDLE_VALUE;
			if (!SetEvent(m_ObjInf.hEventStart)) {
				AddTraceLog(NULL, _T("APICall=SetEvent StartHandle=%08X Error=%08X\r\n"), m_ObjInf.hEventStart, GetLastError());
			}
			if (WaitForSingleObject(m_ObjInf.hEventDone, 1000) == WAIT_TIMEOUT) {
				AddTraceLog(NULL, _T("APICall=WaitForSingleObject DoneEvent=%08X Error=Timeout TerminatingThread=%08X\r\n"), m_ObjInf.hEventDone, m_hObjThread);
				if (!TerminateThread(m_hObjThread, 1)) {
					AddTraceLog(NULL, _T("APICall=TerminateThread Thread=%08X Error=%08X\r\n"), m_hObjThread, GetLastError());
				}
				if (!CloseHandle(m_hObjThread)) {
					AddTraceLog(NULL, _T("APICall=CloseHandle Thread=%08X Error=%08X\r\n"), m_hObjThread, GetLastError());
				}
			}
			m_hObjThread = INVALID_HANDLE_VALUE;
		}
		if (m_ObjInf.hEventStart && (m_ObjInf.hEventStart != INVALID_HANDLE_VALUE)) {
			if (!CloseHandle(m_ObjInf.hEventStart)) {
				AddTraceLog(NULL, _T("APICall=CloseHandle StartEvent=%08X Error=%08X\r\n"), m_ObjInf.hEventStart, GetLastError());
			}
			m_ObjInf.hEventStart = NULL;
		}
		if (m_ObjInf.hEventDone && (m_ObjInf.hEventDone != INVALID_HANDLE_VALUE)) {
			if (!CloseHandle(m_ObjInf.hEventDone)) {
				AddTraceLog(NULL, _T("APICall=CloseHandle DoneEvent=%08X Error=%08X\r\n"), m_ObjInf.hEventDone, GetLastError());
			}
			m_ObjInf.hEventDone = NULL;
		}
	}
	// This function must not use any resources or heap memory before the NtQueryObject in case of deadlock terminate thread causes leaks
	// note that in debug builds memory leaks still show up because _malloca/_freea use normal dynamic memory not the stack
	LPWSTR GetObjectNameInfo(HANDLE hObject, HANDLE hEventStart, HANDLE hEventDone, BOOL* bSuccess) 
	{
		LPWSTR lpwsReturn = NULL; 
		DWORD dwSize = sizeof(OBJECT_NAME_INFORMATION) + 2048;
		POBJECT_NAME_INFORMATION pObjectInfo = (POBJECT_NAME_INFORMATION)_malloca(dwSize);
		if (m_NTDLLAPI.GetNtQueryObject()) {
			NTSTATUS ntReturn = m_NTDLLAPI.GetNtQueryObject()(hObject, ObjectNameInformation, pObjectInfo, dwSize, &dwSize);
			while (ntReturn == STATUS_INFO_LENGTH_MISMATCH) {
				_freea(pObjectInfo);
				pObjectInfo = (POBJECT_NAME_INFORMATION)_malloca(dwSize);
				ntReturn = m_NTDLLAPI.GetNtQueryObject()(hObject, ObjectNameInformation, pObjectInfo, dwSize, &dwSize); 
				//MUST DETECT THREAD EXIT! - kernel driver stops this problem
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
		char* Buf = (char*)_malloca(16384);
		while (	(WaitForSingleObject(((CHandleList*)lParam)->m_ObjInf.hEventStart, INFINITE) == WAIT_OBJECT_0) &&
				(((CHandleList*)lParam)->m_ObjInf.hObject != INVALID_HANDLE_VALUE)) {
			// ObQueryNameString will hang when querying the names of pipes that have been opened for synchronous access (FILE_SYNCHRONOUS_IO_ALERT or FILE_SYNCHRONOUS_IO_NOALERT) and that have a pending read or write operation
			// Named pipes are file type objects and its hard to determine anymore in user mode without risking the permanently stranded thread
			// Must use kernel mode driver cannot be done in user space
			// PFILE_OBJECT has PDEVICE_OBJECT->DeviceType == FILE_DEVICE_NAMED_PIPE(0x11) or Flags = FO_NAMED_PIPE
			((CHandleList*)lParam)->m_ObjInf.lpwsReturn = NULL;
			((CHandleList*)lParam)->m_ObjInf.bSuccess = FALSE;
			DriverNameQuery.PID = ((CHandleList*)lParam)->m_ObjInf.pHandleInfo->uIdProcess;
			DriverNameQuery.ObjectAddress = ((CHandleList*)lParam)->m_ObjInf.pHandleInfo->Object;
			DriverNameQuery.ObjectHandle = (HANDLE)((CHandleList*)lParam)->m_ObjInf.pHandleInfo->Handle;
			ZeroMemory(Buf, 16384);
			if (DriverQuery((DWORD)IOCTL_FOI_GETPROCESSOBJECTNAME, &DriverNameQuery, sizeof(DriverNameQuery), Buf, 16384)) {
				((CHandleList*)lParam)->m_ObjInf.bSuccess = TRUE;
				SetEvent(((CHandleList*)lParam)->m_ObjInf.hEventDone); // starting allocation
				WaitForSingleObject(((CHandleList*)lParam)->m_ObjInf.hEventStart, INFINITE);
				((CHandleList*)lParam)->m_ObjInf.lpwsReturn = _wcsdup(CA2W(Buf));
			} else if (((CHandleList*)lParam)->m_ObjInf.pHandleInfo->ObjectTypeIndex != ((CHandleList*)lParam)->m_FileTypeIndex) {
				// should use driver ISFILEOBJECT to detect file objects with invalid length also since we can query them
				 ((CHandleList*)lParam)->m_ObjInf.lpwsReturn = ((CHandleList*)lParam)->GetObjectNameInfo(((CHandleList*)lParam)->m_ObjInf.hObject, ((CHandleList*)lParam)->m_ObjInf.hEventStart, ((CHandleList*)lParam)->m_ObjInf.hEventDone, &((CHandleList*)lParam)->m_ObjInf.bSuccess);
			}
			SetEvent(((OBJINF*)lParam)->hEventDone); // done
		}
		_freea(Buf);
		return 0;
	}
	void GetHandleName(CObjectTypeListTree* ObjectTypeListTree)
	{
		HANDLE hProcess;
		if ((hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, m_ObjInf.pHandleInfo->uIdProcess)) && hProcess != INVALID_HANDLE_VALUE) {
			if (DuplicateHandle(hProcess, (HANDLE)m_ObjInf.pHandleInfo->Handle, GetCurrentProcess(), &m_ObjInf.hObject, 0, FALSE, 0)) {
				if (!m_ObjInf.hEventStart || m_ObjInf.hEventStart == INVALID_HANDLE_VALUE) {
					if (!(m_ObjInf.hEventStart = CreateEvent(NULL, FALSE, FALSE, NULL))) {
						AddTraceLog(NULL, _T("APICall=CreateEvent Use=Start Error=%08X\r\n"), GetLastError());
					}
				}
				if (!m_ObjInf.hEventDone || m_ObjInf.hEventDone == INVALID_HANDLE_VALUE) {
					if (!(m_ObjInf.hEventDone = CreateEvent(NULL, FALSE, FALSE, NULL))) {
						AddTraceLog(NULL, _T("APICall=CreateEvent Use=Done Error=%08X\r\n"), GetLastError());
					}
				}
				if (!m_hObjThread || m_hObjThread == INVALID_HANDLE_VALUE) {
					if (!(m_hObjThread = (HANDLE)_beginthreadex(NULL, 0, &CHandleList::GetObjectNameThread, this, 0, NULL)) || (m_hObjThread == INVALID_HANDLE_VALUE)) {
						AddTraceLog(NULL, _T("LibraryCall=_beginthreadex Error=%08X\r\n"), errno);
					}
				}
				if (!SetEvent(m_ObjInf.hEventStart)) {
					AddTraceLog(NULL, _T("Start event SetEvent returns error %08X\r\n"), GetLastError());
				}
				if (WaitForSingleObject(m_ObjInf.hEventDone, 1000) == WAIT_TIMEOUT) {
					AddTraceLog(NULL, _T("APICall=WaitForSingleObject Error=WAIT_TIMEOUT Thread=NtQueryObject CurHandleEntry=[Type=%lS PID=%04X Handle=%04X]\r\n"), ObjectTypeListTree->GetKernelObjectName(m_ObjInf.pHandleInfo->ObjectTypeIndex - 1), m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.hObject);
					if (!TerminateThread(m_hObjThread, 1)) {
						AddTraceLog(NULL, _T("APICall=TerminateThread Thread=%08X Error=%08X\r\n"), m_hObjThread, GetLastError());
					}
					if (!CloseHandle(m_hObjThread)) {
						AddTraceLog(NULL, _T("APICall=CloseHandle Thread=%08X Error=%08X\r\n"), m_hObjThread, GetLastError());
					}
					m_hObjThread = INVALID_HANDLE_VALUE;
					m_ObjInf.lpwsReturn = new WCHAR[wcslen(L"<UNABLE TO QUERY NAME>") + 1];
					wcscpy(m_ObjInf.lpwsReturn, L"<UNABLE TO QUERY NAME>");
				} else {
					if (m_ObjInf.bSuccess) {
						if (!SetEvent(m_ObjInf.hEventStart)) {
							AddTraceLog(NULL, _T("Start event SetEvent returns error %08X\r\n"), GetLastError());
						}
						WaitForSingleObject(m_ObjInf.hEventDone, INFINITE);
					} else {
						AddTraceLog(NULL, _T("MyCall=GetHandleName CurHandleEntry=[Type=%lS PID=%04X Handle=%04X]\r\n"), ObjectTypeListTree->GetKernelObjectName(m_ObjInf.pHandleInfo->ObjectTypeIndex - 1), m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
					}
				}
				if (!CloseHandle(m_ObjInf.hObject)) {
					AddTraceLog(NULL, _T("APICall=CloseHandle DuplicatedHandle=%08X Error=%08X CurHandleEntry=[Type=%lS PID=%04X Handle=%04X]\r\n"), m_ObjInf.hObject, GetLastError(), ObjectTypeListTree->GetKernelObjectName(m_ObjInf.pHandleInfo->ObjectTypeIndex - 1), m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
				}
			} else {
				AddTraceLog(NULL, _T("APICall=DuplicateHandle Error=%08X CurHandleEntry=[Type=%lS PID=%04X Handle=%04X]\r\n"), GetLastError(), ObjectTypeListTree->GetKernelObjectName(m_ObjInf.pHandleInfo->ObjectTypeIndex - 1), m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
			}
			if (!CloseHandle(hProcess)) {
				AddTraceLog(NULL, _T("APICall=CloseHandle Process=%08X Error=%08X CurHandleEntry=[Type=%lS PID=%04X Handle=%04X]\r\n"), hProcess, GetLastError(), ObjectTypeListTree->GetKernelObjectName(m_ObjInf.pHandleInfo->ObjectTypeIndex - 1), m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
			}
		} else {
			if (hProcess != INVALID_HANDLE_VALUE) {
				AddTraceLog(NULL, _T("APICall=OpenProcess Rights=PROCESS_DUP_HANDLE Error=%08X CurHandleEntry=[Type=%lS PID=%04X Handle=%04X]\r\n"), GetLastError(), ObjectTypeListTree->GetKernelObjectName(m_ObjInf.pHandleInfo->ObjectTypeIndex - 1), m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
			} else {
				AddTraceLog(NULL, _T("APICall=OpenProcess Rights=PROCESS_DUP_HANDLE Error=INVALID_HANDLE_VALUE CurHandleEntry=[Type=%lS PID=%04X Handle=%04X]\r\n"), ObjectTypeListTree->GetKernelObjectName(m_ObjInf.pHandleInfo->ObjectTypeIndex - 1), m_ObjInf.pHandleInfo->uIdProcess, m_ObjInf.pHandleInfo->Handle);
			}
		}
	}
	//belongs in seperate class
	int ImageLookup(NMLVDISPINFO* pDispInfo, CObjectTypeListTree* ObjectTypeListTree)
	{
		return LookupImageFromType(ObjectTypeListTree->GetKernelObjectName(((HandleEntry*)pDispInfo->item.lParam)->HandleInfo.ObjectTypeIndex - 1));
	}
	void* GetItemData(DWORD & dwElementSize, BOOL & bDereference, size_t & sOffset, int iColIndex)
	{
		bDereference = FALSE;
		switch (iColIndex) {
		case 0:
			sOffset = (size_t)&((CHandleList::HandleEntry*)0)->Name;
			dwElementSize = 1;
			break;
		case 1:
			sOffset = (size_t)&((CHandleList::HandleEntry*)0)->HandleInfo.uIdProcess;
			dwElementSize = 1;
			break;
		case 2:
			sOffset = (size_t)&((CHandleList::HandleEntry*)0)->HandleInfo.Handle;
			dwElementSize = 1;
			break;
		case 3:
			sOffset = (size_t)&((CHandleList::HandleEntry*)0)->SystemIndex;
			dwElementSize = 1;
			break;
		default:
			sOffset = 0;
			dwElementSize = 0;
			break;
		}
		return NULL;
	}
	BOOL OnUpdateDatabase()
	{
		return TRUE;
	}
	void OnUpdateUI()
	{
		/*POSITION pos;
		POSITION npos;
		WORD key;
		WORD keyback;
		DWORD cb;
		CMapWordToPtr* ProcHandleTable;
		HANDLE hObject = NULL;
		DWORD Index = 0;
		HandleEntry* NewEntry;
		CArray<WORD> DeleteArray;
		CArray<WORD> DeleteProcArray;
		LVFINDINFO fi;
		fi.psz = NULL;
		fi.vkDirection = 0;
		fi.pt.x = 0;
		fi.pt.y = 0;
		if (m_NTDLLAPI.GetNtQueryObject() &&
			m_NTDLLAPI.GetNtQuerySystemInformation()) {
			DWORD dwSize = sizeof(SYSTEM_HANDLE_INFORMATION);
			PSYSTEM_HANDLE_INFORMATION pHandleInfo = (PSYSTEM_HANDLE_INFORMATION)new BYTE[dwSize];
			pHandleInfo->uCount = 0;
			NTSTATUS ntReturn = m_NTDLLAPI.GetNtQuerySystemInformation()(SystemHandleInformation, pHandleInfo, dwSize, &dwSize); 
			while (ntReturn == STATUS_INFO_LENGTH_MISMATCH) { 
				delete [] pHandleInfo;
				pHandleInfo = (PSYSTEM_HANDLE_INFORMATION) new BYTE[dwSize];
				ntReturn = m_NTDLLAPI.GetNtQuerySystemInformation()(SystemHandleInformation, pHandleInfo, dwSize, &dwSize);
			}
			if (ntReturn != STATUS_SUCCESS) {
				AddTraceLog(NULL, _T("Call=NtQuerySystemInformation Type=HandleInformation Error=%08X\r\n"), ntReturn);
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
						if ((WORD)(((KernelObjectTreeItem*)MainTree->GetItemData(MainTree->GetSelectedItem()))->dwIndex + 1) == NewEntry->HandleInfo.ObjectTypeIndex) {
							fi.flags = LVFI_PARAM;
							fi.lParam = (LPARAM)NewEntry;
							if ((Index = ProcList->FindItem(&fi)) != -1) {
								ProcList->DeleteItem(Index);
							}
						}
					}
					NewEntry->SystemIndex = cb;
					CopyMemory(&NewEntry->HandleInfo, &pHandleInfo->Handles[cb], sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO));
					ProcHandleTable->SetAt(pHandleInfo->Handles[cb].Handle, NewEntry);
					if ((WORD)(((KernelObjectTreeItem*)MainTree->GetItemData(MainTree->GetSelectedItem()))->dwIndex + 1) == NewEntry->HandleInfo.ObjectTypeIndex) {
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
						if ((Index == -1) || (Index = ProcList->FindItem(&fi)) == -1) {
							if ((Index = ProcList->InsertItem(
													LVIF_IMAGE | LVIF_TEXT | 
													LVIF_PARAM,
													ProcList->GetItemCount(),
													LPSTR_TEXTCALLBACK, 0, 0,
													I_IMAGECALLBACK,
													(LPARAM)NewEntry)) == -1) {
							}
						}
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
						AddTraceLog(NULL, _T("Event=Handle Deletion HandleEntry=[Type=%lS PID=%04X Handle=%04X]\r\n"), m_KernelObjectNames[NewEntry->HandleInfo.ObjectTypeIndex - 1], Index, key);
						if ((WORD)(((KernelObjectTreeItem*)MainTree->GetItemData(MainTree->GetSelectedItem()))->dwIndex + 1) == NewEntry->HandleInfo.ObjectTypeIndex) {
							fi.flags = LVFI_PARAM;
							fi.lParam = (LPARAM)NewEntry;
							if ((Index = ProcList->FindItem(&fi)) != -1)
								ProcList->DeleteItem(Index);
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
					AddTraceLog(NULL, _T("Event=PID Deletion HandleCount=0 PID=%04X\r\n"), keyback);
					DeleteProcArray.Add((WORD)keyback);
					delete ProcHandleTable;
				}
			}
			for (cb = 0; cb < (DWORD)DeleteProcArray.GetCount(); cb++) {
				m_ProcessIdDatabase->RemoveKey(DeleteProcArray[cb]);
			}
			if (pHandleInfo)
				delete [] pHandleInfo;
		}*/
	/*}
	void GetHandleTypeInformation()
	{
		/*NTSTATUS ntReturn;
		FILE_PIPE_INFORMATION fileinfo;
		IO_STATUS_BLOCK StatusBlock;
		StatusBlock.Pointer = NULL;
		StatusBlock.Information = NULL;
		StatusBlock.Status = 0;
		if (pHandleInfo->Handles[cb].ObjectTypeIndex == m_FileTypeIndex) {
			if (m_NTDLLAPI.GetNtQueryInformationFile()) {
				if (ntReturn = m_NTDLLAPI.GetNtQueryInformationFile()(hObject, &StatusBlock, &fileinfo, sizeof(FILE_PIPE_INFORMATION), (FILE_INFORMATION_CLASS)FilePipeInformation) != STATUS_SUCCESS) {
					Str.Format("Call=NtQueryInformationFile Error=%08X\r\n", ntReturn);
					AddTraceLog(Str);
					continue;
				} else {
					Str.Format("Call=NtQueryInformationFile Success=Found named pipe\r\n");
					continue;
				}
			}
		}*/
	/*}
protected:
	//thread communication structure only one thread actually runs at a time so safe
	struct HandleEntry
	{
		DWORD SystemIndex;
		SYSTEM_HANDLE_TABLE_ENTRY_INFO HandleInfo;
		WCHAR* Name;
		BOOL bDirty;
	};
	struct OBJINF {
		HANDLE hObject;//[in]
		SYSTEM_HANDLE_TABLE_ENTRY_INFO* pHandleInfo;//[in]
		HANDLE hEventStart;//[in]
		HANDLE hEventDone;//[in]
		BOOL bSuccess;//[out]
		LPWSTR lpwsReturn;//[out]
	};
	CMapWordToPtr* m_ProcessIdDatabase;
	OBJINF m_ObjInf;
	HANDLE m_hObjThread;
	DWORD m_FileTypeIndex;
	CNTDLLAPI m_NTDLLAPI;
};*/

#include <sddl.h>

class CDirectoryTreeList : public CGatherer
{
public:
	DECLARE_DYNCREATE(CDirectoryTreeList)
	CDirectoryTreeList()
	{
	}
	~CDirectoryTreeList()
	{
		Cleanup();
	}
	void Cleanup()
	{
		m_DirectoryEntries.RemoveAll();
	}
	void GetDependencies(CArray<CRuntimeClass*> & DependencyArray)
	{
		DependencyArray.Add(RUNTIME_CLASS(CKernelObjectDirectoryTree));
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_DirectoryEntries[(DWORD_PTR)pvKey].Name.GetString();
		case 1:
			return (void*)m_DirectoryEntries[(DWORD_PTR)pvKey].Type.GetString();
		case 2:
			return (void*)m_DirectoryEntries[(DWORD_PTR)pvKey].Owner.GetString();
		case 3:
			return (void*)m_DirectoryEntries[(DWORD_PTR)pvKey].DACL.GetString();
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		QueryDirectory(pModel, CT2W(pDepGatherer->GetTreePath(pModel->GetTreeModel(), pDepKeys->GetCount() ? pDepKeys->GetAt(0) : NULL)));
	}
	void QueryDirectory(CEnumModel *pModel, LPWSTR Path)
	{
		HANDLE hDirObj;
		HANDLE hObj;
		BOOLEAN bFirst = TRUE;
		ULONG ulIndex  = 0;
		CStringW Name;
		CStringW Type;
		if (PRU_NtOpenDirectoryObject(hDirObj, Path, NULL, DIRECTORY_QUERY)) {
			DirectoryEntry DirEntry;
			while (PRU_NtQueryDirectoryObject(hDirObj, bFirst, ulIndex, Name, Type)) {
				DirEntry.Name = Name;
				DirEntry.Type = Type;
				DirEntry.Owner.Empty();
				DirEntry.DACL.Empty();
				if (Type.CompareNoCase(DIRTYPESTRW) == 0 &&
					PRU_NtOpenDirectoryObject(hObj, Name, hDirObj, READ_CONTROL)) {
					CByteArray Bytes;
					if (PRU_NtQuerySecurityObject(hObj, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, Bytes)) {
						PSID psid;
						BOOL bDefaulted;
						if (GetSecurityDescriptorOwner((PSECURITY_DESCRIPTOR)Bytes.GetData(), &psid, &bDefaulted))
							PRU_FriendlyNameFromSid(psid, DirEntry.Owner);
						LPTSTR szStr;
						if (ConvertSecurityDescriptorToStringSecurityDescriptor((PSECURITY_DESCRIPTOR)Bytes.GetData(), SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &szStr, NULL)) {
							DirEntry.DACL = szStr;
							LocalFree(szStr);
						}
						PRU_CloseHandle(hObj);
					}
				}
				m_DirectoryEntries.Add(DirEntry);
				pModel->OnItemNew(GetRuntimeClass(), (void*)(m_DirectoryEntries.GetCount() - 1));
				bFirst = FALSE;
			}
			PRU_CloseHandle(hDirObj);
		}
	}
protected:
	struct DirectoryEntry
	{
		CString Name;
		CString Type;
		CString Owner;
		CString DACL;
	};
	CArray<DirectoryEntry, DirectoryEntry&> m_DirectoryEntries;
};

class CServiceGatherer : public CGatherer
{
public:
	DECLARE_DYNCREATE(CServiceGatherer)
	CServiceGatherer()
	{
		m_ServiceInformation = NULL;
		m_ServiceInformationBufferSize = 2048;
		m_NumberOfServices = 0;
	}
	~CServiceGatherer()
	{
		m_NumberOfServices = 0;
		if (m_ServiceInformation) {
			delete [] m_ServiceInformation;
			m_ServiceInformation = NULL;
		}
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		DWORD_PTR dwIndex = (DWORD_PTR)pvKey;
		switch (iIndex) {
		case SERVICE_SERVICENAME:
			return &m_ServiceInformation[dwIndex].lpServiceName;
		case SERVICE_DISPLAYNAME:
            return &m_ServiceInformation[dwIndex].lpDisplayName;
		case SERVICE_CURRENTSTATE:
	        return &m_ServiceInformation[dwIndex].ServiceStatusProcess.dwCurrentState;
		case SERVICE_SERVICETYPE:
            return &m_ServiceInformation[dwIndex].ServiceStatusProcess.dwServiceType;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGather, CArray<void*>* pDepKeys)
	{
		DWORD_PTR dwCount;
		DWORD nServices = 0;
		DWORD ResumeHandle = 0;
		DWORD MoreBytes;
		ServiceManager SM;
		if (SM.GetHandle()) {
			if (!m_ServiceInformation)
				m_ServiceInformation = (LPENUM_SERVICE_STATUS_PROCESS)
								new BYTE[sizeof(ENUM_SERVICE_STATUS_PROCESS) *
										 m_ServiceInformationBufferSize];
			while (m_ServiceInformation && !EnumServicesStatusEx(SM.GetHandle(), SC_ENUM_PROCESS_INFO, 
										 SERVICE_WIN32 |
										 SERVICE_INTERACTIVE_PROCESS, 
										 SERVICE_STATE_ALL, 
										 (LPBYTE)m_ServiceInformation, 
										 m_ServiceInformationBufferSize, 
										 &MoreBytes, &nServices, &ResumeHandle, 
										 NULL) &&
				   (GetLastError() == ERROR_MORE_DATA)) {
				m_ServiceInformationBufferSize += MoreBytes;
				delete [] m_ServiceInformation;
				m_ServiceInformation = (LPENUM_SERVICE_STATUS_PROCESS)
								new BYTE[sizeof(ENUM_SERVICE_STATUS_PROCESS) *
										 m_ServiceInformationBufferSize];
				nServices = 0;
				ResumeHandle = 0;
			}
			if (m_ServiceInformation) {
				for (	dwCount = 0;
						dwCount < nServices; dwCount++) {
					pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
				}
			}
			m_NumberOfServices = nServices;
		}
	}
protected:
	ENUM_SERVICE_STATUS_PROCESS* m_ServiceInformation;
	DWORD m_ServiceInformationBufferSize;
	DWORD m_NumberOfServices;
};

class CServiceConfig : public CGatherer
{
public:
	DECLARE_DYNCREATE(CServiceConfig)
	~CServiceConfig()
	{
		POSITION pos = m_ServiceMap.GetStartPosition();
		DWORD_PTR dwKey;
		LPQUERY_SERVICE_CONFIG pvValue;
		while (pos) {
			m_ServiceMap.GetNextAssoc(pos, dwKey, pvValue);
			delete [] pvValue;
		}
		m_ServiceMap.RemoveAll();
	}
	void GetDependencies(CArray<CRuntimeClass*> & DependencyArray)
	{
		DependencyArray.Add(RUNTIME_CLASS(CServiceGatherer));
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		LPQUERY_SERVICE_CONFIG pServiceConfig;
		if (!m_ServiceMap.Lookup((DWORD_PTR&)pvKey, pServiceConfig)) return NULL;
		switch (iIndex) {
		case 0:
			return &pServiceConfig->dwStartType;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		SC_HANDLE hService;
		DWORD_PTR dwCount;
		DWORD dwBufSize;
		LPQUERY_SERVICE_CONFIG pServiceConfig;
		ServiceManager SM;
		for (dwCount = 0; dwCount < (DWORD)pDepKeys->GetCount(); dwCount++) {
			if ((hService = OpenService(SM.GetHandle(), *(TCHAR**)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), SERVICE_SERVICENAME), SERVICE_QUERY_CONFIG)) != NULL) {
				dwBufSize = 2048;
				pServiceConfig = (LPQUERY_SERVICE_CONFIG)new BYTE[dwBufSize];
				while (pServiceConfig && !QueryServiceConfig(hService, pServiceConfig, dwBufSize, &dwBufSize) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
					delete [] pServiceConfig;
					pServiceConfig = (LPQUERY_SERVICE_CONFIG)new BYTE[dwBufSize];
				}
				CloseServiceHandle(hService);
				if (pServiceConfig) {
					m_ServiceMap.SetAt((DWORD_PTR&)pDepKeys->GetAt(dwCount), pServiceConfig);
					pModel->OnItemNew(GetRuntimeClass(), pDepKeys->GetAt(dwCount));
				}
			}
		}
	}
protected:
	CMap<DWORD_PTR, DWORD_PTR&, LPQUERY_SERVICE_CONFIG, LPQUERY_SERVICE_CONFIG&> m_ServiceMap;
};

class CDriverList : public CGatherer
{
public:
	DECLARE_DYNCREATE(CDriverList)
	CDriverList()
	{
		m_DriverInformation = NULL;
		m_DriverInformationBufferSize = 2048;
		m_NumberOfDrivers = 0;
		m_DriverConfigs = NULL;
	}
	~CDriverList()
	{
		Cleanup();
	}
	void Cleanup()
	{
		DWORD dwCount;
		for (dwCount = 0; dwCount < m_NumberOfDrivers; dwCount++) {
			delete [] m_DriverConfigs[dwCount];
		}
		if (m_DriverConfigs) {
			delete [] m_DriverConfigs;
			m_DriverConfigs = NULL;
		}
		if (m_DriverInformation) {
			delete [] m_DriverInformation;
			m_DriverInformation = NULL;
		}
		m_DriverInformationBufferSize = 2048;
		m_NumberOfDrivers = 0;
		m_DriverTagOrderArray.RemoveAll();
		m_ServiceStringArray.RemoveAll();
		m_GroupOrderListArray.RemoveAll();
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return &m_DriverInformation[(DWORD_PTR)pvKey].lpServiceName;
		case 1:
			return &m_DriverInformation[(DWORD_PTR)pvKey].lpDisplayName;
		case 2:
			return &m_DriverInformation[(DWORD_PTR)pvKey].ServiceStatusProcess.dwCurrentState;
		case 3:
			return &m_DriverInformation[(DWORD_PTR)pvKey].ServiceStatusProcess.dwServiceType;
		case 4:
			return &m_DriverConfigs[(DWORD_PTR)pvKey]->dwStartType;
		case 5:
			return &m_DriverConfigs[(DWORD_PTR)pvKey]->lpBinaryPathName;
		case 6:
			return &m_DriverConfigs[(DWORD_PTR)pvKey]->lpLoadOrderGroup;
		case 7:
			return &m_DriverConfigs[(DWORD_PTR)pvKey]->dwTagId;
		case 8:
			return &m_DriverConfigs[(DWORD_PTR)pvKey]->lpDependencies;
		case 9:
			return &m_DriverTagOrderArray[(DWORD_PTR)pvKey];
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGather, CArray<void*>* pDepKeys)
	{
		DWORD nServices = 0;
		DWORD ResumeHandle = 0;
		DWORD MoreBytes;
		SC_HANDLE hService;
		INT_PTR iCount;
		DWORD_PTR dwCount;
		DWORD dwServiceConfig;
		CByteArray Bytes;
		PRU_ReadRegistryMultiString(HKEY_LOCAL_MACHINE, REG_STR_SERVICEORDERLIST,
								_T("List"), m_ServiceStringArray);
		for (iCount = 0; iCount < m_ServiceStringArray.GetCount(); iCount++) {
			CCopyDWordArray NewOrderList;
			m_GroupOrderListArray.Add(NewOrderList);
			if (PRU_ReadRegistryBinary(	HKEY_LOCAL_MACHINE, REG_STR_GROUPORDERLIST,
									m_ServiceStringArray[iCount], Bytes)) {
				for (	dwCount = 0;
						dwCount < *(DWORD*)Bytes.GetData(); dwCount++) {
					m_GroupOrderListArray[iCount].
								Add(*((DWORD*)Bytes.GetData() + dwCount + 1));
				}
			}
		}
		ServiceManager SM;
		if (SM.GetHandle()) {
			if (!m_DriverInformation) {
				m_DriverInformation = (LPENUM_SERVICE_STATUS_PROCESS)
								new BYTE[sizeof(ENUM_SERVICE_STATUS_PROCESS) *
										 m_DriverInformationBufferSize];
			}
			SetLastError(ERROR_SUCCESS);
			while (m_DriverInformation && !EnumServicesStatusEx(SM.GetHandle(), SC_ENUM_PROCESS_INFO,
											SERVICE_DRIVER | SERVICE_ADAPTER,
											SERVICE_STATE_ALL,
											(LPBYTE)m_DriverInformation,
											m_DriverInformationBufferSize,
											&MoreBytes, &nServices, 
											&ResumeHandle, NULL) &&
				   (GetLastError() == ERROR_MORE_DATA)) {
				m_DriverInformationBufferSize += MoreBytes;
				delete [] m_DriverInformation;
				m_DriverInformation = (LPENUM_SERVICE_STATUS_PROCESS)
								new BYTE[sizeof(ENUM_SERVICE_STATUS_PROCESS) *
										 m_DriverInformationBufferSize];
				nServices = 0;
				ResumeHandle = 0;
			}
			if (GetLastError() != ERROR_SUCCESS) {
				//trace log
			}
			if (m_DriverInformation) {
				m_DriverConfigs = new LPQUERY_SERVICE_CONFIG[nServices];
				if (m_DriverConfigs) {
					for (dwCount = 0; dwCount < nServices; dwCount++) {
						if ((hService = OpenService(SM.GetHandle(),
													m_DriverInformation[dwCount].
																		lpServiceName,
													SERVICE_QUERY_CONFIG)) != NULL) {
							if (!QueryServiceConfig(hService, NULL,
													0, &dwServiceConfig) &&
								GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
								m_DriverConfigs[dwCount] = (LPQUERY_SERVICE_CONFIG)
															new BYTE[dwServiceConfig];
								if (!QueryServiceConfig(hService,
														m_DriverConfigs[dwCount],
														dwServiceConfig,
														&dwServiceConfig)) {
									//trace log
								}
							} else {
								//trace log
							}
							if (!CloseServiceHandle(hService)) {
								//trace log
							}
						}
						for (	iCount = 0; iCount < m_ServiceStringArray.GetCount();
								iCount++) {
							if (m_ServiceStringArray[iCount].Compare(
									m_DriverConfigs[dwCount]->lpLoadOrderGroup) == 0) {
								break;
							}
						}
						m_DriverTagOrderArray.Add(	(iCount != m_ServiceStringArray.GetCount()) &&
													m_GroupOrderListArray[iCount].GetCount() > (INT_PTR)m_DriverConfigs[dwCount]->dwTagId ? 
													m_GroupOrderListArray[iCount][m_DriverConfigs[dwCount]->dwTagId] :
													-1);
						pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
					}
				}
			}
		}
		m_NumberOfDrivers = nServices;
	}
protected:
	CStringArray m_ServiceStringArray;
	CArray<CCopyDWordArray, CCopyDWordArray&> m_GroupOrderListArray;
	CDWordArray m_DriverTagOrderArray;
	LPQUERY_SERVICE_CONFIG* m_DriverConfigs;
	ENUM_SERVICE_STATUS_PROCESS* m_DriverInformation;
	DWORD m_DriverInformationBufferSize;
	DWORD m_NumberOfDrivers;
};

class CLogonSessionList : public CGatherer
{
public:
	DECLARE_DYNCREATE(CLogonSessionList)
	struct LogonSessionData
	{
		LUID			LogonId;
		CStringW		UserName;
		CStringW		LogonDomain;
		CStringW		AuthenticationPackage;
		ULONG			LogonType;
		ULONG			Session;
		PSID			Sid;	
		LARGE_INTEGER	LogonTime;
		CStringW		LogonServer;
		CStringW		DnsDomainName;
		CStringW		Upn;
	};
	CLogonSessionList()
	{
		m_LogonSessionInformation = NULL;
		m_NumberOfLogonSessions = 0;
	}
	~CLogonSessionList()
	{
		Cleanup();
	}
	void Cleanup()
	{
		if (m_LogonSessionInformation) {
			DWORD dwCount;
			for (dwCount = 0; dwCount < m_NumberOfLogonSessions; dwCount++) {
				if (m_LogonSessionInformation[dwCount].Sid) {
					free(m_LogonSessionInformation[dwCount].Sid);
				}
			}
			if (m_LogonSessionInformation) delete [] m_LogonSessionInformation;
			m_LogonSessionInformation = NULL;
			m_NumberOfLogonSessions = 0;
		}
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].LogonId;
		case 1:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].UserName;
		case 2:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].LogonDomain;
		case 3:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].AuthenticationPackage;
		case 4:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].LogonType;
		case 5:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].Session;
		case 6:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].Sid;
		case 7:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].LogonTime;
		case 8:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].LogonServer;
		case 9:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].DnsDomainName;
		case 10:
			return &m_LogonSessionInformation[(DWORD_PTR)pvKey].Upn;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGather, CArray<void*>* pDepKeys)
	{
		ULONG ulCount;
		PLUID pLuid = NULL;
		DWORD_PTR dwCounter;
		PSECURITY_LOGON_SESSION_DATA pLogonSessionData = NULL;
		if (m_Secur32API.Get_LsaEnumerateLogonSessions()(&ulCount, &pLuid) == STATUS_SUCCESS) {
			//Need to handle permission checks and scenarios...
			m_LogonSessionInformation = new LogonSessionData[ulCount];
			if (m_LogonSessionInformation) {
				for (dwCounter = 0; dwCounter < ulCount; dwCounter++) {
					if (m_Secur32API.Get_LsaGetLogonSessionData()(
										&pLuid[dwCounter], 
										&pLogonSessionData) == STATUS_SUCCESS) {
						if (pLogonSessionData) {
							m_LogonSessionInformation[dwCounter].LogonId = pLogonSessionData->LogonId;
							UniStringToCStringW(&pLogonSessionData->UserName, m_LogonSessionInformation[dwCounter].UserName);
							UniStringToCStringW(&pLogonSessionData->LogonDomain, m_LogonSessionInformation[dwCounter].LogonDomain);
							UniStringToCStringW(&pLogonSessionData->AuthenticationPackage, m_LogonSessionInformation[dwCounter].AuthenticationPackage);
							m_LogonSessionInformation[dwCounter].LogonType = pLogonSessionData->LogonType;
							m_LogonSessionInformation[dwCounter].Session = pLogonSessionData->Session;
							if (pLogonSessionData->Sid) {
								m_LogonSessionInformation[dwCounter].Sid = malloc(GetLengthSid(pLogonSessionData->Sid));
								CopySid(GetLengthSid(pLogonSessionData->Sid), m_LogonSessionInformation[dwCounter].Sid, pLogonSessionData->Sid);
							} else {
								m_LogonSessionInformation[dwCounter].Sid = NULL;
							}
							m_LogonSessionInformation[dwCounter].LogonTime = pLogonSessionData->LogonTime;
							UniStringToCStringW(&pLogonSessionData->LogonServer, m_LogonSessionInformation[dwCounter].LogonServer);
							UniStringToCStringW(&pLogonSessionData->DnsDomainName, m_LogonSessionInformation[dwCounter].DnsDomainName);
							UniStringToCStringW(&pLogonSessionData->Upn, m_LogonSessionInformation[dwCounter].Upn);
							m_Secur32API.Get_LsaFreeReturnBuffer()(pLogonSessionData);
							pLogonSessionData = NULL;
						}
					}
					pModel->OnItemNew(GetRuntimeClass(), (void*)dwCounter);
				}
			}
			m_Secur32API.Get_LsaFreeReturnBuffer()(pLuid);
		} else {
			ulCount = 0;
		}
		m_NumberOfLogonSessions = ulCount;
	}
protected:
	CAPISecur32 m_Secur32API;
	LogonSessionData* m_LogonSessionInformation;
	DWORD m_NumberOfLogonSessions;
};

class CGlobalAutoRuns : public CGatherer
{
public:
	DECLARE_DYNCREATE(CGlobalAutoRuns)
	void Cleanup()
	{
		m_AutoRunEntries.RemoveAll();
		m_ResolvedAutoRuns.RemoveAll();
		m_Locations.RemoveAll();
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_AutoRunEntries[(DWORD_PTR)pvKey].Key.GetString();
		case 1:
			return (void*)m_AutoRunEntries[(DWORD_PTR)pvKey].Value.GetData();
		case 2:
			return (void*)m_ResolvedAutoRuns[(DWORD_PTR)pvKey].GetString();
		case 3:
			return (void*)m_Locations[(DWORD_PTR)pvKey].GetString();
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGather, CArray<void*>* pDepKeys)
	{
		INT_PTR iCount;
		INT_PTR iFirstCount;
		INT_PTR iSecondCount;
		CString String;
		//3 WOW64 keys also
		PRU_RegistryEnumValues(HKEY_LOCAL_MACHINE,
						REGSTR_PATH_RUN,
						m_AutoRunEntries);
		iFirstCount = m_AutoRunEntries.GetCount();
		PRU_RegistryEnumValues(HKEY_LOCAL_MACHINE,
						REGSTR_PATH_RUNONCE,
						m_AutoRunEntries, FALSE);
		iSecondCount = m_AutoRunEntries.GetCount();
		PRU_RegistryEnumValues(HKEY_LOCAL_MACHINE,
						REGSTR_PATH_RUNONCEEX,
						m_AutoRunEntries, FALSE);
		if (GetVersion() & 0x80000000) {
			//9x
			PRU_RegistryEnumValues(HKEY_LOCAL_MACHINE,
							REGSTR_PATH_RUNSERVICES,
							m_AutoRunEntries);
			PRU_RegistryEnumValues(HKEY_LOCAL_MACHINE,
							REGSTR_PATH_RUNSERVICESONCE,
							m_AutoRunEntries);
		}
		for (iCount = 0; iCount < m_AutoRunEntries.GetCount(); iCount++) {
			if (iCount < iFirstCount) {
				m_Locations.Add(_T("Run"));
			} else if (iCount < iSecondCount) {
				m_Locations.Add(_T("RunOnce"));
			} else {
				m_Locations.Add(_T("RunOnceEx"));
			}
			if (m_AutoRunEntries[iCount].dwType == REG_EXPAND_SZ &&
				PRU_ExpandEnvironmentStrings((LPCTSTR)m_AutoRunEntries[iCount].Value.GetData(), String)) {
				m_ResolvedAutoRuns.Add(String);
			} else {
				m_ResolvedAutoRuns.Add(_T(""));
			}
			pModel->OnItemNew(GetRuntimeClass(), (void*)iCount);
		}
	}
protected:
	CStringArray m_ResolvedAutoRuns;
	CStringArray m_Locations;
	CKVArray m_AutoRunEntries;
};

class CUserAutoRuns : public CGatherer
{
public:
	DECLARE_DYNCREATE(CUserAutoRuns)
	void Cleanup()
	{
		m_AutoRunEntries.RemoveAll();
		m_ResolvedAutoRuns.RemoveAll();
		m_Locations.RemoveAll();
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_AutoRunEntries[(DWORD_PTR)pvKey].Key.GetString();
		case 1:
			return (void*)m_AutoRunEntries[(DWORD_PTR)pvKey].Value.GetData();
		case 2:
			return (void*)m_ResolvedAutoRuns[(DWORD_PTR)pvKey].GetString();
		case 3:
			return (void*)m_Locations[(DWORD_PTR)pvKey].GetString();
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGather, CArray<void*>* pDepKeys)
	{
		INT_PTR iCount;
		INT_PTR iFirstCount;
		INT_PTR iSecondCount;
		CString String;
		//3 Terminal services keys also
		PRU_RegistryEnumValues(HKEY_CURRENT_USER,
						REGSTR_PATH_RUN,
						m_AutoRunEntries);
		iFirstCount = m_AutoRunEntries.GetCount();
		PRU_RegistryEnumValues(HKEY_CURRENT_USER,
						REGSTR_PATH_RUNONCE,
						m_AutoRunEntries, FALSE);
		iSecondCount = m_AutoRunEntries.GetCount();
		PRU_RegistryEnumValues(HKEY_CURRENT_USER,
						REGSTR_PATH_RUNONCEEX,
						m_AutoRunEntries, FALSE);
		for (iCount = 0; iCount < m_AutoRunEntries.GetCount(); iCount++) {
			if (iCount < iFirstCount) {
				m_Locations.Add(_T("Run"));
			} else if (iCount < iSecondCount) {
				m_Locations.Add(_T("RunOnce"));
			} else {
				m_Locations.Add(_T("RunOnceEx"));
			}	
			if (m_AutoRunEntries[iCount].dwType == REG_EXPAND_SZ &&
				PRU_ExpandEnvironmentStrings((LPCTSTR)m_AutoRunEntries[iCount].Value.GetData(), String)) {
				m_ResolvedAutoRuns.Add(String);
			} else {
				m_ResolvedAutoRuns.Add(_T(""));
			}
			pModel->OnItemNew(GetRuntimeClass(), (void*)iCount);
		}
	}
protected:
	CStringArray m_ResolvedAutoRuns;
	CStringArray m_Locations;
	CKVArray m_AutoRunEntries;
};

class CWindowsInstallerComponentProducts : public CGatherer
{
public:
	DECLARE_DYNCREATE(CWindowsInstallerComponentProducts)
	void GetDependencies(CArray<CRuntimeClass*>& DependencyArray)
	{
		DependencyArray.Add(RUNTIME_CLASS(CWindowsInstallerComponents));
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		CMap<void*, void*&, CCopyStringArray, CCopyStringArray&>::CPair* pPair;
		switch (iIndex) {
		case 0:
			pPair = m_DisplayNames.PLookup(pvKey);
			return pPair ? (void*)&pPair->value : NULL;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		DWORD dwCount;
		DWORD dwSubCount;
		HKEY hKey;
		CString StrCur;
		CString StrBuild;
		CString StrBase = REGSTR_WINDOWS_INSTALLER_PRODUCTS;
		StrBase += _T("\\");
		for (dwCount = 0; dwCount < (DWORD)pDepKeys->GetCount(); dwCount++) {
			CCopyKVArray* pKVArray = (CCopyKVArray*)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 3);
			CStringArray* pKeyArray = (CStringArray*)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 4);
			CCopyStringArray Strings;
			StrCur = StrBase + (LPCTSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 0) + _T("\\Products\\");
			if (pKVArray) {
				for (dwSubCount = 0; dwSubCount < (DWORD)pKVArray->GetCount(); dwSubCount++) {
					StrBuild = StrCur + pKVArray->GetAt(dwSubCount).Key + _T("\\InstallProperties");
					if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, StrBuild, hKey)) {
						CString String;
						if (PRU_ReadRegistryString(hKey, _T("DisplayName"), String)) {
							Strings.Add(String);
						}
						PRU_RegCloseKey(hKey);
					}
				}
			}
			if (pKeyArray) {
				for (dwSubCount = 0; dwSubCount < (DWORD)pKeyArray->GetCount(); dwSubCount++) {
					StrBuild = StrCur + pKeyArray->GetAt(dwSubCount) + _T("\\InstallProperties");
					if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, StrBuild, hKey)) {
						CString String;
						if (PRU_ReadRegistryString(hKey, _T("DisplayName"), String)) {
							Strings.Add(String);
						}
						PRU_RegCloseKey(hKey);
					}
				}
			}
			m_DisplayNames.SetAt(pDepKeys->GetAt(dwCount), Strings);
			pModel->OnItemNew(GetRuntimeClass(), pDepKeys->GetAt(dwCount));
		}
	}
private:
	CMap<void*, void*&, CCopyStringArray, CCopyStringArray&> m_DisplayNames;
};

class CWindowsComponentManifests : public CGatherer
{
	DECLARE_DYNCREATE(CWindowsComponentManifests)
	void* GetItemData(void* pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_Components[(DWORD_PTR)pvKey].Name.GetString();
		case 1:
			return (void*)&m_Components[(DWORD_PTR)pvKey].bManifest;
		case 2:
			return (void*)&m_Components[(DWORD_PTR)pvKey].bCat;
		case 3:
			return (void*)&m_Components[(DWORD_PTR)pvKey].bFolder;
		case 4:
			return (void*)&m_Components[(DWORD_PTR)pvKey].bBackup;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGather, CArray<void*>* pDepKeys)
	{
		DWORD dwCount;
		CString Path;
		CStringArray ComponentStrings;
		if (!PRU_GetWindowsDirectory(Path)) return;
		PRU_EnumDirectory(Path + _T("\\winsxs\\Manifests\\*.cat"), FALSE, TRUE, TRUE, ComponentStrings);
		PRU_EnumDirectory(Path + _T("\\winsxs\\Manifests\\*.manifest"), FALSE, TRUE, TRUE, ComponentStrings);
		PRU_EnumDirectory(Path + _T("\\winsxs\\Backup\\*.manifest"), FALSE, TRUE, TRUE, ComponentStrings);
		LPCTSTR szExclusions[] = {	_T("Backup"), _T("Catalogs"), _T("FileMaps"),
									_T("InstallTemp"), _T("ManifestCache"),
									_T("Manifests"), _T("Temp"), NULL };
		PRU_EnumDirectory(Path + _T("\\winsxs\\*"), TRUE, FALSE, TRUE, ComponentStrings, szExclusions);
		EliminateDupStrings(ComponentStrings, TRUE);
		for (dwCount = 0; dwCount < (DWORD)ComponentStrings.GetCount(); dwCount++) {
			ComponentInfo NewComponentInfo;
			NewComponentInfo.Name = ComponentStrings[dwCount];
			CString CheckString = Path + _T("\\winsxs\\Manifests\\") + ComponentStrings[dwCount] + _T(".manifest");
			NewComponentInfo.bManifest = PRU_PathFileExists(CheckString);
			CheckString = Path + _T("\\winsxs\\Manifests\\") + ComponentStrings[dwCount] + _T(".cat");
			NewComponentInfo.bCat = PRU_PathFileExists(CheckString);
			CheckString = Path + _T("\\winsxs\\Backup\\") + ComponentStrings[dwCount] + _T(".manifest");
			NewComponentInfo.bBackup = PRU_PathFileExists(CheckString);
			CheckString = Path + _T("\\winsxs\\") + ComponentStrings[dwCount];
			NewComponentInfo.bFolder = PRU_PathFileExists(CheckString);
			m_Components.Add(NewComponentInfo);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_Components.GetCount() - 1));
		}
	}
private:
	struct ComponentInfo
	{
		CString Name;
		BOOL bManifest;
		BOOL bCat;
		BOOL bFolder;
		BOOL bBackup;
	};
	CArray<ComponentInfo, ComponentInfo&> m_Components;
};

class CWindowsPackageServicingCache : public CGatherer
{
	DECLARE_DYNCREATE(CWindowsPackageServicingCache)
	void* GetItemData(void* pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_Packages[(DWORD_PTR)pvKey].Name.GetString();
		case 1:
			return (void*)&m_Packages[(DWORD_PTR)pvKey].bMUM;
		case 2:
			return (void*)&m_Packages[(DWORD_PTR)pvKey].bCat;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGather, CArray<void*>* pDepKeys)
	{
		DWORD dwCount;
		CString Path;
		CStringArray PackageStrings;
		if (!PRU_GetWindowsDirectory(Path)) return;
		PRU_EnumDirectory(Path + _T("\\servicing\\Packages\\*.mum"), FALSE, TRUE, TRUE, PackageStrings);
		PRU_EnumDirectory(Path + _T("\\servicing\\Packages\\*.cat"), FALSE, TRUE, TRUE, PackageStrings);
		EliminateDupStrings(PackageStrings, TRUE);
		for (dwCount = 0; dwCount < (DWORD)PackageStrings.GetCount(); dwCount++) {
			PackageInfo NewPackageInfo;
			NewPackageInfo.Name = PackageStrings[dwCount];
			CString CheckString = Path + _T("\\servicing\\Packages\\") + PackageStrings[dwCount] + _T(".mum");
			NewPackageInfo.bMUM = PRU_PathFileExists(CheckString);
			CheckString = Path + _T("\\servicing\\Packages\\") + PackageStrings[dwCount] + _T(".cat");
			NewPackageInfo.bCat = PRU_PathFileExists(CheckString);
			m_Packages.Add(NewPackageInfo);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_Packages.GetCount() - 1));
		}
	}
private:
	struct PackageInfo
	{
		CString Name;
		BOOL bMUM;
		BOOL bCat;
	};
	CArray<PackageInfo, PackageInfo&> m_Packages;
};

class CGroupLoadOrderTagListGatherer : public CGatherer
{
	DECLARE_DYNCREATE(CGroupLoadOrderTagListGatherer)
	void GetDependencies(CArray<CRuntimeClass*> & DependencyArray)
	{
		DependencyArray.Add(RUNTIME_CLASS(CServiceOrderGroups));
	}
	void* GetItemData(void* pvKey, int iIndex)
	{
		CMap<DWORD_PTR, DWORD_PTR&, CCopyByteArray, CCopyByteArray&>::CPair* pPair;
		if ((pPair = m_GroupOrderListMap.PLookup((DWORD_PTR&)pvKey)) != NULL) return NULL;
		switch (iIndex) {
		case 0:
			return &pPair->value;
		case 1:
			return &pPair->value;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGather, CArray<void*>* pDepKeys)
	{
		DWORD_PTR dwCount;
		//enumerate and look for additional entries to find danglers
		for (dwCount = 0; dwCount < (DWORD)pDepKeys->GetCount(); dwCount++) {
			CCopyByteArray Bytes;
			if (PRU_ReadRegistryBinary(HKEY_LOCAL_MACHINE, REG_STR_GROUPORDERLIST,
									*(TCHAR**)pDepGather->GetItemData(pDepKeys->GetAt(dwCount), 0), Bytes)) {
			}
			m_GroupOrderListMap.SetAt((DWORD_PTR&)pDepKeys->GetAt(dwCount), Bytes);
			pModel->OnItemNew(GetRuntimeClass(), pDepKeys->GetAt(dwCount));
		}
	}
protected:
	CMap<DWORD_PTR, DWORD_PTR&, CCopyByteArray, CCopyByteArray&> m_GroupOrderListMap;
};

class CCriticalDeviceReferences : public CGatherer
{
	DECLARE_DYNCREATE(CCriticalDeviceReferences)
	void GetDependencies(CArray<CRuntimeClass*> & DependencyArray)
	{
		DependencyArray.Add(RUNTIME_CLASS(CEnumEntries));
		DependencyArray.Add(RUNTIME_CLASS(CDeviceClasses));
	}
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)&m_References[(DWORD_PTR)pvKey];
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		DWORD dwCount;
		DWORD dwIndex;
		DWORD dwSubCount;
		CMap<CString, LPCTSTR, CCopyStringArray, CCopyStringArray&> MapHardwareIDs;
		//must map using DeviceClasses?
		//CEnumEntries dependency
		for (dwCount = 0; dwCount < (DWORD)pDepKeys->GetCount(); dwCount++) {
			for (dwIndex = 11; dwIndex <= 12; dwIndex++) {
				CStringArray* pStringArray = (CStringArray*)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), dwIndex);
				for (dwSubCount = 0; dwSubCount < (DWORD)pStringArray->GetCount(); dwSubCount++) {
					CCopyStringArray NewStringArray;
					CMap<CString, LPCTSTR, CCopyStringArray, CCopyStringArray&>::CPair* pPair;
					CString RefString = CString((LPCTSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 0)) + _T("\\") + (LPCTSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 1) + _T("\\") + (LPCTSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 2);
					if ((pPair = MapHardwareIDs.PLookup((*pStringArray)[dwSubCount].MakeLower())) != NULL) {
						pPair->value.Add(RefString);
					} else {
						NewStringArray.Add(RefString);
						MapHardwareIDs.SetAt((*pStringArray)[dwSubCount].MakeLower(), NewStringArray);
					}
				}
			}
		}
		//CDeviceClasses dependency
		for (dwCount = 0; dwCount < (DWORD)pDepKeys->GetCount(); dwCount++) {
			CCopyStringArray Reference;
			CString Str = (LPCTSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(dwCount), 0);
			CString Lookup = Str;
			Str.Replace('#', '\\');
			if (!MapHardwareIDs.Lookup(Str.MakeLower(), Reference)) {
				MapHardwareIDs.Lookup(Lookup.MakeLower(), Reference);
			}
			m_References.Add(Reference);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_References.GetCount() - 1));
		}
	}
private:
	CArray<CCopyStringArray, CCopyStringArray&> m_References;
};

static void WindowsComponentPackageSessionDeleteAction(CGatherer* pGatherer, void* pvKey)
{
	HKEY hKey;
	PSID psid = NULL;
	LPCTSTR szKey = (LPCTSTR)pGatherer->GetItemData(pvKey, 0);
	CString String = _T("MACHINE\\");
	String += REGSTR_WINDOWS_COMPONENT_SESSIONS;
	PRU_GetOwner(String.GetBuffer(), SE_REGISTRY_KEY, &psid);
	//PRU_AddToAcl(String.GetBuffer(), SE_REGISTRY_KEY, FALSE, DELETE);
	String += _T("\\");
	String += szKey;
	PRU_TakeOwnership(String.GetBuffer(), SE_REGISTRY_KEY, FALSE);
	if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_WINDOWS_COMPONENT_SESSIONS, hKey)) {
		PRU_RegDeleteKey(hKey, szKey);
		PRU_RegCloseKey(hKey);
	}
	String = _T("MACHINE\\");
	String += REGSTR_WINDOWS_COMPONENT_SESSIONS;
	//PRU_RemoveFromAcl(String.GetBuffer(), SE_REGISTRY_KEY, FALSE, DELETE);
	PRU_TakeOwnership(String.GetBuffer(), SE_REGISTRY_KEY, FALSE, psid);
	delete [] psid;
}

static void WindowsInstallerSourceDeleteAction(CGatherer* pGatherer, void* pvKey)
{
	HKEY hKey;
	CString Str = (LPCTSTR)pGatherer->GetItemData(pvKey, 0);
	if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, CString(REGSTR_WINDOWS_INSTALLER_PRODUCTS) + _T("\\") + Str + _T("\\Products\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 2) + _T("\\InstallProperties"), hKey)) {
		PRU_RegDeleteValue(hKey, _T("InstallSource"));
		PRU_RegCloseKey(hKey);
	}
}

static void WindowsInstallerModifyAction(CGatherer* pGatherer, void* pvKey)
{
	PRU_LaunchProcess(NULL, (LPTSTR)pGatherer->GetItemData(pvKey, 9));
}

static void WindowsInstallerUninstallAction(CGatherer* pGatherer, void* pvKey)
{
	PRU_LaunchProcess(NULL, (LPTSTR)pGatherer->GetItemData(pvKey, 11));
}

static CString WindowsInstallerIDToGUID(LPCTSTR szWIID)
{
	CString String;
	String.Format(_T("%C%C%C%C%C%C%C%C-%C%C%C%C-%C%C%C%C-%C%C%C%C-%C%C%C%C%C%C%C%C%C%C%C%C"),
			szWIID[7], szWIID[6], szWIID[5], szWIID[4], szWIID[3], szWIID[2], szWIID[1], szWIID[0],
			szWIID[11], szWIID[10], szWIID[9], szWIID[8],
			szWIID[15], szWIID[14], szWIID[13], szWIID[12],
			szWIID[17], szWIID[16], szWIID[19], szWIID[18], 
			szWIID[21], szWIID[20], szWIID[23], szWIID[22], 
			szWIID[25], szWIID[24], szWIID[27], szWIID[26], 
			szWIID[29], szWIID[28], szWIID[31], szWIID[30]);
	return String;
}

static void WindowsInstallerPatchUninstallBuildAction(CGatherer* pGatherer, void* pvKey)
{
	CString String = _T("msiexec.exe /package {");
	String += WindowsInstallerIDToGUID((LPTSTR)pGatherer->GetItemData(pvKey, 2));
	String += _T("} /uninstall {");
	String += WindowsInstallerIDToGUID((LPTSTR)pGatherer->GetItemData(pvKey, 4));
	String += _T("} /qb+ REBOOTPROMPT=\"\"");
	PRU_LaunchProcess(NULL, (LPTSTR)String.GetString());
}

static void WindowsInstallerPackageInstallAction(CGatherer* pGatherer, void* pvKey)
{
	LPTSTR pStr = (LPTSTR)pGatherer->GetItemData(pvKey, 8);
	CString Str = _T("msiexec.exe /i ");
	Str += pStr;
	if (pStr) PRU_LaunchProcess(NULL, Str.GetBuffer());
}

static void WindowsInstallerPackageUninstallAction(CGatherer* pGatherer, void* pvKey)
{
	LPTSTR pStr = (LPTSTR)pGatherer->GetItemData(pvKey, 8);
	CString Str = _T("msiexec.exe /x ");
	Str += pStr;
	if (pStr) PRU_LaunchProcess(NULL, Str.GetBuffer());
}

#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>

class CExplicitOwnership : public CGatherer
{
public:
	DECLARE_DYNCREATE(CExplicitOwnership)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_ExplicitOwners[(DWORD_PTR)pvKey].FileName.GetString();
		case 1:
			return (void*)m_ExplicitOwners[(DWORD_PTR)pvKey].Sid.GetString();
		case 2:
			return (void*)m_ExplicitOwners[(DWORD_PTR)pvKey].Account.GetString();
		default:
			return NULL;
		}
	}
	void RecurseCheckOwners(LPTSTR szPath, BOOL bIsDirectory, PSID pParentSID, BOOL& bHasChange)
	{
		PSID psid = NULL;
		PRU_GetOwner(szPath, SE_FILE_OBJECT, &psid);
		BOOL bChildChange = FALSE;
		INT_PTR iIndex = m_ExplicitOwners.GetCount();
		if (bIsDirectory) {
			CFileFind FindFiles;
			BOOL bFind;
			bFind = FindFiles.FindFile(CString(szPath) + _T("\\*"));
			while (bFind) {
				bFind = FindFiles.FindNextFile();
				if (!FindFiles.IsDots()) RecurseCheckOwners(FindFiles.GetFilePath().GetBuffer(), FindFiles.IsDirectory(), psid, bChildChange);
			}
			FindFiles.Close();
		}
		if (psid) {
			if (bChildChange || !pParentSID || !EqualSid(psid, pParentSID)) {
				if (!pParentSID || !EqualSid(psid, pParentSID))
					bHasChange = TRUE;
				ExplicitOwner NewExplicitOwner;
				LPTSTR pStrSid;
				NewExplicitOwner.FileName = szPath;
				if (ConvertSidToStringSid(psid, &pStrSid)) {
					NewExplicitOwner.Sid = pStrSid;
					PRU_FriendlyNameFromSid(psid, NewExplicitOwner.Account);
					LocalFree(pStrSid);
				}
				m_ExplicitOwners.InsertAt(iIndex, NewExplicitOwner);
			}
			delete [] psid;
		}
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer *pDepGather, CArray<void*>* pDepKeys)
	{
		CString Path;
		DWORD_PTR dwCount;
		BOOL bCheck;
		if (!PRU_GetWindowsDirectory(Path)) return;
		RecurseCheckOwners(Path.GetBuffer(), TRUE, NULL, bCheck);
		for (dwCount = 0; dwCount < (DWORD_PTR)m_ExplicitOwners.GetCount(); dwCount++) {
			pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
		}
	}
	struct ExplicitOwner
	{
		CString FileName;
		CString Sid;
		CString Account;
	};
	CArray<ExplicitOwner, ExplicitOwner&> m_ExplicitOwners;
};

class CExplicitOwnershipReg : public CGatherer
{
public:
	DECLARE_DYNCREATE(CExplicitOwnershipReg)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_ExplicitOwners[(DWORD_PTR)pvKey].RegName.GetString();
		case 1:
			return (void*)m_ExplicitOwners[(DWORD_PTR)pvKey].Sid.GetString();
		case 2:
			return (void*)m_ExplicitOwners[(DWORD_PTR)pvKey].Account.GetString();
		default:
			return NULL;
		}
	}
	void RecurseCheckOwners(LPTSTR szPath, PSID pParentSID, BOOL& bHasChange)
	{
		HKEY hKey;
		PSID psid = NULL;
		CString Path = _T("MACHINE");
		if (_tcslen(szPath)) {
			Path += _T("\\");
			Path += szPath;
		}
		PRU_GetOwner(Path.GetBuffer(), SE_REGISTRY_KEY, &psid);
		BOOL bChildChange = FALSE;
		INT_PTR iIndex = m_ExplicitOwners.GetCount();
		if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, szPath, hKey)) {
			CStringArray Keys;
			if (PRU_RegistryEnumKey(hKey, Keys)) {
				DWORD dwCount;
				CString String;
				for (dwCount = 0; dwCount < (DWORD)Keys.GetCount(); dwCount++) {
					String.Empty();
					if (_tcslen(szPath)) {
						String = szPath;
						String += _T("\\");
					}
					String += Keys[dwCount];
					RecurseCheckOwners(String.GetBuffer(), psid, bChildChange);
				}
			} else {
				AddTraceLog(_T("Failed to enumerate Key=%s\r\n"), szPath);
			}
			PRU_RegCloseKey(hKey);
		}
		if (psid) {
			if (bChildChange || !pParentSID || !EqualSid(psid, pParentSID)) {
				if (!pParentSID || !EqualSid(psid, pParentSID))
					bHasChange = TRUE;
				ExplicitOwner NewExplicitOwner;
				LPTSTR pStrSid;
				NewExplicitOwner.RegName = szPath;
				if (ConvertSidToStringSid(psid, &pStrSid)) {
					NewExplicitOwner.Sid = pStrSid;
					PRU_FriendlyNameFromSid(psid, NewExplicitOwner.Account);
					LocalFree(pStrSid);
				}
				m_ExplicitOwners.InsertAt(iIndex, NewExplicitOwner);
			}
			delete [] psid;
		}
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer *pDepGather, CArray<void*>* pDepKeys)
	{
		DWORD_PTR dwCount;
		BOOL bCheck;
		RecurseCheckOwners(_T(""), NULL, bCheck);
		for (dwCount = 0; dwCount < (DWORD_PTR)m_ExplicitOwners.GetCount(); dwCount++) {
			pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
		}
	}
	struct ExplicitOwner
	{
		CString RegName;
		CString Sid;
		CString Account;
	};
	CArray<ExplicitOwner, ExplicitOwner&> m_ExplicitOwners;
};

static void DefaultOwnershipAction(CGatherer* pGatherer, void* pvKey)
{
	PSID psid = NULL;
	LPTSTR szPath = (LPTSTR)pGatherer->GetItemData(pvKey, 0);
	if (PRU_GetOwner(szPath, SE_FILE_OBJECT, &psid)) {
		PRU_SetPrivilege(SE_RESTORE_NAME, TRUE);
		PRU_TakeOwnership(szPath, SE_FILE_OBJECT, TRUE, psid);
		PRU_SetPrivilege(SE_RESTORE_NAME, FALSE);
		delete [] psid;
	}
}

static void DefaultOwnershipRegAction(CGatherer* pGatherer, void* pvKey)
{
	PSID psid = NULL;
	LPTSTR szPath = (LPTSTR)pGatherer->GetItemData(pvKey, 0);
	CString Path = _T("MACHINE");
	if (!szPath) return;
	if (_tcslen(szPath)) {
		Path += _T("\\");
		Path += (LPTSTR)pGatherer->GetItemData(pvKey, 0);
	}
	if (PRU_GetOwner(Path.GetBuffer(), SE_REGISTRY_KEY, &psid)) {
		PRU_SetPrivilege(SE_RESTORE_NAME, TRUE);
		PRU_TakeOwnership(Path.GetBuffer(), SE_REGISTRY_KEY, TRUE, psid);
		PRU_SetPrivilege(SE_RESTORE_NAME, FALSE);
		delete [] psid;
	}
}

static void WindowsComponentManifestsDeleteAction(CGatherer* pGatherer, void* pvKey)
{
	CString Path;
	if (!PRU_GetWindowsDirectory(Path)) return;
	CString ManifestFile = Path + _T("\\winsxs\\Manifests\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 0) + _T(".manifest");
	CString CatFile = Path + _T("\\winsxs\\Manifests\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 0) + _T(".cat");
	PRU_TakeOwnership(ManifestFile.GetBuffer(), SE_FILE_OBJECT, FALSE);
	PRU_TakeOwnership(CatFile.GetBuffer(), SE_FILE_OBJECT, FALSE);
	PRU_SetPrivilege(SE_RESTORE_NAME, TRUE);
	PRU_DeleteFile(ManifestFile);
	PRU_DeleteFile(CatFile);
	PRU_SetPrivilege(SE_RESTORE_NAME, FALSE);
}

static void WindowsComponentManifestCacheDeleteAction(CGatherer* pGatherer, void* pvKey)
{
	CString Path;
	CStringArray Files;
	DWORD dwCount;
	if (!PRU_GetWindowsDirectory(Path)) return;
	Path += _T("\\winsxs\\ManifestCache\\*");
	PRU_EnumDirectory(Path, FALSE, TRUE, FALSE, Files);
	for (dwCount = 0; dwCount < (DWORD)Files.GetCount(); dwCount++) {
		PRU_TakeOwnership(Files[dwCount].GetBuffer(), SE_FILE_OBJECT, FALSE);
		PRU_SetPrivilege(SE_RESTORE_NAME, TRUE);
		PRU_DeleteFile(Files[dwCount]);
		PRU_SetPrivilege(SE_RESTORE_NAME, FALSE);
	}
}

static void WindowsComponentComponentDeleteAction(CGatherer* pGatherer, void* pvKey)
{
	CString Path;
	if (!PRU_GetWindowsDirectory(Path)) return;
	Path += _T("\\winsxs\\");
	Path += (LPCTSTR)pGatherer->GetItemData(pvKey, 0);
	PRU_TakeOwnership(Path.GetBuffer(), SE_FILE_OBJECT, TRUE);
	PRU_SetPrivilege(SE_RESTORE_NAME, TRUE);
	PRU_DeleteFileTree(Path);
	PRU_SetPrivilege(SE_RESTORE_NAME, FALSE);
}

static void WindowsPackageServicingCacheDeleteAction(CGatherer* pGatherer, void* pvKey)
{
	CString Path;
	if (!PRU_GetWindowsDirectory(Path)) return;
	CString MumFile = Path + _T("\\servicing\\Packages\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 0) + _T(".mum");
	CString CatFile = Path + _T("\\servicing\\Packages\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 0) + _T(".cat");
	PRU_TakeOwnership(MumFile.GetBuffer(), SE_FILE_OBJECT, FALSE);
	PRU_TakeOwnership(CatFile.GetBuffer(), SE_FILE_OBJECT, FALSE);
	PRU_SetPrivilege(SE_RESTORE_NAME, TRUE);
	PRU_DeleteFile(MumFile);
	PRU_DeleteFile(CatFile);
	PRU_SetPrivilege(SE_RESTORE_NAME, FALSE);
}

static void WindowsPackageServicingCacheRemoveAction(CGatherer* pGatherer, void* pvKey)
{
	CString Path;
	if (!PRU_GetWindowsDirectory(Path)) return;
	CString Str = _T("dism.exe /Online /Remove-Package /PackageName:") + Path + _T("\\servicing\\Packages\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 0) + _T(".mum");
	PRU_LaunchProcess(NULL, Str.GetBuffer());
}

static void WindowsPackageServicingCacheAddAction(CGatherer* pGatherer, void* pvKey)
{
	CString Path;
	if (!PRU_GetWindowsDirectory(Path)) return;
	CString Str = _T("dism.exe /Online /Remove-Package /PackageName:") + Path + _T("\\servicing\\Packages\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 0) + _T(".mum");
	PRU_LaunchProcess(NULL, Str.GetBuffer());
}

#define DRIVER_ACTION_VERIFY_GUID _T("F750E6C3-38EE-11D1-85E5-00C04FC295EE")

class CInfFiles : public CGatherer
{
public:
	DECLARE_DYNCREATE(CInfFiles)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return &m_InfInfos[(DWORD_PTR)pvKey].Index;
		case 1:
			return (void*)m_InfInfos[(DWORD_PTR)pvKey].FileName.GetString();
		case 2:
			return (void*)m_InfInfos[(DWORD_PTR)pvKey].ClassName.GetString();
		case 3:
			return (void*)m_InfInfos[(DWORD_PTR)pvKey].ClassGUID.GetString();
		case 4:
			return (void*)m_InfInfos[(DWORD_PTR)pvKey].Version.GetString();
		case 5:
			return (void*)m_InfInfos[(DWORD_PTR)pvKey].VersionDate.GetString();
		case 6:
			return (void*)m_InfInfos[(DWORD_PTR)pvKey].Provider.GetString();
		case 7:
			return (void*)m_InfInfos[(DWORD_PTR)pvKey].Catalog.GetString();
		case 8:
			return &m_InfInfos[(DWORD_PTR)pvKey].dwNumberInfs;
		case 9:
			return &m_InfInfos[(DWORD_PTR)pvKey].Reference;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		CAPISetup SetupAPI;
		DWORD dwIndex;
		DWORD dwReqSize;
		CString Path;
		if (!PRU_GetWindowsDirectory(Path)) return;
		CStringArray OemStrings;
		PRU_EnumDirectory(Path + _T("\\system32\\catroot\\{") DRIVER_ACTION_VERIFY_GUID _T("}\\oem*.cat"), FALSE, TRUE, TRUE, OemStrings);
		PRU_EnumDirectory(Path + + _T("\\inf\\oem*.pnf"), FALSE, TRUE, TRUE, OemStrings);
		PRU_EnumDirectory(Path + + _T("\\inf\\oem*.inf"), FALSE, TRUE, TRUE, OemStrings);
		EliminateDupStrings(OemStrings, TRUE);
		for (dwIndex = 0; dwIndex < (DWORD)OemStrings.GetCount(); dwIndex++) {
			InfInfo NewInfInfo;
			NewInfInfo.Index = ~0UL;
			_stscanf(OemStrings[dwIndex].Mid(3), _T("%lu"), &NewInfInfo.Index);// == 1;
			NewInfInfo.FileName = OemStrings[dwIndex];
			NewInfInfo.dwNumberInfs = 0;
			if (SetupAPI.Get_SetupGetInfInformation()(OemStrings[dwIndex] + _T(".inf"), INFINFO_DEFAULT_SEARCH, NULL, 0, &dwReqSize)) {
				PSP_INF_INFORMATION pBuf = (PSP_INF_INFORMATION)new BYTE[dwReqSize];
				if (pBuf) {
					SetupAPI.Get_SetupGetInfInformation()(OemStrings[dwIndex] + _T(".inf"), INFINFO_DEFAULT_SEARCH, pBuf, dwReqSize, &dwReqSize);
					NewInfInfo.dwNumberInfs = pBuf->InfCount; //only reading the first one here
					if (SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("Class"), NULL, 0, &dwReqSize)) {
						SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("Class"), NewInfInfo.ClassName.GetBuffer(dwReqSize), dwReqSize, &dwReqSize);
						NewInfInfo.ClassName.ReleaseBuffer();
					}
					if (SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("ClassGUID"), NULL, 0, &dwReqSize)) {
						SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("ClassGUID"), NewInfInfo.ClassGUID.GetBuffer(dwReqSize), dwReqSize, &dwReqSize);
						NewInfInfo.ClassGUID.ReleaseBuffer();
					}
					if (SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("DriverVer"), NULL, 0, &dwReqSize)) {
						SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("DriverVer"), NewInfInfo.VersionDate.GetBuffer(dwReqSize), dwReqSize, &dwReqSize);
						NewInfInfo.VersionDate.ReleaseBuffer();
					}
					if (SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("Provider"), NULL, 0, &dwReqSize)) {
						SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("Provider"), NewInfInfo.Provider.GetBuffer(dwReqSize), dwReqSize, &dwReqSize);
						NewInfInfo.Provider.ReleaseBuffer();
					}
					if (SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("CatalogFile"), NULL, 0, &dwReqSize)) {
						SetupAPI.Get_SetupQueryInfVersionInformation()(pBuf, 0, _T("CatalogFile"), NewInfInfo.Catalog.GetBuffer(dwReqSize), dwReqSize, &dwReqSize);
						NewInfInfo.Catalog.ReleaseBuffer();
					}
					delete [] pBuf;
				}
				INFCONTEXT context;
				HINF hInf;
				if (hInf = SetupAPI.Get_SetupOpenInfFile()(OemStrings[dwIndex] + _T(".inf"), NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL)) {
					if (SetupAPI.Get_SetupFindFirstLine()(hInf, _T("Version"), _T("DriverVer"), &context) &&
						SetupAPI.Get_SetupGetStringField()(&context, 2, NULL, 0, &dwReqSize)) {
						SetupAPI.Get_SetupGetStringField()(&context, 2, NewInfInfo.Version.GetBuffer(dwReqSize), dwReqSize, &dwReqSize);
						NewInfInfo.Version.ReleaseBuffer();
					}
					SetupAPI.Get_SetupCloseInfFile()(hInf);
				}
			}
			//Collect all references to inf
			CString Str;
			DWORD dwCount;
			HKEY hKey;
			if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_CLASS_NT) + _T("\\") + NewInfInfo.ClassGUID, hKey)) {
				PRU_ReadRegistryString(hKey, REGSTR_VAL_CLASSDESC, Str);
				if (Str.Mid(1, Str.Find(',') - 1).CompareNoCase(OemStrings[dwIndex] + _T(".inf")) == 0) NewInfInfo.Reference.Add(_T("ClassDesc"));
				PRU_RegCloseKey(hKey);
			}
			CStringArray Entries;
			if (PRU_RegistryEnumKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_CLASS_NT) + _T("\\") + NewInfInfo.ClassGUID, Entries)) {
				for (dwCount = 0; dwCount < (DWORD)Entries.GetCount(); dwCount++) {
					if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_CLASS_NT) + _T("\\") + NewInfInfo.ClassGUID + _T("\\") + Entries[dwCount], hKey)) {
						PRU_ReadRegistryString(hKey, REGSTR_VAL_INFPATH, Str);
						if (Str.CompareNoCase(OemStrings[dwIndex] + _T(".inf")) == 0) NewInfInfo.Reference.Add(Entries[dwCount] + _T("\\InfPath"));
						PRU_RegCloseKey(hKey);
					}
				}
			}
			Entries.RemoveAll();
			if (PRU_RegistryEnumKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_CURRENT_CONTROL_SET) + _T("\\Network\\") + NewInfInfo.ClassGUID, Entries)) {
				for (dwCount = 0; dwCount < (DWORD)Entries.GetCount(); dwCount++) {
					if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_CURRENT_CONTROL_SET) + _T("\\Network\\") + NewInfInfo.ClassGUID + _T("\\") + Entries[dwCount], hKey)) {
						PRU_ReadRegistryString(hKey, REGSTR_VAL_INFPATH, Str);
						if (Str.CompareNoCase(OemStrings[dwIndex] + _T(".inf")) == 0) NewInfInfo.Reference.Add(_T("Network\\") + NewInfInfo.ClassGUID+ _T("\\") + Entries[dwCount] + _T("\\InfPath"));
						PRU_ReadRegistryString(hKey, _T("LocDescription"), Str);
						if (Str.Mid(1, Str.Find(',') - 1).CompareNoCase(OemStrings[dwIndex] + _T(".inf")) == 0) NewInfInfo.Reference.Add(_T("Network\\") + NewInfInfo.ClassGUID+ _T("\\") + Entries[dwCount] + _T("\\LocDescription"));
						PRU_RegCloseKey(hKey);
					}
				}
			}
			Entries.RemoveAll();
			if (PRU_RegistryEnumKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_CURRENT_CONTROL_SET) + _T("\\Video"), Entries)) {
				for (dwCount = 0; dwCount < (DWORD)Entries.GetCount(); dwCount++) {
					CStringArray VersionEntries;
					DWORD dwSubCount;
					if (PRU_RegistryEnumKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_CURRENT_CONTROL_SET) + _T("\\Video\\") + Entries[dwCount], VersionEntries)) {
						for (dwSubCount = 0; dwSubCount < (DWORD)VersionEntries.GetCount(); dwSubCount++) {
							if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_CURRENT_CONTROL_SET) + _T("\\Video\\") + Entries[dwCount] + _T("\\") + VersionEntries[dwSubCount], hKey)) {
								PRU_ReadRegistryString(hKey, REGSTR_VAL_INFPATH, Str);
								if (Str.CompareNoCase(OemStrings[dwIndex] + _T(".inf")) == 0) NewInfInfo.Reference.Add(_T("Video\\") + Entries[dwCount] + _T("\\") + VersionEntries[dwSubCount] + _T("\\InfPath"));
								PRU_RegCloseKey(hKey);
							}
						}
					}
				}
			}
			if (PRU_RegistryEnumKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_ENVIRONMENTS, Entries)) {
				for (dwCount = 0; dwCount < (DWORD)Entries.GetCount(); dwCount++) {
					CStringArray VersionEntries;
					DWORD dwSubCount;
					if (PRU_RegistryEnumKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_ENVIRONMENTS) + _T("\\") + Entries[dwCount] + REGSTR_KEY_DRIVERS, VersionEntries)) {
						for (dwSubCount = 0; dwSubCount < (DWORD)VersionEntries.GetCount(); dwSubCount++) {
							CStringArray DriverEntries;
							DWORD dwDrvCount;
							if (PRU_RegistryEnumKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_ENVIRONMENTS) + _T("\\") + Entries[dwCount] + REGSTR_KEY_DRIVERS + _T("\\") + VersionEntries[dwSubCount], DriverEntries)) {
								for (dwDrvCount = 0; dwDrvCount < (DWORD)DriverEntries.GetCount(); dwDrvCount++) {
									if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, CString(REGSTR_PATH_ENVIRONMENTS) + _T("\\") + Entries[dwCount] + REGSTR_KEY_DRIVERS + _T("\\") + VersionEntries[dwSubCount] + _T("\\") + DriverEntries[dwDrvCount], hKey)) {
										PRU_ReadRegistryString(hKey, _T("InfPath"), Str);
										if (Str.CompareNoCase(OemStrings[dwIndex] + _T(".inf")) == 0) NewInfInfo.Reference.Add(_T("Print\\Environments\\") + Entries[dwCount] + REGSTR_KEY_DRIVERS + _T("\\") + VersionEntries[dwSubCount] + _T("\\") + DriverEntries[dwDrvCount] + _T("\\InfPath"));
										PRU_RegCloseKey(hKey);
									}
								}
							}
						}
					}
				}
			}
			m_InfInfos.Add(NewInfInfo);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_InfInfos.GetCount() - 1));
		}
	}
private:
	struct InfInfo {
		DWORD Index;
		DWORD dwNumberInfs;
		CString FileName;
		CString ClassName;
		CString ClassGUID;
		CString Version;
		CString VersionDate;
		CString Provider;
		CString Catalog;
		CCopyStringArray Reference;
	};
	CArray<InfInfo, InfInfo&> m_InfInfos;
};

static void InfFilesUninstallInfAction(CGatherer* pGatherer, void* pvKey)
{
	CAPISetup SetupAPI;
	//E000023C is ERROR_NOT_AN_INSTALLED_OEM_INF
	if (!SetupAPI.Get_SetupUninstallOEMInf()(CString((TCHAR*)pGatherer->GetItemData(pvKey, 1)) + _T(".inf"), 0, NULL)) {
		AddTraceLog(_T("APICall=SetupUninstallOEMInf InfFileName=%s Error=%08X\r\n"), (TCHAR*)pGatherer->GetItemData(pvKey, 1), GetLastError());
	}
}

static void InfFilesForceUninstallInfAction(CGatherer* pGatherer, void* pvKey)
{
	CAPISetup SetupAPI;
	//E000023C is ERROR_NOT_AN_INSTALLED_OEM_INF
	if (!SetupAPI.Get_SetupUninstallOEMInf()(CString((TCHAR*)pGatherer->GetItemData(pvKey, 1)) + _T(".inf"), SUOI_FORCEDELETE, NULL)) {
		AddTraceLog(_T("APICall=SetupUninstallOEMInf InfFileName=%s Error=%08X\r\n"), (TCHAR*)pGatherer->GetItemData(pvKey, 1), GetLastError());
	}
}

static void InfFilesOldUninstallInfAction(CGatherer* pGatherer, void* pvKey)
{
	CString Path;
	if (!PRU_GetWindowsDirectory(Path)) return;
	//Windows 2000 or XP and greater force delete
	PRU_DeleteFile(Path + _T("\\inf\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 1) + _T(".inf"));
	PRU_DeleteFile(Path + _T("\\inf\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 1) + _T(".pnf"));
	//Windows XP and greater
	Path = Path + _T("\\system32\\catroot\\{") DRIVER_ACTION_VERIFY_GUID _T("}\\") + (LPCTSTR)pGatherer->GetItemData(pvKey, 1) + _T(".cat");
	CFileStatus rStatus;
	CFile::GetStatus(Path, rStatus);
	rStatus.m_attribute &= ~CFile::system; //turn off system attribute
	CFile::SetStatus(Path, rStatus);
	PRU_DeleteFile(Path);
}

class CDevices : public CGatherer
{
public:
	DECLARE_DYNCREATE(CDevices)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].FriendlyName.GetString();
		case 1:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].DeviceDesc.GetString();
		case 2:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].Mfg.GetString();
		case 3:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].Driver.GetString();
		case 4:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].Service.GetString();
		case 5:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].Class.GetString();
		case 6:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].ClassGUID.GetString();
		case 7:
			return (void*)&m_DevInfos[(DWORD_PTR)pvKey].HardwareIDs;
		case 8:
			return (void*)&m_DevInfos[(DWORD_PTR)pvKey].CompatibleIDs;
		case 9:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].LocationInfo.GetString();
		case 10:
			return (void*)m_DevInfos[(DWORD_PTR)pvKey].EnumeratorName.GetString();
		case 11:
			return (void*)&m_DevInfos[(DWORD_PTR)pvKey].dwConfigFlags;
		case 12:
			return (void*)&m_DevInfos[(DWORD_PTR)pvKey].dwCapabilities;
		case 13:
			return (void*)&m_DevInfos[(DWORD_PTR)pvKey].dwDevType;
		case 14:
			return (void*)&m_DevInfos[(DWORD_PTR)pvKey].dwInstallState;
		case 15:
			return (void*)&m_DevInfos[(DWORD_PTR)pvKey].ConfigRet;
		case 16:
			return (void*)&m_DevInfos[(DWORD_PTR)pvKey].DevInst;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		CAPISetup SetupAPI;
		DWORD_PTR dwIndex;
		HDEVINFO hDevInfo = SetupAPI.Get_SetupDiGetClassDevs()(NULL, NULL, NULL, DIGCF_ALLCLASSES); //DIGCF_PROFILE
		if (hDevInfo != INVALID_HANDLE_VALUE) {
			SP_DEVINFO_DATA spDevInfoData;
			dwIndex = 0;
			spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
			while (SetupAPI.Get_SetupDiEnumDeviceInfo()(hDevInfo, (DWORD)dwIndex, &spDevInfoData)) {
				DevInfo NewDevInfo;
				NewDevInfo.dwConfigFlags = 0;
				NewDevInfo.dwDevType = 0;
				NewDevInfo.dwInstallState = ~0UL;
				NewDevInfo.DevInst = spDevInfoData.DevInst;
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_FRIENDLYNAME, NewDevInfo.FriendlyName);
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_DEVICEDESC, NewDevInfo.DeviceDesc);
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_MFG, NewDevInfo.Mfg);
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_DRIVER, NewDevInfo.Driver);
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_SERVICE, NewDevInfo.Service);
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_CLASS, NewDevInfo.Class);
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_CLASSGUID, NewDevInfo.ClassGUID);
				PRU_GetDeviceRegistryPropertyMultiString(hDevInfo, &spDevInfoData, SPDRP_HARDWAREID, NewDevInfo.HardwareIDs);
				PRU_GetDeviceRegistryPropertyMultiString(hDevInfo, &spDevInfoData, SPDRP_COMPATIBLEIDS, NewDevInfo.CompatibleIDs);
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_LOCATION_INFORMATION, NewDevInfo.LocationInfo);
				PRU_GetDeviceRegistryPropertyString(hDevInfo, &spDevInfoData, SPDRP_ENUMERATOR_NAME, NewDevInfo.EnumeratorName);
				PRU_GetDeviceRegistryPropertyDWORD(hDevInfo, &spDevInfoData, SPDRP_CONFIGFLAGS, NewDevInfo.dwConfigFlags);
				PRU_GetDeviceRegistryPropertyDWORD(hDevInfo, &spDevInfoData, SPDRP_CAPABILITIES, NewDevInfo.dwCapabilities);
				PRU_GetDeviceRegistryPropertyDWORD(hDevInfo, &spDevInfoData, SPDRP_DEVTYPE, NewDevInfo.dwDevType);
				//default value if cannot be retrieved is installed...
				PRU_GetDeviceRegistryPropertyDWORD(hDevInfo, &spDevInfoData, SPDRP_INSTALL_STATE, NewDevInfo.dwInstallState);
				ULONG ulStatus;
				ULONG ulPBMNumber;
				NewDevInfo.ConfigRet = SetupAPI.Get_CM_Get_DevNode_Status()(&ulStatus, &ulPBMNumber, spDevInfoData.DevInst, 0);
				m_DevInfos.Add(NewDevInfo);
				pModel->OnItemNew(GetRuntimeClass(), (void*)dwIndex);
				dwIndex++;
			}
			SetupAPI.Get_SetupDiDestroyDeviceInfoList()(hDevInfo);
		}
	}
private:
	struct DevInfo
	{
		CString FriendlyName;
		CString DeviceDesc;
		CString Mfg;
		CString Driver;
		CString Service;
		CString Class;
		CString ClassGUID;
		CCopyStringArray HardwareIDs;
		CCopyStringArray CompatibleIDs;
		CString LocationInfo;
		CString EnumeratorName;
		DWORD dwConfigFlags;
		DWORD dwCapabilities;
		DWORD dwDevType;
		DWORD dwInstallState;
		CONFIGRET ConfigRet;
		DWORD DevInst;
	};
	CArray<DevInfo, DevInfo&> m_DevInfos;
};

#define INITGUID
#include <Devpropdef.h>
#include <Devpkey.h>

#define DI_NOSYNCPROCESSING		0x00400000L

static void DevicesRemoveAction(CGatherer* pGatherer, void* pvKey)
{
	CAPISetup SetupAPI;
	DWORD dwIndex;
	DWORD DevInst = *(DWORD*)pGatherer->GetItemData(pvKey, 16);
	HDEVINFO hDevInfo = SetupAPI.Get_SetupDiGetClassDevs()(NULL, NULL, NULL, DIGCF_ALLCLASSES); //DIGCF_PROFILE
	if (hDevInfo != INVALID_HANDLE_VALUE) {
		SP_DEVINFO_DATA spDevInfoData;
		CStringArray HardwareIDs;
		dwIndex = 0;
		spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		while (SetupAPI.Get_SetupDiEnumDeviceInfo()(hDevInfo, dwIndex, &spDevInfoData)) {
			HardwareIDs.RemoveAll();
			PRU_GetDeviceRegistryPropertyMultiString(hDevInfo, &spDevInfoData, SPDRP_HARDWAREID, HardwareIDs);
			if (DevInst == spDevInfoData.DevInst) {
				//< 0x0600 or WOW64 method
				BOOL bIsWow64 = FALSE;
				if (IsWow64Process(GetModuleHandle(NULL), &bIsWow64) && bIsWow64) {
					if (!SetupAPI.Get_SetupDiRemoveDevice()(hDevInfo, &spDevInfoData)) {
						AddTraceLog(_T("APICall=SetupDiRemoveDevice DeviceHandle=%08X Error=%08X\r\n"), DevInst, GetLastError());
					}
				} else {
					//>= 0x0600 method
					CStringW InfPath;
					DEVPROPTYPE ulDevPropType;
					if (SetupAPI.Get_SetupDiGetDevicePropertyW()(hDevInfo, &spDevInfoData, &DEVPKEY_Device_DriverInfPath, &ulDevPropType, (PBYTE)InfPath.GetBuffer(MAX_PATH + 1), MAX_PATH + 1, NULL, 0) && ulDevPropType == DEVPROP_TYPE_STRING) {
						InfPath.ReleaseBuffer();
						if (!InfPath.IsEmpty()) {
							ULONG ulIsInbox = TRUE; //check if shipped with Windows
							//>= 0x0601
							if (SetupAPI.Get_pSetupInfIsInbox()(InfPath, &ulIsInbox) && !ulIsInbox &&
								MessageBox(NULL, _T("Delete the driver software for this device?"), _T("Confirm Device Uninstall"), MB_YESNO) == IDYES) {
								SetupAPI.Get_SetupUninstallOEMInf()(CW2T(InfPath), SUOI_FORCEDELETE, NULL);
							}
						}
					}
					SP_DEVINSTALL_PARAMS spDevInstallParams = { sizeof(SP_DEVINSTALL_PARAMS) };
					SetupAPI.Get_SetupDiGetDeviceInstallParams()(hDevInfo, &spDevInfoData, &spDevInstallParams);
					struct _SpClassInstallHeader { SP_CLASSINSTALL_HEADER SpClass; DI_FUNCTION NextFunction; DI_FUNCTION EmptyFunction; } SpClassInstallHeader = { { sizeof(SP_CLASSINSTALL_HEADER), DIF_REMOVE }, DIF_SELECTDEVICE, 0 };
					if (!SetupAPI.Get_SetupDiSetClassInstallParams()(hDevInfo, &spDevInfoData, &SpClassInstallHeader.SpClass, sizeof(SpClassInstallHeader))) {
						AddTraceLog(_T("APICall=SetupDiSetClassInstallParams DeviceHandle=%08X Error=%08X\r\n"), DevInst, GetLastError());
					}
					//must call from 64-bit native process
					if (!SetupAPI.Get_SetupDiCallClassInstaller()(DIF_REMOVE, hDevInfo, &spDevInfoData)) {
						AddTraceLog(_T("APICall=SetupDiCallClassInstaller DeviceHandle=%08X Error=%08X\r\n"), DevInst, GetLastError());
					}
					if (!SetupAPI.Get_SetupDiSetClassInstallParams()(hDevInfo, &spDevInfoData, NULL, 0)) {
						AddTraceLog(_T("APICall=SetupDiSetClassInstallParams DeviceHandle=%08X Error=%08X\r\n"), DevInst, GetLastError());
					}
					//Offer restart
					BOOL bSync = FALSE;
					BOOL bRestart = FALSE;
					if (spDevInstallParams.Flags & (DI_NEEDREBOOT | DI_NEEDRESTART)) {
						if (((spDevInstallParams.Flags & DI_NEEDRESTART) == 0) && (spDevInstallParams.Flags & DI_NOSYNCPROCESSING)) {
							bSync = TRUE;
						}
						bRestart = TRUE;
					}
					if (bRestart) RestartDialogEx(NULL, L"", bSync ? EWX_SHUTDOWN : EWX_REBOOT,
												SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION);
					break;
				}
			}
			dwIndex++;
		}
		//ERROR_NO_MORE_ITEMS
		SetupAPI.Get_SetupDiDestroyDeviceInfoList()(hDevInfo);
	}
}

class CKernelDeviceList : public CGatherer
{
public:
	DECLARE_DYNCREATE(CKernelDeviceList)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_DeviceObjNames[(DWORD_PTR)pvKey].GetString();
		case 1:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].pdo;
		case 2:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].Size;
		case 3:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].Type;
		case 4:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].ReferenceCount;
		case 5:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].DriverObject;
		case 6:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].NextDevice;
		case 7:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].AttachedDevice;
		case 8:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].CurrentIrp;
		case 9:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].Timer;
		case 10:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].Flags;
		case 11:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].Characteristics;
		case 12:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].Vpb;
		case 13:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].DeviceExtension;
		case 14:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].DeviceType;
		case 15:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].StackSize;
		case 16:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].AlignmentRequirement;
		case 17:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].ActiveThreadCount;
		case 18:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].SecurityDescriptor;
		case 19:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].SectorSize;
		case 20:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].Spare1;
		case 21:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].DeviceObjectExtension;
		case 22:
			return (void*)&m_DeviceObjects[(DWORD_PTR)pvKey].Reserved;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		DWORD_PTR dwCount;
		CByteArray ImageBytes;
		PRU_GetAllTypeObjects(m_DeviceObjNames, L"Device");
		CDriverUse Use;
		for (dwCount = 0; dwCount < (DWORD_PTR)m_DeviceObjNames.GetCount(); dwCount++) {
			DEVICE_RECEIVE_OBJECT dro = { NULL };
			if (DriverQuery(IOCTL_FOI_READDEVICEOBJECT, m_DeviceObjNames[dwCount].GetBuffer(), (m_DeviceObjNames[dwCount].GetLength() + 1) * sizeof(WCHAR), &dro, sizeof(DEVICE_RECEIVE_OBJECT))) {
				m_DeviceObjects.Add(dro);
			} else m_DeviceObjects.Add(dro);
			pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
		}
	}
private:
	CStringArray m_DeviceObjNames;
	CArray<DEVICE_RECEIVE_OBJECT, DEVICE_RECEIVE_OBJECT&> m_DeviceObjects;
};

#define PAGE_SIZE 4096

class CKernelModuleList : public CGatherer
{
public:
	DECLARE_DYNCREATE(CKernelModuleList)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)&m_DriverSections[(DWORD_PTR)pvKey];
		case 1:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].DllBase;
		case 2:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].EntryPoint;
		case 3:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].SizeOfImage;
		case 4:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].Flags;
		case 5:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].LoadCount;
		case 6:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].TlsIndex;
		case 7:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].SectionPointer;
		case 8:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].CheckSum;
		case 9:
			return (void*)&m_ModuleTableEntries[(DWORD_PTR)pvKey].LoadedImports;
		case 10:
			return (void*)m_ModuleTableFullNameEntries[(DWORD_PTR)pvKey].GetString();
		case 11:
			return (void*)m_ModuleTableBaseNameEntries[(DWORD_PTR)pvKey].GetString();
		case 12:
			return (void*)&m_ModuleMatches[(DWORD_PTR)pvKey];
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		DWORD_PTR dwCount;
		CByteArray ImageBytes;
		CDriverUse Use;
		ImageBytes.SetSize(sizeof(ULONG));
		do {
			if (!DriverQuery(IOCTL_FOI_READMODULELIST, NULL, 0, ImageBytes.GetData(), (DWORD)(DWORD_PTR)ImageBytes.GetSize())) {
				break;
			}
			if (*((ULONG*)ImageBytes.GetData()) <= (ImageBytes.GetSize() - sizeof(ULONG)) / sizeof(PVOID)) {
				m_DriverSections.SetSize(*((ULONG*)ImageBytes.GetData()));
				memcpy(m_DriverSections.GetData(), ImageBytes.GetData() + sizeof(ULONG), m_DriverSections.GetSize() * sizeof(PVOID));
				break;
			} else {
				ImageBytes.SetSize(sizeof(ULONG) + *((ULONG*)ImageBytes.GetData()) * sizeof(PVOID));
			}
		} while (TRUE);
		for (dwCount = 0; dwCount < (DWORD_PTR)m_DriverSections.GetCount(); dwCount++) {
			CStringW Str;
			if (DriverQueryUnicodeString(IOCTL_FOI_READMODULEFULLNAME, &m_DriverSections[dwCount], sizeof(PVOID), Str)) {
			}
			m_ModuleTableFullNameEntries.Add(CW2T(Str));
			if (DriverQueryUnicodeString(IOCTL_FOI_READMODULEBASENAME, &m_DriverSections[dwCount], sizeof(PVOID), Str)) {
			}
			m_ModuleTableBaseNameEntries.Add(CW2T(Str));
			LDR_DATA_TABLE_RECEIVE_ENTRY ldtre = { NULL };
			if (DriverQuery(IOCTL_FOI_READMODULESECTION, &m_DriverSections[dwCount], sizeof(PVOID), &ldtre, sizeof(ldtre))) {
			}
			m_ModuleTableEntries.Add(ldtre);
			m_ModuleMatches.Add(FALSE); //VerifyImage(&m_ModuleTableEntries[dwCount].DllBase, m_ModuleTableEntries[dwCount].SizeOfImage, m_ModuleTableBaseNameEntries[dwCount], m_ModuleTableFullNameEntries[dwCount]));
			pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
		}
	}
	static BOOL VerifyImage(PVOID* pDllBase, ULONG ulSizeOfImage, LPCTSTR szBaseName, LPCTSTR szFullName, BOOL bWrite = FALSE)
	{
		BOOL bMatch = FALSE;
		CByteArray ImageBytes;
		ImageBytes.RemoveAll();
		ImageBytes.SetSize(sizeof(IMAGE_DOS_HEADER));
		if (DriverQuery(IOCTL_FOI_COPYNONPAGEDMEMORY, pDllBase, sizeof(PVOID), ImageBytes.GetData(), (DWORD)(DWORD_PTR)ImageBytes.GetSize()) &&
			((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_magic == IMAGE_DOS_SIGNATURE) {
			ImageBytes.SetSize(((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader));
			PVOID pvAddress = (PUCHAR)*pDllBase + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew;
			if (DriverQuery(IOCTL_FOI_COPYNONPAGEDMEMORY, &pvAddress, sizeof(PVOID), ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew, FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader))) {
				DWORD dwSecCount = ((PIMAGE_NT_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->FileHeader.NumberOfSections;
				ImageBytes.SetSize(((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + ((PIMAGE_NT_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_SECTION_HEADER) * dwSecCount);
				pvAddress = (PUCHAR)*pDllBase + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader);
				if (DriverQuery(IOCTL_FOI_COPYNONPAGEDMEMORY, &pvAddress, sizeof(PVOID), ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader), ((PIMAGE_NT_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_SECTION_HEADER) * dwSecCount)) {
					if (((PIMAGE_NT_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
						ImageBytes.SetSize(((PIMAGE_NT_HEADERS64)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->OptionalHeader.SizeOfImage);
					} else if (((PIMAGE_NT_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
						ImageBytes.SetSize(((PIMAGE_NT_HEADERS32)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->OptionalHeader.SizeOfImage);
					} else if (((PIMAGE_NT_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->OptionalHeader.Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
						//ImageBytes.SetSize(((PIMAGE_ROM_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->OptionalHeader);
					}
					PIMAGE_SECTION_HEADER pSectionTable = IMAGE_FIRST_SECTION((IMAGE_NT_HEADERS*)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew));
					DWORD dwSubCount;
					for (dwSubCount = 0; dwSubCount < dwSecCount; dwSubCount++) {
						//discarded initialization sections begin with 'INIT' or '.eda' for .edata
						if ((pSectionTable[dwSubCount].Name[0] != 'I' || pSectionTable[dwSubCount].Name[1] != 'N' ||
							pSectionTable[dwSubCount].Name[2] != 'I' || pSectionTable[dwSubCount].Name[3] != 'T') &&
							(pSectionTable[dwSubCount].Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0 &&
							pSectionTable[dwSubCount].SizeOfRawData != 0) {
							pvAddress = (PUCHAR)*pDllBase + pSectionTable[dwSubCount].VirtualAddress;
							CFile File;
							if (File.Open(_T("..\\..\\Output\\CurSec.txt"), CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate)) {
								CString String;
#ifdef WIN64
								String.Format(_T("Module: %s Section Name: %.8hs Dll Base: %016llX Virtual Address:%08X Physical Size: %08X Characteristics: %08X\r\n"), szBaseName, pSectionTable[dwSubCount].Name, *pDllBase, pSectionTable[dwSubCount].VirtualAddress, pSectionTable[dwSubCount].SizeOfRawData, pSectionTable[dwSubCount].Characteristics);
#else
								String.Format(_T("Module: %s Section Name: %.8hs Dll Base: %08X Virtual Address:%08X Physical Size: %08X Characteristics: %08X\r\n"), szBaseName, pSectionTable[dwSubCount].Name, *pDllBase, pSectionTable[dwSubCount].VirtualAddress, pSectionTable[dwSubCount].SizeOfRawData, pSectionTable[dwSubCount].Characteristics);
#endif
								File.SeekToEnd();
								File.Write(String.GetString(), String.GetLength() * sizeof(TCHAR));
								File.Flush();
								File.Close();
							}
							//pageable if begins with 'PAGE' or is set in MmPageEntireDriver
							//DisablePagingExecutive setting should override it
							//NTOSKRNL.exe has special settings for its 'PAGE' sections and reading it will cause a PAGE_FAULT_IN_NON_PAGED_AREA
							//many are not marked as discardable but discarded based on the initialization, driver verifier, kernel debugger, etc
							//its dependencies pshed.dll, hal.dll, kdcom.dll, clfs.dll, ci.dll may also be unsafe but probably not since not drivers probably not need init code?
							//IMAGE_SCN_MEM_NOT_PAGED set for sections not described above
							if ((pSectionTable[dwSubCount].Name[0] == 'P' && pSectionTable[dwSubCount].Name[1] == 'A' &&
								pSectionTable[dwSubCount].Name[2] == 'G' && pSectionTable[dwSubCount].Name[3] == 'E') ||
								(pSectionTable[dwSubCount].Name[0] == '.' && pSectionTable[dwSubCount].Name[1] == 'e' &&
								pSectionTable[dwSubCount].Name[2] == 'd' && pSectionTable[dwSubCount].Name[3] == 'a')) {
								if (_tcsicmp(szBaseName, _T("ntoskrnl.exe")) &&
									_tcsicmp(szBaseName, _T("hal.dll")) &&
									_tcsicmp(szBaseName, _T("kdcom.dll")) &&
									_tcsicmp(szBaseName, _T("clfs.sys")) &&
									_tcsicmp(szBaseName, _T("ci.dll"))) {
									if ((pSectionTable[dwSubCount].Characteristics & IMAGE_SCN_CNT_CODE) != 0) {
										//if (DriverQuery(IOCTL_FOI_COPYPAGEDMEMORY, &pvAddress, sizeof(PVOID), ImageBytes.GetData() + pSectionTable[dwSubCount].VirtualAddress, pSectionTable[dwSubCount].SizeOfRawData)) {
										//}
									} else {
										//if (DriverQuery(IOCTL_FOI_COPYMEMORY, &pvAddress, sizeof(PVOID), ImageBytes.GetData() + pSectionTable[dwSubCount].VirtualAddress, pSectionTable[dwSubCount].SizeOfRawData)) {
										//}
									}
								}
							} else {
								if ((pSectionTable[dwSubCount].Characteristics & IMAGE_SCN_CNT_CODE) != 0) {
									if (DriverQuery(IOCTL_FOI_COPYNONPAGEDMEMORY, &pvAddress, sizeof(PVOID), ImageBytes.GetData() + pSectionTable[dwSubCount].VirtualAddress, pSectionTable[dwSubCount].SizeOfRawData)) {
									}	
								}
							}
						}
					}
					if (VerifyImageCode(ImageBytes, szFullName, bWrite)) bMatch = TRUE;
				}
			}
		} else {
			ImageBytes.SetSize(ulSizeOfImage);
			if (ImageBytes.GetSize() && DriverQuery(IOCTL_FOI_COPYNONPAGEDMEMORY, pDllBase, sizeof(PVOID), ImageBytes.GetData(), (DWORD)(DWORD_PTR)ImageBytes.GetSize())) {
				if (VerifyImageCode(ImageBytes, szFullName, bWrite)) bMatch = TRUE;
			}
		}
		return bMatch;
	}
	static DWORD LookupSizeOfImageOnDisk(CString ImageFilePath, BOOL bHeaders = FALSE)
	{
		WORD   e_magic;
		LONG   e_lfanew;
		DWORD Signature;
		WORD Magic;
		union {
			IMAGE_NT_HEADERS32 NTHeader32;
			IMAGE_NT_HEADERS64 NTHeader64;
			IMAGE_ROM_HEADERS ROMHeader;
		} NTHeader;
		CFile File;
		if (!File.Open(ResolveNTPath(ImageFilePath), CFile::modeRead)) return 0;
		File.Read(&e_magic, sizeof(e_magic));
		if (e_magic == IMAGE_DOS_SIGNATURE) {
			File.Seek(offsetof(IMAGE_DOS_HEADER, e_lfanew), CFile::begin);
			File.Read(&e_lfanew, sizeof(e_lfanew));
			if (e_lfanew) {
				File.Seek(e_lfanew, CFile::begin);
				File.Read(&Signature, sizeof(Signature));
				if (Signature == IMAGE_NT_SIGNATURE) {
					File.Seek(e_lfanew + offsetof(IMAGE_NT_HEADERS, OptionalHeader), CFile::begin);
					File.Read(&Magic, sizeof(Magic));
					if (Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
						File.Seek(e_lfanew, CFile::begin);
						File.Read(&NTHeader.NTHeader32, sizeof(NTHeader.NTHeader32));
						if (bHeaders) return e_lfanew + sizeof(IMAGE_NT_HEADERS32) + NTHeader.NTHeader32.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
						return NTHeader.NTHeader32.OptionalHeader.SizeOfImage;
					} else if (Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
						File.Seek(e_lfanew, CFile::begin);
						File.Read(&NTHeader.NTHeader64, sizeof(NTHeader.NTHeader64));
						if (bHeaders) return e_lfanew + sizeof(IMAGE_NT_HEADERS64) + NTHeader.NTHeader64.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
						return NTHeader.NTHeader64.OptionalHeader.SizeOfImage;
					} else if (Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
						File.Seek(e_lfanew, CFile::begin);
						File.Read(&NTHeader.ROMHeader, sizeof(NTHeader.ROMHeader));
						if (bHeaders) return e_lfanew + sizeof(IMAGE_ROM_HEADERS) + NTHeader.ROMHeader.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
						return 0;
					}
				}
			}
		}
		return 0;
	}
	static BOOL VerifyImageCode(CByteArray & ImageBytes, CString ImageFilePath, BOOL bWrite)
	{
		WORD   e_magic;
		LONG   e_lfanew;
		DWORD Signature;
		WORD Magic;
		union {
			IMAGE_NT_HEADERS32 NTHeader32;
			IMAGE_NT_HEADERS64 NTHeader64;
			IMAGE_ROM_HEADERS ROMHeader;
		} NTHeader;
		CArray<IMAGE_SECTION_HEADER, IMAGE_SECTION_HEADER> SectionHeaders;
		DWORD dwCount;
		CFile File;
		CFile OutFile;
		if (ImageFilePath.IsEmpty() || !File.Open(ResolveNTPath(ImageFilePath), CFile::modeRead)) {
			if (bWrite) {
				CString Str = ResolveNTPath(ImageFilePath);
				if (Str.IsEmpty()) {
					GetTempFileName(_T("..\\..\\Output\\"), _T("DRV"), 0, Str.GetBuffer(MAX_PATH + 1));
					Str.ReleaseBuffer();
				} else {
					if (Str.ReverseFind('\\') != -1)
						Str = Str.Mid(Str.ReverseFind('\\') + 1);
					Str = _T("..\\..\\Output\\") + Str;
				}
				if (OutFile.Open(Str, CFile::modeWrite | CFile::modeCreate)) {
					if (((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_magic == IMAGE_DOS_SIGNATURE) {
						PIMAGE_SECTION_HEADER pSectionTable = IMAGE_FIRST_SECTION((IMAGE_NT_HEADERS*)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew));
						OutFile.SetLength((((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_cp - (((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_cblp == 0 ? 0 : 1)) * 512 + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_cblp);
						DWORD dwSecCount = ((PIMAGE_NT_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->FileHeader.NumberOfSections;
						OutFile.Write(ImageBytes.GetData(), ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + ((PIMAGE_NT_HEADERS)(ImageBytes.GetData() + ((PIMAGE_DOS_HEADER)ImageBytes.GetData())->e_lfanew))->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_SECTION_HEADER) * dwSecCount);
						for (dwCount = 0; dwCount < dwSecCount; dwCount++) {
							OutFile.Seek(pSectionTable[dwCount].PointerToRawData, CFile::begin);
							OutFile.Write(ImageBytes.GetData() + pSectionTable[dwCount].VirtualAddress, pSectionTable[dwCount].SizeOfRawData);
						}
					} else {
						OutFile.Write(ImageBytes.GetData(), (UINT)ImageBytes.GetSize());
					}
					OutFile.Close();
				}
			}
			return FALSE;
		}
		File.Read(&e_magic, sizeof(e_magic));
		if (e_magic == IMAGE_DOS_SIGNATURE) {
			File.Seek(offsetof(IMAGE_DOS_HEADER, e_lfanew), CFile::begin);
			File.Read(&e_lfanew, sizeof(e_lfanew));
			if (e_lfanew) {
				File.Seek(e_lfanew, CFile::begin);
				File.Read(&Signature, sizeof(Signature));
				if (Signature == IMAGE_NT_SIGNATURE) {
					File.Seek(e_lfanew + offsetof(IMAGE_NT_HEADERS, OptionalHeader), CFile::begin);
					File.Read(&Magic, sizeof(Magic));
					if (Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
						File.Seek(e_lfanew, CFile::begin);
						File.Read(&NTHeader.NTHeader32, sizeof(NTHeader.NTHeader32));
					} else if (Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
						File.Seek(e_lfanew, CFile::begin);
						File.Read(&NTHeader.NTHeader64, sizeof(NTHeader.NTHeader64));
					} else if (Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
						File.Seek(e_lfanew, CFile::begin);
						File.Read(&NTHeader.ROMHeader, sizeof(NTHeader.ROMHeader));
					}
					SectionHeaders.SetSize(NTHeader.NTHeader32.FileHeader.NumberOfSections);
					File.Read(SectionHeaders.GetData(), IMAGE_SIZEOF_SECTION_HEADER * NTHeader.NTHeader32.FileHeader.NumberOfSections);
				}
			}
		}
		BOOL bMatch = TRUE;
		CByteArray AllBytes;
		File.Seek(0, CFile::begin);
		AllBytes.SetSize(File.GetLength());
		File.Read(AllBytes.GetData(), (UINT)File.GetLength());
		File.Close();
		for (dwCount = 0; dwCount < (DWORD)SectionHeaders.GetSize(); dwCount++) {
			if ((SectionHeaders[dwCount].Name[0] != 'I' || SectionHeaders[dwCount].Name[1] != 'N' ||
				SectionHeaders[dwCount].Name[2] != 'I' || SectionHeaders[dwCount].Name[3] != 'T') &&
				(SectionHeaders[dwCount].Characteristics & IMAGE_SCN_CNT_CODE) != 0 &&
				(SectionHeaders[dwCount].Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0 &&
				SectionHeaders[dwCount].SizeOfRawData != 0) {
				if (memcmp(	AllBytes.GetData() + SectionHeaders[dwCount].PointerToRawData,
							ImageBytes.GetData() + SectionHeaders[dwCount].VirtualAddress,
							SectionHeaders[dwCount].SizeOfRawData) != 0) bMatch = FALSE;
				if (bWrite)
					memcpy(	AllBytes.GetData() + SectionHeaders[dwCount].PointerToRawData,
							ImageBytes.GetData() + SectionHeaders[dwCount].VirtualAddress,
							SectionHeaders[dwCount].SizeOfRawData);
			}
		}
		if (bWrite) {
			if (OutFile.Open(CString(_T("..\\..\\Output\\")) + File.GetFileName(), CFile::modeWrite | CFile::modeCreate)) {
				OutFile.Write(AllBytes.GetData(), (UINT)AllBytes.GetSize());
				OutFile.Close();
			}
		}
		return bMatch;
	}
private:
	CArray<LDR_DATA_TABLE_RECEIVE_ENTRY, LDR_DATA_TABLE_RECEIVE_ENTRY&> m_ModuleTableEntries;
	CStringArray m_ModuleTableFullNameEntries;
	CStringArray m_ModuleTableBaseNameEntries;
	CArray<PVOID, PVOID> m_DriverSections;
	CArray<BOOL, BOOL> m_ModuleMatches;
};

static void ModuleDumpAction(CGatherer* pGatherer, void* pvKey)
{
	CKernelModuleList::VerifyImage((PVOID*)pGatherer->GetItemData(pvKey, 1), *(ULONG*)pGatherer->GetItemData(pvKey, 3), (LPCTSTR)pGatherer->GetItemData(pvKey, 11), (LPCTSTR)pGatherer->GetItemData(pvKey, 10), TRUE);
}

class CKernelDriverList : public CGatherer
{
public:
	DECLARE_DYNCREATE(CKernelDriverList)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_DriverObjNames[(DWORD_PTR)pvKey].GetString();
		case 1:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].pdo;
		case 2:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].Size;
		case 3:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].Type;
		case 4:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].DeviceObject;
		case 5:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].Flags;
		case 6:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].DriverStart)->value;
		case 7:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].DriverSize;
		case 8:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].DriverSection;
		case 9:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].DriverExtension;
		case 10:
			return (void*)&m_DriverObjects[(DWORD_PTR)pvKey].FastIoDispatch;
		case 11:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].DriverInit)->value;
		case 12:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].DriverStartIo)->value;
		case 13:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].DriverUnload)->value;
		case 14:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_CREATE])->value;
		case 15:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_CREATE_NAMED_PIPE])->value;
		case 16:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_CLOSE])->value;
		case 17:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_READ])->value;
		case 18:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_WRITE])->value;
		case 19:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_QUERY_INFORMATION])->value;
		case 20:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_SET_INFORMATION])->value;
		case 21:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_QUERY_EA])->value;
		case 22:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_SET_EA])->value;
		case 23:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_FLUSH_BUFFERS])->value;
		case 24:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION])->value;
		case 25:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION])->value;
		case 26:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_DIRECTORY_CONTROL])->value;
		case 27:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL])->value;
		case 28:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_DEVICE_CONTROL])->value;
		case 29:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL])->value;
		case 30:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_SHUTDOWN])->value;
		case 31:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_LOCK_CONTROL])->value;
		case 32:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_CLEANUP])->value;
		case 33:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_CREATE_MAILSLOT])->value;
		case 34:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_QUERY_SECURITY])->value;
		case 35:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_SET_SECURITY])->value;
		case 36:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_POWER])->value;
		case 37:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_SYSTEM_CONTROL])->value;
		case 38:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_DEVICE_CHANGE])->value;
		case 39:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_QUERY_QUOTA])->value;
		case 40:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_SET_QUOTA])->value;
		case 41:
			return (void*)&m_AddressResolutionMap.PLookup(m_DriverObjects[(DWORD_PTR)pvKey].MajorFunction[IRP_MJ_PNP])->value;
		case 42:
			return (void*)m_KernelDriverNames[(DWORD_PTR)pvKey].GetString();
		case 43:
			return (void*)m_HardwareDatabases[(DWORD_PTR)pvKey].GetString();
		case 44:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].DllBase;
		case 45:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].EntryPoint;
		case 46:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].SizeOfImage;
		case 47:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].Flags;
		case 48:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].LoadCount;
		case 49:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].TlsIndex;
		case 50:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].SectionPointer;
		case 51:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].CheckSum;
		case 52:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)&m_ModuleTableEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].LoadedImports;
		case 53:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)m_ModuleTableFullNameEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].GetString();
		case 54:
			if (m_ModuleTableIndexes[(DWORD_PTR)pvKey] == ~0UL) return NULL;
			return (void*)m_ModuleTableBaseNameEntries[m_ModuleTableIndexes[(DWORD_PTR)pvKey]].GetString();
		case 55:
			return (void*)&m_DeviceArray[(DWORD_PTR)pvKey];
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		DWORD_PTR dwCount;
		DWORD_PTR dwSubCount;
		CByteArray ImageBytes;
		PRU_GetAllTypeObjects(m_DriverObjNames, L"Driver");
		CDriverUse Use;
		ImageBytes.SetSize(sizeof(ULONG));
		do {
			if (!DriverQuery(IOCTL_FOI_READMODULELIST, NULL, 0, ImageBytes.GetData(), (DWORD)(DWORD_PTR)ImageBytes.GetSize())) {
				break;
			}
			if (*((ULONG*)ImageBytes.GetData()) <= (ImageBytes.GetSize() - sizeof(ULONG)) / sizeof(PVOID)) {
				m_DriverSections.SetSize(*((ULONG*)ImageBytes.GetData()));
				memcpy(m_DriverSections.GetData(), ImageBytes.GetData() + sizeof(ULONG), m_DriverSections.GetSize() * sizeof(PVOID));
				break;
			} else {
				ImageBytes.SetSize(sizeof(ULONG) + *((ULONG*)ImageBytes.GetData()) * sizeof(PVOID));
			}
		} while (TRUE);
		for (dwCount = 0; dwCount < (DWORD_PTR)m_DriverObjNames.GetCount(); dwCount++) {
			DRIVER_RECEIVE_OBJECT dro = { NULL };
			CStringW Str;
			if (DriverQuery(IOCTL_FOI_READDRIVEROBJECT, m_DriverObjNames[dwCount].GetBuffer(), (m_DriverObjNames[dwCount].GetLength() + 1) * sizeof(WCHAR), &dro, sizeof(DRIVER_RECEIVE_OBJECT))) {
				m_DriverObjects.Add(dro);
				if (DriverQueryUnicodeString(IOCTL_FOI_READDRIVERNAME, m_DriverObjNames[dwCount].GetBuffer(), (m_DriverObjNames[dwCount].GetLength() + 1) * sizeof(WCHAR), Str)) {
				}
				m_KernelDriverNames.Add(CW2T(Str));
				if (DriverQueryUnicodeString(IOCTL_FOI_READDRIVERHARDWAREDATABASE, m_DriverObjNames[dwCount].GetBuffer(), (m_DriverObjNames[dwCount].GetLength() + 1) * sizeof(WCHAR), Str)) {
				}
				m_HardwareDatabases.Add(CW2T(Str));
				if (dro.DriverSection != NULL) {
					//n^2
					for (dwSubCount = 0; dwSubCount < (DWORD_PTR)m_DriverSections.GetCount(); dwSubCount++) {
						if (m_DriverSections[dwSubCount] == dro.DriverSection) break;
					}
					if (dwSubCount == m_DriverSections.GetCount()) m_DriverSections.Add(dro.DriverSection);
					m_ModuleTableIndexes.Add((DWORD)dwSubCount);
				} else {
					m_ModuleTableIndexes.Add(~0UL);
				}
				CCopyStringArray Devices;
				ImageBytes.SetSize(0xFFFF);
				do {
					if (!DriverQuery(IOCTL_FOI_ENUMDRIVERDEVICES, m_DriverObjNames[dwCount].GetBuffer(), (m_DriverObjNames[dwCount].GetLength() + 1) * sizeof(WCHAR), ImageBytes.GetData(), (DWORD)(DWORD_PTR)ImageBytes.GetSize())) {
						break;
					}
					if (*((ULONG*)ImageBytes.GetData()) <= (ImageBytes.GetSize() - sizeof(ULONG)) / sizeof(PVOID)) {
						DWORD dwOffset = 0;
						for (dwSubCount = 0; dwSubCount < *((ULONG*)ImageBytes.GetData()); dwSubCount++) {
							((PUNICODE_STRING)(ImageBytes.GetData() + sizeof(ULONG) + *((ULONG*)ImageBytes.GetData()) * sizeof(PVOID) + sizeof(ULONG) + dwOffset))->Buffer = (PWSTR)(ImageBytes.GetData() + sizeof(ULONG) + *((ULONG*)ImageBytes.GetData()) * sizeof(PVOID) + sizeof(ULONG) + dwOffset + sizeof(UNICODE_STRING));
							UniStringToCStringW((PUNICODE_STRING)(ImageBytes.GetData() + sizeof(ULONG) + *((ULONG*)ImageBytes.GetData()) * sizeof(PVOID) + sizeof(ULONG) + dwOffset), Str);
							dwOffset += sizeof(UNICODE_STRING) + min(((PUNICODE_STRING)(ImageBytes.GetData() + sizeof(ULONG) + *((ULONG*)ImageBytes.GetData()) * sizeof(PVOID) + sizeof(ULONG) + dwOffset))->Length, ((PUNICODE_STRING)(ImageBytes.GetData() + sizeof(ULONG) + *((ULONG*)ImageBytes.GetData()) * sizeof(PVOID) + sizeof(ULONG) + dwOffset))->MaximumLength);
							Devices.Add(CW2T(Str));
						}
						break;
					} else {
						ImageBytes.SetSize(sizeof(ULONG) + *((ULONG*)ImageBytes.GetData()) * sizeof(PVOID));
					}
				} while (TRUE);
				m_DeviceArray.Add(Devices);
			} else {
				m_DriverObjects.Add(dro);
				Str.Empty();
				m_KernelDriverNames.Add(CW2T(Str));
				m_HardwareDatabases.Add(CW2T(Str));
				m_ModuleTableIndexes.Add(~0UL);
			}
			pModel->OnItemNew(GetRuntimeClass(), (void*)dwCount);
		}
		//also try PSAPI EnumDeviceDrivers, GetDeviceDriverBaseName, GetDeviceDriverFullName
		for (dwCount = 0; dwCount < (DWORD_PTR)m_DriverSections.GetCount(); dwCount++) {
			CStringW Str;
			if (DriverQueryUnicodeString(IOCTL_FOI_READMODULEFULLNAME, &m_DriverSections[dwCount], sizeof(PVOID), Str)) {
			}
			m_ModuleTableFullNameEntries.Add(CW2T(Str));
			if (DriverQueryUnicodeString(IOCTL_FOI_READMODULEBASENAME, &m_DriverSections[dwCount], sizeof(PVOID), Str)) {
			}
			m_ModuleTableBaseNameEntries.Add(CW2T(Str));
			LDR_DATA_TABLE_RECEIVE_ENTRY ldtre = { NULL };
			if (DriverQuery(IOCTL_FOI_READMODULESECTION, &m_DriverSections[dwCount], sizeof(PVOID), &ldtre, sizeof(ldtre))) {
			}
			m_ModuleTableEntries.Add(ldtre);
			m_ModuleTableSortIndexes.Add((DWORD)dwCount);
		}
		qsort_s(m_ModuleTableSortIndexes.GetData(), m_ModuleTableSortIndexes.GetCount(), sizeof(DWORD), CompareModuleBases, this);
		for (dwCount = 0; dwCount < (DWORD_PTR)m_DriverObjNames.GetCount(); dwCount++) {
			AddressResolutionTable NewAddressResolutionTable;
			DWORD* pdwIndex;
			for (dwSubCount = 0; dwSubCount < IRP_MJ_PNP + 1 + 4; dwSubCount++) {
				switch (dwSubCount) {
				case 0:
					NewAddressResolutionTable.pvAddress = m_DriverObjects[dwCount].DriverStart;
					break;
				case 1:
					NewAddressResolutionTable.pvAddress = m_DriverObjects[dwCount].DriverInit;
					break;
				case 2:
					NewAddressResolutionTable.pvAddress = m_DriverObjects[dwCount].DriverStartIo;
					break;
				case 3:
					NewAddressResolutionTable.pvAddress = m_DriverObjects[dwCount].DriverUnload;
					break;
				default:
					NewAddressResolutionTable.pvAddress = m_DriverObjects[dwCount].MajorFunction[dwSubCount - 4];
					break;
				}
				if ((pdwIndex = (DWORD*)bsearch_s(NewAddressResolutionTable.pvAddress, m_ModuleTableSortIndexes.GetData(), m_ModuleTableSortIndexes.GetCount(), sizeof(DWORD), SearchModuleBases, this)) != NULL) {
					NewAddressResolutionTable.pvDLLBase = m_ModuleTableEntries[*pdwIndex].DllBase;
					NewAddressResolutionTable.szBaseName = m_ModuleTableBaseNameEntries[*pdwIndex].GetString();
				} else {
					NewAddressResolutionTable.pvDLLBase = NULL;
					NewAddressResolutionTable.szBaseName = NULL;
				}
				m_AddressResolutionMap.SetAt(NewAddressResolutionTable.pvAddress, NewAddressResolutionTable);
			}
		}
	}
	int DoSearchModuleBases(const PVOID pLeft, const DWORD pRight)
	{
		DWORD dwSizeOfImage = m_ModuleTableEntries[pRight].SizeOfImage ? m_ModuleTableEntries[pRight].SizeOfImage : CKernelModuleList::LookupSizeOfImageOnDisk(m_ModuleTableFullNameEntries[pRight]);
		if (((ULONG_PTR)pLeft >= (ULONG_PTR)m_ModuleTableEntries[pRight].DllBase) && ((ULONG_PTR)pLeft < (ULONG_PTR)m_ModuleTableEntries[pRight].DllBase + dwSizeOfImage)) {
			return 0;
		}
		if ((ULONG_PTR)pLeft < (ULONG_PTR)m_ModuleTableEntries[pRight].DllBase) return -1;
		return 1;
	}
	static int SearchModuleBases(void* pContext, const void* pLeft, const void* pRight)
	{
		return ((CKernelDriverList*)pContext)->DoSearchModuleBases((PVOID)pLeft, *(DWORD*)pRight);
	}
	int DoCompareModuleBases(const DWORD pLeft, const DWORD pRight)
	{
		if (m_ModuleTableEntries[pLeft].DllBase == m_ModuleTableEntries[pRight].DllBase) return 0;
		return (m_ModuleTableEntries[pLeft].DllBase < m_ModuleTableEntries[pRight].DllBase) ? -1 : 1;
	}
	static int CompareModuleBases(void* pContext, const void* pLeft, const void* pRight)
	{
		return ((CKernelDriverList*)pContext)->DoCompareModuleBases(*(DWORD*)pLeft, *(DWORD*)pRight);
	}
private:
	CStringArray m_DriverObjNames;
	CArray<DRIVER_RECEIVE_OBJECT, DRIVER_RECEIVE_OBJECT&> m_DriverObjects;
	CStringArray m_KernelDriverNames;
	CStringArray m_HardwareDatabases;
	CArray<CCopyStringArray, CCopyStringArray&> m_DeviceArray;
	CDWordArray m_ModuleTableIndexes;
	CArray<LDR_DATA_TABLE_RECEIVE_ENTRY, LDR_DATA_TABLE_RECEIVE_ENTRY&> m_ModuleTableEntries;
	CStringArray m_ModuleTableFullNameEntries;
	CStringArray m_ModuleTableBaseNameEntries;
	CArray<PVOID, PVOID> m_DriverSections;
	CDWordArray m_ModuleTableSortIndexes;
	CMap<PVOID, PVOID&, AddressResolutionTable, AddressResolutionTable&> m_AddressResolutionMap;
};

class CDisksInfo : public CGatherer
{
public:
	DECLARE_DYNCREATE(CDisksInfo)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_DriveDevices[(DWORD_PTR)pvKey].GetString();
		case 1:
			return &m_DiskGeometries[(DWORD_PTR)pvKey].Cylinders.QuadPart;
		case 2:
			return &m_DiskGeometries[(DWORD_PTR)pvKey].MediaType;
		case 3:
			return &m_DiskGeometries[(DWORD_PTR)pvKey].TracksPerCylinder;
		case 4:
			return &m_DiskGeometries[(DWORD_PTR)pvKey].SectorsPerTrack;
		case 5:
			return &m_DiskGeometries[(DWORD_PTR)pvKey].BytesPerSector;
		case 6:
			return &m_DiskSizes[(DWORD_PTR)pvKey].QuadPart;
		case 7:
			return &m_DiskPartInfos[(DWORD_PTR)pvKey].PartitionStyle;
		case 8:
			return &m_DiskPartInfos[(DWORD_PTR)pvKey].Mbr.Signature;
		case 9:
			return &m_DiskPartInfos[(DWORD_PTR)pvKey].Mbr.CheckSum;
		case 10:
			return &m_DiskPartInfos[(DWORD_PTR)pvKey].Gpt.DiskId;
		case 11:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].DetectionType;
		case 12:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].Int13.DriveSelect;
		case 13:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].Int13.MaxCylinders;
		case 14:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].Int13.SectorsPerTrack;
		case 15:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].Int13.MaxHeads;
		case 16:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].Int13.NumberDrives;
		case 17:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].ExInt13.ExBufferSize;
		case 18:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].ExInt13.ExFlags;
		case 19:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].ExInt13.ExCylinders;
		case 20:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].ExInt13.ExHeads;
		case 21:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].ExInt13.ExSectorsPerTrack;
		case 22:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].ExInt13.ExSectorsPerDrive;
		case 23:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].ExInt13.ExSectorSize;
		case 24:
			return &m_DiskDetectionInfos[(DWORD_PTR)pvKey].ExInt13.ExReserved;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer *pDepGatherer, CArray<void*> *pDepKeys)
	{
		CDriverUse DriverUse;
		DWORD dwCount;
		if (DriverQueryLPWSTRs(IOCTL_FOI_GETDISKS, NULL, 0, m_DriveDevices)) {
			for (dwCount = 0; dwCount < (DWORD)m_DriveDevices.GetCount(); dwCount++) {
				if (m_DriveDevices[dwCount].IsEmpty()) {
					m_DriveDevices.RemoveAt(dwCount);
					dwCount--;
					continue;
				}
				if (PRU_CheckWindowsVersionMinumum(5, 0)) {
					CByteArray GeometryBytes;
					GeometryBytes.SetSize(MAX_DISK_GEOMETRY_EX_SIZE);
					if (DriverQuery(IOCTL_FOI_GETDISKGEOMETRYEX, (LPVOID)m_DriveDevices[dwCount].GetString(), (m_DriveDevices[dwCount].GetLength() + 1) * sizeof(WCHAR), GeometryBytes.GetData(), (DWORD)GeometryBytes.GetSize())) {
					}
					m_DiskGeometries.Add(((DISK_GEOMETRY_EX*)GeometryBytes.GetData())->Geometry);
					m_DiskSizes.Add(((DISK_GEOMETRY_EX*)GeometryBytes.GetData())->DiskSize);
					m_DiskPartInfos.Add(*DiskGeometryGetPartition((DISK_GEOMETRY_EX*)GeometryBytes.GetData())); //fix extra bytes...
					m_DiskDetectionInfos.Add(*DiskGeometryGetDetect((DISK_GEOMETRY_EX*)GeometryBytes.GetData()));
				} else {
					DISK_GEOMETRY dg = { 0 };
					if (DriverQuery(IOCTL_FOI_GETDISKGEOMETRY, (LPVOID)m_DriveDevices[dwCount].GetString(), (m_DriveDevices[dwCount].GetLength() + 1) * sizeof(WCHAR), &dg, sizeof(DISK_GEOMETRY))) {
					}
					m_DiskGeometries.Add(dg);
				}
				pModel->OnItemNew(GetRuntimeClass(), (void*)(DWORD_PTR)(m_DiskGeometries.GetCount() - 1));
			}
		}
	}
private:
	CStringWArray m_DriveDevices;
	CArray<DISK_GEOMETRY, DISK_GEOMETRY&> m_DiskGeometries;
	CArray<LARGE_INTEGER, LARGE_INTEGER> m_DiskSizes;
	CArray<DISK_PARTITION_INFO, DISK_PARTITION_INFO&> m_DiskPartInfos;
	CArray<DISK_DETECTION_INFO, DISK_DETECTION_INFO&> m_DiskDetectionInfos;
};

class CDiskTree : public CGatherer
{
public:
	DECLARE_DYNCREATE(CDiskTree)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			if ((DWORD_PTR)pvKey == 0) return L"Disks";
			return (void*)m_DriveDevices[(DWORD_PTR)pvKey - 1].GetString();
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer *pDepGatherer, CArray<void*> *pDepKeys)
	{
		CDriverUse DriverUse;
		DWORD_PTR dwCount;
		CArray<void*, void*&> Children;
		if (DriverQueryLPWSTRs(IOCTL_FOI_GETDISKS, NULL, 0, m_DriveDevices)) {
			for (dwCount = 0; dwCount < (DWORD)m_DriveDevices.GetCount(); dwCount++) {
				if (m_DriveDevices[dwCount].IsEmpty()) {
					m_DriveDevices.RemoveAt(dwCount);
					dwCount--;
					continue;
				}
				DWORD_PTR dwAdd = 1 + dwCount;
				pModel->OnItemNew(GetRuntimeClass(), (void*)dwAdd);
				Children.Add((void*&)dwAdd);
			}
		}
		pModel->OnItemNew(GetRuntimeClass(), (void*)0, &Children);
	}
private:
	CStringWArray m_DriveDevices;
};

class CDiskDump : public CGatherer
{
public:
	DECLARE_DYNCREATE(CDiskDump)
	void GetDependencies(CArray<CRuntimeClass*> & DependencyArray)
	{
		DependencyArray.Add(RUNTIME_CLASS(CDiskTree));
	}
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return &m_ullStartOffset;
		case 1:
			return &m_ullLength;
		case 2:
			return &m_Bytes;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer *pDepGatherer, CArray<void*> *pDepKeys)
	{
		CByteArray InputBytes;
		if (pDepKeys->GetCount() == 0) return;
		LPCWSTR wszDiskDevice = (LPCWSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(0), 0);
		InputBytes.SetSize(sizeof(ULONGLONG) + (wcslen(wszDiskDevice) + 1) * sizeof(WCHAR));
		*((ULONGLONG*)InputBytes.GetData()) = 0;
		wcscpy((wchar_t*)(InputBytes.GetData() + sizeof(ULONGLONG)), wszDiskDevice);
		m_Bytes.SetSize(512);
		m_ullStartOffset = 0;
		m_ullLength = 512;
		if (DriverQuery(IOCTL_FOI_RAWDISKREAD, InputBytes.GetData(), (DWORD)InputBytes.GetSize(), m_Bytes.GetData(), (DWORD)(DWORD_PTR)m_Bytes.GetSize())) {
		}
	}
private:
	unsigned long long m_ullStartOffset;
	unsigned long long m_ullLength;
	CByteArray m_Bytes;
};

class CPartitionMBRInfo : public CGatherer
{
public:
	DECLARE_DYNCREATE(CPartitionMBRInfo)
	void GetDependencies(CArray<CRuntimeClass*> & DependencyArray)
	{
		DependencyArray.Add(RUNTIME_CLASS(CDiskTree));
	}
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return &m_MBRPartitions[(DWORD_PTR)pvKey].BootIndicator;
		case 1:
			return &m_MBRPartitions[(DWORD_PTR)pvKey].FirstSector;
		case 2:
			return &m_MBRPartitions[(DWORD_PTR)pvKey].PartitionType;
		case 3:
			return &m_MBRPartitions[(DWORD_PTR)pvKey].LastSector;
		case 4:
			return &m_MBRPartitions[(DWORD_PTR)pvKey].RelativeSectors;
		case 5:
			return &m_MBRPartitions[(DWORD_PTR)pvKey].TotalSectors;
		case 6:
			return &m_BPBEntries[(DWORD_PTR)pvKey].BytesPerSector;
		case 7:
			return &m_BPBEntries[(DWORD_PTR)pvKey].SectorsPerCluster;
		case 8:
			return &m_BPBEntries[(DWORD_PTR)pvKey].ReservedSectors;
		case 9:
			return &m_BPBEntries[(DWORD_PTR)pvKey].FATCopies;
		case 10:
			return &m_BPBEntries[(DWORD_PTR)pvKey].RootDirEntries;
		case 11:
			return &m_BPBEntries[(DWORD_PTR)pvKey].NumSectors;
		case 12:
			return &m_BPBEntries[(DWORD_PTR)pvKey].MediaType;
		case 13:
			return &m_BPBEntries[(DWORD_PTR)pvKey].SectorsPerFAT;
		case 14:
			return &m_BPBEntries[(DWORD_PTR)pvKey].SectorsPerTrack;
		case 15:
			return &m_BPBEntries[(DWORD_PTR)pvKey].NumberOfHeads;
		case 16:
			return &m_BPBEntries[(DWORD_PTR)pvKey].HiddenSectors;
		case 17:
			return &m_BPBEntries[(DWORD_PTR)pvKey].SectorsBig;
		case 18:
			return &m_BPBEntries[(DWORD_PTR)pvKey].PhysicalDriveNumber;
		case 19:
			return &m_BPBEntries[(DWORD_PTR)pvKey].Reserved;
		case 20:
			return &m_BPBEntries[(DWORD_PTR)pvKey].ExtendedBootSignature;
		case 21:
			return &m_BPBEntries[(DWORD_PTR)pvKey].TotalSectors;
		case 22:
			return &m_BPBEntries[(DWORD_PTR)pvKey].MFTStartingClusterNumber;
		case 23:
			return &m_BPBEntries[(DWORD_PTR)pvKey].MFTMirrorStartingClusterNumber;
		case 24:
			return &m_BPBEntries[(DWORD_PTR)pvKey].ClustersPerFileRecord;
		case 25:
			return &m_BPBEntries[(DWORD_PTR)pvKey].ClustersPerIndexBuffer;
		case 26:
			return &m_BPBEntries[(DWORD_PTR)pvKey].VolumeSerialNumber;
		case 27:
			return &m_BPBEntries[(DWORD_PTR)pvKey].Checksum;
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel *pModel, CGatherer *pDepGatherer, CArray<void*> *pDepKeys)
	{
		CDriverUse DriverUser;
		CByteArray BRBytes;
		CByteArray InputBytes;
		DWORD dwCount;
		DWORD dwFirstSector = 0xFFFFFFFF;
		DWORD dwLastSector = 0;
		if (pDepKeys->GetCount() == 0) return;
		LPCWSTR wszDiskDevice = (LPCWSTR)pDepGatherer->GetItemData(pDepKeys->GetAt(0), 0);
		InputBytes.SetSize(sizeof(ULONGLONG) + (wcslen(wszDiskDevice) + 1) * sizeof(WCHAR));
		*((ULONGLONG*)InputBytes.GetData()) = 0;
		wcscpy((wchar_t*)(InputBytes.GetData() + sizeof(ULONGLONG)), wszDiskDevice);
		BRBytes.SetSize(512); //MBR is 512 bytes
		if (DriverQuery(IOCTL_FOI_RAWDISKREAD, InputBytes.GetData(), (DWORD)InputBytes.GetSize(), BRBytes.GetData(), (DWORD)(DWORD_PTR)BRBytes.GetSize())) {
			//check it is MBR and not GPT or none
			memcpy(m_MBRPartitions, BRBytes.GetData() + 0x1BE, sizeof(MBRPartitionEntry) * 4);
			//read PBR BPBs
			for (dwCount = 0; dwCount < 4; dwCount++) {
				if (m_MBRPartitions[dwCount].PartitionType != PARTITION_ENTRY_UNUSED) {
					*((ULONGLONG*)InputBytes.GetData()) = m_MBRPartitions[dwCount].RelativeSectors;
					if (DriverQuery(IOCTL_FOI_RAWDISKREAD, InputBytes.GetData(), (DWORD)InputBytes.GetSize(), BRBytes.GetData(), (DWORD)(DWORD_PTR)BRBytes.GetSize())) {
						memcpy(&m_BPBEntries[dwCount], BRBytes.GetData() + 0xB, sizeof(BPBEntry));
					}
				}
				pModel->OnItemNew(GetRuntimeClass(), (void*)(DWORD_PTR)dwCount);
			}
		}
		//get all disk file pointers:
		//MFT_ENUM_DATA mft_enum_data;
		//USN_JOURNAL_DATA * journal;
		//USN maxusn;
		//drive = CreateFile(L"\\\\?\\c:", GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
		//if (!DeviceIoControl(drive, FSCTL_QUERY_USN_JOURNAL, NULL, 0, buffer, BUFFER_SIZE, &bytecount, NULL))
		//maxusn = journal->MaxUsn;
		//mft_enum_data.StartFileReferenceNumber = 0;
		//mft_enum_data.LowUsn = 0;
		//mft_enum_data.HighUsn = maxusn;
		//if (!DeviceIoControl(drive, FSCTL_ENUM_USN_DATA, &mft_enum_data, sizeof(mft_enum_data), buffer, BUFFER_SIZE, &bytecount, NULL))
		//mft_enum_data.StartFileReferenceNumber = record->ParentFileReferenceNumber;
		//mft_enum_data.LowUsn = 0;
		//mft_enum_data.HighUsn = maxusn;
		//if (!DeviceIoControl(drive, FSCTL_ENUM_USN_DATA, &mft_enum_data, sizeof(mft_enum_data), buffer, BUFFER_SIZE, &bytecount, NULL))
		//parent_record = (USN_RECORD *)((USN *)buffer + 1); 
		//FSCTL_GET_RETRIEVAL_POINTERS to get all sectors on disk, FSCTL_READ_USN_JOURNAL to get all changes after
		//create search for partition algorithm
		//63352, 334296, 6543608, 26205600, 32604696, 35709312, 47207132, 47210012, 47212892, 47215772, 47806512, 55535768, 66211008, 109623096, 111229104, 141015776, 141681632, 147578728, 148438000, 148976032, 153703224, 154046592, 159398752, 178807592, 185409472, 185530880, 188031552, 190583200 
		//69525, 340469, 6546487, 26208479, 32610869, 35715485, 47210011, 47212891, 47215771, 47218651, 47809391, 55541941, 66217181, 109625975, 111231983, 141021949, 141687805, 147581607, 148444173, 148982205, 153706103, 154052765, 159401631, 178810471, 185412351, 185533759, 188037725, 190586079 
		//DWORD dwStart[] = { };
		//DWORD dwEnd[] = { };
		/*CByteArray Bytes;
		CFile File;
		DWORD dwCount;
		for (dwCount = 0; dwCount < sizeof(dwStart) / sizeof(DWORD); dwCount++) {
			ullOffset = dwStart[dwCount];
			Bytes.SetSize((dwEnd[dwCount] - dwStart[dwCount] + 1) * 512);
			if (DriverQuery(IOCTL_FOI_RAWDISKREAD, &ullOffset, sizeof(ULONGLONG), Bytes.GetData(), (DWORD)(DWORD_PTR)Bytes.GetSize())) {
				CString String;
				String.Format(_T("..\\..\\Output\\LostPart%lu"), dwCount);
				if (File.Open(String, CFile::modeWrite | CFile::modeCreate)) {
					File.Write(Bytes.GetData(), (UINT)(DWORD_PTR)Bytes.GetSize());
					File.Close();
					Bytes.SetSize(Bytes.GetSize() + sizeof(ULONGLONG));
					*(ULONGLONG*)Bytes.GetData() = ullOffset;
					memset(Bytes.GetData() + sizeof(ULONGLONG), 0, Bytes.GetSize() - sizeof(ULONGLONG));
					if (DriverQuery(IOCTL_FOI_RAWDISKWRITE, Bytes.GetData(), (DWORD)(DWORD_PTR)Bytes.GetSize(), NULL, 0)) {
					}
				}
			}
		}*/
	}
private:
	MBRPartitionEntry m_MBRPartitions[4];
	BPBEntry m_BPBEntries[4];
};

static void CleanUnusedAreas(CGatherer* pGatherer, void* pvKey)
{
	/*DWORD dwCount;
	for (dwCount = 0; dwCount < 4; dwCount++) {
		if (((MBRPartitionEntry*)(MBRBytes.GetData() + 0x1BE + 16 * dwCount))->RelativeSectors)
			dwFirstSector = min(dwFirstSector, ((MBRPartitionEntry*)(MBRBytes.GetData() + 0x1BE + 16 * dwCount))->RelativeSectors);
		dwLastSector = max(dwLastSector, ((MBRPartitionEntry*)(MBRBytes.GetData() + 0x1BE + 16 * dwCount))->RelativeSectors + ((MBRPartitionEntry*)(MBRBytes.GetData() + 0x1BE + 16 * dwCount))->TotalSectors);
	}
	//code to zero out the gaps
	InputBytes.SetSize(sizeof(ULONGLONG) + (wcslen(wszDiskDevice) + 1) * sizeof(WCHAR) + 512);
	wcscpy((wchar_t*)(InputBytes.GetData() + sizeof(ULONGLONG)), wszDiskDevice);
	//zero out all bytes between MBR and first partition
	for (dwCount = 1; dwCount < dwFirstSector; dwCount++) {
		*((ULONGLONG*)InputBytes.GetData()) = dwCount;
		if (DriverQuery(IOCTL_FOI_RAWDISKWRITE, InputBytes.GetData(), (DWORD)InputBytes.GetSize(), NULL, 0)) {
		}
	}
	//zero out all bytes after the last partition
	for (dwCount = dwLastSector; dwCount < *((ULONGLONG*)pDepGatherer->GetItemData(pDepKeys->GetAt(0), 6)) / 512; dwCount++) {
		*((ULONGLONG*)InputBytes.GetData()) = dwCount;
		if (DriverQuery(IOCTL_FOI_RAWDISKWRITE, InputBytes.GetData(), (DWORD)InputBytes.GetSize(), NULL, 0)) {
		}
	}*/
};

class CKernelInfo : public CGatherer
{
public:
	DECLARE_DYNCREATE(CKernelInfo)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return (void*)m_KernelDescEntries[(DWORD_PTR)pvKey].GetString();
		case 1:
			return &m_KernelInfoEntries[(DWORD_PTR)pvKey];
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		CDriverUse Use;
		ULONG_PTR ulInfo;
		CONFIGURATION_INFORMATION ConfigInfo;
		if (DriverQuery(IOCTL_FOI_READDEBUGGERNOTPRESENT, NULL, 0, &ulInfo, sizeof(ULONG_PTR))) {
			m_KernelDescEntries.Add(_T("Debugger Not Present"));
			m_KernelInfoEntries.Add(ulInfo);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
		}
		if (DriverQuery(IOCTL_FOI_READCONFIGURATIONINFORMATION, NULL, 0, &ConfigInfo, sizeof(CONFIGURATION_INFORMATION))) {
			m_KernelDescEntries.Add(_T("Disk Count"));
			m_KernelInfoEntries.Add(ConfigInfo.DiskCount);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Floppy Count"));
			m_KernelInfoEntries.Add(ConfigInfo.FloppyCount);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("CDRom Count"));
			m_KernelInfoEntries.Add(ConfigInfo.CdRomCount);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Tape Count"));
			m_KernelInfoEntries.Add(ConfigInfo.TapeCount);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("SCSI Port Count"));
			m_KernelInfoEntries.Add(ConfigInfo.ScsiPortCount);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Serial Count"));
			m_KernelInfoEntries.Add(ConfigInfo.SerialCount);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Parallel Count"));
			m_KernelInfoEntries.Add(ConfigInfo.ParallelCount);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Primary Disk Address Claimed"));
			m_KernelInfoEntries.Add(ConfigInfo.AtDiskPrimaryAddressClaimed);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Secondary Disk Address Claimed"));
			m_KernelInfoEntries.Add(ConfigInfo.AtDiskSecondaryAddressClaimed);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
		}
		KSERVICE_TABLE_DESCRIPTOR_RECEIVE ServiceTable;
		if (DriverQuery(IOCTL_FOI_GETSSDT, NULL, 0, &ServiceTable, sizeof(KSERVICE_TABLE_DESCRIPTOR_RECEIVE))) {
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Address"));
			m_KernelInfoEntries.Add((ULONG_PTR)ServiceTable.pKeServiceTable);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Base"));
			m_KernelInfoEntries.Add((ULONG_PTR)ServiceTable.ServiceTableBase);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Counter Base"));
			m_KernelInfoEntries.Add((ULONG_PTR)ServiceTable.ServiceCounterTableBase);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Number of Services"));
			m_KernelInfoEntries.Add(ServiceTable.NumberOfServices);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Parameter Base"));
			m_KernelInfoEntries.Add((ULONG_PTR)ServiceTable.ParamTableBase);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
		}
		if (DriverQuery(IOCTL_FOI_GETSHADOWSSDT, NULL, 0, &ServiceTable, sizeof(KSERVICE_TABLE_DESCRIPTOR_RECEIVE))) {
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Shadow Address"));
			m_KernelInfoEntries.Add((ULONG_PTR)ServiceTable.pKeServiceTable);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Shadow Base"));
			m_KernelInfoEntries.Add((ULONG_PTR)ServiceTable.ServiceTableBase);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Shadow Counter Base"));
			m_KernelInfoEntries.Add((ULONG_PTR)ServiceTable.ServiceCounterTableBase);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Shadow Number of Services"));
			m_KernelInfoEntries.Add(ServiceTable.NumberOfServices);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("System Service Descriptor Table Shadow Parameter Base"));
			m_KernelInfoEntries.Add((ULONG_PTR)ServiceTable.ParamTableBase);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
		}
		UINT_PTR uiCRDR[COUNTCRDR];
		DWORD dwCount;
		if (DriverQuery(IOCTL_FOI_READCRDR, NULL, 0, &uiCRDR, sizeof(UINT_PTR) * COUNTCRDR)) {
			dwCount = 0;
			m_KernelDescEntries.Add(_T("Register CR0"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Register CR2"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Register CR3"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Register CR4"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
#ifdef WIN64
			m_KernelDescEntries.Add(_T("Register CR8"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
#endif
			m_KernelDescEntries.Add(_T("Register DR0"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Register DR1"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Register DR2"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Register DR3"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Register DR6"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
			m_KernelDescEntries.Add(_T("Register DR7"));
			m_KernelInfoEntries.Add((ULONG_PTR)uiCRDR[dwCount++]);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
		}
		DTSTRUCT DT;
		if (DriverQuery(IOCTL_FOI_READIDT, NULL, 0, &DT, sizeof(DTSTRUCT))) {
			m_KernelDescEntries.Add(_T("Interupt Descriptor Table Limit"));
			m_KernelInfoEntries.Add((ULONG_PTR)DT.wLimit);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
			m_KernelDescEntries.Add(_T("Interupt Descriptor Table Offset"));
			m_KernelInfoEntries.Add((ULONG_PTR)DT.pvAddress);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
			CArray<KIDTENTRY, KIDTENTRY> DTEntries;
			DTEntries.SetSize((DT.wLimit + 1) / sizeof(KIDTENTRY));
			if (DriverQuery(IOCTL_FOI_READIDTVALUES, NULL, 0, DTEntries.GetData(), (DWORD)(DWORD_PTR)DTEntries.GetSize() * sizeof(KIDTENTRY))) {
				for (dwCount = 0; dwCount < (DWORD)DTEntries.GetCount(); dwCount++) {
					m_KernelDescEntries.Add(_T("IDT Selector"));
					m_KernelInfoEntries.Add((ULONG_PTR)DTEntries[dwCount].Selector);
					pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
					m_KernelDescEntries.Add(_T("IDT Offset"));
					m_KernelInfoEntries.Add((ULONG_PTR)DTEntries[dwCount].Offset | (ULONG_PTR)DTEntries[dwCount].ExtendedOffset << 16
#ifdef WIN64
						| (ULONG_PTR)DTEntries[dwCount].Offset64 << 32
#endif
						);
					pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
					m_KernelDescEntries.Add(_T("IDT Access"));
					m_KernelInfoEntries.Add((ULONG_PTR)DTEntries[dwCount].Access);
					pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
				}
			}
		}
		if (DriverQuery(IOCTL_FOI_READGDT, NULL, 0, &DT, sizeof(DTSTRUCT))) {
			m_KernelDescEntries.Add(_T("Global Descriptor Table Limit"));
			m_KernelInfoEntries.Add((ULONG_PTR)DT.wLimit);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
			m_KernelDescEntries.Add(_T("Global Descriptor Table Offset"));
			m_KernelInfoEntries.Add((ULONG_PTR)DT.pvAddress);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
			CArray<KGDTENTRY, KGDTENTRY> DTEntries;
			DTEntries.SetSize((DT.wLimit + 1) / sizeof(KGDTENTRY));
			if (DriverQuery(IOCTL_FOI_READGDTVALUES, NULL, 0, DTEntries.GetData(), (DWORD)(DWORD_PTR)DTEntries.GetSize() * sizeof(KGDTENTRY))) {
				for (dwCount = 0; dwCount < (DWORD)DTEntries.GetCount(); dwCount++) {
					m_KernelDescEntries.Add(_T("GDT Limit"));
					m_KernelInfoEntries.Add((ULONG_PTR)DTEntries[dwCount].LimitLow | (ULONG_PTR)DTEntries[dwCount].HighWord.Bits.LimitHi << 16);
					pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
					m_KernelDescEntries.Add(_T("GDT Base"));
					m_KernelInfoEntries.Add((ULONG_PTR)DTEntries[dwCount].BaseLow | (ULONG_PTR)DTEntries[dwCount].HighWord.Bytes.BaseMid << 16 | (ULONG_PTR)DTEntries[dwCount].HighWord.Bytes.BaseHi << 24);
					pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
					m_KernelDescEntries.Add(_T("GDT Access"));
					m_KernelInfoEntries.Add((ULONG_PTR)DTEntries[dwCount].HighWord.Bytes.Flags1 | ((ULONG_PTR)DTEntries[dwCount].HighWord.Bytes.Flags2 >> 4) << 8);
					pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));			
				}
			}
		}
		WORD wReg;
		if (DriverQuery(IOCTL_FOI_READLDT, NULL, 0, &wReg, sizeof(WORD))) {
			m_KernelDescEntries.Add(_T("Local Descriptor Table Segment"));
			m_KernelInfoEntries.Add((ULONG_PTR)wReg);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
		}
		if (DriverQuery(IOCTL_FOI_READTR, NULL, 0, &wReg, sizeof(WORD))) {
			m_KernelDescEntries.Add(_T("Task Register"));
			m_KernelInfoEntries.Add((ULONG_PTR)wReg);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
		}
		if (DriverQuery(IOCTL_FOI_READMSW, NULL, 0, &wReg, sizeof(WORD))) {
			m_KernelDescEntries.Add(_T("Machine Status Word"));
			m_KernelInfoEntries.Add((ULONG_PTR)wReg);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_KernelInfoEntries.GetCount() - 1));
		}
	}
private:
	CStringArray m_KernelDescEntries;
	CArray<ULONG_PTR, ULONG_PTR> m_KernelInfoEntries;
};

class CSSDT : public CGatherer
{
public:
	DECLARE_DYNCREATE(CSSDT)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return &m_SSDTEntries[(DWORD_PTR)pvKey];
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		CDriverUse Use;
		ULONG_PTR ulIndex = 0;
		ULONG_PTR ulAddress;
		while (DriverQuery(IOCTL_FOI_READSSDT, &ulIndex, sizeof(ULONG_PTR), &ulAddress, sizeof(ULONG_PTR))) {
			if (ulAddress == NULL) break;
			m_SSDTEntries.Add(ulAddress);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_SSDTEntries.GetCount() - 1));
			ulIndex++;
		}
	}
private:
	CArray<ULONG_PTR, ULONG_PTR> m_SSDTEntries;
};

class CShadowSSDT : public CGatherer
{
public:
	DECLARE_DYNCREATE(CShadowSSDT)
	void* GetItemData(void *pvKey, int iIndex)
	{
		switch (iIndex) {
		case 0:
			return &m_SSDTEntries[(DWORD_PTR)pvKey];
		default:
			return NULL;
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		CDriverUse Use;
		ULONG_PTR ulIndex = 0;
		ULONG_PTR ulAddress;
		while (DriverQuery(IOCTL_FOI_READSHADOWSSDT, &ulIndex, sizeof(ULONG_PTR), &ulAddress, sizeof(ULONG_PTR))) {
			if (ulAddress == NULL) break;
			m_SSDTEntries.Add(ulAddress);
			pModel->OnItemNew(GetRuntimeClass(), (void*)(m_SSDTEntries.GetCount() - 1));
			ulIndex++;
		}
	}
private:
	CArray<ULONG_PTR, ULONG_PTR> m_SSDTEntries;
};

struct XMLEnumArgValues
{
	TCHAR* szValueName;
	TCHAR* szSubPath[3];
};

struct XMLEnumArgs
{
	TCHAR* szFileFind;
	TCHAR* szDocElem;
	int iPrimaryItemIndex;
	XMLEnumArgValues ArgValues[24];
};

#define MAKEXMLENUM(cname) class cname : public CXMLEnumerator\
{\
public:\
	DECLARE_DYNCREATE(cname)\
	cname() : CXMLEnumerator(&Args) { }\
	private:\
	static XMLEnumArgs Args;\
};

#include <msxml.h>

class CXMLEnumerator : public CGatherer
{
public:
	CXMLEnumerator(XMLEnumArgs* pArgs) : m_pArgs(pArgs) { }
	void* GetItemData(void *pvKey, int iIndex)
	{
		DWORD dwIndex = m_Indexes[(DWORD_PTR)pvKey][iIndex];
		return (dwIndex == ~0) ? NULL : (void*)m_Items[dwIndex].GetString();
	}
	void Recurse(IXMLDOMNode** pspNode, DWORD & dwArgIndex, DWORD dwSubIndex, CCopyDWordArray & NewIndexes, CEnumModel* pModel)
	{
		CComPtr<IXMLDOMNamedNodeMap> spAttrMap;
		CComPtr<IXMLDOMNode> spAttr;
		CComPtr<IXMLDOMNodeList> spNodes;
		CComPtr<IXMLDOMNode> spNode;
		CComBSTR bstrName;
		long lNodes;
		DWORD dwCount;
		CComVariant val;
		for (; m_pArgs->ArgValues[dwArgIndex].szValueName || m_pArgs->ArgValues[dwArgIndex].szSubPath[0]; dwArgIndex++, dwSubIndex = 0) {
			if (m_pArgs->ArgValues[dwArgIndex].szSubPath[dwSubIndex]) {
				if (pspNode && SUCCEEDED((*pspNode)->get_childNodes(&spNodes)) && spNodes &&
					SUCCEEDED(spNodes->get_length(&lNodes))) {
					DWORD dwOldArgIndex = dwArgIndex;
					INT_PTR iOldIndex = NewIndexes.GetCount();
					for (dwCount = 0; dwCount < (DWORD)lNodes; dwCount++) {
						if (SUCCEEDED(spNodes->get_item(dwCount, &spNode)) && spNode &&
							(!*m_pArgs->ArgValues[dwOldArgIndex].szSubPath[dwSubIndex] ||
							(SUCCEEDED(spNode->get_nodeName(&bstrName)) &&
							(bstrName == CComBSTR(m_pArgs->ArgValues[dwOldArgIndex].szSubPath[dwSubIndex]))))) {
							dwArgIndex = dwOldArgIndex;
							Recurse(&spNode.p, dwArgIndex, dwSubIndex + 1, NewIndexes, pModel);
							if ((DWORD)m_pArgs->iPrimaryItemIndex == dwOldArgIndex) {
								//still need indexes
								CCopyDWordArray CopyIndexes;
								CopyIndexes.Copy(NewIndexes);
								m_Indexes.Add(CopyIndexes);
								pModel->OnItemNew(GetRuntimeClass(), (void*)(m_Indexes.GetCount() - 1));
							}
						} else if (dwOldArgIndex >= (DWORD)m_pArgs->iPrimaryItemIndex) {
							//Recurse(NULL, dwArgIndex, dwSubIndex + 1, NewIndexes, pModel);
						}
						bstrName.Empty();
						spNode.Release();
						if ((DWORD)m_pArgs->iPrimaryItemIndex >= dwOldArgIndex) {
							NewIndexes.RemoveAt(iOldIndex, NewIndexes.GetCount() - iOldIndex);
						}
					}
				}
				spNodes.Release();
			} else if (m_pArgs->ArgValues[dwArgIndex].szValueName) {
				//empty string represents the node name
				if (!*m_pArgs->ArgValues[dwArgIndex].szValueName &&
					pspNode && SUCCEEDED((*pspNode)->get_nodeName(&bstrName))) {
					NewIndexes.Add((DWORD)m_Items.Add(COLE2T(bstrName)));
				} else if (pspNode && SUCCEEDED((*pspNode)->get_attributes(&spAttrMap)) &&
					SUCCEEDED(spAttrMap->getNamedItem(CComBSTR(m_pArgs->ArgValues[dwArgIndex].szValueName), &spAttr)) &&
					spAttr && SUCCEEDED(spAttr->get_nodeValue(&val))) {
					NewIndexes.Add((DWORD)m_Items.Add(COLE2T(val.bstrVal)));
				} else NewIndexes.Add(~0UL);
				bstrName.Empty();
				val.Clear();
				spAttr.Release();
				spAttrMap.Release();
			}
		}
	}
	void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys)
	{
		CComPtr<IXMLDOMDocument> spXMLDOM;
		CComPtr<IXMLDOMElement> spDocElem;
		CComPtr<IXMLDOMNodeList> spNodes;
		VARIANT_BOOL bSuccess;
		CComBSTR bstrName;
		long lNodes;
		CString Path;
		if (!PRU_GetWindowsDirectory(Path)) return;
		Path += m_pArgs->szFileFind;
		CFileFind FileFind;
		BOOL bFind = FileFind.FindFile(Path);
		while (bFind) {
			bFind = FileFind.FindNextFile();
			HRESULT hr = spXMLDOM.CoCreateInstance(__uuidof(DOMDocument));
			if (SUCCEEDED(hr) &&
				SUCCEEDED(hr = spXMLDOM->load(CComVariant(FileFind.GetFilePath()), &bSuccess)) &&
				bSuccess) {
				if (SUCCEEDED(spXMLDOM->get_documentElement(&spDocElem)) && spDocElem &&
					SUCCEEDED(spDocElem->get_nodeName(&bstrName)) &&
					((bstrName == CComBSTR(m_pArgs->szDocElem)) ||
					(bstrName == CComBSTR(m_pArgs->ArgValues->szSubPath[0]))) &&
					SUCCEEDED(spDocElem->get_childNodes(&spNodes)) && spNodes &&
					SUCCEEDED(spNodes->get_length(&lNodes))) {
					DWORD dwArgIndex;
					dwArgIndex = 0;
					CCopyDWordArray NewIndexes;
					NewIndexes.Add((DWORD)m_Items.Add(FileFind.GetFilePath()));
					Recurse((IXMLDOMNode**)&spDocElem.p, dwArgIndex, (bstrName == CComBSTR(m_pArgs->szDocElem)) ? 0 : 1, NewIndexes, pModel);
				}
				spNodes.Release();
				bstrName.Empty();
				spDocElem.Release();
			}
			spXMLDOM.Release();
		}
		FileFind.Close();
	}
	void RecurseForDelete(IXMLDOMNode** pspNode, DWORD & dwArgIndex, DWORD dwSubIndex, BOOL & bMatch, DWORD & dwIndex, BOOL & bCheck, void* pvKey)
	{
		CComPtr<IXMLDOMNamedNodeMap> spAttrMap;
		CComPtr<IXMLDOMNode> spAttr;
		CComPtr<IXMLDOMNodeList> spNodes;
		CComPtr<IXMLDOMNode> spNode;
		CComBSTR bstrName;
		long lNodes;
		DWORD dwCount;
		CComVariant val;
		for (; m_pArgs->ArgValues[dwArgIndex].szValueName || m_pArgs->ArgValues[dwArgIndex].szSubPath[0]; dwArgIndex++, dwSubIndex = 0) {
			if (m_pArgs->ArgValues[dwArgIndex].szSubPath[dwSubIndex]) {
				if (bMatch && pspNode && SUCCEEDED((*pspNode)->get_childNodes(&spNodes)) && spNodes &&
					SUCCEEDED(spNodes->get_length(&lNodes))) {
					DWORD dwOldArgIndex = dwArgIndex;
					INT_PTR iOldIndex = dwIndex;
					for (dwCount = 0; dwCount < (DWORD)lNodes; dwCount++) {
						if (SUCCEEDED(spNodes->get_item(dwCount, &spNode)) && spNode &&
							(SUCCEEDED(spNode->get_nodeName(&bstrName)) &&
							(bstrName == CComBSTR(*m_pArgs->ArgValues[dwOldArgIndex].szSubPath[dwSubIndex] ? 
													m_pArgs->ArgValues[dwOldArgIndex].szSubPath[dwSubIndex] :
													(LPCTSTR)GetItemData(pvKey, (int)iOldIndex))))) {
							dwArgIndex = dwOldArgIndex;
							RecurseForDelete(&spNode.p, dwArgIndex, dwSubIndex + 1, bMatch, dwIndex, bCheck, pvKey);
							if (bCheck) {
								VARIANT_BOOL bHasChild;
								CComPtr<IXMLDOMNode> spDelNode;
								if (SUCCEEDED(spNode->hasChildNodes(&bHasChild)) && !bHasChild) {
									SUCCEEDED((*pspNode)->removeChild(spNode, &spDelNode));
								}
								bstrName.Empty();
								spNode.Release();
								spDelNode.Release();
								spNodes.Release();
								return;
							}
							if ((DWORD)m_pArgs->iPrimaryItemIndex == dwOldArgIndex) {
								if (bMatch) {
									CComPtr<IXMLDOMNode> spDelNode;
									SUCCEEDED((*pspNode)->removeChild(spNode, &spDelNode));
									bCheck = TRUE;
									bstrName.Empty();
									spNode.Release();
									spDelNode.Release();
									spNodes.Release();
									return;
								}
							}
							bMatch = TRUE;
						} else if (dwOldArgIndex >= (DWORD)m_pArgs->iPrimaryItemIndex) {
							//RecurseForDelete(NULL, dwArgIndex, dwSubIndex + 1, dwIndex, bCheck, pvKey);
						}
						if ((DWORD)m_pArgs->iPrimaryItemIndex >= dwOldArgIndex) {
							dwIndex = (DWORD)iOldIndex;
						}
						bstrName.Empty();
						spNode.Release();
					}
				}
				spNodes.Release();
			} else if (m_pArgs->ArgValues[dwArgIndex].szValueName) {
				//empty string represents the node name
				if (!*m_pArgs->ArgValues[dwArgIndex].szValueName &&
					pspNode && SUCCEEDED((*pspNode)->get_nodeName(&bstrName))) {
					if (CComBSTR((LPCTSTR)GetItemData(pvKey, dwIndex)) != bstrName) bMatch = FALSE;
				} else if (pspNode && SUCCEEDED((*pspNode)->get_attributes(&spAttrMap)) &&
					SUCCEEDED(spAttrMap->getNamedItem(CComBSTR(m_pArgs->ArgValues[dwArgIndex].szValueName), &spAttr)) &&
					spAttr && SUCCEEDED(spAttr->get_nodeValue(&val))) {
					if (CString(COLE2T(val.bstrVal)).Compare((LPCTSTR)GetItemData(pvKey, dwIndex)) != 0) bMatch = FALSE;
				} else if (GetItemData(pvKey, dwIndex) != NULL) bMatch = FALSE;
				bstrName.Empty();
				val.Clear();
				spAttr.Release();
				spAttrMap.Release();
				if (!bMatch) return;
				dwIndex++;
			}
		}
	}
	void DeleteAction(CString & Path, CArray<void*, void*&>* pvKeys)
	{
		CComPtr<IXMLDOMDocument> spXMLDOM;
		CComPtr<IXMLDOMElement> spDocElem;
		CComPtr<IXMLDOMNodeList> spNodes;
		VARIANT_BOOL bSuccess;
		CComBSTR bstrName;
		long lNodes;
		BOOL bCheck = FALSE;
		DWORD dwCount;
		HRESULT hr = spXMLDOM.CoCreateInstance(__uuidof(DOMDocument));
		if (SUCCEEDED(hr) &&
			SUCCEEDED(hr = spXMLDOM->load(CComVariant(Path), &bSuccess)) &&
			bSuccess) {
			if (SUCCEEDED(spXMLDOM->get_documentElement(&spDocElem)) && spDocElem &&
				SUCCEEDED(spDocElem->get_nodeName(&bstrName)) &&
				((bstrName == CComBSTR(m_pArgs->szDocElem)) ||
				(bstrName == CComBSTR(m_pArgs->ArgValues->szSubPath[0]))) &&
				SUCCEEDED(spDocElem->get_childNodes(&spNodes)) && spNodes &&
				SUCCEEDED(spNodes->get_length(&lNodes))) {
				DWORD dwArgIndex;
				DWORD dwIndex;
				BOOL bMatch;
				for (dwCount = 0; dwCount < (DWORD)pvKeys->GetCount(); dwCount++) {
					dwArgIndex = 0;
					dwIndex = 1;
					bMatch = TRUE;
					RecurseForDelete((IXMLDOMNode**)&spDocElem.p, dwArgIndex, (bstrName == CComBSTR(m_pArgs->szDocElem)) ? 0 : 1, bMatch, dwIndex, bCheck, pvKeys->GetAt(dwCount));
					if (bCheck) {
						VARIANT_BOOL bHasChild;
						if (!SUCCEEDED(spDocElem->hasChildNodes(&bHasChild)) || bHasChild) bCheck = FALSE; else break;
					}
				}
			}
			spNodes.Release();
			bstrName.Empty();
			spDocElem.Release();
		}
		if (bCheck) {
			PRU_TakeOwnership(Path.GetBuffer(), SE_FILE_OBJECT, FALSE);
			DeleteFile(Path);
		} else {
			PSID psid = NULL;
			if (PRU_GetOwner(Path.GetBuffer(), SE_FILE_OBJECT, &psid)) {
				PRU_TakeOwnership(Path.GetBuffer(), SE_FILE_OBJECT, FALSE);
				PRU_SetPrivilege(SE_RESTORE_NAME, TRUE);
				CFileStatus rStatus;
				CFile::GetStatus(Path, rStatus);
				CComPtr<IStream> spStream;
				if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &spStream))) {
					STATSTG streamStat;
					HANDLE hFile;
					if (SUCCEEDED(spXMLDOM->save(CComVariant(spStream))) &&
						SUCCEEDED(spStream->Stat(&streamStat, STATFLAG_NONAME)) &&
						(hFile = CreateFile(Path, GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL)) != INVALID_HANDLE_VALUE) {
						DWORD dwLen = (DWORD)streamStat.cbSize.LowPart;
						CByteArray buf;
						LARGE_INTEGER liOffset = { 0 };
						buf.SetSize(dwLen);
						if (SUCCEEDED(spStream->Seek(liOffset, STREAM_SEEK_SET, NULL)) &&
							SUCCEEDED(spStream->Read(buf.GetData(), dwLen, &dwLen))) {
							WriteFile(hFile, buf.GetData(), dwLen, &dwLen, NULL);
						}
						PRU_CloseHandle(hFile);
					}
					spStream.Release();
				}
				PRU_TakeOwnership(Path.GetBuffer(), SE_FILE_OBJECT, FALSE, psid);
				PRU_SetPrivilege(SE_RESTORE_NAME, FALSE);
				delete [] psid;
			}
		}
		spXMLDOM.Release();
	}
	static void DeleteAction(CGatherer* pGatherer, void* pvKey)
	{
		CArray<void*, void*&>* Keys = (CArray<void*, void*&>*)pvKey;
		CMapStringToPtr FileMaps;
		void* rValue;
		DWORD dwCount;
		CArray<CCopyArray<void*, void*&>, CCopyArray<void*, void*&>&> KeyValues;
		for (dwCount = 0; dwCount < (DWORD)Keys->GetCount(); dwCount++) {
			LPCTSTR szPath = (LPCTSTR)pGatherer->GetItemData(Keys->GetAt(dwCount), 0);
			if (FileMaps.Lookup(szPath, rValue)) {
				KeyValues[(DWORD_PTR)rValue].Add(Keys->GetAt(dwCount));
			} else {
				CCopyArray<void*, void*&> NewKeyValues;
				NewKeyValues.Add(Keys->GetAt(dwCount));
				FileMaps.SetAt(szPath, (void*)KeyValues.Add(NewKeyValues));
			}
		}
		POSITION pos = FileMaps.GetStartPosition();
		while (pos) {
			CString Path;
			FileMaps.GetNextAssoc(pos, Path, rValue);
			((CXMLEnumerator*)pGatherer)->DeleteAction(Path, &KeyValues[(DWORD_PTR)rValue]);
		}
	}
private:
	XMLEnumArgs* m_pArgs;
	CArray<CCopyDWordArray, CCopyDWordArray&> m_Indexes;
	CStringArray m_Items;
};

MAKEXMLENUM(CSessionTaskPackages)
MAKEXMLENUM(CSessionActionPackages)

XMLEnumArgs CSessionTaskPackages::Args = { _T("\\servicing\\Sessions\\*.xml"), _T("Sessions"), 13,
{	{ _T("version"), { _T("Session") } },
	{ _T("id"), NULL },
	{ _T("client"), NULL },
	{ _T("options"), NULL },
	{ _T("currentPhase"), NULL },
	{ _T("lastSuccessfulState"), NULL },
	{ _T("pendingFollower"), NULL },
	{ _T("retry"), NULL },
	{ _T("Queued"), NULL },
	{ _T("Started"), NULL },
	{ _T("Complete"), NULL },
	{ _T("status"), NULL },
	{ _T("seq"), { _T("Tasks"), _T("Phase") } },
	{ _T("id"), { _T("package") } },
	{ _T("name"), NULL },
	{ _T("targetState"), NULL },
	{ _T("options"), NULL }
} };

XMLEnumArgs CSessionActionPackages::Args = { _T("\\servicing\\Sessions\\*.xml"), _T("Sessions"), 15,
{	{ _T("version"), { _T("Session") } },
	{ _T("id"), NULL },
	{ _T("client"), NULL },
	{ _T("options"), NULL },
	{ _T("currentPhase"), NULL },
	{ _T("lastSuccessfulState"), NULL },
	{ _T("pendingFollower"), NULL },
	{ _T("retry"), NULL },
	{ _T("Queued"), NULL },
	{ _T("Started"), NULL },
	{ _T("Complete"), NULL },
	{ _T("status"), NULL },
	{ _T("seq"), { _T("Actions"), _T("Phase") } },
	{ _T("rebootRequired"), NULL },
	{ _T("Installed"), NULL },
	{ _T(""), { _T("") } },
	{ _T("package"), NULL },
	{ _T("update"), NULL }
} };

//IMPLEMENT_DYNAMIC(CEnumModel, CObject)
//IMPLEMENT_DYNAMIC(CTreeModel, CEnumModel)

//IMPLEMENT_DYNCREATE(CHandleList, CEnumModel)
//IMPLEMENT_DYNCREATE(CProcessList, CTreeModel)
IMPLEMENT_DYNCREATE(CVolumeGatherer, CObject)
IMPLEMENT_DYNCREATE(CDriveGatherer, CObject)
IMPLEMENT_DYNCREATE(CDirectoryTreeList, CObject)
IMPLEMENT_DYNCREATE(CKernelObjectDirectoryTree, CObject)
//IMPLEMENT_DYNCREATE(CObjectTypeListTree, CEnumModel)
//CServiceList and CDriverList combined with filter?
IMPLEMENT_DYNCREATE(CServiceGatherer, CObject)
IMPLEMENT_DYNCREATE(CServiceConfig, CObject)
IMPLEMENT_DYNCREATE(CDriverList, CObject)
IMPLEMENT_DYNCREATE(CLogonSessionList, CObject)
IMPLEMENT_DYNCREATE(CGroupLoadOrderTagListGatherer, CObject)
IMPLEMENT_DYNCREATE(CGlobalAutoRuns, CObject)
IMPLEMENT_DYNCREATE(CUserAutoRuns, CObject)
IMPLEMENT_DYNCREATE(CCriticalDeviceReferences, CObject)
//IMPLEMENT_DYNCREATE(CEnumEntries, CObject)
IMPLEMENT_DYNCREATE(CInfFiles, CObject)
IMPLEMENT_DYNCREATE(CDevices, CObject)
IMPLEMENT_DYNCREATE(CKernelDeviceList, CObject)
IMPLEMENT_DYNCREATE(CKernelModuleList, CObject)
IMPLEMENT_DYNCREATE(CKernelDriverList, CObject)
IMPLEMENT_DYNCREATE(CDisksInfo, CObject)
IMPLEMENT_DYNCREATE(CDiskTree, CObject)
IMPLEMENT_DYNCREATE(CDiskDump, CObject)
IMPLEMENT_DYNCREATE(CPartitionMBRInfo, CObject)
IMPLEMENT_DYNCREATE(CKernelInfo, CObject)
IMPLEMENT_DYNCREATE(CSSDT, CObject)
IMPLEMENT_DYNCREATE(CShadowSSDT, CObject)
IMPLEMENT_DYNCREATE(CExplicitOwnership, CObject)
IMPLEMENT_DYNCREATE(CExplicitOwnershipReg, CObject)
IMPLEMENT_DYNCREATE(CWindowsPackageServicingCache, CObject)
IMPLEMENT_DYNCREATE(CWindowsComponentManifests, CObject)

IMPLEMENT_DYNCREATE(CSessionTaskPackages, CObject)
IMPLEMENT_DYNCREATE(CSessionActionPackages, CObject)
//IMPLEMENT_DYNCREATE(CGlobalAutoruns, CObject)
//IMPLEMENT_DYNCREATE(CUserAutoruns, CObject)
IMPLEMENT_DYNCREATE(CWindowsSharedDLLEntries, CObject)
IMPLEMENT_DYNCREATE(CWindowsAppPathsEntries, CObject)
IMPLEMENT_DYNCREATE(CWindowsUninstallEntries, CObject)
IMPLEMENT_DYNCREATE(CWindowsInstallerAssemblies, CObject)
IMPLEMENT_DYNCREATE(CWindowsInstallerWin32Assemblies, CObject)
IMPLEMENT_DYNCREATE(CWindowsClassesTypelibEntries, CObject)
IMPLEMENT_DYNCREATE(CWindowsClassesTypelibs, CObject)
IMPLEMENT_DYNCREATE(CWindowsClassesAppIDs, CObject)
IMPLEMENT_DYNCREATE(CWindowsClassesInterfaces, CObject)
IMPLEMENT_DYNCREATE(CWindowsClassesCLSIDs, CObject)
IMPLEMENT_DYNCREATE(CWindowsNetworkSignaturesUnmanaged, CObject)
IMPLEMENT_DYNCREATE(CWindowsNetworkProfiles, CObject)
IMPLEMENT_DYNCREATE(CWindowsNetworkNlaWireless, CObject)
IMPLEMENT_DYNCREATE(CWindowsNetworkNlaCacheIntranet, CObject)
IMPLEMENT_DYNCREATE(CWindowsNetworkConnections, CObject)
IMPLEMENT_DYNCREATE(CWindowsNetworkDevices, CObject)
IMPLEMENT_DYNCREATE(CWindowsServiceDrivers, CObject)
IMPLEMENT_DYNCREATE(CWindowsComponentPackages, CObject)
IMPLEMENT_DYNCREATE(CWindowsComponentPackageIndexes, CObject)
IMPLEMENT_DYNCREATE(CWindowsComponentPackageSessions, CObject)
IMPLEMENT_DYNCREATE(CWindowsSideBySideComponentWinners, CObject)
IMPLEMENT_DYNCREATE(CWindowsComponents, CObject)
IMPLEMENT_DYNCREATE(CWindowsComponentFamilies, CObject)
IMPLEMENT_DYNCREATE(CWindowsInstallerProducts, CObject)
IMPLEMENT_DYNCREATE(CWindowsInstallerFolders, CObject)
IMPLEMENT_DYNCREATE(CWindowsInstallerProductPatches, CObject)
IMPLEMENT_DYNCREATE(CWindowsInstallerPatches, CObject)
IMPLEMENT_DYNCREATE(CWindowsInstallerComponents, CObject)
IMPLEMENT_DYNCREATE(CWindowsInstallerComponentProducts, CObject)
IMPLEMENT_DYNCREATE(CEnumEntries, CObject)
IMPLEMENT_DYNCREATE(CHardwareClasses, CObject)
IMPLEMENT_DYNCREATE(CHardwareClassList, CObject)
IMPLEMENT_DYNCREATE(CCriticalDevices, CObject)
IMPLEMENT_DYNCREATE(CMountedDevices, CObject)
IMPLEMENT_DYNCREATE(CServiceOrderGroups, CObject)
IMPLEMENT_DYNCREATE(CDeviceClasses, CObject)
//VerifyColumnData() for default column info bundled structure in test build!!!

enum eRenderers
{
	eRIndexRendererUI2,
	eRIndexRendererUI4,
	eRKeyRendererLPWSTR,
	eRKernelObjectDirectoryType,
    eRKernelObjectProcessID,
    eRKernelObjectHandle,
    eRKernelObjectName,
};

void TextRender(CGatherer* /*pGatherer*/, void* /*pvKey*/, void* pvArg, DWORD /*dwType*/, void* pvOut) { *(CString*)pvOut = (LPCTSTR)pvArg; }
int TextCompare(CGatherer* /*pGatherer*/, void* /*pvKey1*/, void* /*pvKey2*/, void* /*pvArg*/, DWORD /*dwType*/) { return 0; }

void ItemRender(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, void* pvOut) { GetDisplayString(dwType, pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg), pvKey, *(CString*)pvOut); }
int ItemCompare(CGatherer* pGatherer, void* pvKey1, void* pvKey2, void* pvArg, DWORD dwType) { return CompareDisplayType(dwType, pGatherer->GetItemData(pvKey1, (DWORD)(DWORD_PTR)pvArg), pvKey1, pGatherer->GetItemData(pvKey2, (DWORD)(DWORD_PTR)pvArg), pvKey2); }

void DataRender(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, void* pvOut) { *((void**)pvOut) = pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg); }

void FileExistItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{ LPCTSTR szFilePath = (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg); clr = !szFilePath || PRU_PathFileExists(szFilePath) ? 0x000000 : 0x0000FF; }

void SessionExistItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{
	LPCTSTR szFilePath = (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	if (szFilePath) {
		CString String;
		if (!PRU_GetWindowsDirectory(String)) { clr = 0x000000; return; }
		String += _T("\\servicing\\Sessions\\");
		String += szFilePath;
		CString Final = String + _T(".xml");
		String += _T(".back.xml");
		clr = PRU_PathFileExists(Final) && PRU_PathFileExists(String) ? 0x000000 : 0x0000FF;
	} else clr = 0x000000;
}

void PackageExistItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{
	LPCTSTR szFilePath = (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	if (szFilePath) {
		CString String;
		if (!PRU_GetWindowsDirectory(String)) { clr = 0x000000; return; }
		String += _T("\\servicing\\Packages\\");
		String += szFilePath;
		CString Final = String + _T(".mum");
		String += _T(".cat");
		clr = PRU_PathFileExists(Final) && PRU_PathFileExists(String) ? 0x000000 : 0x0000FF;
	} else clr = 0x000000;
}

void ComponentExistItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{
	LPCTSTR szFilePath = (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	if (szFilePath) {
		CString String;
		if (!PRU_GetWindowsDirectory(String)) { clr = 0x000000; return; }
		String += _T("\\winsxs\\Manifests\\");
		String += szFilePath;
		String += _T(".manifest");
		clr = PRU_PathFileExists(String) ? 0x000000 : 0x0000FF;
	} else clr = 0x000000;
}

void SessionRegKeyExistItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{
	BOOL bExists;
	CString String = REGSTR_WINDOWS_COMPONENT_SESSIONS;
	String += _T("\\");
	String += (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	clr = PRU_RegistryKeyExists(HKEY_LOCAL_MACHINE, String, bExists) && bExists ? 0x000000 : 0x0000FF;
}

void PackageRegKeyExistItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{
	BOOL bExists;
	CString String = REGSTR_WINDOWS_COMPONENT_PACKAGES;
	String += _T("\\");
	String += (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	clr = PRU_RegistryKeyExists(HKEY_LOCAL_MACHINE, String, bExists) && bExists ? 0x000000 : 0x0000FF;
}

void ComponentRegKeyExistItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{
	BOOL bExists;
	CString String = REGSTR_WINDOWS_COMPONENTS;
	String += _T("\\");
	String += (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	clr = PRU_RegistryKeyExists(HKEY_LOCAL_MACHINE, String, bExists) && bExists ? 0x000000 : 0x0000FF;
}

void GetCommandLineBinary(LPCTSTR szCmdLine, CString & String)
{
	CString Path;
	Path = ResolveNTPath(szCmdLine);
	//if no quotes then must parse token by token and check each base for validity
	if (Path.GetAt(0) != '\"') {
		int iNext = -1;
		int iLast = -1;
		while ((iNext = Path.Find(' ', iNext + 1)) != -1) {
			if (SearchPath(NULL, Path.Left(iNext), _T(".exe"), 0, NULL, NULL)) {
				iLast = iNext;
			}
		}
		if (iLast != -1) String = Path.Left(iLast);
		else String = Path;
	} else PRU_GetCommandLineLaunchArg(Path, String);
}

void NetworkConnectionItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{
	BOOL bFound = FALSE;
	LPCTSTR szGUID = (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	HKEY hKey;
	HKEY hNetKey;
	if (PRU_RegOpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"), hKey)) {
		CStringArray Adapters;
		CString String;
		DWORD dwCount;
		PRU_RegistryEnumKey(hKey, Adapters);
		for (dwCount = 0; dwCount < (DWORD)Adapters.GetCount(); dwCount++) {
			if (PRU_RegOpenKey(hKey, Adapters[dwCount], hNetKey)) {
				if (PRU_ReadRegistryString(hNetKey, _T("NetCfgInstanceId"), String) && String.Compare(szGUID) == 0)
					bFound = TRUE;
				PRU_RegCloseKey(hNetKey);
			}
		}
		PRU_RegCloseKey(hKey);
	}
	clr = bFound ? 0x000000 : 0x0000FF;
}

//for CreateProcess only, need to impersonate the appropriate user
//  for appropriate environment variables and security permissions
void CmdLineItemColor(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, COLORREF& clr)
{
	CString String;
	LPCTSTR szCmdLine = (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	if (szCmdLine) GetCommandLineBinary(szCmdLine, String);
	clr = String.IsEmpty() || SearchPath(NULL, String, _T(".exe"), 0, NULL, NULL) ? 0x000000 : 0x0000FF;
}

void IconRender(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, void* pvOut)
{
	PRU_GetShellIcon((LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg), *(HICON*)pvOut);
}

void IconRenderCmdLine(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, void* pvOut)
{
	DWORD dwSize;
	CString String;
	CString PathString;
	LPCTSTR szCmdLine = (LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg);
	if (szCmdLine) GetCommandLineBinary(szCmdLine, String);
	if ((dwSize = SearchPath(NULL, String, _T(".exe"), 0, NULL, NULL)) != 0) {
		dwSize = SearchPath(NULL, String, _T(".exe"), dwSize, PathString.GetBuffer(dwSize + 1), NULL);
		PathString.ReleaseBuffer();
		PRU_GetShellIcon(PathString, *(HICON*)pvOut);
	}
	//if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
}

void IconRenderDirectoryType(CGatherer* pGatherer, void* pvKey, void* pvArg, DWORD dwType, void* pvOut)
{
	CStringW Type = CT2W((LPCTSTR)pGatherer->GetItemData(pvKey, (DWORD)(DWORD_PTR)pvArg));
	if (_wcsicmp(Type, L"Device") == 0) {
		pvOut = (HICON)0;
	} else if (_wcsicmp(Type, L"Directory") == 0) {
		pvOut = (HICON)1;
	} else if (_wcsicmp(Type, L"Driver") == 0) {
		pvOut = (HICON)2;
	} else if (_wcsicmp(Type, L"Event") == 0) {
		pvOut = (HICON)3;
	} else if (_wcsicmp(Type, L"Key") == 0) {
		pvOut = (HICON)4;
	} else if (_wcsicmp(Type, L"Mutant") == 0) {
		pvOut = (HICON)5;
	} else if (_wcsicmp(Type, L"Port") == 0) {
		pvOut = (HICON)6;
	} else if (_wcsicmp(Type, L"Profile") == 0) {
		pvOut = (HICON)7;
	} else if (_wcsicmp(Type, L"Section") == 0) {
		pvOut = (HICON)8;
	} else if (_wcsicmp(Type, L"Semaphore") == 0) {
		pvOut = (HICON)9;
	} else if (_wcsicmp(Type, L"SymbolicLink") == 0) {
		pvOut = (HICON)10;
	} else if (_wcsicmp(Type, L"Timer") == 0) {
		pvOut = (HICON)11;
	} else if (_wcsicmp(Type, L"WindowStation") == 0) {
		pvOut = (HICON)13;
	} else {
		pvOut = (HICON)12;
	}
}

//IOCTL_STORAGE_RESET_BUS, IOCTL_STORAGE_RESET_DEVICE
//IOCTL_ATA_PASS_THROUGH with command Device Reset DrvRst DRST = 0x08
//ULONG buffsize; // size of the buffer
//ATA_PASS_THROUGH_EX hdr;
//hdr.DataTransferLength = size in bytes of the data transfer
//buffsize = sizeof (ATA_PASS_THROUGH_EX) + hdr.DataTransferLength
//		pApte->CurrentTaskFile[0] = 0x00;  // na
//		pApte->CurrentTaskFile[1] = 0x00;  // na
//		pApte->CurrentTaskFile[2] = 0x00;  // na
//		pApte->CurrentTaskFile[3] = 0x00;  // na
//		pApte->CurrentTaskFile[4] = 0x00;  // na
//		pApte->CurrentTaskFile[5] = 0xA0;  // Drive Select(A0:Master,B0:Slave)
//		pApte->CurrentTaskFile[6] = 0x08;  // IDE_COMMAND_RESET_DEVICE

DisplaySet DisplaySets[] = {
	{	{ TextRender, TextCompare, _T("Shared DLLs") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsSharedDLLEntries) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Shared DLL Path") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsSharedDLLEntries) }, CmdLineItemColor }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Value") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsSharedDLLEntries) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsSharedDLLEntries), CWindowsSharedDLLEntries::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("App Paths") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsAppPathsEntries) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("File Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsAppPathsEntries) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Full Path") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsAppPathsEntries) }, CmdLineItemColor }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Path") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsAppPathsEntries) }, CmdLineItemColor }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Save URL") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsAppPathsEntries) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Use URL") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsAppPathsEntries) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Block On TS Non-Install Mode") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsAppPathsEntries) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsAppPathsEntries), CWindowsAppPathsEntries::DeleteAction, TRUE } } },
	
	{	{ TextRender, TextCompare, _T("Uninstall Entries") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Key") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Authorized CDF Prefix") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Comments") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Contact") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Display Name") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Display Version") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Estimated Size") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Help Link") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Help Telephone") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Date") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Location") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Source") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Language") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Modify Path") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("No Modify") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 15, 1, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("No Repair") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 16, 1, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Publisher") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 17, 1, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("Readme") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 18, 1, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("Size") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 19, 1, 19, TRUE
	},
	{	{ TextRender, TextCompare, _T("Uninstall String") },
		{ ItemRender, ItemCompare, (PVOID)19, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 20, 1, 20, TRUE
	},
	{	{ TextRender, TextCompare, _T("URL Info About") },
		{ ItemRender, ItemCompare, (PVOID)20, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 21, 1, 21, TRUE
	},
	{	{ TextRender, TextCompare, _T("URL Update Info") },
		{ ItemRender, ItemCompare, (PVOID)21, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 22, 1, 22, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version") },
		{ ItemRender, ItemCompare, (PVOID)22, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 23, 1, 23, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version Major") },
		{ ItemRender, ItemCompare, (PVOID)23, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 24, 1, 24, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version Minor") },
		{ ItemRender, ItemCompare, (PVOID)24, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 25, 1, 25, TRUE
	},
	{	{ TextRender, TextCompare, _T("Windows Installer") },
		{ ItemRender, ItemCompare, (PVOID)25, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsUninstallEntries) } }, { }, { }, TRUE, 26, 1, 26, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsUninstallEntries), CWindowsUninstallEntries::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Installer Assemblies") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsInstallerAssemblies) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Entry") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerAssemblies) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("IDs with Value") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_LPAKEYVALUES, { RUNTIME_CLASS(CWindowsInstallerAssemblies) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsInstallerAssemblies), CWindowsInstallerAssemblies::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Win32 Installer Assemblies") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsInstallerWin32Assemblies) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Entry") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerWin32Assemblies) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("IDs with Value") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_LPAKEYVALUES, { RUNTIME_CLASS(CWindowsInstallerWin32Assemblies) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsInstallerWin32Assemblies), CWindowsInstallerWin32Assemblies::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Type Library Entries") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsClassesTypelibEntries) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("GUID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibEntries) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibEntries) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Help Directory") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibEntries) }, CmdLineItemColor }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Flags") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibEntries) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Count") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsClassesTypelibEntries) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsClassesTypelibEntries), CWindowsClassesTypelibEntries::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Type Libraries") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsClassesTypelibs) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("GUID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibs) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibs) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("LCID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibs) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Platform") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibs) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Path") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesTypelibs) }, CmdLineItemColor }, { }, { }, TRUE, 5, 0, 5, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsClassesTypelibs), CWindowsClassesTypelibs::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("App IDs") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("AppID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Label") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("AppID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Activate At Storage") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Dll Surrogate") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Dll Surrogate Executable") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Local Service") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Service Parameters") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Remote Server Name") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 9, 0, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Run As") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 10, 0, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("End Points") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 11, 0, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Access Permission") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 12, 0, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Launch Permission") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 13, 0, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("App ID Flags") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 14, 0, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Authentication Level") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 15, 0, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Load User Settings") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 16, 0, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Preferred Server Bitness") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 17, 0, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("ROT Flags") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 18, 0, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("SRP Trust Level") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsClassesAppIDs) } }, { }, { }, TRUE, 19, 0, 19, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsClassesAppIDs), CWindowsClassesAppIDs::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Interfaces") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("IID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Interface Name") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Base Interface") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Number of Methods") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Proxy Stub Class ID") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Proxy Stub Class ID 32") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Type Library GUID") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Type Library Version") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesInterfaces) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsClassesInterfaces), CWindowsClassesInterfaces::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Class IDs") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class ID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Label") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("AppID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("In Process Server 32") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) }, CmdLineItemColor }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("In Process Server 32 Threading Model") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Local Server 32") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) }, CmdLineItemColor }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Local Server 32 Server Executable") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) }, CmdLineItemColor }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Program ID") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version Independent Program ID") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 9, 0, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Miscellaneous Status") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_LPAKEYVALUES, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 10, 0, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Tool Box Bitmap 32") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 11, 0, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("In Process Server") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 12, 0, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Local Server") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 13, 0, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("In Process Handler 32") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 14, 0, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("In Process Handler") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 15, 0, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Auto Convert To") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 16, 0, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Auto Treat As") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 17, 0, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("Treat As") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 18, 0, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("Default Icon") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 19, 0, 19, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version") },
		{ ItemRender, ItemCompare, (PVOID)19, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 20, 0, 20, TRUE
	},
	{	{ TextRender, TextCompare, _T("Control") },
		{ ItemRender, ItemCompare, (PVOID)20, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 21, 0, 21, TRUE
	},
	{	{ TextRender, TextCompare, _T("Insertable") },
		{ ItemRender, ItemCompare, (PVOID)21, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 22, 0, 22, TRUE
	},
	{	{ TextRender, TextCompare, _T("Verb") },
		{ ItemRender, ItemCompare, (PVOID)22, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 23, 0, 23, TRUE
	},
	{	{ TextRender, TextCompare, _T("Auxiliary User Type 2") },
		{ ItemRender, ItemCompare, (PVOID)24, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 24, 0, 24, TRUE
	},
	{	{ TextRender, TextCompare, _T("Auxiliary User Type 3") },
		{ ItemRender, ItemCompare, (PVOID)25, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 25, 0, 25, TRUE
	},
	{	{ TextRender, TextCompare, _T("Conversion Readable Main") },
		{ ItemRender, ItemCompare, (PVOID)29, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 26, 0, 26, TRUE
	},
	{	{ TextRender, TextCompare, _T("Conversion Read Writable Main") },
		{ ItemRender, ItemCompare, (PVOID)31, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 27, 0, 27, TRUE
	},
	{	{ TextRender, TextCompare, _T("Data Formats Default File") },
		{ ItemRender, ItemCompare, (PVOID)35, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 28, 0, 28, TRUE
	},
	{	{ TextRender, TextCompare, _T("Data Formats Get Set") },
		{ ItemRender, ItemCompare, (PVOID)36, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsClassesCLSIDs) } }, { }, { }, TRUE, 29, 0, 29, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsClassesCLSIDs), CWindowsClassesCLSIDs::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Unmanaged Network Signatures") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Signature") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Default Gateway MAC") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Description") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("DNS Suffix") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("First Network") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Profile GUID") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Source") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsNetworkSignaturesUnmanaged), CWindowsNetworkSignaturesUnmanaged::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Network Profiles") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("GUID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Category") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Category Type") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Date Created") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Date Last Connected") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Description") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Icon Type") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Managed") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name Type") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Profile Name") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkProfiles) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsNetworkProfiles), CWindowsNetworkProfiles::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Network Location Awareness Wireless Names") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsNetworkNlaWireless) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkNlaWireless) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Wireless Network Names") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_LPAHEXKEYVALUES, { RUNTIME_CLASS(CWindowsNetworkNlaWireless) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsNetworkNlaWireless), CWindowsNetworkNlaWireless::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Network Location Awareness Intranet Cache") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsNetworkNlaCacheIntranet) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkNlaCacheIntranet) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Failures") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkNlaCacheIntranet) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Successes") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkNlaCacheIntranet) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("MAC Mappings") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_LPAKEYVALUES, { RUNTIME_CLASS(CWindowsNetworkNlaCacheIntranet) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsNetworkNlaCacheIntranet), CWindowsNetworkNlaCacheIntranet::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Network Connections") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsNetworkConnections) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("GUID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkConnections) }, NetworkConnectionItemColor }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkConnections) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Media Sub Type") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkConnections) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("PNP Instance ID") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkConnections) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Default Name Index") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkConnections) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Default Name Resource Id") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkConnections) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsNetworkConnections), CWindowsNetworkConnections::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Network Devices") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Type GUID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("GUID") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Characteristics") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Component ID") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Description") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("INF Path") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("INF Section") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Time Stamp") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Loc Description") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 9, 0, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class ID") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 10, 0, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Component DLL") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 11, 0, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("CoServices") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 12, 0, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Exclude Setup Start Services") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 13, 0, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Filter Class") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 14, 0, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Filter Device INF ID") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 15, 0, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Filter Run Type") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 16, 0, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Filter Type") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 17, 0, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("Help Text") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 18, 0, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("Service") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 19, 0, 19, TRUE
	},
	{	{ TextRender, TextCompare, _T("Time Stamp") },
		{ ItemRender, ItemCompare, (PVOID)19, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 20, 0, 20, TRUE
	},
	{	{ TextRender, TextCompare, _T("Filter Media Types") },
		{ ItemRender, ItemCompare, (PVOID)20, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 21, 0, 21, TRUE
	},
	{	{ TextRender, TextCompare, _T("Lower Range") },
		{ ItemRender, ItemCompare, (PVOID)21, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 22, 0, 22, TRUE
	},
	{	{ TextRender, TextCompare, _T("Upper Range") },
		{ ItemRender, ItemCompare, (PVOID)22, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsNetworkDevices) } }, { }, { }, TRUE, 23, 0, 23, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsNetworkDevices), CWindowsNetworkDevices::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Volumes") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CVolumeGatherer) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CVolumeGatherer) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Names") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CVolumeGatherer) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Volume Path Names") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CVolumeGatherer) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Volume Serial Number") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_UI4, { RUNTIME_CLASS(CVolumeGatherer) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Maximum Component Length") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UI4, { RUNTIME_CLASS(CVolumeGatherer) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("File System Flags") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_FILESYSTEMFLAGS, { RUNTIME_CLASS(CVolumeGatherer) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("File System Name") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CVolumeGatherer) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	} } },	
	{	{ TextRender, TextCompare, _T("Logical Drives") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CDriveGatherer) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CDriveGatherer) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Names") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CDriveGatherer) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Drive Type") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_DRIVETYPE, { RUNTIME_CLASS(CDriveGatherer) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Sectors Per Cluster") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_UI4, { RUNTIME_CLASS(CDriveGatherer) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Bytes Per Sector") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UI4, { RUNTIME_CLASS(CDriveGatherer) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Free Clusters") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UI4, { RUNTIME_CLASS(CDriveGatherer) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Total Clusters") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI4, { RUNTIME_CLASS(CDriveGatherer) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	} } },	
	{	{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelObjectDirectoryTree) } }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CDirectoryTreeList) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CDirectoryTreeList) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Type") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CDirectoryTreeList) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Owner") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CDirectoryTreeList) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("DACL") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CDirectoryTreeList) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Service and Driver Registry") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Boot Flags") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Depend On Services") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Description") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("DisplayName") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Package Id") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Error Control") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Group") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("ImagePath") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsServiceDrivers) }, CmdLineItemColor }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("ObjectName") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Required Privileges") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Service Sid Type") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Start") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Tag") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Type") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 15, 1, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("WOW64") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsServiceDrivers) } }, { }, { }, TRUE, 16, 1, 16, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Services") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CServiceGatherer) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Service Name") },
		{ ItemRender, ItemCompare, (PVOID)SERVICE_SERVICENAME, DISPLAY_LPTSTR, { RUNTIME_CLASS(CServiceGatherer) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Display Name") },
		{ ItemRender, ItemCompare, (PVOID)SERVICE_DISPLAYNAME, DISPLAY_LPTSTR, { RUNTIME_CLASS(CServiceGatherer) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Status") },
		{ ItemRender, ItemCompare, (PVOID)SERVICE_CURRENTSTATE, DISPLAY_SERVICESTATUS, { RUNTIME_CLASS(CServiceGatherer) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Service Type") },
		{ ItemRender, ItemCompare, (PVOID)SERVICE_SERVICETYPE, DISPLAY_SERVICETYPE, { RUNTIME_CLASS(CServiceGatherer) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Startup Type") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_SERVICESTARTUPTYPE, { RUNTIME_CLASS(CServiceConfig) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Drivers") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_LPTSTR, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Display Name") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_LPTSTR, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Status") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_SERVICESTATUS, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Type") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_DRIVERTYPE, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Startup Type") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_SERVICESTARTUPTYPE, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Image Path") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_LPTSTR, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Load Order Group") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_LPTSTR, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Tag") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_UI4, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Dependencies") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_LPTSTRS, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 9, 0, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Tag Order") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_UI4_WITHEMPTY, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 10, 0, 10, TRUE
	}/*,
	{	{ TextRender, TextCompare, _T("Load Order") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_UI4_WITHEMPTY, { RUNTIME_CLASS(CDriverList) } }, { }, { }, TRUE, 11, 0, 11, TRUE
	}*/ } },
	{	{ TextRender, TextCompare, _T("Driver Order Groups") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CServiceOrderGroups) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Group Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_LPTSTR, { RUNTIME_CLASS(CServiceOrderGroups) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Number of Tags") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_COUNT, { RUNTIME_CLASS(CGroupLoadOrderTagListGatherer) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Order Tags") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_DWORDS, { RUNTIME_CLASS(CGroupLoadOrderTagListGatherer) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Logon Sessions") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Logon ID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_LUID, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("User Name") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_LPWSTR, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Logon Domain") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_LPWSTR, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Authentication Package") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_LPWSTR, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Logon Type") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_LOGONTYPE, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Session") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UI4, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Sid") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_SID, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Logon Time") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_FILETIME, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Logon Server") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_LPWSTR, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 9, 0, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Dns Domain Name") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_LPWSTR, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 10, 0, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Upn") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_LPWSTR, { RUNTIME_CLASS(CLogonSessionList) } }, { }, { }, TRUE, 11, 0, 11, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Hardware Classes") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("GUID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Description") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("EnumPropPages32") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Icon") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Installer32") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("LowerFilters") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("NoDisplayClass") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("NoInstallClass") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("NoUseClass") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("SilentInstall") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("TroubleShooter0") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("UpperFilters") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class Description") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Default Service") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 15, 1, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Legacy Inf Option") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 16, 1, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Wmi Config Classes") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 17, 1, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("Icon Path") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 18, 1, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("Legacy Adapter Detection") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_UI4_WITHEMPTY, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 19, 1, 19, TRUE
	},
	{	{ TextRender, TextCompare, _T("Instance Count") },
		{ ItemRender, ItemCompare, (PVOID)19, DISPLAY_UI4, { RUNTIME_CLASS(CHardwareClasses) } }, { }, { }, TRUE, 20, 1, 20, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Hardware Class Items") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class GUID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Instance ID") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("CoInstallers32") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Date") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Description") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Version") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("EnumPropPages32") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Inf Path") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Inf Section") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Matching Device Id") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Provider Name") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Resource Picker Tags") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("DriverDateData") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Migrated") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_UI4, { RUNTIME_CLASS(CHardwareClassList) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CHardwareClassList), CHardwareClassList::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Device Classes") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("GUID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Reference") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Instance") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Reference Count") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_UI4_WITHEMPTY, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("SubDevice Reference") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Symbolic Link") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Linked") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI4_WITHEMPTY, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("CLSID") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Filter Data") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 9, 0, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Friendly Name") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CDeviceClasses) } }, { }, { }, TRUE, 10, 0, 10, TRUE
	},
	} },
	{	{ TextRender, TextCompare, _T("Global Autoruns") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CGlobalAutoRuns) } },
		{ IconRenderCmdLine, NULL, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CGlobalAutoRuns) } }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CGlobalAutoRuns) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Path") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CGlobalAutoRuns) }, CmdLineItemColor }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Expanded Path") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CGlobalAutoRuns) }, CmdLineItemColor }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Section") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CGlobalAutoRuns) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("User Autoruns") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CUserAutoRuns) } },
		{ IconRenderCmdLine, NULL, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CUserAutoRuns) } }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CUserAutoRuns) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Path") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CUserAutoRuns) }, CmdLineItemColor }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Expanded Path") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CUserAutoRuns) }, CmdLineItemColor }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Section") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CUserAutoRuns) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Critical Devices") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CCriticalDevices) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CCriticalDevices) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class GUID") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CCriticalDevices) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Package Id") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CCriticalDevices) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Service") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CCriticalDevices) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Lower Filters") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CCriticalDevices) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Upper Filters") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CCriticalDevices) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	}/*,
	{	{ TextRender, TextCompare, _T("Reference") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CCriticalDeviceReferences) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	}*/ },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CCriticalDevices), CCriticalDevices::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Mounted Devices") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CMountedDevices) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("ID") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CMountedDevices) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Data") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CMountedDevices) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CMountedDevices), CMountedDevices::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Enum Entries") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Type") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Category") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Instance") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class GUID") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Container ID") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Description") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Friendly Name") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Manufacturer") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Service") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Compatible IDs") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Hardware ID") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Configuration Flags") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_CONFIGFLAGS, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Capabilities") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_UI4, { RUNTIME_CLASS(CEnumEntries) } }, { }, { }, TRUE, 15, 1, 15, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("OEM Inf Files") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("File Index") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_UI4, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("File Name") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class GUID") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version Date") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Provider") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Catalog") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Inf Count") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_UI4, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Reference") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CInfFiles) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	} },
	{ { _T("Setup"), { TextRender, TextCompare, _T("Uninstall") }, RUNTIME_CLASS(CInfFiles), InfFilesUninstallInfAction },
	{ _T("Setup"), { TextRender, TextCompare, _T("Force Uninstall") }, RUNTIME_CLASS(CInfFiles), InfFilesForceUninstallInfAction },
	{ _T("Setup"), { TextRender, TextCompare, _T("Old Uninstall") }, RUNTIME_CLASS(CInfFiles), InfFilesOldUninstallInfAction } } },
	{	{ TextRender, TextCompare, _T("Devices") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Friendly Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Description") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Manufacturer") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Service") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Class GUID") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Hardware IDs") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Compatible IDs") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Location Information") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Enumerator Name") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Configuration Flags") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_CONFIGFLAGS, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Capabilities") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_DEVCAPS, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Type") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_DEVTYPE, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install State") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_INSTALLSTATE, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 15, 1, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Status") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_DEVICESTATUS, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 16, 1, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Handle") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_UI4, { RUNTIME_CLASS(CDevices) } }, { }, { }, TRUE, 17, 1, 17, TRUE
	} },
	{ { _T("Setup"), { TextRender, TextCompare, _T("Remove") }, RUNTIME_CLASS(CDevices), DevicesRemoveAction } } },
	{	{ TextRender, TextCompare, _T("Kernel Module List") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Section") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 1, 1, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Dll Base") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Entry Point") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Size Of Image") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_UI4, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Flags") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_LDRPFLAGS, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Load Count") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UI2, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Tls Index") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI2, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Section Pointer") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("CheckSum") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_UI4, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Loaded Imports") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Full File Name") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Base Name") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Image Match") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UI4, { RUNTIME_CLASS(CKernelModuleList) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	} },
	{ { _T("Dump"), { TextRender, TextCompare, _T("Code Segments") }, RUNTIME_CLASS(CKernelModuleList), ModuleDumpAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Kernel Device List") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Object Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Object") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Size") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UI2, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Type") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_UI2, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Reference Count") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UI4, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Object") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Next Device") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Attached Device") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Current IRP") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Timer") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Flags") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_DEVOBJECTFLAGS, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Characteristics") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_DEVCHARACTERISTICS, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Vpb") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Extension") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Type") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_DEVTYPE, { RUNTIME_CLASS(CKernelDeviceList) } }, { }, { }, TRUE, 15, 1, 15, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Kernel Driver List") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Object Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Object") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Size") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UI2, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Type") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_UI2, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Device Object") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Flags") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_DRVOBJFLAGS, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Start") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Size") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_UI4, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Section") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Extension") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Fast IO Dispatch") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Init") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Start IO") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Unload") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_CREATE") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 15, 1, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_CREATE_NAMED_PIPE") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 16, 1, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_CLOSE") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 17, 1, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_READ") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 18, 1, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_WRITE") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 19, 1, 19, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_QUERY_INFORMATION") },
		{ ItemRender, ItemCompare, (PVOID)19, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 20, 1, 20, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_SET_INFORMATION") },
		{ ItemRender, ItemCompare, (PVOID)20, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 21, 1, 21, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_QUERY_EA") },
		{ ItemRender, ItemCompare, (PVOID)21, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 22, 1, 22, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_SET_EA") },
		{ ItemRender, ItemCompare, (PVOID)22, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 23, 1, 23, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_FLUSH_BUFFERS") },
		{ ItemRender, ItemCompare, (PVOID)23, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 24, 1, 24, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_QUERY_VOLUME_INFORMATION") },
		{ ItemRender, ItemCompare, (PVOID)24, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 25, 1, 25, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_SET_VOLUME_INFORMATION") },
		{ ItemRender, ItemCompare, (PVOID)25, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 26, 1, 26, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_DIRECTORY_CONTROL") },
		{ ItemRender, ItemCompare, (PVOID)26, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 27, 1, 27, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_FILE_SYSTEM_CONTROL") },
		{ ItemRender, ItemCompare, (PVOID)27, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 28, 1, 28, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_DEVICE_CONTROL") },
		{ ItemRender, ItemCompare, (PVOID)28, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 29, 1, 29, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_INTERNAL_DEVICE_CONTROL") },
		{ ItemRender, ItemCompare, (PVOID)29, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 30, 1, 30, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_SHUTDOWN") },
		{ ItemRender, ItemCompare, (PVOID)30, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 31, 1, 31, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_LOCK_CONTROL") },
		{ ItemRender, ItemCompare, (PVOID)31, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 32, 1, 32, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_CLEANUP") },
		{ ItemRender, ItemCompare, (PVOID)32, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 33, 1, 33, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_CREATE_MAILSLOT") },
		{ ItemRender, ItemCompare, (PVOID)33, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 34, 1, 34, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_QUERY_SECURITY") },
		{ ItemRender, ItemCompare, (PVOID)34, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 35, 1, 35, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_SET_SECURITY") },
		{ ItemRender, ItemCompare, (PVOID)35, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 36, 1, 36, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_POWER") },
		{ ItemRender, ItemCompare, (PVOID)36, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 37, 1, 37, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_SYSTEM_CONTROL") },
		{ ItemRender, ItemCompare, (PVOID)37, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 38, 1, 38, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_DEVICE_CHANGE") },
		{ ItemRender, ItemCompare, (PVOID)38, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 39, 1, 39, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_QUERY_QUOTA") },
		{ ItemRender, ItemCompare, (PVOID)39, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 40, 1, 40, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_SET_QUOTA") },
		{ ItemRender, ItemCompare, (PVOID)40, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 41, 1, 41, TRUE
	},
	{	{ TextRender, TextCompare, _T("IRP_MJ_PNP") },
		{ ItemRender, ItemCompare, (PVOID)41, DISPLAY_UIPTRRESOLUTION, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 42, 1, 42, TRUE
	},
	{	{ TextRender, TextCompare, _T("Driver Name") },
		{ ItemRender, ItemCompare, (PVOID)42, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 43, 1, 43, TRUE
	},
	{	{ TextRender, TextCompare, _T("Hardware Database") },
		{ ItemRender, ItemCompare, (PVOID)43, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 44, 1, 44, TRUE
	},
	{	{ TextRender, TextCompare, _T("Dll Base") },
		{ ItemRender, ItemCompare, (PVOID)44, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 45, 1, 45, TRUE
	},
	{	{ TextRender, TextCompare, _T("Entry Point") },
		{ ItemRender, ItemCompare, (PVOID)45, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 46, 1, 46, TRUE
	},
	{	{ TextRender, TextCompare, _T("Size Of Image") },
		{ ItemRender, ItemCompare, (PVOID)46, DISPLAY_UI4, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 47, 1, 47, TRUE
	},
	{	{ TextRender, TextCompare, _T("Flags") },
		{ ItemRender, ItemCompare, (PVOID)47, DISPLAY_LDRPFLAGS, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 48, 1, 48, TRUE
	},
	{	{ TextRender, TextCompare, _T("Load Count") },
		{ ItemRender, ItemCompare, (PVOID)48, DISPLAY_UI2, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 49, 1, 49, TRUE
	},
	{	{ TextRender, TextCompare, _T("Tls Index") },
		{ ItemRender, ItemCompare, (PVOID)49, DISPLAY_UI2, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 50, 1, 50, TRUE
	},
	{	{ TextRender, TextCompare, _T("Section Pointer") },
		{ ItemRender, ItemCompare, (PVOID)50, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 51, 1, 51, TRUE
	},
	{	{ TextRender, TextCompare, _T("CheckSum") },
		{ ItemRender, ItemCompare, (PVOID)51, DISPLAY_UI4, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 52, 1, 52, TRUE
	},
	{	{ TextRender, TextCompare, _T("Loaded Imports") },
		{ ItemRender, ItemCompare, (PVOID)52, DISPLAY_UIPTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 53, 1, 53, TRUE
	},
	{	{ TextRender, TextCompare, _T("Full File Name") },
		{ ItemRender, ItemCompare, (PVOID)53, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 54, 1, 54, TRUE
	},
	{	{ TextRender, TextCompare, _T("Base Name") },
		{ ItemRender, ItemCompare, (PVOID)54, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 55, 1, 55, TRUE
	},
	{	{ TextRender, TextCompare, _T("Devices") },
		{ ItemRender, ItemCompare, (PVOID)55, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CKernelDriverList) } }, { }, { }, TRUE, 56, 1, 56, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Disk Information") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Cylinders") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI8, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Media Type") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_MEDIATYPE, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Tracks Per Cylinder") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Sectors Per Track") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Bytes Per Sector") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Disk Size") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI8, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Partition Style") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_PARTITIONSTYLE, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("MBR Signature") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 9, 0, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("MBR Checksum") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 10, 0, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("GUID Partition Table") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_GUID, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 11, 0, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Detection Type") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_DETECTIONTYPE, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 12, 0, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Int13 Drive Select") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UI2, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 13, 0, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Max Cylinders") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 14, 0, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Sectors Per Track") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_UI2, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 15, 0, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Max Heads") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_UI2, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 16, 0, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Number of Drives") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_UI2, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 17, 0, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Buffer Size") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_UI2, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 18, 0, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Flags") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_UI2, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 19, 0, 19, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Cylinders") },
		{ ItemRender, ItemCompare, (PVOID)19, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 20, 0, 20, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Heads") },
		{ ItemRender, ItemCompare, (PVOID)20, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 21, 0, 21, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Sectors Per Track") },
		{ ItemRender, ItemCompare, (PVOID)21, DISPLAY_UI4, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 22, 0, 22, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Sectors Per Drive") },
		{ ItemRender, ItemCompare, (PVOID)22, DISPLAY_UI8, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 23, 0, 23, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Sector Size") },
		{ ItemRender, ItemCompare, (PVOID)23, DISPLAY_UI2, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 24, 0, 24, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Reserved") },
		{ ItemRender, ItemCompare, (PVOID)24, DISPLAY_UI2, { RUNTIME_CLASS(CDisksInfo) } }, { }, { }, TRUE, 25, 0, 25, TRUE
	} } },
	{	{ ItemRender, ItemCompare, NULL, DISPLAY_TSTR, { RUNTIME_CLASS(CDiskTree) } }, { }, { }, 0, {
	} },
	{	{ TextRender, TextCompare, _T("Hex Dump"), 0, { RUNTIME_CLASS(CDiskDump) } }, { }, { }, ~0, {
	{	{ TextRender, TextCompare, _T("") },
		{ DataRender, NULL, (PVOID)0, 0, { RUNTIME_CLASS(CDiskDump) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("") },
		{ DataRender, NULL, (PVOID)1, 0, { RUNTIME_CLASS(CDiskDump) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("") },
		{ DataRender, NULL, (PVOID)2, 0, { RUNTIME_CLASS(CDiskDump) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Partition Information"), 0, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Partition Indicator") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_PARTITIONINDICATOR, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Initial Head Sector Cylinder") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_CHS, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Partition Type") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_PARTITIONTYPE, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Final Head Sector Cylinder") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_CHS, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Relative Sector") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UI4, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Total Sectors") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UI4, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Bytes Per Sector") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI2, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 7, 0, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Sectors Per Cluster") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_UI1, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 8, 0, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Reserved Sectors") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_UI2, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 9, 0, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("FAT Copies") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_UI1, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 10, 0, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Root Directory Entries") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_UI2, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 11, 0, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Number of Sectors") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_UI2, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 12, 0, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Media Type") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UI1, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 13, 0, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Sectors Per FAT") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_UI2, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 14, 0, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Sectors Per Track") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_UI2, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 15, 0, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Number of Heads") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_UI2, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 16, 0, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Hidden Sectors") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_UI4, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 17, 0, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("Big Sectors") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_UI4, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 18, 0, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("Physical Drive Number") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_UI1, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 19, 0, 19, TRUE
	},
	{	{ TextRender, TextCompare, _T("Reserved") },
		{ ItemRender, ItemCompare, (PVOID)19, DISPLAY_UI1, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 20, 0, 20, TRUE
	},
	{	{ TextRender, TextCompare, _T("Extended Boot Signature") },
		{ ItemRender, ItemCompare, (PVOID)20, DISPLAY_UI2, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 21, 0, 21, TRUE
	},
	{	{ TextRender, TextCompare, _T("Total Sectors") },
		{ ItemRender, ItemCompare, (PVOID)21, DISPLAY_UI8, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 22, 0, 22, TRUE
	},
	{	{ TextRender, TextCompare, _T("MFT Starting Cluster Number") },
		{ ItemRender, ItemCompare, (PVOID)22, DISPLAY_UI8, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 23, 0, 23, TRUE
	},
	{	{ TextRender, TextCompare, _T("MFT Mirror Starting Cluster Number") },
		{ ItemRender, ItemCompare, (PVOID)23, DISPLAY_UI8, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 24, 0, 24, TRUE
	},
	{	{ TextRender, TextCompare, _T("Clusters Per File Record") },
		{ ItemRender, ItemCompare, (PVOID)24, DISPLAY_UI1, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 25, 0, 25, TRUE
	},
	{	{ TextRender, TextCompare, _T("Clusters Per Index Buffer") },
		{ ItemRender, ItemCompare, (PVOID)25, DISPLAY_UI1, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 26, 0, 26, TRUE
	},
	{	{ TextRender, TextCompare, _T("Volume Serial Number") },
		{ ItemRender, ItemCompare, (PVOID)26, DISPLAY_UI8, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 27, 0, 27, TRUE
	},
	{	{ TextRender, TextCompare, _T("Checksum") },
		{ ItemRender, ItemCompare, (PVOID)27, DISPLAY_UI4, { RUNTIME_CLASS(CPartitionMBRInfo) } }, { }, { }, TRUE, 28, 0, 28, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Kernel Information") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CKernelInfo) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CKernelInfo) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Value") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI8, { RUNTIME_CLASS(CKernelInfo) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("SSDT") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CSSDT) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Address") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_UIPTR, { RUNTIME_CLASS(CSSDT) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Shadow SSDT") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CShadowSSDT) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Address") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_UIPTR, { RUNTIME_CLASS(CShadowSSDT) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Explicit File Owners") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CExplicitOwnership) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("File Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CExplicitOwnership) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("SID") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CExplicitOwnership) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Account") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CExplicitOwnership) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	} },
	{ { _T("Repair"), { TextRender, TextCompare, _T("Set Child Folders and Files To Match") }, RUNTIME_CLASS(CExplicitOwnership), DefaultOwnershipAction } } },
	{	{ TextRender, TextCompare, _T("Explicit Registry Owners") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CExplicitOwnershipReg) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Key Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CExplicitOwnershipReg) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("SID") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CExplicitOwnershipReg) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Account") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CExplicitOwnershipReg) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	} },
	{ { _T("Repair"), { TextRender, TextCompare, _T("Set Child Folders and Files To Match") }, RUNTIME_CLASS(CExplicitOwnership), DefaultOwnershipRegAction } } },
	{	{ TextRender, TextCompare, _T("Windows Package Servicing Cache") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsPackageServicingCache) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Package Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsPackageServicingCache) }, PackageRegKeyExistItemColor }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("MUM File") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsPackageServicingCache) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Catalog File") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsPackageServicingCache) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Add Package") }, RUNTIME_CLASS(CWindowsPackageServicingCache), WindowsPackageServicingCacheAddAction },
	  { _T("Clean"), { TextRender, TextCompare, _T("Remove Package") }, RUNTIME_CLASS(CWindowsPackageServicingCache), WindowsPackageServicingCacheRemoveAction },
	  { _T("Clean"), { TextRender, TextCompare, _T("Delete Package Files") }, RUNTIME_CLASS(CWindowsPackageServicingCache), WindowsPackageServicingCacheDeleteAction } } },
	{	{ TextRender, TextCompare, _T("Windows Servicing Session Log Tasks") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("FileName") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("ID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) }, SessionRegKeyExistItemColor }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Client") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Options") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Current Phase") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Last Successful State") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Pending Follower") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Retry") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Queued") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Started") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Complete") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Status") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Phase Sequence") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Package ID") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) }, PackageRegKeyExistItemColor }, { }, { }, TRUE, 15, 1, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Package Name") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 16, 1, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Package Target State") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 17, 1, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("Package Options") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionTaskPackages) } }, { }, { }, TRUE, 18, 1, 18, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete Entry") }, RUNTIME_CLASS(CSessionTaskPackages), CSessionTaskPackages::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Windows Servicing Session Log Actions") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("FileName") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Version") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("ID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) }, SessionRegKeyExistItemColor }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Client") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Options") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Current Phase") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Last Successful State") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Pending Follower") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Retry") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Queued") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Started") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Complete") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Status") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Phase Sequence") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	},
	{	{ TextRender, TextCompare, _T("Reboot Required") },
		{ ItemRender, ItemCompare, (PVOID)14, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 15, 1, 15, TRUE
	},
	{	{ TextRender, TextCompare, _T("Installed") },
		{ ItemRender, ItemCompare, (PVOID)15, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 16, 1, 16, TRUE
	},
	{	{ TextRender, TextCompare, _T("Action") },
		{ ItemRender, ItemCompare, (PVOID)16, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 17, 1, 17, TRUE
	},
	{	{ TextRender, TextCompare, _T("package") },
		{ ItemRender, ItemCompare, (PVOID)17, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) }, PackageRegKeyExistItemColor }, { }, { }, TRUE, 18, 1, 18, TRUE
	},
	{	{ TextRender, TextCompare, _T("update") },
		{ ItemRender, ItemCompare, (PVOID)18, DISPLAY_TSTR, { RUNTIME_CLASS(CSessionActionPackages) } }, { }, { }, TRUE, 19, 1, 19, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete Entry") }, RUNTIME_CLASS(CSessionActionPackages), CSessionActionPackages::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Windows Component Packages") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Package Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentPackages) }, PackageExistItemColor }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Client") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Location") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentPackages) }, FileExistItemColor }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Name") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install User") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Current State") }, //0x20 is Not Present, 0x50 is Superseded, 0x70 is Installed
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Time High") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Time Low") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Last Error") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Last Progress State") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Self Update") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	},
	{	{ TextRender, TextCompare, _T("Trusted") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 12, 1, 12, TRUE
	},
	{	{ TextRender, TextCompare, _T("Visibility") },//1 is Visible, 2 is Hidden
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 13, 1, 13, TRUE
	},
	{	{ TextRender, TextCompare, _T("Owners") },
		{ ItemRender, ItemCompare, (PVOID)13, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsComponentPackages) } }, { }, { }, TRUE, 14, 1, 14, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Windows Component Package Indexes") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsComponentPackageIndexes) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Package Index") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentPackageIndexes) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Packages") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsComponentPackageIndexes) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Windows Component Package Sessions") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsComponentPackageSessions) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Session Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentPackageSessions) }, SessionExistItemColor }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Complete") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentPackageSessions) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsComponentPackageSessions), WindowsComponentPackageSessionDeleteAction } } },
	{	{ TextRender, TextCompare, _T("Windows Side By Side Component Winners") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsSideBySideComponentWinners) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Component") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsSideBySideComponentWinners) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Default Base Version") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsSideBySideComponentWinners) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Base Version") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsSideBySideComponentWinners) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Versions") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_LPAKEYVALUES, { RUNTIME_CLASS(CWindowsSideBySideComponentWinners) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Windows Components") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsComponents) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Component)") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponents) }, ComponentExistItemColor }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Closure Flags") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponents) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Identity") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsComponents) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("S256H") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsComponents) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Windows Component Families") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsComponentFamilies) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Versioned Index") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentFamilies) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Component Family") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentFamilies) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Some Unparsed Versions Exist") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsComponentFamilies) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Versions") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsComponentFamilies) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	} } },
	{	{ TextRender, TextCompare, _T("Windows Component Manifests") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsComponentManifests) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Component Name") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsComponentManifests) }, ComponentRegKeyExistItemColor }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Manifest File") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentManifests) } }, { }, { }, TRUE, 2, 1, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Catalog File") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentManifests) } }, { }, { }, TRUE, 3, 1, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Storage Folder") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentManifests) } }, { }, { }, TRUE, 4, 1, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Backup Folder") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsComponentManifests) } }, { }, { }, TRUE, 5, 1, 5, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete Component Files") }, RUNTIME_CLASS(CWindowsComponentManifests), WindowsComponentManifestsDeleteAction },
	  { _T("Clean"), { TextRender, TextCompare, _T("Delete Component Folder") }, RUNTIME_CLASS(CWindowsComponentManifests), WindowsComponentComponentDeleteAction },
	  { _T("Clean"), { TextRender, TextCompare, _T("Delete Manifest Cache") }, RUNTIME_CLASS(CWindowsComponentManifests), WindowsComponentManifestCacheDeleteAction } } },
	{	{ TextRender, TextCompare, _T("Windows Installer Products") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsInstallerProducts) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install User") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Product ID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Display Name") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Display Version") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Date") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Location") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) }, FileExistItemColor }, { }, { }, TRUE, 6, 1, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install Source") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) }, FileExistItemColor }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("Local Package") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) }, FileExistItemColor }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Modify Path") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) }, CmdLineItemColor }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("Publisher") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Uninstall String") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProducts) }, CmdLineItemColor }, { }, { }, TRUE, 11, 1, 11, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsInstallerProducts), CWindowsInstallerProducts::DeleteAction, TRUE },
	  { _T("Clean"), { TextRender, TextCompare, _T("Delete Install Source") }, RUNTIME_CLASS(CWindowsInstallerProducts), WindowsInstallerSourceDeleteAction },
	  { _T("Clean"), { TextRender, TextCompare, _T("Run Modifier") }, RUNTIME_CLASS(CWindowsInstallerProducts), WindowsInstallerModifyAction },
	  { _T("Clean"), { TextRender, TextCompare, _T("Run Uninstall String") }, RUNTIME_CLASS(CWindowsInstallerProducts), WindowsInstallerUninstallAction },
	  { _T("Clean"), { TextRender, TextCompare, _T("Run Package for Install") }, RUNTIME_CLASS(CWindowsInstallerProducts), WindowsInstallerPackageInstallAction },
	  { _T("Clean"), { TextRender, TextCompare, _T("Run Package for Uninstall") }, RUNTIME_CLASS(CWindowsInstallerProducts), WindowsInstallerPackageUninstallAction } } },
	{	{ TextRender, TextCompare, _T("Windows Installer Folders") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsInstallerFolders) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Folder") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerFolders) }, FileExistItemColor }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Info Type") },
		{ ItemRender, ItemCompare, (PVOID)1, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsInstallerFolders) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Info Value") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_HEXBYTES, { RUNTIME_CLASS(CWindowsInstallerFolders) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsInstallerFolders), CWindowsInstallerFolders::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Windows Installer Product Patches") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install User") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Product ID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Patch ID") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Display Name") },
		{ ItemRender, ItemCompare, (PVOID)5, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Installed") },
		{ ItemRender, ItemCompare, (PVOID)6, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	},
	{	{ TextRender, TextCompare, _T("LUA Enabled") },
		{ ItemRender, ItemCompare, (PVOID)7, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 6, 0, 6, TRUE
	},
	{	{ TextRender, TextCompare, _T("More Info URL") },
		{ ItemRender, ItemCompare, (PVOID)8, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 7, 1, 7, TRUE
	},
	{	{ TextRender, TextCompare, _T("MSI3") },
		{ ItemRender, ItemCompare, (PVOID)9, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 8, 1, 8, TRUE
	},
	{	{ TextRender, TextCompare, _T("Patch Type") },
		{ ItemRender, ItemCompare, (PVOID)10, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 9, 1, 9, TRUE
	},
	{	{ TextRender, TextCompare, _T("State") },
		{ ItemRender, ItemCompare, (PVOID)11, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 10, 1, 10, TRUE
	},
	{	{ TextRender, TextCompare, _T("Uninstallable") },
		{ ItemRender, ItemCompare, (PVOID)12, DISPLAY_UI4, { RUNTIME_CLASS(CWindowsInstallerProductPatches) } }, { }, { }, TRUE, 11, 1, 11, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Uninstall") }, RUNTIME_CLASS(CWindowsInstallerProductPatches), WindowsInstallerPatchUninstallBuildAction, FALSE } } },
	{	{ TextRender, TextCompare, _T("Windows Installer Patches") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsInstallerPatches) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install User") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerPatches) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Patch ID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerPatches) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Local Package") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerPatches) }, FileExistItemColor }, { }, { }, TRUE, 3, 0, 3, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsInstallerPatches), CWindowsInstallerPatches::DeleteAction, TRUE } } },
	{	{ TextRender, TextCompare, _T("Windows Installer Components") }, { }, { }, 0, {
	{	{ TextRender, TextCompare, _T("Index") },
		{ ItemRender, ItemCompare, NULL, DISPLAY_INDEX, { RUNTIME_CLASS(CWindowsInstallerComponents) } }, { }, { }, TRUE, 0, 0, 0, TRUE
	},
	{	{ TextRender, TextCompare, _T("Install User") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerComponents) } }, { }, { }, TRUE, 1, 0, 1, TRUE
	},
	{	{ TextRender, TextCompare, _T("Component ID") },
		{ ItemRender, ItemCompare, (PVOID)2, DISPLAY_TSTR, { RUNTIME_CLASS(CWindowsInstallerComponents) } }, { }, { }, TRUE, 2, 0, 2, TRUE
	},
	{	{ TextRender, TextCompare, _T("Product IDs") },
		{ ItemRender, ItemCompare, (PVOID)3, DISPLAY_LPAKEYTSTRS, { RUNTIME_CLASS(CWindowsInstallerComponents) } }, { }, { }, TRUE, 3, 0, 3, TRUE
	},
	{	{ TextRender, TextCompare, _T("Patch Product IDs") },
		{ ItemRender, ItemCompare, (PVOID)4, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsInstallerComponents) } }, { }, { }, TRUE, 4, 0, 4, TRUE
	},
	{	{ TextRender, TextCompare, _T("Display Names") },
		{ ItemRender, ItemCompare, (PVOID)0, DISPLAY_LPATSTRS, { RUNTIME_CLASS(CWindowsInstallerComponentProducts) } }, { }, { }, TRUE, 5, 0, 5, TRUE
	} },
	{ { _T("Clean"), { TextRender, TextCompare, _T("Delete") }, RUNTIME_CLASS(CWindowsInstallerComponents), CWindowsInstallerComponents::DeleteAction, TRUE } } },
	NULL
 };

 //RendererInfo Renderers[] = {
	/*{	_T("Identification"), FALSE, _T("Index"),
		DISPLAY_INDEX, { { 0, RUNTIME_CLASS(CIndexGatherer) } } },
	{	_T("Identification"), FALSE, _T("Index"),
		DISPLAY_INDEX, { { 1, RUNTIME_CLASS(CIndexGatherer) } } },
	{	_T("Identification"), FALSE, _T("Name"),
		DISPLAY_LPWSTR, { { 0, RUNTIME_CLASS(CKeyGatherer) } } },
	{	_T("Kernel Object Directory Item Properties"), FALSE, _T("Type"),
		DISPLAY_LPWSTR,
		{ { 0, RUNTIME_CLASS(CKernelObjectDirectoryTypeGatherer) } } },
	{	_T("Kernel Object Properties"), FALSE, _T("Process ID"),
		DISPLAY_UI2, { { 0, RUNTIME_CLASS(CKernelObjectProcessIDGatherer) } } },
	{	_T("Kernel Object Properties"), FALSE, _T("Handle"),
		DISPLAY_UI2, { { 0, RUNTIME_CLASS(CKernelObjectHandleGatherer) } } },
	{	_T("Kernel Object Properties"), FALSE, _T("Name"),
		DISPLAY_LPWSTR, { { 0, RUNTIME_CLASS(CKernelObjectNameGatherer) } } }*/
//};

	/*{	_T("Kernel Object Directory"), RUNTIME_CLASS(CDirectoryTreeList), NULL,
		NULL, NULL },
	{	_T("Object Types"), RUNTIME_CLASS(CObjectTypeListTree), NULL, NULL, NULL },
	{	_T("Processes"), }, RUNTIME_CLASS(CProcessList), NULL, NULL, NULL },*/

/*
TopLevelTreeItem TopLevelTreeItems[] = {
	{	TRUE, RUNTIME_CLASS(CDirectoryTreeList),
		RUNTIME_CLASS(CKernelObjectDirectoryTree),
		_T("Kernel Object Directory"), TRUE,
	  { {	_T("Index"), DISPLAY_UI4, NULL, TRUE, 2, 0, 2, TRUE },
		{	_T("Name"), DISPLAY_LPWSTR, NULL, TRUE, 0, 0, 0, TRUE },
		{	_T("Type"), DISPLAY_LPWSTR, NULL, TRUE, 1, 0, 1, TRUE } } },

	{	TRUE, RUNTIME_CLASS(CHandleList),
		RUNTIME_CLASS(CObjectTypeListTree),
		_T("ObjectTypes"),		TRUE,
	  { {	_T("Name"), DISPLAY_LPWSTR, NULL },
		{	_T("Process ID"), DISPLAY_UI2, NULL },
		{	_T("Handle"), DISPLAY_UI2, NULL },
		{	_T("Index"), DISPLAY_UI2, NULL } } },

	{	TRUE, RUNTIME_CLASS(CProcessList), NULL,
		_T("Processes"),		TRUE,
	  { {	_T("Name"), DISPLAY_TSTR, NULL },
		{	_T("ID"), DISPLAY_UI2, NULL },
		{	_T("Parent ID"), DISPLAY_UI2, NULL },
		{	_T("User Name"), DISPLAY_LPTSTR, NULL },
		{	_T("Command Line"), DISPLAY_LPTSTR, NULL },
		{	_T("Description"), DISPLAY_LPTSTR, NULL },
		{	_T("Company Name"), DISPLAY_LPTSTR, NULL },
		{	_T("Version"), DISPLAY_LPTSTR, NULL },
		{	_T("DEP Status"), DISPLAY_DEPSTATUS, NULL },
		{	_T("Window Title"), DISPLAY_LPTSTR, NULL },
		{	_T("Window Status"), DISPLAY_WINDOWSTATUS, NULL },
		{	_T("Session ID"), DISPLAY_UI4_WITHEMPTY, NULL } },
	  { { _T("&Kill"), _T("&Process") },
		{	(TCHAR*)&CProcessList::SuspendResumeCaptionFunc, _T("&Process"),
			{}, {}, TRUE },
		{ _T("&Graceful shutdown"), _T("&Process") },
		{ _T("Graceful &Restart"), _T("&Process") },
		{ _T("Kill &and Restart"), _T("&Process") },
		{ _T("&Kill"), _T("Process &Tree") },
		{ _T("&Suspend"), _T("Process &Tree") },
		{ _T("Resu&me"), _T("Process &Tree") },
		{ _T("&Restart"), _T("Process &Tree") } } },
*/