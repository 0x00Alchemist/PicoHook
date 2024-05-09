#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "advapi32.lib")


/**
 * \brief Starts PicoHook service
 *
 * \return TRUE - Service has been started
 * \return FALSE - Unable to start PicoHook service
 */
BOOLEAN
WINAPI
StartPicoHookService(
	VOID
) {
	SC_HANDLE hSc = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(hSc == NULL) {
		wprintf(L"[ PicoClient ] Unable to open SC Manager!\n");
		return FALSE;
	}

	SC_HANDLE hService = OpenServiceW(hSc, L"PicoHook", (SERVICE_START | SERVICE_STOP | DELETE));
	if(hService == NULL) {
		wprintf(L"[ PicoClient ] Cannot open service!\n");
		return FALSE;
	}

	if(!StartServiceW(hService, 0, NULL)) {
		DWORD dwErr = GetLastError();
		if (dwErr == ERROR_SERVICE_ALREADY_RUNNING) {
			wprintf(L"[ PicoClient ] Service already running!\n");
		} else {
			wprintf(L"[ PicoClient ] Unable to start service!\n");
			return FALSE;
		}
	} else {
		wprintf(L"[ PicoClient ] Service was started succesfully\n");
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSc);

	return TRUE;
}

/**
 * \brief Stops PicoHook service
 */
VOID
WINAPI
StopPicoHookService(
	VOID
) {
	SC_HANDLE hSc = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(hSc == NULL) {
		wprintf(L"[ PicoClient ] Unable to open SC Manager!\n");
		return;
	}

	SC_HANDLE hService = OpenServiceW(hSc, L"PicoHook", (SERVICE_START | SERVICE_STOP | DELETE));
	if(hService == NULL) {
		wprintf(L"[ PicoClient ] Cannot open service!\n");
		return;
	}

	SERVICE_STATUS SvStatus = { 0 };
	if(!ControlService(hService, SERVICE_CONTROL_STOP, &SvStatus)) {
		wprintf(L"[ PicoClient ] Cannot stop service!\n");
	} else {
		wprintf(L"[ PicoClient ] Service was stopped succesfully\n");
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSc);
}

/**
 * \brief Deletes PicoHook service
 *
 * \return TRUE - Service has been deleted
 * \return FALSE - Unable to delete service
 */
BOOLEAN
WINAPI
DeletePicoHookService(
	VOID
) {
	SC_HANDLE hSc = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(hSc == NULL) {
		wprintf(L"[ PicoClient ] Unable to open SC Manager!\n");
		return FALSE;
	}

	SC_HANDLE hService = OpenServiceW(hSc, L"PicoHook", (SERVICE_START | SERVICE_STOP | DELETE));
	if(hService == NULL) {
		wprintf(L"[ PicoClient ] Cannot open service!\n");
		return FALSE;
	}

	if(!DeleteService(hService)) {
		wprintf(L"[ PicoClient ] Cannot delete PicoHookService! You can delete it manually\n");
	} else {
		wprintf(L"[ PicoClient ] Succesfully deleted PicoHook service\n");
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSc);

	return TRUE;
}

/**
 * \brief Creates PicoHook service
 *
 * \param DriverPath Path to PicoHook driver
 *
 * \return TRUE - Service has been created
 * \return FALSE - Unable to create service
 */
BOOLEAN
WINAPI
CreatePicoHookService(
	_In_ WCHAR *DriverPath
) {
	SC_HANDLE hSc = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if(hSc == NULL) {
		wprintf(L"[ PicoClient ] Unable to open SC Manager!\n");
		return FALSE;
	}

	SC_HANDLE hService = CreateServiceW(hSc, L"PicoHook", L"PicoHook", (SERVICE_START | SERVICE_STOP | DELETE), SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, DriverPath, NULL, NULL, NULL, NULL, NULL);
	if(hService == NULL) {
		DWORD dwError = GetLastError();
		if(dwError == ERROR_SERVICE_EXISTS) {
			/// \note @0x00Alchemist: we're not uninstalling service after exit, just stopping it
			wprintf(L"[ PicoClient ] Service already exists, we'll start it..\n");
		} else {
			wprintf(L"[ PicoClient ] Cannot create service!\n");
			return FALSE;
		}
	}

	wprintf(L"[ PicoClient ] PicoHook service initialized\n");

	CloseServiceHandle(hService);
	CloseServiceHandle(hSc);

	return TRUE;
}