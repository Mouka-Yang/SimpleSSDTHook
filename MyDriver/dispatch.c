#include <ntddk.h>
#include <tchar.h>

#include "driver.h"

NTSTATUS BufferedWrite(
	PDEVICE_OBJECT pDO,
	PIRP Irp)
{
	UNREFERENCED_PARAMETER(pDO);

	NTSTATUS ns = STATUS_SUCCESS;

	PIO_STACK_LOCATION pIoStackIrp = NULL;
	PCHAR pInBuffer = NULL;

	DebugPrint(("[ MyDriver ] IO device has been written to.\n"));

	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (pIoStackIrp)
	{
		pInBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
		if (pInBuffer)
		{

			DebugPrint(("[ MyDriver ] Processing Message: SZ: %u, STR: %s", pIoStackIrp->Parameters.Write.Length, pInBuffer));
		}
		else
		{
			DebugPrint(("[ MyDriver ] Write called with null buffer pointer."));
		}
	}
	else
	{
		DebugPrint(("[ MyDriver ] Invalid IRP stack pointer.."));
	}

	return ns;
}

NTSTATUS IoUnsupported(
	PDEVICE_OBJECT pDO,
	PIRP Irp)
{
	UNREFERENCED_PARAMETER(pDO);
	UNREFERENCED_PARAMETER(Irp);

	NTSTATUS ns = STATUS_SUCCESS;

	DebugPrint(("[ MyDriver ] Unsupported Major function requested. Returning STATUS_SUCCESS.\n"));

	/*Irp->IoStatus.Status = ns;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);*/

	return ns;
}

NTSTATUS CreateCloseDeviceIo(
	PDEVICE_OBJECT pDO,
	PIRP Irp)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(pDO);
	UNREFERENCED_PARAMETER(Irp);

	PIO_STACK_LOCATION irpStack;
	PFILE_CONTEXT fileContext;
	NTSTATUS ns = STATUS_SUCCESS;

	irpStack = IoGetCurrentIrpStackLocation(Irp);

	ASSERT(irpStack->FileObject != NULL);

	switch (irpStack->MajorFunction)
	{
	case IRP_MJ_CREATE:
		DebugPrint(("IRP_MJ_CREATE.\n"));

		fileContext = ExAllocatePoolWithQuotaTag(
			NonPagedPool,
			sizeof(FILE_CONTEXT),
			TAG);
		if (!fileContext)
		{
			ns = STATUS_INSUFFICIENT_RESOURCES;
			return ns;
		}

		IoInitializeRemoveLock(&fileContext->FileRundownLock, TAG, 0, 0);

		//
		// Make sure nobody is using the FsContext scratch area.
		//
		ASSERT(irpStack->FileObject->FsContext == NULL);

		//
		// Store the context in the FileObject's scratch area.
		//
		irpStack->FileObject->FsContext = (PVOID)fileContext;

		ns = STATUS_SUCCESS;
		break;

	case IRP_MJ_CLOSE:
		DebugPrint(("IRP_MJ_CLOSE."));

		fileContext = irpStack->FileObject->FsContext;

		ExFreePoolWithTag(fileContext, TAG);

		ns = STATUS_SUCCESS;
		break;

	default:
		ASSERT(FALSE);
		break;

	}
	DebugPrint(("[ MyDriver ] Handle to IO device has been opened.\n"));


	Irp->IoStatus.Status = ns;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ns;
}



NTSTATUS ControlDeviceIo(
	IN PDEVICE_OBJECT pDO,
	IN PIRP Irp)
{
	UNREFERENCED_PARAMETER(pDO);

	PIO_STACK_LOCATION irpStack;
	PREGISTER_EVENT registerEvent;
	PFILE_CONTEXT fileContext;
	NTSTATUS status = STATUS_SUCCESS;

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	if (!irpStack->FileObject)
	{
		DebugPrint(("[ MyDriver ] Invalid IRP request.\n"));
		return status;
	}

	fileContext = irpStack->FileObject->FsContext;

	status = IoAcquireRemoveLock(&fileContext->FileRundownLock, Irp);
	if (!NT_SUCCESS(status))
	{
		//
		// Lock is in a removed state. That means we have already received 
		// cleaned up request for this handle. 
		//
		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}

	PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
	ULONG inLen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outLen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case MYDRIVER_SEND_STR:

		ASSERT(buffer != NULL);
		/*ASSERT(inLen > 0);
		ASSERT(outLen == 0);*/
		if (inLen <= 0 || outLen != 0)
		{
			DebugPrint(("[ MyDriver ] Invalid send request.\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		DebugPrint(("[ MyDriver ] Input text is %s, text size is %u.\n", (PTCHAR)buffer, inLen));
		break;

	case MYDRIVER_RECV_STR:
	case IOCTL_REGISTER_EVENT:

		DebugPrint(("IOCTL_REGISTER_EVENT"));

		//
		// First validate the parameters.
		//
		if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
			SIZEOF_REGISTER_EVENT) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		registerEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;
		switch (registerEvent->Type)
		{
		case IRP_BASED:
			status = RegisterIrpBasedNotification(pDO, Irp);
			break;
		default:
			DebugPrint(("Unknown user-mode parameters.\n"));
			status = STATUS_NOT_IMPLEMENTED;
			break;
		}
	default:
		status = STATUS_INVALID_PARAMETER;

	} // switch

	if (status != STATUS_PENDING) {
		//
		// complete the Irp
		//
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	//
	// We don't hold the lock for IRP that's pending in the list because this
	// lock is meant to rundown currently dispatching threads when the cleanup
	// is handled.
	//
	IoReleaseRemoveLock(&fileContext->FileRundownLock, Irp);

	return status;
}

VOID
EventCancelRoutine(
	PDEVICE_OBJECT   DeviceObject,
	PIRP             Irp
)

/*++

Routine Description:

The cancel routine. It will remove the IRP from the queue
and will complete it. The cancel spin lock is already acquired
when this routine is called. This routine is not required if
you are just using the event based notification.

Arguments:

DeviceObject - pointer to the device object.

Irp - pointer to the IRP to be cancelled.


Return Value:

VOID.

--*/
{
	PDEVICE_EXTENSION   deviceExtension;
	KIRQL               oldIrql;
	PNOTIFY_RECORD      notifyRecord;

	DebugPrint(("==>EventCancelRoutine irp %p\n", Irp));

	deviceExtension = DeviceObject->DeviceExtension;

	//
	// Release the cancel spinlock
	//
	IoReleaseCancelSpinLock(Irp->CancelIrql);

	//
	// Acquire the queue spinlock
	//
	KeAcquireSpinLock(&deviceExtension->QueueLock, &oldIrql);

	notifyRecord = Irp->Tail.Overlay.DriverContext[3];
	ASSERT(NULL != notifyRecord);
	ASSERT(IRP_BASED == notifyRecord->Type);

	RemoveEntryList(&notifyRecord->ListEntry);

	//
	// Clear the pending Irp field because we complete the IRP no matter whether
	// we succeed or fail to cancel the timer. TimerDpc will check this field
	// before dereferencing the IRP.
	//
	notifyRecord->Message.PendingIrp = NULL;

	if (KeCancelTimer(&notifyRecord->Timer)) {
		DebugPrint(("\t canceled timer\n"));
		ExFreePoolWithTag(notifyRecord, TAG);
		notifyRecord = NULL;
	}
	else {
		//
		// Here the possibilities are:
		// 1) DPC is fired and waiting to acquire the lock.
		// 2) DPC has run to completion.
		// 3) DPC has been cancelled by the cleanup routine.
		// By checking the CancelRoutineFreeMemory, we can figure out whether
		// dpc is waiting to acquire the lock and access the notifyRecord memory.
		//
		if (notifyRecord->CancelRoutineFreeMemory == FALSE) {
			//
			// This is case 1 where the DPC is waiting to run.
			//
			InitializeListHead(&notifyRecord->ListEntry);
		}
		else {
			//
			// This is either 2 or 3.
			//
			ExFreePoolWithTag(notifyRecord, TAG);
			notifyRecord = NULL;
		}

	}

	KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

	DebugPrint(("\t canceled IRP %p\n", Irp));
	Irp->Tail.Overlay.DriverContext[3] = NULL;
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DebugPrint(("<==EventCancelRoutine irp %p\n", Irp));
	return;

}

VOID
CustomTimerDPC(
	PKDPC Dpc,
	PVOID DeferredContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2
)

/*++

Routine Description:

This is the DPC associated with this drivers Timer object setup in ioctl routine.

Arguments:

Dpc             -   our DPC object associated with our Timer
DeferredContext -   Context for the DPC that we setup in DriverEntry
SystemArgument1 -
SystemArgument2 -

Return Value:

Nothing.

--*/
{
	PNOTIFY_RECORD      notifyRecord = DeferredContext;
	PDEVICE_EXTENSION deviceExtension;
	PIRP irp;

	UNREFERENCED_PARAMETER(Dpc);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	DebugPrint(("==> CustomTimerDPC \n"));

	ASSERT(notifyRecord != NULL); // can't be NULL
	_Analysis_assume_(notifyRecord != NULL);

	deviceExtension = notifyRecord->DeviceExtension;

	KeAcquireSpinLockAtDpcLevel(&deviceExtension->QueueLock);

	RemoveEntryList(&notifyRecord->ListEntry);

	switch (notifyRecord->Type) {
	case IRP_BASED:
		irp = notifyRecord->Message.PendingIrp;
		if (irp != NULL) {
			if (IoSetCancelRoutine(irp, NULL) != NULL) {

				irp->Tail.Overlay.DriverContext[3] = NULL;

				//
				// Drop the lock before completing the request.
				//
				KeReleaseSpinLockFromDpcLevel(&deviceExtension->QueueLock);

				irp->IoStatus.Status = STATUS_SUCCESS;
				irp->IoStatus.Information = 0;
				IoCompleteRequest(irp, IO_NO_INCREMENT);

				KeAcquireSpinLockAtDpcLevel(&deviceExtension->QueueLock);

			}
			else {
				//
				// Cancel routine will run as soon as we release the lock.
				// So let it complete the request and free the record.
				//
				InitializeListHead(&notifyRecord->ListEntry);
				notifyRecord->CancelRoutineFreeMemory = TRUE;
				notifyRecord = NULL;
			}
		}
		else {
			//
			// Cancel routine has run and completed the IRP. So just free
			// the record.
			//
			ASSERT(notifyRecord->CancelRoutineFreeMemory == FALSE);
		}

		break;

	case EVENT_BASED:
		//
		// Signal the Event created in user-mode.
		//
		KeSetEvent(notifyRecord->Message.Event, 0, FALSE);

		//
		// Dereference the object as we are done with it.
		//
		ObDereferenceObject(notifyRecord->Message.Event);

		break;

	default:
		ASSERT(FALSE);
		break;
	}

	KeReleaseSpinLockFromDpcLevel(&deviceExtension->QueueLock);

	//
	// Free the memory outside the lock for better performance.
	//
	if (notifyRecord != NULL) {
		ExFreePoolWithTag(notifyRecord, TAG);
		notifyRecord = NULL;
	}

	DebugPrint(("<== CustomTimerDPC\n"));

	return;
}

NTSTATUS
RegisterIrpBasedNotification(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
)

/*++

Routine Description:

This routine  queues a IRP based notification record to be
handled by a DPC.


Arguments:

DeviceObject - Context for the activity.
Irp          - The device control argument block.

Return Value:

NTSTATUS - If the status is not STATUS_PENDING, the caller
will complete the request.


--*/
{
	PDEVICE_EXTENSION   deviceExtension;
	PNOTIFY_RECORD notifyRecord;
	PIO_STACK_LOCATION irpStack;
	KIRQL   oldIrql;
	PREGISTER_EVENT registerEvent;

	DebugPrint(("\tRegisterIrpBasedNotification\n"));

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	deviceExtension = DeviceObject->DeviceExtension;
	registerEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

	//
	// Allocate a record and save all the event context.
	//

	notifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool,
		sizeof(NOTIFY_RECORD),
		TAG);

	if (NULL == notifyRecord) {
		return  STATUS_INSUFFICIENT_RESOURCES;
	}

	InitializeListHead(&notifyRecord->ListEntry);

	notifyRecord->FileObject = irpStack->FileObject;
	notifyRecord->DeviceExtension = deviceExtension;
	notifyRecord->Type = IRP_BASED;
	notifyRecord->Message.PendingIrp = Irp;

	//
	// Start the timer to run the CustomTimerDPC in DueTime seconds to
	// simulate an interrupt (which would queue a DPC).
	// The user's event object is signaled or the IRP is completed in the DPC to
	// notify the hardware event.
	//

	// ensure relative time for this sample

	if (registerEvent->DueTime.QuadPart > 0) {
		registerEvent->DueTime.QuadPart = -(registerEvent->DueTime.QuadPart);
	}

	KeInitializeDpc(&notifyRecord->Dpc, // Dpc
		CustomTimerDPC,     // DeferredRoutine
		notifyRecord        // DeferredContext
	);

	KeInitializeTimer(&notifyRecord->Timer);

	//
	// We will set the cancel routine and TimerDpc within the
	// lock so that they don't modify the list before we are
	// completely done.
	//
	KeAcquireSpinLock(&deviceExtension->QueueLock, &oldIrql);

	//
	// Set the cancel routine. This is required if the app decides to
	// exit or cancel the event prematurely.
	//
	IoSetCancelRoutine(Irp, EventCancelRoutine);

	//
	// Before we queue the IRP, we must check to see if it's cancelled.
	//
	if (Irp->Cancel) {

		//
		// Clear the cancel-routine automically and check the return value.
		// We will complete the IRP here if we succeed in clearing it. If
		// we fail then we will let the cancel-routine complete it.
		//
		if (IoSetCancelRoutine(Irp, NULL) != NULL) {

			//
			// We are able to successfully clear the routine. Either the
			// the IRP is cancelled before we set the cancel-routine or
			// we won the race with I/O manager in clearing the routine.
			// Return STATUS_CANCELLED so that the caller can complete
			// the request.

			KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

			ExFreePoolWithTag(notifyRecord, TAG);

			return STATUS_CANCELLED;
		}
		else {
			//
			// The IRP got cancelled after we set the cancel-routine and the
			// I/O manager won the race in clearing it and called the cancel
			// routine. So queue the request so that cancel-routine can dequeue
			// and complete it. Note the cancel-routine cannot run until we
			// drop the queue lock.
			//
		}
	}

	IoMarkIrpPending(Irp);

	InsertTailList(&deviceExtension->EventQueueHead,
		&notifyRecord->ListEntry);

	notifyRecord->CancelRoutineFreeMemory = FALSE;

	//
	// We will save the record pointer in the IRP so that we can get to
	// it directly in the CancelRoutine.
	//
	Irp->Tail.Overlay.DriverContext[3] = notifyRecord;

	KeSetTimer(&notifyRecord->Timer,   // Timer
		registerEvent->DueTime,         // DueTime
		&notifyRecord->Dpc      // Dpc
	);

	KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

	//
	// We will return pending as we have marked the IRP pending.
	//
	return STATUS_PENDING;;

}

//NTSTATUS CloseDeviceIo(
//	PDEVICE_OBJECT pDO,
//	PIRP Irp)
//{
//	PAGED_CODE();
//
//	UNREFERENCED_PARAMETER(pDO);
//	UNREFERENCED_PARAMETER(Irp);
//
//	NTSTATUS ns = STATUS_SUCCESS;
//
//	DebugPrint("[ MyDriver ] Handle to IO device has been closed.\n");
//
//	/*Irp->IoStatus.Status = ns;
//	Irp->IoStatus.Information = 0;
//	IoCompleteRequest(Irp, IO_NO_INCREMENT);*/
//
//	return ns;
//}
VOID forceNullTermination(PTCHAR str, UINT len)
{
	if (str[len] != 0)
	{
		str[len] = 0;
	}
}