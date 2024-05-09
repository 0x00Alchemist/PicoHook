#pragma once

HANDLE
WINAPI
OpenPicoDeviceSession(
	VOID
);

UINT64
WINAPI
HashString(
	_In_ const WCHAR *Buf
);
