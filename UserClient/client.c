#pragma once
# include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

#include "client.h"
#include "drvctl.h"

CONST UINT FUNC_ID_TABLE[] = {
	FUNC_SETUP_SERVICE,
	FUNC_DELETE_SERVICE,
	FUNC_START_SERVICE,
	FUNC_STOP_SERVICE,
	FUNC_WRITE_IO,
	FUNC_OPEN_IO,
	FUNC_CLOSE_IO
};

LPCTSTR FUNC_NAME_TABLE[] = {
	_T("install"),
	_T("delete"),
	_T("start"),
	_T("stop"),
	_T("write"),
	_T("open"),
	_T("close")
};

UINT translateCommand(LPTSTR command)
{
	UINT tblCmdSize = sizeof(FUNC_ID_TABLE) / sizeof(UINT);
	UINT tblCmdName = sizeof(FUNC_NAME_TABLE) / sizeof(TCHAR*);
	UINT i;

	for (i = 0; i < tblCmdSize && i < tblCmdName; i++)
		if (!_tcsncmp(command, FUNC_NAME_TABLE[i], sizeof(command) - 1))
			return FUNC_ID_TABLE[i];
			
	return FUNC_UNKNOWN;
}

BOOLEAN 
processCommand(LPTSTR cmd)
{
	static HANDLE hFileHandle = INVALID_HANDLE_VALUE;
	UINT cmdID = 0;
	TCHAR drvPath[MAX_PATH];

	TCHAR buffer[200];
	ZeroMemory(&buffer, 200);

	// Get command ID
	cmdID = translateCommand(cmd);

	// Get driver path
	if (!SetupDriverPath(drvPath))
	{
		return FALSE;
	}

	ManageDriver(
		DRIVER_NAME,
		drvPath,
		cmdID);

	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR buffer[200] = { 0 };

	do
	{
		printf("\n>>");
		scanf_s("%200s", buffer, (UINT)sizeof(buffer));

		if (buffer  == NULL)
		{
			printf("Get command line arguments failed!\n");
			exit(1);
		}
		if (!_tcsncmp(buffer, _T("q"), 1))
		{
			exit(0);
		}

		processCommand(buffer);
	} while (TRUE);

	//if (argc != 2)
	//{
	//	printf("Invalid command argument!\n\n");
	//	exit(1);
	//}

	

	/*if (processCommand(cmd))
	{
		printf("Driver %s successfully.\n", cmd);
		return 0;
	}*/
}