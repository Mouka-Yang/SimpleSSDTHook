#pragma once
#include <stdlib.h>
#include <tchar.h>
#include <strsafe.h>

#include "drvctl.h"
#include "devio.h"

HANDLE g_hDevice = INVALID_HANDLE_VALUE;

BOOLEAN
SetupDriverPath(
	_Out_writes_(MAX_PATH) LPTSTR DriverPath)
{
	HANDLE fHandle;
	DWORD drvPathLen = 0;

	// Get the current directory
	drvPathLen = GetCurrentDirectory(
		MAX_PATH,
		DriverPath);

	if (!drvPathLen)
	{
		ErrorExit(_T("GetCurrentDirectory failed!\n"));
	}

	// Setup path name to driver file
	if (StringCchPrintf(
		&DriverPath[_tcslen(DriverPath)],
		(MAX_PATH - _tcslen(DriverPath)),
		_T("\\%s.sys"),
		DRIVER_NAME) != S_OK)
	{
		ErrorExit(_T("Generate Driver Path failed!\n"));
	}

	// Ensure driver file is in the specified directory
	if ((fHandle = CreateFile(
		DriverPath,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL)) == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("Driver is not in the current directory!\n"));
		exit(1);
	}

	if (fHandle)
	{
		CloseHandle(fHandle);
	}

	return TRUE;
}	// SetupDriverPath

VOID
InstallDriver(
	IN	SC_HANDLE SchSCManager,
	IN LPCTSTR	DriverName,
	IN LPCTSTR DriverPath)
{
	SC_HANDLE schService;
	DWORD err;

	schService = CreateService(
		SchSCManager,	// handle of serivice control manager database
		DriverName,	// address of name of service to start
		DriverName, //	address of  display name
		SERVICE_ALL_ACCESS,	// type of access to service
		SERVICE_KERNEL_DRIVER,	// type of service
		SERVICE_DEMAND_START,	// when to start service
		SERVICE_ERROR_NORMAL,	// severity if service fails to start
		DriverPath, // address of name of binary file
		NULL,	// service does not belong to a group
		NULL,	// no tag requested
		NULL,	// no dependency names
		NULL,	// use LocalSystem account
		NULL	// no password for service account
	);

	if (schService == NULL)
	{
		err = GetLastError();
		if (err == ERROR_SERVICE_EXISTS)
		{
			// Ignore thes error
			_tprintf("The Service is already installed.\n");
			return;
		}
		else
		{
			ErrorExit(_T("CreateService failed! \n"));
		}

	}

	if (schService)
	{
		CloseServiceHandle(schService);
	}

	_tprintf(_T("Install driver successfully.\n"));
}	 // InstallDriver

VOID
RemoveDriver(
	IN SC_HANDLE SchSCManager,
	IN LPCTSTR	Drivername)
{
	SC_HANDLE	schService;

	// Open the handle to the existing service
	schService = OpenService(
		SchSCManager,
		Drivername,
		SERVICE_ALL_ACCESS);

	if (schService == NULL)
	{
		ErrorExit(_T("OpenService failed!\n"));
	}

	if (!DeleteService(schService))
	{
		ErrorExit(_T("DeleteService failed!\n"));
	}

	if (schService)
	{
		CloseServiceHandle(schService);
	}

	_tprintf(_T("Driver removed successfully.\n"));
	return;
}	// RemoveDriver

VOID
StartDriver(
	IN SC_HANDLE	SchSCManager,
	IN LPCTSTR		DriverName)
{
	SC_HANDLE	schService;
	DWORD		err;

	schService = OpenService(
		SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS);

	if (schService == NULL)
	{
		ErrorExit(_T("OpenService failed!\n"));

	}

	// Start the execution of the service ( start the driver)

	if (!StartService(
		schService,		// service identifier
		0,				// number of arguments
		NULL		// pointer to arguments
	))
	{
		err = GetLastError();
		if (err == ERROR_SERVICE_ALREADY_RUNNING)
		{
			// Ignore this error
			_tprintf(_T("The service is already running!\n"));
			return;
		}
		else
		{
			ErrorExit(_T("StartService failed!\n"));
		}
	}

	if (schService)
	{
		CloseServiceHandle(schService);
	}

	_tprintf(_T("Start driver successfully.\n"));
	return;
}	// StartDriver

VOID
StopDriver(
	IN SC_HANDLE	SchSCManager,
	IN LPCTSTR		DriverName)
{
	SC_HANDLE	schService;
	SERVICE_STATUS ss;

	schService = OpenService(
		SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS);

	if (schService == NULL)
	{
		ErrorExit(_T("OpenService failed!\n"));

	}

	// Query if  the service is running
	if (!(QueryServiceStatus(schService, &ss)))
	{
		ErrorExit(_T("QueryServiceStatus failed!\n"));
	}
	else if (ss.dwCurrentState == SERVICE_STOPPED)
	{
		// if the service is not running then exit.
		_tprintf(_T("The service is not running.\n"));
		return;
	}

	// Request the service to stop
	if (!ControlService(
		schService,
		SERVICE_CONTROL_STOP,
		&ss))
	{
		ErrorExit(_T("ControlService failed!\n"));
	}

	if (schService)
	{
		CloseServiceHandle(schService);
	}

	_tprintf(_T("Stop driver successfully.\n"));
	return;
}	// StopDriver

BOOLEAN
ManageDriver(
	IN LPCTSTR	DriverName,
	IN LPCTSTR	DriverPath,
	IN UINT		Function
)
{
	SC_HANDLE	schSCManager;
	BOOLEAN	rCode = TRUE;

	TCHAR buffer[200] = { 0 };
	//HANDLE static hFile = INVALID_HANDLE_VALUE;

	if (!DriverName || !DriverPath)
	{
		_tprintf(_T("Invalid Driver or Service provided to .\n"));
		return FALSE;
	}

	// Connect to the Service Control Manager and open Services database

	schSCManager = OpenSCManager(
		NULL,	// local machine
		NULL,	// local database
		SC_MANAGER_ALL_ACCESS	// access required
	);

	if (!schSCManager)
	{
		ErrorExit(_T("Open SC Manager failed!\n"));
	}

	switch (Function)
	{
	case FUNC_SETUP_SERVICE:
		// Install the driver service
		InstallDriver(
			schSCManager,
			DriverName,
			DriverPath);
		break;

	case FUNC_DELETE_SERVICE:
		// Stop the driver before delet
		StopDriver(
			schSCManager,
			DriverName);

		// Remove driver
		RemoveDriver(
			schSCManager,
			DriverName);
		break;

	case FUNC_START_SERVICE:
		StartDriver(
			schSCManager,
			DriverName);
		break;

	case FUNC_STOP_SERVICE:
		// Stop the driver
		StopDriver(
			schSCManager,
			DriverName);
		break;

	case FUNC_WRITE_IO:
		WriteToDevice(g_hDevice, buffer);
		break;

	case FUNC_OPEN_IO:
		OpenIoHandle(&g_hDevice);
		break;

	case FUNC_CLOSE_IO:
		CloseIoHandle(g_hDevice);
		break;

	default:
		_tprintf(_T("Unknown command argument!\n"));
		break;
	}

	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
	}
	return rCode;
}	// ManageDriver


VOID ErrorExit(LPCTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s ---- error code %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	//MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
	_tprintf((LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	system("PAUSE");
	ExitProcess(dw);
}


// Reinstall service
// Try to delete the service first
//if (!StopDriver(
//	SchSCManager,
//	DriverName))
//{
//	ErrorExit(_T("Stop driver failed!\n"));
//}
//
//// Remove driver
//if (!RemoveDriver(
//	SchSCManager,
//	DriverName))
//{
//	ErrorExit(_T("Remove driver failed!\n"));
//}
//else
//{
//	_tprintf(_T("Driver removed successfully.\n"));
//}
// Then install the service
//schService = CreateService(
//	SchSCManager,
//	DriverName,
//	DriverName,
//	SERVICE_ALL_ACCESS,
//	SERVICE_KERNEL_DRIVER,
//	SERVICE_DEMAND_START,
//	SERVICE_ERROR_NORMAL,
//	DriverPath,
//	NULL,
//	NULL,
//	NULL,
//	NULL,
//	NULL
//);
//if (!schService)
//{
//	ErrorExit(_T("CreateService failed! \n"));
//}
//
//if (schService)
//{
//	CloseServiceHandle(schService);
//}