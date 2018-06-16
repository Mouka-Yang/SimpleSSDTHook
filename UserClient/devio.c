#include "devio.h"


BOOLEAN CloseIoHandle(HANDLE hDevice)
{
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("Device handle is invalid. \n"));
		return FALSE;
	}
	if (!CloseHandle(hDevice))
	{
		ErrorExit(_T("Close device handle failed.\n"));
	}

	_tprintf(_T("Close device handle successfully. \n"));

	return TRUE;
}

BOOLEAN WriteToDevice(HANDLE hDevice, PTCHAR Buffer)
{
	
	/*if (!WriteFile(
		hDevice,
		inBuffer,
		(DWORD)(_tcslen(inBuffer)) + 1,
		0,
		0))
	{
		ErrorExit(_T("WriteFile failed.\n"));
	}*/

	//DWORD outBuffer[200];
	DWORD outBufferCount;
	TCHAR inBuffer[200];

	ZeroMemory(inBuffer, sizeof(inBuffer));
	scanf_s("%s", inBuffer, (UINT)sizeof(inBuffer));
	_tprintf(_T("Device handle is %p.	inBuffer is %p.\n"), hDevice, inBuffer);

	if (hDevice == INVALID_HANDLE_VALUE ||
		inBuffer == NULL)
	{
		_tprintf(_T("Write to device failed. Device handle or input inBuffer is null.\n"));
		return FALSE;
	}

	if (!DeviceIoControl(
		hDevice,
		MYDRIVER_SEND_STR,
		inBuffer,
		(DWORD)_tcslen(inBuffer) + 1,
		NULL,
		0,
		&outBufferCount,
		NULL))
	{
		ErrorExit(_T("Cannot send message to device.\n"));
	}
	_tprintf(_T("Send message successfully.\n"));

	return TRUE;

}

VOID OpenIoHandle(PHANDLE pDeviceHandle)
{
	HANDLE hDevice;
	hDevice = CreateFile(
		DEVICE_SYMLINK,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		ErrorExit(_T("Get device handle failed.\n"));
	}

	*pDeviceHandle = hDevice;
	_tprintf(_T("Get device handle successfully. Device handle is %p\n"), hDevice);
	return ;
}
