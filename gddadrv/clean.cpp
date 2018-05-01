#include "gddadrv.hpp"
#include "error.hpp"

DWORD WINAPI
GDAudioDriver::CleanerThreadProc( LPVOID lpParameter )
{
	GDAudioDriver * gdda = (GDAudioDriver *) lpParameter;

	DWORD dwThreadId = GetCurrentThreadId();
	int currentContextIndex = gdda->gddaContextIndex;

#ifdef DEBUG
	DebugOutput( TEXT("CleanerThread: (%d) [0x%08x] Start!\n"), currentContextIndex, dwThreadId );
#endif

//	EnterCriticalSection( &gdda->csThread );

	for( int contextIndex = 0; contextIndex < MAX_GDDA_CONTEXT; contextIndex++ )
	{
		if ( contextIndex != gdda->gddaContextIndex || gdda->isInstanceDestroyed ) // do nothing with current context if the app is running.
		{
			GDDA_CONTEXT * gddaContext = gdda->GetContext( contextIndex );
			bool isContextShouldBeCleaned = ( !gddaContext->fStarted || ( gddaContext->fDonePlaying && !gddaContext->fCleaned ) );
			if ( isContextShouldBeCleaned )
			{				
				gddaContext->fPlayBackgroundSound = false;
				
				if ( gddaContext->hPlayCommandThread )
				{
					CloseHandle( gddaContext->hPlayCommandThread );
					gddaContext->hPlayCommandThread = NULL;
				}

				if ( gddaContext->hSoundResumeEvent )
				{
					CloseHandle( gddaContext->hSoundResumeEvent );
					gddaContext->hSoundResumeEvent = NULL;
				}

				gddaContext->fCleaned = true;			
			}
		}
	}

	gdda->isCleaningFinished = true;

//	LeaveCriticalSection( &gdda->csThread );

#ifdef DEBUG
	DebugOutput( TEXT("CleanerThread: (%d) [0x%08x] Done!\n"), currentContextIndex, dwThreadId );
#endif

	return 0;
}
