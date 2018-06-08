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
	FUNC_STOP_SERVICE
};

LPCTSTR FUNC_NAME_TABLE[] = {
	"install",
	"delete",
	"start",
	"stop"
};

UINT translateCommand(LPTSTR command)
{
	UINT tblCmdSize = sizeof(FUNC_ID_TABLE) / sizeof(unsigned long);
	UINT tblCmdName = sizeof(FUNC_NAME_TABLE) / sizeof(char*);
	UINT i;

	for (i = 0; i < tblCmdSize && i < tblCmdName; i++)
		if (!strcmp(command, FUNC_NAME_TABLE[i]))
			return FUNC_ID_TABLE[i];

	return FUNC_UNKNOWN;
}

BOOLEAN 
processCommand(LPTSTR cmd)
{
	static HANDLE hFileHandle = INVALID_HANDLE_VALUE;
	UINT cmdID = 0;
	LPTSTR drvPath = NULL;

	CHAR buffer[200];
	ZeroMemory(&buffer, 200);

	// Get command ID
	cmdID = translateCommand(cmd);

	// Get driver path
	SetupDriverPath(drvPath);

	ManageDriver(
		DRIVER_NAME,
		drvPath,
		cmdID);

	return TRUE;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("Invalid command argument!\n\n");
		exit(1);
	}

	LPTSTR cmd = argv[1];
	if (processCommand(cmd))
	{
		printf("Driver %s successfully.\n", cmd);
		return 0;
	}
}