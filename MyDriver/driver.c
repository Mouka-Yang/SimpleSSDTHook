#include <ntddk.h>
#include <tchar.h>

#include "driver.h"
#include "dispatch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, CreateDeviceIo )
#pragma alloc_text( PAGE, CloseDeviceIo )
#pragma alloc_text( PAGE, DriverUnload )

//#pragma alloc_text( PAGE, IoUnsupported )
//#pragma alloc_text( PAGE, IoDispatch )
//#pragma alloc_text( PAGE, BufferedWrite )
#endif // ALLOC_PRAGMA


PDEVICE_OBJECT g_pDeviceObject = NULL;


NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegstryPath)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(RegstryPath);
	DriverObject->DriverUnload = DriverUnload;

	/*for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = IoDispatch;
	}*/

	//
	// Add IO dispatch function
	//
	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseDeviceIo;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseDeviceIo;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = BufferedWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlDeviceIo;


	SetupDeviceIo(DriverObject);

	DebugPrint(("[ MyDriver ] MyDriver loaded.\n"));

	return STATUS_SUCCESS;
}

VOID DriverUnload(
	IN	PDRIVER_OBJECT DriverObject)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(DriverObject);

	UNICODE_STRING linkName;
	NTSTATUS ns;

	//
	// Delete the user-mode symbolic link and deviceobjct.
	//
	RtlInitUnicodeString(&linkName, DEVICE_SYM_LINK);
	if (g_pDeviceObject)
	{
		// Delete device symbolic link before delete device object
		if ((ns = IoDeleteSymbolicLink(&linkName)) != STATUS_SUCCESS)
		{
			DebugPrint(("[ MyDriver ] Error delete symbolic link. status = %x.\n", ns));
		}
		IoDeleteDevice(g_pDeviceObject);
		DebugPrint(("[ MyDriver ] Deleted IO device and symbolic link.\n"));
	}
	DebugPrint(("[ MyDriver ] MyDriver unloaded.\n"));
}

NTSTATUS SetupDeviceIo(
	IN PDRIVER_OBJECT pDriverOject
)
{
	NTSTATUS ns;
	PDEVICE_EXTENSION pDeviceExtension;
	UNICODE_STRING uDeviceName;
	UNICODE_STRING uSymLink;

	// Initialize device name and symbolic name
	RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&uSymLink, DEVICE_SYM_LINK);

	DebugPrint(("[ MyDriver ] Creating I\\O device with name: %ws\n", uDeviceName.Buffer));

	ns = IoCreateDevice(
		pDriverOject,
		0,
		&uDeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&g_pDeviceObject);
	if (!NT_SUCCESS(ns))
	{
		DebugPrint(("[ MyDriver ] Error creating device. status = %x\n", (PVOID)ns));
		return ns;
	}

	g_pDeviceObject->Flags |= DO_BUFFERED_IO;
	//g_pDeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	DebugPrint(("[ MyDriver ] Device created.\n"));

	ns = IoCreateSymbolicLink(&uSymLink, &uDeviceName);
	if (!NT_SUCCESS(ns))
	{
		DebugPrint(("[ MyDriver ] Error creating symbolic link to %ws, named: %ws. status = %x\n", uDeviceName.Buffer, DEVICE_SYM_LINK, (PVOID)ns));
		IoDeleteDevice(g_pDeviceObject);
		return ns;
	}

	DebugPrint(("[ MyDriver ] Create symbolic link to %ws, named: %ws\n", uDeviceName.Buffer, DEVICE_SYM_LINK));

	//
	// Initialize the device extension.
	//
	pDeviceExtension = g_pDeviceObject->DeviceExtension;

	InitializeListHead(&pDeviceExtension->EventQueueHead);

	KeInitializeSpinLock(&pDeviceExtension->QueueLock);

	pDeviceExtension->Self = g_pDeviceObject;
	return ns;

}

//NTSTATUS IoDispatch(
//	IN PDEVICE_OBJECT pDO,
//	IN PIRP Irp
//)
//{
//	NTSTATUS status = STATUS_SUCCESS;
//	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
//
//	if (!irpStack->FileObject)
//	{
//		DebugPrint(("[ MyDriver ] Get IRP stack failed. Cannot dispatch IO event.\n"));
//		return STATUS_SUCCESS;
//	}
//
//	if (pDO != g_pDeviceObject)
//	{
//		DebugPrint(("[ MyDriver ] Unknown device request, ignored.\n"));
//		return STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
//	}
//
//	switch (irpStack->MajorFunction)
//	{
//	case IRP_MJ_CREATE:
//		DebugPrint(("[ MyDriver ] *** IRP_MJ_CREATE ***\n"));
//		CreateDeviceIo(pDO, Irp);
//		break;
//
//	case IRP_MJ_CLOSE:
//		DebugPrint(("[ MyDriver ] *** IRP_MJ_CLOSE ***\n"));
//		CloseDeviceIo(pDO, Irp);
//		break;
//
//	case IRP_MJ_WRITE:
//		DebugPrint(("[ MyDriver ] *** IRP_MJ_WRITE ***\n"));
//		BufferedWrite(pDO, Irp);
//		break;
//
//	case IRP_MJ_DEVICE_CONTROL:
//		DebugPrint(("[ MyDriver ] *** IRP_MJ_DEVICE_CONTROL ***\n"));
//		ControlDeviceIo(pDO, Irp);
//		break;
//
//	default:
//		IoUnsupported(pDO, Irp);
//
//	}
//
//	Irp->IoStatus.Status = status;
//	Irp->IoStatus.Information = 0;
//	IoCompleteRequest(Irp, IO_NO_INCREMENT);
//	return status;
//}

//UINT InitUnicodeStrings()
//{
//	RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
//	RtlInitUnicodeString(&uSymLink, DEVICE_SYM_LINK);
//	return 0;
//}


