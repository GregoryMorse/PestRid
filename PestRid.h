#pragma once

// PestRid.h : main header file for the PROJECT_NAME application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

#include "WinAPI.h"

//global registry class should consider thread safety and other sync issues
class CPestRidSettings
{
public:
	CPestRidSettings()
	{
		//if (!OpenRegKey(HKEY_LOCAL_MACHINE, PESTRID_REGISTRY_PATH, m_hKey)) ASSERT(FALSE);
		//CreateRegKey(HKEY_LOCAL_MACHINE, PESTRID_REGISTRY_PATH, m_hKey);
		
	}
	~CPestRidSettings()
	{
		//if (!CloseRegKey(m_hKey)) ASSERT(FALSE);
	}
	BOOL ReadPaneSize(DWORD & dwSize)
	{
		return PRU_ReadRegistryDWORD(	HKEY_LOCAL_MACHINE,
									PESTRID_REGISTRY_PATH
									PESTRID_DISPLAY_SUBKEY,
									PESTRID_PANE_VALUENAME, dwSize);
	}
	BOOL WritePaneSize(DWORD dwSize)
	{
		return PRU_WriteRegistryDWORD(	HKEY_LOCAL_MACHINE,
									PESTRID_REGISTRY_PATH
									PESTRID_DISPLAY_SUBKEY,
									PESTRID_PANE_VALUENAME, dwSize);
	}
	//may want to leave HKEY cached for root key and some subkeys...
	BOOL WriteColumnMap(CByteArray & RegDataArray, LPCTSTR szName)
	{
		return PRU_WriteRegistryBinary(	HKEY_LOCAL_MACHINE,
									PESTRID_REGISTRY_PATH
									PESTRID_DISPLAY_SUBKEY,
									szName, RegDataArray);
	}
	BOOL ReadColumnMap(CByteArray & RegDataArray, LPCTSTR szName)
	{
		return PRU_ReadRegistryBinary(	HKEY_LOCAL_MACHINE,
									PESTRID_REGISTRY_PATH
									PESTRID_DISPLAY_SUBKEY,
									szName, RegDataArray);
	}
protected:
	HKEY m_hKey;
};

extern CPestRidSettings g_PestRidSettings;

// CPestRidApp:
// See PestRid.cpp for the implementation of this class
//

class CPestRidApp : public CWinApp
{
public:
	CPestRidApp();

// Overrides
	virtual BOOL InitInstance();
	virtual int ExitInstance() { CoUninitialize(); return CWinApp::ExitInstance(); }
	void OnAppAbout();

// Implementation
	DECLARE_MESSAGE_MAP()
};

extern CPestRidApp theApp;