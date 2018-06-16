#ifndef DISPATCH_H_
#define DISPATCH_H_

#include <ntddk.h>
#include <tchar.h>
//
//#ifndef UINT
//#define UINT unsigned int
//#endif // !UINT
//
//#define MYDRIVER_SEND_STR \
//	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801,	\
//	METHOD_BUFFERED, FILE_WRITE_DATA )
//
//#define MYDRIVER_RECV_STR \
//	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, \
//	METHOD_BUFFERED, FILE_READ_DATA )
//
//NTSTATUS BufferedWrite(
//	PDEVICE_OBJECT pDO,
//	PIRP	Irp
//);
//
//NTSTATUS IoUnsupported(
//	PDEVICE_OBJECT pDO,
//	PIRP	Irp
//);
//
//NTSTATUS CreateDeviceIo(
//	PDEVICE_OBJECT	pDO,
//	PIRP	Irp
//);
//
//NTSTATUS CloseDeviceIo(
//	PDEVICE_OBJECT pDO,
//	PIRP	Irp
//);
//
//NTSTATUS ControlDeviceIo(
//	IN PDEVICE_OBJECT pDO,
//	IN PIRP Irp
//);
//
//VOID forceNullTermination(PTCHAR str, UINT len);

#endif // !DISPATCH_H_
