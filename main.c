#include <windows.h>
#include <stdio.h>
#include <setupapi.h>
#include <Dbt.h>
#include <Winusb.h> 

WCHAR pClassname[MAX_PATH] = L"Usb Notification";
GUID hUsbUid = { 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };

#define MAX_DEVICE_ID_LEN     						200
#define MAX_DEVNODE_ID_LEN    						MAX_DEVICE_ID_LEN

typedef DWORD RETURN_TYPE;
typedef RETURN_TYPE	CONFIGRET;

typedef DWORD DEVNODE, DEVINST;
typedef DEVNODE *PDEVNODE, *PDEVINST;

typedef CONFIGRET (WINAPI *CMGETDEVICEID)(DEVINST, PWSTR, ULONG, ULONG);

DWORD dwError = ERROR_SUCCESS;
BOOL SetupUsbInformationEx(PWCHAR pUsb);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL doRegisterDeviceInterfaceHwnd(GUID InterfaceClassGUID, HWND hWnd, HDEVNOTIFY *hDeviceNotify);
BOOL WinUsbInformation(PWCHAR Path);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEXW Ex;
	INT nReturn;
	MSG Message;
	ZeroMemory(&Ex, sizeof(WNDCLASSEX));
	HWND hWnd = NULL;
	
	Ex.cbSize			= sizeof(WNDCLASSEX);
	Ex.lpfnWndProc		= (WNDPROC)WindowProc;
	Ex.hInstance		= hInstance;
	Ex.lpszMenuName		= NULL;
	Ex.lpszClassName	= pClassname;
	
	if(!RegisterClassExW(&Ex))
		goto FAILURE;
		
	hWnd = CreateWindowExW(0, pClassname, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if(hWnd == NULL)
		goto FAILURE;
		
	while((nReturn = GetMessage(&Message, NULL, 0, 0)) != 0)
	{
		if(nReturn == -1)
			goto FAILURE;
		
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	
	return ERROR_SUCCESS;
	
FAILURE:
	
	dwError = GetLastError();
	
	printf("Failure: Error code: %ld\r\n", dwError);
	
	return dwError;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDEVNOTIFY hDeviceNotify;
	DWORD dwX = 0;
	
	switch(uMsg)
	{
		case WM_CREATE:
		{
			if(!doRegisterDeviceInterfaceHwnd(hUsbUid, hWnd, &hDeviceNotify))
				ExitProcess(dwError);
			
			break;
		}
		
		case WM_DEVICECHANGE:
		{
			PDEV_BROADCAST_DEVICEINTERFACE Broadcast = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
			
			switch(wParam)
			{
				case DBT_DEVICEARRIVAL:
				{
					printf("USB Device Inserted - Device Type: %ld\r\nDevice Name: %ws\r\n", pHdr->dbch_devicetype, Broadcast->dbcc_name);

					for(dwX = 0; dwX < wcslen((PWCHAR)Broadcast->dbcc_name); dwX++){  if(Broadcast->dbcc_name[dwX] == '#') Broadcast->dbcc_name[dwX] = '\\';  }
					
					dwX = wcslen((PWCHAR)Broadcast->dbcc_name);
					dwX = dwX / 2;
					Broadcast->dbcc_name[dwX + 10] = '\0';
					memmove(Broadcast->dbcc_name, Broadcast->dbcc_name + 8, dwX + 10 + 1);
					
					printf("Sanitized name: %ws\r\n", Broadcast->dbcc_name);
					
					if(!SetupUsbInformationEx((PWCHAR)Broadcast->dbcc_name))
						ExitProcess(dwError);
						
					//if(!WinUsbInformation((PWCHAR)Broadcast->dbcc_name))
					//	break;
					
					break;
				}
				case DBT_DEVICEREMOVECOMPLETE:
				case DBT_DEVNODES_CHANGED:
				default:
					break;
			}
			break;
		}
		
		case WM_CLOSE:
		{
			UnregisterDeviceNotification(hDeviceNotify);
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(ERROR_SUCCESS);
			break;
		}
		
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return ERROR_SUCCESS;
}

BOOL doRegisterDeviceInterfaceHwnd(GUID InterfaceClassGUID, HWND hWnd, HDEVNOTIFY *hDeviceNotify)
{
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    
    NotificationFilter.dbcc_size        = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype  = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid   = InterfaceClassGUID;

    *hDeviceNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
    if(*hDeviceNotify != NULL)
    	return TRUE;
    	
    dwError = GetLastError();
    return FALSE;	
}

BOOL SetupUsbInformationEx(PWCHAR pUsb)
{
	HDEVINFO hHandle;
	SP_DEVINFO_DATA DevInfo;
	DWORD dwX = ERROR_SUCCESS, dwProperty = ERROR_SUCCESS, dwSize = ERROR_SUCCESS;
	WCHAR szBuffer[4096];
	HMODULE hMod;
	CMGETDEVICEID CmGetDeviceId;
	CONFIGRET Status;
	WCHAR szDeviceInstance[MAX_DEVICE_ID_LEN], szPropertyData[4096];
	
	
	DWORD dwPropertyTypes[17] = {SPDRP_CHARACTERISTICS, SPDRP_CLASS, SPDRP_CLASSGUID, SPDRP_DEVICEDESC, SPDRP_DRIVER, 
							     SPDRP_ENUMERATOR_NAME,  SPDRP_FRIENDLYNAME, SPDRP_HARDWAREID, 
							     SPDRP_INSTALL_STATE, SPDRP_LOCATION_INFORMATION, SPDRP_LOCATION_PATHS, SPDRP_LOWERFILTERS,
							     SPDRP_MFG, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, SPDRP_SECURITY_SDS, SPDRP_UI_NUMBER_DESC_FORMAT, SPDRP_UPPERFILTERS};

	ZeroMemory(&DevInfo, sizeof(SP_DEVINFO_DATA));
	ZeroMemory(&szBuffer, sizeof(szBuffer));
	ZeroMemory(&szDeviceInstance, sizeof(MAX_DEVICE_ID_LEN));
	ZeroMemory(&szPropertyData, sizeof(szPropertyData));
	
	hHandle = SetupDiGetClassDevs(&hUsbUid, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if(hHandle == INVALID_HANDLE_VALUE)
		goto FAILURE;
		
	hMod = LoadLibrary("Cfgmgr32.dll");
	if(hMod == NULL)
		goto FAILURE;
		
	CmGetDeviceId = (CMGETDEVICEID)GetProcAddress(hMod, "CM_Get_Device_IDW");
	if(CmGetDeviceId == NULL)
		goto FAILURE;
	else
		FreeLibrary(hMod);
		
	for(dwX = 0;;dwX++)
	{
		DevInfo.cbSize = sizeof(DevInfo);
		DWORD dwNext = 0;
		
		if(!SetupDiEnumDeviceInfo(hHandle, dwX, &DevInfo))
			break;
			
		Status = CmGetDeviceId(DevInfo.DevInst, szDeviceInstance, MAX_DEVICE_ID_LEN, ERROR_SUCCESS);
		if(Status == ERROR_SUCCESS)
			_putws(szDeviceInstance);
			
		for(;dwNext < 17; dwNext++)
		{
			if(SetupDiGetDeviceRegistryPropertyW(hHandle, &DevInfo, dwPropertyTypes[dwNext], &dwProperty, (PBYTE)szPropertyData, sizeof(szPropertyData), &dwSize))
				_putws(szPropertyData);
		}
		
		printf("\r\n\r\n");
	}
	
	if(hHandle)
		SetupDiDestroyDeviceInfoList(hHandle);
	
	return TRUE;
	
FAILURE:
	
	dwError = GetLastError();
	
	if(hHandle)
		SetupDiDestroyDeviceInfoList(hHandle);
	
	return FALSE;
}

BOOL WinUsbInformation(PWCHAR Path)
{
	WINUSB_INTERFACE_HANDLE Usb;
	HANDLE hHandle = CreateFileW(Path, GENERIC_READ, 
								 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
								 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
												 
	if(hHandle == INVALID_HANDLE_VALUE)
		goto FAILURE;
		
	if(!WinUsb_Initialize(hHandle, &Usb))
		goto FAILURE;
		
	//do stuff
		
	WinUsb_Free(Usb);
		
		
	CloseHandle(hHandle);

	return TRUE;
	
FAILURE:
	
	dwError = GetLastError();
	
	if(hHandle)
		CloseHandle(hHandle);

	return FALSE;
	
}







