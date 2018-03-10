#ifndef __IATHOOK_HPP__
#define __IATHOOK_HPP__

#include <windows.h>

#define ADDRESS_POINTER_SIZE sizeof(DWORD)

#ifdef __cplusplus
extern "C" {
#endif

BOOL SetKMode( BOOL fMode );
DWORD SetProcPermissions( DWORD newperms );

#ifdef __cplusplus
}
#endif

typedef struct _HOOK_CONTEXT
{
	DWORD	dwOldPermissions;
	HANDLE	hProcess;
} HOOK_CONTEXT, *PHOOK_CONTEXT;

HOOK_CONTEXT HookBegin( VOID );
DWORD HookWrite( HOOK_CONTEXT context, DWORD dwAddress, LPVOID lpReplacementFunctionPointer );
VOID HookEnd( HOOK_CONTEXT context );

#endif /* __IATHOOK_HPP__ */
