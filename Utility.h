#pragma once
#include <afxtempl.h>

#define DECLARE_DYNAMIC_T(class_name, temp_name) \
public: \
	static const CRuntimeClass class##class_name; \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#define DECLARE_DYNCREATE_T(class_name, temp_name) \
	DECLARE_DYNAMIC_T(class_name, temp_name) \
	static CObject* PASCAL CreateObject();

#define _RUNTIME_CLASS_T(class_name, temp_name) ((CRuntimeClass*)(&class_name<temp_name>::class##class_name))

#define RUNTIME_CLASS_T(class_name, temp_name) _RUNTIME_CLASS_T(class_name, temp_name)

#define IMPLEMENT_RUNTIMECLASS_T(class_name, temp_name, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT const CRuntimeClass class_name<temp_name>::class##class_name = { \
	#class_name #temp_name, sizeof(class class_name<temp_name>), wSchema, pfnNew, \
			RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name<temp_name>::GetRuntimeClass() const \
		{ return RUNTIME_CLASS_T(class_name, temp_name); }

#define IMPLEMENT_DYNCREATE_T(class_name, temp_name, base_class_name) \
	CObject* PASCAL class_name<temp_name>::CreateObject() \
		{ return new class_name<temp_name>; } \
	IMPLEMENT_RUNTIMECLASS_T(class_name, temp_name, base_class_name, 0xFFFF, \
		class_name<temp_name>::CreateObject, NULL)

#define PESTRID_REGISTRY_PATH _T("SOFTWARE\\PestRid")
#define PESTRID_DISPLAY_SUBKEY _T("\\Display")
#define PESTRID_PANE_VALUENAME _T("PaneWidth")

//thread communication messages
#define WM_PESTRID_UPDATEUI				WM_USER + 0

class CCopyDWordArray : public CDWordArray
{
public:
	CCopyDWordArray() {}
	CCopyDWordArray& operator=(CCopyDWordArray& src)
	{
		this->Transfer(src);
		return *this;
	}
	void Transfer(CCopyDWordArray& src)
	{
		ASSERT_VALID(this);
		RemoveAll();
		m_nGrowBy = src.m_nGrowBy;
		m_nMaxSize = src.m_nMaxSize;
		m_nSize = src.m_nSize;
		m_pData = src.m_pData;
		src.m_pData = NULL;
		src.m_nSize = src.m_nMaxSize = 0;
	}
protected:
	CCopyDWordArray(CCopyDWordArray& src)
	{
		*this = src;
	}
};

class CCopyStringArray : public CStringArray
{
public:
	CCopyStringArray() {}
	CCopyStringArray& operator=(CCopyStringArray& src)
	{
		this->Transfer(src);
		return *this;
	}
	void Transfer(CCopyStringArray& src)
	{
		ASSERT_VALID(this);
		RemoveAll();
		m_nGrowBy = src.m_nGrowBy;
		m_nMaxSize = src.m_nMaxSize;
		m_nSize = src.m_nSize;
		m_pData = src.m_pData;
		src.m_pData = NULL;
		src.m_nSize = src.m_nMaxSize = 0;
	}
	BOOL operator==(CStringArray& src)
	{
		DWORD dwCount;
		if (GetCount() != src.GetCount()) return FALSE;
		for (dwCount = 0; dwCount < (DWORD)GetCount(); dwCount++) {
			if (GetData()[dwCount].Compare(src.GetData()[dwCount]) != 0) return FALSE;
		}
		return TRUE;
	}
	int Compare(CStringArray& src)
	{
		if (GetCount() == src.GetCount()) {
			DWORD dwCount;
			for (dwCount = 0; dwCount < (DWORD)GetCount(); dwCount++) {
				int iRet = GetAt(dwCount).Compare(src.GetAt(dwCount));
				if (iRet != 0) return iRet;
			}
			return 0;
		} else
			return GetCount() > src.GetCount() ? 1 : -1;
	}
protected:
	CCopyStringArray(CCopyStringArray& src)
	{
		*this = src;
	}
};

class CCopyByteArray : public CByteArray
{
public:
	CCopyByteArray() {}
	CCopyByteArray& operator=(CCopyByteArray& src)
	{
		this->Transfer(src);
		return *this;
	}
	void Transfer(CCopyByteArray& src)
	{
		ASSERT_VALID(this);
		RemoveAll();
		m_nGrowBy = src.m_nGrowBy;
		m_nMaxSize = src.m_nMaxSize;
		m_nSize = src.m_nSize;
		m_pData = src.m_pData;
		src.m_pData = NULL;
		src.m_nSize = src.m_nMaxSize = 0;
	}
protected:
	CCopyByteArray(CCopyByteArray& src)
	{
		*this = src;
	}
};

template<class TYPE, class ARG_TYPE = const TYPE&>
class CCopyArray : public CArray<TYPE, ARG_TYPE>
{
public:
	CCopyArray() {}
	CCopyArray& operator=(CCopyArray<TYPE, ARG_TYPE>& src)
	{
		this->Transfer(src);
		return *this;
	}
	void Transfer(CCopyArray<TYPE, ARG_TYPE>& src)
	{
		ASSERT_VALID(this);
		RemoveAll();
		m_nGrowBy = src.m_nGrowBy;
		m_nMaxSize = src.m_nMaxSize;
		m_nSize = src.m_nSize;
		m_pData = src.m_pData;
		src.m_pData = NULL;
		src.m_nSize = src.m_nMaxSize = 0;
	}
	BOOL operator==(CArray<TYPE, ARG_TYPE>& src)
	{
		DWORD dwCount;
		if (GetCount() != src.GetCount()) return FALSE;
		for (dwCount = 0; dwCount < (DWORD)GetCount(); dwCount++) {
			if (!(GetData()[dwCount] == src.GetData()[dwCount])) return FALSE;
		}
		return TRUE;
	}
	int Compare(CArray<TYPE, ARG_TYPE>& src)
	{
		if (GetCount() == src.GetCount()) {
			DWORD dwCount;
			for (dwCount = 0; dwCount < (DWORD)GetCount(); dwCount++) {
				int iRet = GetAt(dwCount).Compare(src.GetAt(dwCount));
				if (iRet != 0) return iRet;
			}
			return 0;
		} else
			return GetCount() > src.GetCount() ? 1 : -1;
	}
protected:
	CCopyArray(CCopyArray& src)
	{
		*this = src;
	}
};

struct KeyValue
{
	CString Key;
	CCopyByteArray Value;
	DWORD dwType;
	KeyValue& operator=(KeyValue& src)
	{
		Key = src.Key;
		Value = src.Value;
		dwType = src.dwType;
		return *this;
	}
	BOOL operator==(KeyValue& src)
	{
		return	dwType == src.dwType &&
				Key.Compare(src.Key) == 0 &&
				Value.GetSize() == src.Value.GetSize() &&
				memcmp(Value.GetData(), src.Value.GetData(), Value.GetSize()) == 0;
	}
	int Compare(KeyValue& src)
	{
		if (dwType == src.dwType) {
			int iRet = Key.Compare(src.Key);
			if (iRet != 0) return iRet;
			if (Value.GetSize() == src.Value.GetSize()) {
				return memcmp(Value.GetData(), src.Value.GetData(), Value.GetSize());
			} else
				return Value.GetSize() > src.Value.GetSize() ? 1 : -1;
		} else
			return dwType > src.dwType ? 1 : -1;
	}
};

typedef CArray<KeyValue, KeyValue&> CKVArray;
typedef CCopyArray<KeyValue, KeyValue&> CCopyKVArray;

class CTransferMapPtrToPtr : public CMapPtrToPtr
{
public:
	void Transfer(CTransferMapPtrToPtr & Map)
	{
		RemoveAll();
		m_pHashTable = Map.m_pHashTable;
		m_nHashTableSize = Map.m_nHashTableSize;
		m_nCount = Map.m_nCount;
		m_pFreeList = Map.m_pFreeList;
		m_pBlocks = Map.m_pBlocks;
		m_nBlockSize = Map.m_nBlockSize;
		Map.m_pHashTable = NULL;
		Map.m_nCount = 0;
		Map.m_pFreeList = NULL;
		Map.m_pBlocks = NULL;
	}
	//copies pointers keys and values which may need to be also copied
	void Copy(CTransferMapPtrToPtr & Map)
	{
		RemoveAll();
		InitHashTable(Map.GetHashTableSize());
		POSITION pos = Map.GetStartPosition();
		while (pos) {
			void* rKey;
			void* rValue;
			Map.GetNextAssoc(pos, rKey, rValue);
			SetAt(rKey, rValue);
		}
	}
};

struct AddressResolutionTable
{
	PVOID pvAddress;
	PVOID pvDLLBase;
	LPCTSTR szBaseName;
};

#include <pshpack1.h>

struct CHS
{
	BYTE Head;
	WORD Sector:6;
	WORD Cylinder:10;
};

#define CYLINDER_VALUE(cyl) (((cyl & 0x3FC) >> 2) | ((cyl & 2) << 8))

struct MBRPartitionEntry
{
	BYTE BootIndicator;
	CHS FirstSector;
    BYTE  PartitionType;
	CHS LastSector;
	DWORD RelativeSectors;
	DWORD TotalSectors;
};

struct BPBEntry
{
	WORD BytesPerSector;
	BYTE SectorsPerCluster;
	WORD ReservedSectors;
	BYTE FATCopies;
	WORD RootDirEntries;
	WORD NumSectors;
	BYTE MediaType;
	WORD SectorsPerFAT;
	WORD SectorsPerTrack;
	WORD NumberOfHeads;
	DWORD HiddenSectors;
	DWORD SectorsBig;
	BYTE PhysicalDriveNumber;
	BYTE Reserved;
	WORD ExtendedBootSignature;
	QWORD TotalSectors;
	QWORD MFTStartingClusterNumber;
	QWORD MFTMirrorStartingClusterNumber;
	DWORD ClustersPerFileRecord; //If this number is positive (up to 0x7F), it represents Clusters per MFT record. If the number is negative (0x80 to 0xFF), the size of the File Record is 2 raised to the absolute value of this number.
	DWORD ClustersPerIndexBuffer;
	QWORD VolumeSerialNumber;
	DWORD Checksum;
};

#include <poppack.h>

typedef CArray<CStringW, CStringW&> CStringWArray;

void EliminateDupStrings(CStringArray & Strings, BOOL bSort);
void StringArrayFromLPTSTRS(TCHAR* pStrs, CStringArray& Strings);
void StringArrayFromLPWSTRS(WCHAR* pStrs, CStringWArray & Strings);
void LPTSTRSFromStringArray(CStringArray& Strings, CByteArray & Bytes);
void UniStringToCStringW(UNICODE_STRING* ustr, CStringW & Str);
void RemoveTrailingBackslash(CString & String);
CString GetHexBytes(BYTE* pbBytes, DWORD dwLength);
CString GetAsciiBytes(BYTE* pbBytes, DWORD dwLength);

template <class T>
int natcmp(const T* szFirst, const T* szSecond);
template <class T>
int naticmp(const T* szFirst, const T* szSecond);

#define strnatcmp natcmp<char>
#define _tcsnatcmp natcmp<TCHAR>
#define _wcsnatcmp natcmp<WCHAR>
#define strinatcmp naticmp<char>
#define _tcsinatcmp naticmp<TCHAR>
#define _wcsinatcmp naticmp<WCHAR>

BOOL InstallAndStartDriver(LPCTSTR DriverName, LPCTSTR ServiceExe);
BOOL StartDriver(LPCTSTR DriverName);
BOOL UnloadDriver(LPCTSTR DosName, LPCTSTR DosNameGlobal,
						 LPCTSTR DriverName);
HANDLE LoadDriver(BOOL *fNTDynaLoaded, LPCTSTR DosName,
						 LPCTSTR DosNameGlobal, LPCTSTR DriverName,
						 LPCTSTR ServicePathExe);
BOOL DriverQueryUnicodeString(DWORD IoControlCode, LPVOID InputBuffer,
							  DWORD InputBufferLength, CStringW & Str);
BOOL DriverQueryLPWSTRs(DWORD IoControlCode, LPVOID InputBuffer,
						DWORD InputBufferLength, CStringWArray & Strings);
BOOL DriverQuery(DWORD IoControlCode, LPVOID InputBuffer,
						DWORD InputBufferLength, LPVOID OutputBuffer,
						DWORD OutputBufferLength);