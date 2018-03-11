#include "gddadrv.hpp"
#include "error.hpp"

// CleanerThread
DWORD WINAPI
GDAudioDriver::CleanerThreadProc( LPVOID lpParameter )
{
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;
	int currentContextIndex = gdda->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] CleanerThread: Start!\r\n"), currentContextIndex);
#endif

	EnterCriticalSection( &gdda->csThread );

	for( int contextIndex = 0; contextIndex < MAX_GDDA_CONTEXT; contextIndex++ )
	{
		if ( contextIndex != gdda->gddaContextIndex || gdda->isInstanceDestroyed ) // do nothing with current context if the app is running.
		{
			GDDA_CONTEXT* gddaContext = gdda->GetContext( contextIndex );
			bool isContextShouldBeCleaned = ( !gddaContext->fStarted || ( gddaContext->fDonePlaying && !gddaContext->fCleaned ) );
			if ( isContextShouldBeCleaned )
			{
#ifdef DEBUG
				DebugOutput(TEXT("[%d] CleanerThread: Slot #%d will be cleaned up...\r\n"), currentContextIndex, contextIndex);
#endif
				if ( gddaContext->hSoundNotifyEvent )
				{
					CloseHandle( gddaContext->hSoundNotifyEvent );
					gddaContext->hSoundNotifyEvent = NULL;
				}

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

				if ( gddaContext->hSoundFile )
				{
					CloseHandle( gddaContext->hSoundFile );
					gddaContext->hSoundFile = NULL;
				}

				if ( gddaContext->pdsbBackground )
				{
					gddaContext->pdsbBackground->Stop();
					gddaContext->pdsbBackground->Release();
					gddaContext->pdsbBackground = NULL;
				}

				gddaContext->fCleaned = true;
				
#ifdef DEBUG
				DebugOutput(TEXT("[%d] CleanerThread: Slot #%d has been cleaned up!\r\n"), currentContextIndex, contextIndex);
#endif				
			}
		}
	}

	gdda->isCleaningFinished = true;
	LeaveCriticalSection( &gdda->csThread );

#ifdef DEBUG
	DebugOutput(TEXT("[%d] CleanerThread: Done!\r\n"), currentContextIndex);
#endif

	return 0;
}