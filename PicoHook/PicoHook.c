#include <ntifs.h>

#define PROCESS_INFO_ACTIVATE_ALT_CALLBACK 0x64

typedef BOOLEAN PICO_CALLBACK_HANDLER(_In_ PKTRAP_FRAME);

NTKERNELAPI
NTSTATUS
NTAPI
PsRegisterAltSystemCallHandler(
	_In_ PICO_CALLBACK_HANDLER Handler,
	_In_ INT                   HandlerIndex
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
	_In_ HANDLE hProcess,
	_In_ ULONG  ProcessInfoClass,
	_In_ PVOID  ProcessInfo,
	_In_ ULONG  ProcessInfoLength
);


/**
 * \brief Alt hook routine. Monitors all syscalls and modifies specified syscalls
 * 
 * \param CurrentFrame Pointer on KTRAP_FRAME structure with all info before syscall dispatch
 * 
 * \returns TRUE if everything lit
 */
_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
NTAPI
PicoCallback(
	_In_ PKTRAP_FRAME CurrentFrame
) {
	PAGED_CODE();

	// log called syscall
	ULONG SyscallNumber = CurrentFrame->Rax;
	KdPrint(("[ PicoHook ] Syscall invoked: %d (0x%X)\n", SyscallNumber, SyscallNumber));

	/// \todo @0x00Alchemist: realize handling of certain system calls by their numbers

	return TRUE;
}

/**
 * \brief Activates free Pico handler, should be called once
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
PicoCheckAndActivateFreeHandler(
	VOID
) {
	PAGED_CODE();

	/// \note @0x00Alchemist: the second handler is usually a free handler. However, this may not 
	/// always be the case. On one build of the system I had 1 (literally) written for some strange reason, 
	/// I still haven't figured out why. If we try to re-register the handler, the system throws bugcheck 
	/// 0x1E0 (INVALID_ALTERNATE_SYSTEM_CALL_HANDLER_REGISTRATION). In order to avoid the bugcheck, 
	/// we will check if this handler is already busy.
	 
	// get PsAltSystemCallHandlers address at offset 0x6A
	PVOID Raw = (PVOID)((UINT8 *)PsRegisterAltSystemCallHandler + 0x6A);

	// extract address of PsAltSystemCallHandlers (3 bc lea instruction)
	PVOID PsAltSystemCallHandlers = (PVOID)((UINT8 *)Raw + *(INT32 *)((UINT8 *)Raw + 3) + 3 + sizeof(INT32));
	KdPrint(("Handlers: 0x%llX\n", PsAltSystemCallHandlers));

	// check if handler is free
	if(*((UINT8 *)PsAltSystemCallHandlers + sizeof(PVOID)) != NULL) {
		KdPrint(("[ PicoHook ] Handler already registered\n"));
		return STATUS_ALREADY_INITIALIZED;
	} else {
		KdPrint(("[ PicoHook ] Handler is free\n"));
		PsRegisterAltSystemCallHandler(&PicoCallback, 1);
	}

	return STATUS_SUCCESS;
}

/**
 * \brief Activates Alt handler for the target process
 * 
 * \param hTarget Handle of the target process
 * 
 * \return STATUS_SUCCESS - Callback activated succesfully
 * \return Other - Unable to open process with all accesses or set process information
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
PicoActivateAltCallback(
	_In_ HANDLE hTarget
) {
	PAGED_CODE();

	/// \note @0x00Alchemist: For some reason, direct modification of EPROCESS/KPROCESS 
	/// and ETHREAD/KTHREAD structures is unstable, because of which only 4 system call 
	/// messages will be output. Therefore, we use the same method as in the WinAltSyscall repository.

	CLIENT_ID ClientId;
	ClientId.UniqueProcess = hTarget;
	ClientId.UniqueThread = 0;

	OBJECT_ATTRIBUTES ObjAttributes;
	InitializeObjectAttributes(&ObjAttributes, NULL, 0, NULL, NULL);

	// open new handle with full accesses to the target process
	HANDLE hFull;
	NTSTATUS Status = ZwOpenProcess(&hFull, PROCESS_ALL_ACCESS, &ObjAttributes, &ClientId);
	if(!NT_SUCCESS(Status)) {
		KdPrint(("[ PicoHook ] Unable to open process\n"));
		return Status;
	}

	// activate alt callback handler for the whole process
	Status = ZwSetInformationProcess(hFull, PROCESS_INFO_ACTIVATE_ALT_CALLBACK, &hTarget, 1);
	if(!NT_SUCCESS(Status))
		KdPrint(("[ PicoHook ] Unable to set process info\n"));
	
	ObCloseHandle(hFull, KernelMode);

	return Status;
}

/**
 * \brief Main controller of Pico (de)installation
 * 
 * \param TargetInfo Handle of the target process
 * \param RegisterCallback Logical switch which shows 
 * 
 * \return STATUS_SUCCESS - Pico handler installed succesfully
 * \return STATUS_INVALID_PARAMETER - Invalid process handle
 * \return Other - Unable to activate alt callback
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
PicoHookController(
	_In_ PHANDLE TargetInfo,
	_In_ BOOLEAN RegisterCallback
) {
	PAGED_CODE();

	// check if process handle invalid
	if(TargetInfo == NULL) {
		KdPrint(("[ PicoHook ] Invalid input buffer\n"));
		return STATUS_INVALID_PARAMETER;
	}

	NTSTATUS Status = STATUS_SUCCESS;

	// process activation or deactivation routine
	HANDLE hTarget = *TargetInfo;
	if(RegisterCallback) {
		Status = PicoActivateAltCallback(hTarget);
		if(!NT_SUCCESS(Status))
			KdPrint(("[ PicoHook ] Unable to activate alt callback! (0x%X)\n", Status));
	} else {
		/// \todo @0x00Alchemist: implement deactivation.
		/// Perhaps disabling the AltSyscall bit in EPROCESS/KPROCESS 
		/// and threads (ETHREAD/KTHREAD) will suffice
	}

	return Status;
}
