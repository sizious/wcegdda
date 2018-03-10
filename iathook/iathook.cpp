#include "iathook.hpp"

HOOK_CONTEXT HookBegin( VOID )
{
	HOOK_CONTEXT context;

	SetKMode( TRUE );
	context.dwOldPermissions = SetProcPermissions( 0xFFFFFFFF );	
	context.hProcess = GetCurrentProcess();

	return context;
}

DWORD HookWrite( HOOK_CONTEXT context, DWORD dwAddress, LPVOID lpReplacementFunctionPointer )
{
	DWORD dwOriginalFunctionAddress = 0xDEADBEEF;

	DWORD *thunk = (DWORD*)dwAddress;
	DWORD lpflOldProtect;

	// In order to write our hook, we need to unprotect the memory just the time to perform the operation...
	if ( VirtualProtect( thunk, ADDRESS_POINTER_SIZE, PAGE_EXECUTE_READWRITE, &lpflOldProtect ) ) 
	{
		// Read the address of the old function
		ReadProcessMemory( context.hProcess, (LPVOID)dwAddress, &dwOriginalFunctionAddress, ADDRESS_POINTER_SIZE, NULL );

		// Write our hook
		WriteProcessMemory( context.hProcess, (LPVOID)dwAddress, &lpReplacementFunctionPointer, ADDRESS_POINTER_SIZE, 0 );

		// Check if our hook is property written
		DWORD dwNewFunctionAddress;
		ReadProcessMemory( GetCurrentProcess(), (LPVOID)dwAddress, &dwNewFunctionAddress, ADDRESS_POINTER_SIZE, NULL );
		if ( (DWORD)lpReplacementFunctionPointer != dwNewFunctionAddress ) 
		{
			dwOriginalFunctionAddress = 0xDEADBEEF;
		}

		// Restore old protection flags
		VirtualProtect( thunk, ADDRESS_POINTER_SIZE, lpflOldProtect, NULL );
	}

	return dwOriginalFunctionAddress;
}

VOID HookEnd( HOOK_CONTEXT context )
{
	SetProcPermissions( context.dwOldPermissions );
	SetKMode( FALSE );
}
