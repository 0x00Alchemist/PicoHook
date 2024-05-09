#include <Windows.h>

/**
 * \brief Receives handle object of the driver
 * 
 * \returns Handle if it has been obtained
 */
HANDLE
WINAPI
OpenPicoDeviceSession(
	VOID
) {
	HANDLE hDevice = CreateFileW(L"\\\\.\\PicoHook", (FILE_READ_ACCESS | FILE_WRITE_ACCESS), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
	if(hDevice == INVALID_HANDLE_VALUE)
		return NULL;

	return hDevice;
}

/**
 * \brief Calculates hash from received string
 * 
 * \param Buf User input
 * 
 * \returns Hash value
 */
UINT64
WINAPI
HashString(
	_In_ const WCHAR *Buf
) {
	UINT64 Hash = 0x1505UL;
	INT C = 0;

	while(C = *Buf++)
		Hash = ((Hash << 5) + Hash) + C;

	return Hash;
}
