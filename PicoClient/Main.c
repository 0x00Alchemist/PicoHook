#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>

#include "Service.h"
#include "Utils.h"

#define IOCTL_REGISTER_PICO_HOOK   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1337, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_UNREGISTER_PICO_HOOK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1338, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define CMD_REGISTER_PICO_HOOK   229481147974926ULL
#define CMD_UNREGISTER_PICO_HOOK 249909586710286449ULL
#define CMD_SHOW_PICO_POC_HELP   7572797654397241ULL
#define CMD_DELETE_PICO_SERVICE  229478716795557ULL
#define CMD_EXIT_PICO_POC        7572348464577546ULL

static HANDLE hPicoDevice;


/**
 * \brief Shows help message
 */
VOID
WINAPI
ShowUsage(
	VOID
) {
	wprintf(L"[ PicoClient ] Usage: \n");
	wprintf(L"regpico - registers Pico handler (only one per OS session)\n");
	wprintf(L"unregpico - deletes Pico handler\n");
	wprintf(L"picohelp - shows this message\n");
	wprintf(L"picodel - terminates Pico service\n");
	wprintf(L"exitpico - exits from PoC\n");
}

/**
 * \brief Main controller routine
 * 
 * \param Command - User command
 * \param ServiceDeleted - Logical variable which signalizes if service has been deleted
 * 
 * \returns TRUE if user finished session 
 */
BOOLEAN
WINAPI
CommandsHandler(
	_In_  const WCHAR   *Command,
	_Out_       BOOLEAN *ServiceDeleted
) {
	DWORD Ret = 0;
	ULONG Pid = 0;
	BOOLEAN Exit = FALSE;

	*ServiceDeleted = FALSE;

	if(Command == NULL)
		return Exit;

	// calculate hash from user command
	switch(HashString(Command)) {
		case CMD_REGISTER_PICO_HOOK:
			wprintf(L"[ PicoClient ] Provide PID of target process: ");
			wscanf(L"%ld", &Pid);

			HANDLE hTarget = ULongToHandle(Pid);
			
			if(!DeviceIoControl(hPicoDevice, IOCTL_REGISTER_PICO_HOOK, &hTarget, sizeof(HANDLE), NULL, 0, &Ret, NULL))
				wprintf(L"[ PicoClient ] Unable to register Pico hook!\n");
			else
				wprintf(L"[ PicoClient ] Pico hook registered succesfully\n");
		break;
		case CMD_UNREGISTER_PICO_HOOK:
			wprintf(L"[ PicoClient ] Not implemented yet..\n");
		break;
		case CMD_SHOW_PICO_POC_HELP:
			ShowUsage();
		break;
		case CMD_DELETE_PICO_SERVICE:
			StopPicoHookService();
			DeletePicoHookService();
			
			Exit = TRUE;
			*ServiceDeleted = TRUE;
		break;
		case CMD_EXIT_PICO_POC:
			Exit = TRUE;
		break;
		default:
			ShowUsage();
		break;
	}

	return Exit;
}

/**
 * \brief Entry point of programm
 * 
 * \param Argc Number of arguments
 * \param Argv Array of arguments
 * 
 * \returns 0 if succeded 
 */
INT
CDECL
main(
	_In_ INT   Argc,
	_In_ CHAR *Argv[]
) {
	WCHAR Path[MAX_PATH] = { 0 };

	wprintf(L"[ PicoClient ] Provide path to the driver: ");
	wscanf(L"%s", Path);
	if(wcslen(Path) > MAX_PATH || Path == NULL) {
		wprintf(L"[ PicoClient ] Invalid driver path\n");
		return 1;
	}

	if(!CreatePicoHookService(Path))
		return 1;

	if(!StartPicoHookService())
		return 1;

	hPicoDevice = OpenPicoDeviceSession();
	if(hPicoDevice == NULL) {
		wprintf(L"[ PicoClient ] Unable to open device\n");
		return 1;
	}

	// parse user input
	BOOLEAN ServiceDeleted = FALSE;
	BOOLEAN ProcessCommands = FALSE;
	do {
		WCHAR Cmd[32] = { 0 };

		wprintf(L"[ PicoClient ] Provide command: ");
		wscanf(L"%s", Cmd);

		ProcessCommands = CommandsHandler(Cmd, &ServiceDeleted);
	} while(!ProcessCommands);

	// if service has been deleted - skip stop routine
	if(!ServiceDeleted)
		StopPicoHookService();

	CloseHandle(hPicoDevice);

	wprintf(L"[ PicoClient ] PoC was termianted\n");

	return 0;
}
