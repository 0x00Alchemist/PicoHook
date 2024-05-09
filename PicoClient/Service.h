#pragma once

BOOLEAN
WINAPI
StartPicoHookService(
	VOID
);

VOID
WINAPI
StopPicoHookService(
	VOID
);

BOOLEAN
WINAPI
DeletePicoHookService(
	VOID
);

BOOLEAN
WINAPI
CreatePicoHookService(
	_In_ WCHAR *DriverPath
);
