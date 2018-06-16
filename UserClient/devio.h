#pragma once
#ifndef DEVIO_H_
#define DEVIO_H_

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "drvctl.h"

#define DEVICE_SYMLINK _T("\\\\.\\MyDriver")

#define MYDRIVER_SEND_STR \
	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801,	\
	METHOD_BUFFERED, FILE_WRITE_DATA )

#define MYDRIVER_RECV_STR \
	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, \
	METHOD_BUFFERED, FILE_READ_DATA )

BOOLEAN CloseIoHandle(HANDLE hDevice);
BOOLEAN WriteToDevice(HANDLE hDevice, PTCHAR inBuffer);
VOID OpenIoHandle(PHANDLE pDeviceHandle);

#endif // !DEVIO_H_
