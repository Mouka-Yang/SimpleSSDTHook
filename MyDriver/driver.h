#ifndef ENTRY_H
#define ENTRY_H


#ifndef UINT
#define UINT unsigned int
#endif // !UINT

#define DBG 1

#if DBG
#define DebugPrint(_x_) \
                DbgPrint("[ MyDriver ]: ");\
                DbgPrint _x_

#else

#define DebugPrint(_x_)

#endif

#define DEVICE_NAME L"\\Device\\MYDRIVER"
#define	DEVICE_SYM_LINK L"\\??\\MYDRIVER"
#define TAG (ULONG)'TEVE'

// 
// Define Device Extension
//


typedef enum {
	IRP_BASED,
	EVENT_BASED
} NOTIFY_TYPE;

typedef struct _REGISTER_EVENT
{
	NOTIFY_TYPE Type;
	HANDLE  hEvent;
	LARGE_INTEGER DueTime; // requested DueTime in 100-nanosecond units

} REGISTER_EVENT, *PREGISTER_EVENT;

#define SIZEOF_REGISTER_EVENT  sizeof(REGISTER_EVENT )


#define IOCTL_REGISTER_EVENT \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define MYDRIVER_SEND_STR \
	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801,	\
	METHOD_BUFFERED, FILE_WRITE_DATA )

#define MYDRIVER_RECV_STR \
	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, \
	METHOD_BUFFERED, FILE_READ_DATA )

//
// DATA
//
typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT  Self;
	LIST_ENTRY      EventQueueHead; // where all the user notification requests are queued
	KSPIN_LOCK      QueueLock;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


typedef struct _NOTIFY_RECORD {
	NOTIFY_TYPE     Type;
	LIST_ENTRY      ListEntry;
	union {
		PKEVENT     Event;
		PIRP        PendingIrp;
	} Message;
	KDPC            Dpc;
	KTIMER          Timer;
	PFILE_OBJECT    FileObject;
	PDEVICE_EXTENSION   DeviceExtension;
	BOOLEAN         CancelRoutineFreeMemory;
} NOTIFY_RECORD, *PNOTIFY_RECORD;

typedef struct _FILE_CONTEXT {
	//
	// Lock to rundown threads that are dispatching I/Os on a file handle 
	// while the cleanup for that handle is in progress.
	//
	IO_REMOVE_LOCK  FileRundownLock;
} FILE_CONTEXT, *PFILE_CONTEXT;


//
// Function prototypes
//
DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH(CreateCloseDeviceIo);

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH(ControlDeviceIo);

DRIVER_DISPATCH(IoDispatch);

DRIVER_DISPATCH(BufferedWrite);

DRIVER_DISPATCH(RegisterIrpBasedNotification);

DRIVER_CANCEL EventCancelRoutine;

KDEFERRED_ROUTINE CustomTimerDPC;

DRIVER_UNLOAD(DriverUnload);

NTSTATUS
SetupDeviceIo(
	IN	PDRIVER_OBJECT pDriverOject
);

//VOID
//DriverUnload(
//	IN	PDRIVER_OBJECT DriverObject
//	);
//

//NTSTATUS IoDispatch(
//	IN PDEVICE_OBJECT pDO,
//	IN PIRP Irp
//);
//VOID DebugError(LPTSTR lszFunction);

//NTSTATUS InitUnicodeStrings();

#endif