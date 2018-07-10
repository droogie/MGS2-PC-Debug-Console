#include <Windows.h>
#include <iostream>
#include <stdio.h>

#include "detours.h"
#include <excpt.h>

DWORD addrPrintf = 0;
DWORD addrNopPrint= 0;

void myInvalidParameterHandler(const wchar_t* expression,
	const wchar_t* function,
	const wchar_t* file,
	unsigned int line,
	uintptr_t pReserved)
{
	//PASS...
	//Yes, this is gross.
}

int hookNopPrint(const char *format, ...) {
	return 0;
}

int hookPrintf(const char *format, ...)
{
	char  buffer[256];
	memset(&buffer, 0x00, sizeof(buffer));
	
	int ret = -1;

	va_list args;
	va_start(args, format);
	ret = _vsnprintf_s(buffer, sizeof(buffer), format, args);

	if (ret > 0) {
		if (WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buffer, ret, NULL, NULL) == FALSE)
		{
			if (AllocConsole() == TRUE)
			{
				WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buffer, ret, NULL, NULL);
			}
		}
	}

	va_end(args);
	return 0;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = myInvalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);
	// Disable the message box for assertions.
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

	// create console window for printing debug lines
	AllocConsole();

	if (dwReason == DLL_PROCESS_ATTACH)
	{	
		// address to the functions we are hooking
		DWORD addrPrintf = 0x006FA180;
		DWORD addrJunkPrint = 0x008E0280;

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		// hook the null debug printf
		DetourAttach(&(LPVOID&)addrPrintf, &hookPrintf);

		// NOP "print:" message that appears to be kanji or ...?
		DetourAttach(&(LPVOID&)addrJunkPrint, &hookNopPrint);

		DetourTransactionCommit();
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		// unhook
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetourDetach(&(LPVOID&)addrPrintf, &hookPrintf);
		DetourDetach(&(LPVOID&)addrNopPrint, &hookNopPrint);

		DetourTransactionCommit();
	}
	return TRUE;
}