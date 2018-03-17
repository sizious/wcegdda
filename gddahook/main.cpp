#include <windows.h>
#include <dsound.h>
#include <mapledev.h>
#include "error.hpp"
#include "gddadrv.hpp"
#include "../iathook/iathook.hpp"

// Use IDA to dissassemble PB4.EXE and get address from Import tab
#define HOOKADDR_PB4_DEVICEIOCONTROL		0x0005D054
#define HOOKADDR_PB4_DIRECTSOUNDCREATE		0x0005D0C8

GDAudioDriver gdda;
BOOL g_HookInstalled = FALSE;

/* DeviceIoControl Hook */
BOOL WINAPI __DeviceIoControl( HANDLE hDevice, 
							   DWORD dwIoControlCode, 
							   LPVOID lpInBuffer, 
							   DWORD nInBufferSize, 
							   LPVOID lpOutBuffer, 
							   DWORD nOutBufferSize, 
							   LPDWORD lpBytesReturned, 
							   LPOVERLAPPED lpOverlapped ) 
{

#ifdef DEBUG
	DebugOutput( TEXT("DeviceIoControl called, code=0x%08x, dwIoControlCode="), dwIoControlCode );
#endif

	bool fIntercepted = true;

	// For these control codes, we are doing our specific code
	switch( dwIoControlCode ) 
	{
		// Play fake GD-DA audio
		case IOCTL_SEGACD_CD_PLAYTRACK:
		{	
			SEGACD_PLAYTRACK playtrack = *((SEGACD_PLAYTRACK*) lpInBuffer);

#ifdef DEBUG
			DebugOutput(TEXT("IOCTL_SEGACD_CD_PLAYTRACK, dwStartTrack=%d, dwEndTrack=%d, dwRepeat=%d\n"), playtrack.dwStartTrack, playtrack.dwEndTrack, playtrack.dwRepeat);
#endif

			gdda.Play( playtrack );	
			break;
		}
		
		// Pause
		case IOCTL_CDROM_PAUSE_AUDIO:
		{
#ifdef DEBUG
			DebugOutput(TEXT("IOCTL_CDROM_PAUSE_AUDIO\n"));
#endif
			
			gdda.Pause();
			break;
		}

		// Resume
		case IOCTL_CDROM_RESUME_AUDIO:
		{
#ifdef DEBUG
			DebugOutput(TEXT("IOCTL_CDROM_RESUME_AUDIO\n"));
#endif

			gdda.Resume();
			break;
		}

		// Stop fake GD-DA audio
		case IOCTL_CDROM_STOP_AUDIO:
		{
#ifdef DEBUG
			DebugOutput(TEXT("IOCTL_CDROM_STOP_AUDIO\n"));
#endif

			gdda.Stop();
			break;
		}

		// Get Volume
		case IOCTL_CDROM_GET_VOLUME:
		{
#ifdef DEBUG
			DebugOutput(TEXT("IOCTL_CDROM_GET_VOLUME\n"));			
#endif
			break;
		}

		// Set Volume
		case IOCTL_CDROM_SET_VOLUME:
		{		
			VOLUME_CONTROL volume = *((VOLUME_CONTROL*) lpInBuffer);

#ifdef DEBUG
			UCHAR ucLeftVolume  = volume.PortVolume[0];
			UCHAR ucRightVolume = volume.PortVolume[1];
			DebugOutput(TEXT("IOCTL_CDROM_SET_VOLUME, left=%d, right=%d\n"), ucLeftVolume, ucRightVolume);
#endif

			gdda.SetVolume( volume );
			break;
		}

		case IOCTL_SEGACD_GET_STATUS:
		{
#ifdef DEBUG
			DebugOutput(TEXT("IOCTL_SEGACD_GET_STATUS\n"));
#endif
			SEGACD_STATUS status;
		
			status.dwStatus = SEGACD_STAT_STANDBY;
			status.dwDiscFormat = SEGACD_FORMAT_GDROM;

			if ( gdda.IsPlaying() )
			{
				status.dwStatus = SEGACD_STAT_PLAY;
			} 
			else if ( gdda.IsPaused() )
			{
				status.dwStatus = SEGACD_STAT_PAUSE;
			}			

			*((SEGACD_STATUS*) lpOutBuffer) = status;
			
			break;
		}

		default:
		{			
#ifdef DEBUG
			DebugOutput(TEXT("__USELESS_HOOK__\n"));
#endif
			fIntercepted = false;
			break;
		}

	}

#ifdef DEBUG
	DebugOutput(TEXT("\n"));
#endif

	// We already managed the API...
	if ( fIntercepted )
	{
		return TRUE;
	}

	// Normal API call
	return DeviceIoControl( hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped );
}

/* DirectSoundCreate Hook */
HRESULT WINAPI __DirectSoundCreate( LPGUID lpGuid, LPDIRECTSOUND *ppDS, IUnknown FAR *pUnkOuter )
{
#ifdef DEBUG
	DebugOutput(TEXT("__DirectSoundCreate called!\n"));
#endif

	// Calling the real API
	HRESULT result = DirectSoundCreate( lpGuid, ppDS, pUnkOuter );
	
	// Create the GD-DA Emulator
	gdda.Initialize( *ppDS );
	
	// Returning the value to the main program
	return result;
}

/* Initialize Hook */
VOID InstallHook( VOID ) 
{
	if ( !g_HookInstalled )
	{

#ifdef DEBUG
		DebugOutput(TEXT("Installing Hook...\n"));
#endif

		HOOK_CONTEXT context = HookBegin();

		// Create the DeviceIoControl hook
		HookWrite( context, HOOKADDR_PB4_DEVICEIOCONTROL, __DeviceIoControl );

		// Create the DirectSoundCreate hook
		HookWrite( context, HOOKADDR_PB4_DIRECTSOUNDCREATE, __DirectSoundCreate );

		HookEnd( context );

#ifdef DEBUG
		DebugOutput(TEXT("Hook is now installed!\n"));
#endif

		g_HookInstalled = TRUE;
	}
}

HRESULT WINAPI __MapleCreateDevice( const GUID *pguidDevice, IUnknown **ppIUnknown )
{
#ifdef DEBUG
	DebugOutput(TEXT("__MapleCreateDevice called\n"));
#endif
	InstallHook();
	return MapleCreateDevice( pguidDevice, ppIUnknown );
}

HRESULT __MapleEnumerateDevices( MAPLEDEVTYPE mapledevtype, LPFNMAPLEENUMDEVICECALLBACK pfn, PVOID pvContext, DWORD dwFlags )
{
#ifdef DEBUG
	DebugOutput(TEXT("__MapleEnumerateDevices called\n"));
#endif
	InstallHook();
	return MapleEnumerateDevices( mapledevtype, pfn, pvContext, dwFlags );
}
