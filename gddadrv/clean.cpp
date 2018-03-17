#include "gddadrv.hpp"
#include "error.hpp"

// CleanerThread
DWORD WINAPI
GDAudioDriver::CleanerThreadProc( LPVOID lpParameter )
{
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;
	int currentContextIndex = gdda->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] CleanerThread: Start!\n"), currentContextIndex);
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
				DebugOutput(TEXT("[%d] CleanerThread: Slot #%d will be cleaned up...\n"), currentContextIndex, contextIndex);
#endif
				gddaContext->fPlayBackgroundSound = false;
#ifdef DEBUG
				DebugOutput(TEXT("[%d]   CleanerThread: [%d] Cleaning pdsbBackground (%d)\n"), currentContextIndex, contextIndex, gddaContext->pdsbBackground);
#endif
				if ( gddaContext->pdsbBackground )
				{
					Sleep(100);

					__try
					{
						gddaContext->pdsbBackground->Stop();
						gddaContext->pdsbBackground->Release();
					}
					__finally
					{
						gddaContext->pdsbBackground = NULL;
					}
				}

#ifdef DEBUG
				DebugOutput(TEXT("[%d]   CleanerThread: [%d] Cleaning hSoundNotifyEvent\n"), currentContextIndex, contextIndex);
#endif
				if ( gddaContext->hSoundNotifyEvent )
				{
					CloseHandle( gddaContext->hSoundNotifyEvent );
					gddaContext->hSoundNotifyEvent = NULL;
				}

#ifdef DEBUG
				DebugOutput(TEXT("[%d]   CleanerThread: [%d] Cleaning hPlayCommandThread\n"), currentContextIndex, contextIndex);
#endif
				if ( gddaContext->hPlayCommandThread )
				{
					CloseHandle( gddaContext->hPlayCommandThread );
					gddaContext->hPlayCommandThread = NULL;
				}

#ifdef DEBUG
				DebugOutput(TEXT("[%d]   CleanerThread: [%d] Cleaning hSoundResumeEvent\n"), currentContextIndex, contextIndex);
#endif
				if ( gddaContext->hSoundResumeEvent )
				{
					CloseHandle( gddaContext->hSoundResumeEvent );
					gddaContext->hSoundResumeEvent = NULL;
				}

#ifdef DEBUG
				DebugOutput(TEXT("[%d]   CleanerThread: [%d] Cleaning hSoundFile\n"), currentContextIndex, contextIndex);
#endif
				if ( gddaContext->hSoundFile )
				{
					CloseHandle( gddaContext->hSoundFile );
					gddaContext->hSoundFile = NULL;
				}

				gddaContext->fCleaned = true;
				
#ifdef DEBUG
				DebugOutput(TEXT("[%d] CleanerThread: Slot #%d has been cleaned up!\n"), currentContextIndex, contextIndex);
#endif				
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