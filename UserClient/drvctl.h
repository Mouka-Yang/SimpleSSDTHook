#pragma once
#include <stdio.h>
#include <windows.h>

#define FUNC_UNKNOWN		0
#define FUNC_SETUP_SERVICE 1
#define FUNC_DELETE_SERVICE 2
#define FUNC_START_SERVICE 3
#define FUNC_STOP_SERVICE 4

#define MAX_PATH		260
#define DRIVER_NAME		_T("MyDriver")

BOOLEAN
SetupDriverPath(
	_Out_writes_(MAX_PATH)	LPTSTR DriverPath
);

BOOLEAN
InstallDriver(
	IN SC_HANDLE  SchSCManager,
	IN LPCTSTR    DriverName,
	IN LPCTSTR    DriverPath
);


BOOLEAN
RemoveDriver(
	IN SC_HANDLE  SchSCManager,
	IN LPCTSTR    DriverName
);

BOOLEAN
StartDriver(
	IN SC_HANDLE  SchSCManager,
	IN LPCTSTR    DriverName
);

BOOLEAN
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
ErrorExit(LPCSTR lpszFunction);
