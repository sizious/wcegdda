#include "gddadrv.hpp"
#include "error.hpp"

DWORD WINAPI
GDAudioDriver::CleanerThreadProc( LPVOID lpParameter )
{
	GDAudioDriver * gdda = (GDAudioDriver *) lpParameter;

	int currentContextIndex = gdda->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] CleanerThread: Start!\n"), currentContextIndex);
#endif

	EnterCriticalSection( &gdda->csThread );

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
	LeaveCriticalSection( &gdda->csThread );

#ifdef DEBUG
	DebugOutput(TEXT("[%d] CleanerThread: Done!\n"), currentContextIndex);
#endif

	return 0;
}
