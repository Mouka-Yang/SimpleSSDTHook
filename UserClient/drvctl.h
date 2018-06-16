#pragma once
#ifndef DRVCTL_H_
#define DRVCTL_H_

#include <stdio.h>
#include <windows.h>

#define FUNC_UNKNOWN		0
#define FUNC_SETUP_SERVICE 1
#define FUNC_DELETE_SERVICE 2
#define FUNC_START_SERVICE 3
#define FUNC_STOP_SERVICE 4
#define FUNC_WRITE_IO	5
#define FUNC_OPEN_IO	6
#define FUNC_CLOSE_IO	7

#define MAX_PATH		260
#define DRIVER_NAME		_T("MyDriver")


BOOLEAN
SetupDriverPath(
	_Out_writes_(MAX_PATH)	LPTSTR DriverPath
);

VOID
InstallDriver(
	IN SC_HANDLE  SchSCManager,
	IN LPCTSTR    DriverName,
	IN LPCTSTR    DriverPath
);


VOID
RemoveDriver(
	IN SC_HANDLE  SchSCManager,
	IN LPCTSTR    DriverName
);

VOID
StartDriver(
	IN SC_HANDLE  SchSCManager,
	IN LPCTSTR    DriverName
);

VOID
StopDriver(
	IN SC_HANDLE  SchSCManager,
	IN LPCTSTR    DriverName
);

BOOLEAN
ManageDriver(
	IN LPCTSTR  DriverName,
	IN LPCTSTR  DriverPath,
	IN UINT		Function
);

VOID
ErrorExit(LPCTSTR lpszFunction);

#endif