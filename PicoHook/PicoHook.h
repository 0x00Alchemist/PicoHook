#pragma once

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
PicoCheckAndActivateFreeHandler(
	VOID
);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
PicoHookController(
	_In_ PHANDLE hTarget,
	_In_ BOOLEAN RegisterCallback
);

