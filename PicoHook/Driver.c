#include <ntddk.h>
#include "PicoHook.h"

#define IOCTL_REGISTER_PICO_HOOK   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1337, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_UNREGISTER_PICO_HOOK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1338, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

static BOOLEAN PicoAlreadyAssigned = FALSE;


/**
 * \brief Main control routine which handles commands from PicoClient
 * 
 * \param DriverObject Driver object
 * \param Irp I/O Request packet
 * 
 * \return STATUS_SUCCESS - Succesfully dispatched command
 * \return STATUS_INVALID_PARAMETER - Catched second pico handler registration
 * \return STATUS_INVALID_DEVICE_REQUEST - Invalid IOCTL code
 * \return Other - Unable to register or unregister pico handler
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DriverDispatch(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PIRP           Irp
) {
	PAGED_CODE();

	UNREFERENCED_PARAMETER(DriverObject);

	PIO_STACK_LOCATION IoLocation = IoGetCurrentIrpStackLocation(Irp);

	// Dispatch IOCTL code
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG DispatchCode = IoLocation->Parameters.DeviceIoControl.IoControlCode;
	switch(DispatchCode) {
		case IOCTL_REGISTER_PICO_HOOK:
			// Only 1 handler per session
			if(!PicoAlreadyAssigned) {
				KdPrint(("[ PicoHook ] Registering pico hook handler..\n"));
				
				// Activate pico handler
				Status = PicoHookController(Irp->AssociatedIrp.SystemBuffer, TRUE);
				if (!NT_SUCCESS(Status)) {
					KdPrint(("[ PicoHook ] Unable to register pico hook (0x%X)\n", Status));
				} else {
					KdPrint(("[ PicoHook ] Succesfully registered pico hook\n"));
					PicoAlreadyAssigned = TRUE;
				}
			} else {
				KdPrint(("[ PicoHook ] Unable to assign more than 1 pico handler!\n"));
				Status = STATUS_INVALID_PARAMETER;
			}

			Irp->IoStatus.Information = 0;
		break;
		case IOCTL_UNREGISTER_PICO_HOOK:
			KdPrint(("[ PicoHook ] Unregistering pico hook handler..\n"));
			Irp->IoStatus.Information = 0;
			Status = STATUS_SUCCESS;
		break;
		default:
			KdPrint(("[ PicoHook ] Invalid dispatch code\n"));
			
			Irp->IoStatus.Information = 0;
			Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}

/**
 * \brief Unimplemented routine
 *
 * \param DriverObject Driver object
 * \param Irp I/O request packet
 *
 * \return STATUS_NOT_IMPLEMENTED - Signalize that this routine not implemented yet
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DriverUnimplemented(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PIRP           Irp
) {
	PAGED_CODE();

	UNREFERENCED_PARAMETER(DriverObject);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_NOT_IMPLEMENTED;
}

/**
 * \brief Create/Close routine
 * 
 * \param DriverObject Driver object
 * \param Irp I/O request packet
 * 
 * \return STATUS_SUCCESS - Succesfully handled create/close event
 */
_IRQL_requires_(PASSIVE_LEVEL) 
NTSTATUS
NTAPI
DriverCreateClose(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PIRP           Irp
) {
	PAGED_CODE();

	UNREFERENCED_PARAMETER(DriverObject);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS; 
}

/**
 * \brief Unloads driver from the system
 * 
 * \param DriverObject Driver object
 * 
 * \return STATUS_SUCCESS - Driver succesfully unloaded
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
) {
	PAGED_CODE();

	KdPrint(("[ PicoHook ] Called unload routine\n"));

	// Delete symlink and device
	UNICODE_STRING Symlink = RTL_CONSTANT_STRING(L"\\DosDevices\\PicoHook");
	IoDeleteSymbolicLink(&Symlink);

	IoDeleteDevice(DriverObject->DeviceObject);

	return STATUS_SUCCESS;
}

/**
 * \brief Driver entry point
 * 
 * \param DriverObject Driver object
 * \param RegistryPath Registry path
 * 
 * \return STATUS_SUCCESS - Driver initalized
 * \return Other - Cannot register device, symlink or pico handler
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
) {
	PAGED_CODE();

	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrint(("[ PicoHook ] Initializating driver...\n"));

	// reassign IRP handlers
	for(INT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		DriverObject->MajorFunction[i] = DriverUnimplemented;

	DriverObject->DriverUnload = DriverUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatch;

	DEVICE_OBJECT DeviceObject;
	RtlSecureZeroMemory(&DeviceObject, sizeof(DeviceObject));

	// create device object
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\PicoHook");
	NTSTATUS Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &DeviceObject);
	if(!NT_SUCCESS(Status)) {
		KdPrint(("[ PicoHook ] Unable to create device object\n"));
		return Status;
	}

	DeviceObject.Flags |= DO_BUFFERED_IO;
	DeviceObject.Flags &= DO_DEVICE_INITIALIZING;

	// create DOS symlink
	UNICODE_STRING Symlink = RTL_CONSTANT_STRING(L"\\DosDevices\\PicoHook");
	Status = IoCreateSymbolicLink(&Symlink, &DeviceName);
	if(!NT_SUCCESS(Status)) {
		KdPrint(("[ PicoHook ] Unable to create symbolic link for device\n"));
		return Status;
	}

	// Register free alt handler. He should be only 1 in the system (per session) 
	Status = PicoCheckAndActivateFreeHandler();
	if(!NT_SUCCESS(Status))
		KdPrint(("[ PicoHook ] Unable to register alt handler\n"));

	return Status;
}
