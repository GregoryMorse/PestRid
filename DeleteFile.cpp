#include "stdafx.h"

//CArray<HANDLE> m_LockFileArray;

/*while (m_LockFileArray.GetCount()) {
	if (!CloseHandle(m_LockFileArray[0])) {
		AddTraceLog(_T("APICall=CloseHandle LockFile=%08X Error=%08X\r\n"),
					m_LockFileArray[0], GetLastError());
	}
	m_LockFileArray.RemoveAt(0);
}*/

/*void CPestRidSplitterWnd::OnLbnDblclkProclist()
{
	// need NSIS wrapper
	
	DWORD Count;
	DWORD Counter;
	HANDLE hFile;
	CStringArray FileNames;
	INT* SelItems;
	HANDLE* SelHandles;
	DWORD SelCount;
	int nItem = -1;
	if ((SelCount = m_ProcList->GetSelectedCount()) != -1) {
		SelItems = new INT[SelCount];
		SelHandles = new HANDLE[SelCount];
		for (Count = 0; Count < SelCount; Count++) {
			nItem = m_ProcList->GetNextItem(nItem, LVNI_SELECTED);
			SelItems[Count] = nItem;
		}
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		for (Counter = 0; Counter < SelCount; Counter++) {
			if ((SelHandles[Counter] =
					OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION |
								PROCESS_VM_READ, FALSE,
								(DWORD)m_ProcList->
										GetItemData(SelItems[Counter]))) && 
								(SelHandles[Counter] != 
								 INVALID_HANDLE_VALUE)) {
				if (m_PidPathArray[SelItems[Counter]].IsEmpty()) {
					FileNames.RemoveAll();
					Str.Format(	"Error getting image file name [%08X] code = %08X",
								m_ProcList->GetItemData(SelItems[Counter]),
								GetLastError());
					SetThreadPriority(	GetCurrentThread(),
										THREAD_PRIORITY_NORMAL);
					MessageBox(Str);
					break;
				} else
					FileNames.Add(m_PidPathArray[SelItems[Counter]]);
				TerminateProcess(SelHandles[Counter], 0);
				CloseHandle(SelHandles[Counter]);
			} else {
				FileNames.RemoveAll();
				Str.Format(	"Error opening process [%08X] code = %08X",
							m_ProcList->GetItemData(SelItems[Counter]),
							GetLastError());
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
				MessageBox(Str);
				break;
			}
		}
		for (Counter = 0; Counter < (DWORD)FileNames.GetCount(); Counter++) {
			Count = 0;
			while (true) {
				//try better strategies open with no share first
				//  then also allow write share and fight, etc
				if ((hFile = CreateFile(FileNames[Counter], 
										GENERIC_WRITE, FILE_SHARE_READ, NULL,
										CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
										NULL)) != INVALID_HANDLE_VALUE) {
					m_LockFileArray.Add(hFile);
					break;
				} else {
					if (GetLastError() != ERROR_SHARING_VIOLATION) {
						Str.Format(	"Create File failed with error %08X",
									GetLastError());
						MessageBox(Str);
						break;
					}
				}
				Count++;
				if (Count > 2000)
					Sleep(55);
			}
		}
		//should also enumerate through all handles on system
		//  and manually close any handles still open for the process
		//should also do the same for the file handle on the object
		//  not just the process handle
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		delete [] SelItems;
		delete [] SelHandles;
	}
}*/

