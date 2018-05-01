#include "gddadrv.hpp"
#include "error.hpp"
#include "utils.hpp"



void
GDAudioDriver::CleanThread( HANDLE hThread )
{
	if ( hThread != NULL )
	{
		WaitForSingleObject( hThread, INFINITE );
		CloseHandle( hThread );
	}
}

bool
GDAudioDriver::IsThreadCreated( HANDLE hThread, DWORD dwThreadId )
{
	bool result = ( hThread != NULL );

#ifdef DEBUG
	if ( !result )
	{
		DebugOutput( TEXT("Thread [0x%08x] was not created!\n"), dwThreadId );
	}
	else
	{
		DebugOutput( TEXT("Thread [0x%08x] created and now ready.\n"), dwThreadId );
	}
#endif

	return result;
}



void
GDAudioDriver::Reset()
{
	int currentContextIndex = this->gddaContextIndex;

	GDDA_CONTEXT * gddaContext = this->GetCurrentContext();
	
	gddaContext->fDonePlaying = false;
	gddaContext->fStarted = false;
	gddaContext->fPlayBackgroundSound = false;	
	gddaContext->fExiting = false;
	gddaContext->fPaused = false;
	gddaContext->errLast = 0;
}


bool
GDAudioDriver::IsReady()
{
	return ( this->pds != NULL );
}



bool
GDAudioDriver::ChangePlayingStatus( bool allowPlaying )
{
	int currentContextIndex = this->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] ChangePlayingStatus request: %s\n"), currentContextIndex, allowPlaying ? TEXT("RESUME") : TEXT("PAUSE"));
#endif

	bool doAction = false;

	GDDA_CONTEXT *gddaContext = this->GetCurrentContext();

	HANDLE hIOMutex = CreateMutex( NULL, FALSE, NULL );
	if ( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );

		// Do the pause/resume action if possible
		if ( this->IsReady() && !gddaContext->fExiting )
		{

			doAction = gddaContext->fStarted && (gddaContext->fPaused != !allowPlaying);
			if (doAction)
			{
				gddaContext->fPaused = !allowPlaying;
				gddaContext->fPlayBackgroundSound = allowPlaying;

				if (gddaContext->pdsbBackground)
				{
					gddaContext->pdsbBackground->Stop();
					if (allowPlaying)
					{
						gddaContext->pdsbBackground->Play(0, 0, DSBPLAY_LOOPING);
					}
				}
			}
#ifdef DEBUG
			else
			{
				DebugOutput(TEXT("[%d] ChangePlayingStatus was IGNORED!\n"), currentContextIndex);
			}
#endif			
		}

		ReleaseMutex( hIOMutex );
	}

	// Send the Sound Resume event!
	if ( doAction && allowPlaying && gddaContext->hSoundResumeEvent )
	{
#ifdef DEBUG
		DebugOutput(TEXT("[%d] Sound Resume Event sent!\n"), currentContextIndex);
#endif
		SetEvent( gddaContext->hSoundResumeEvent );
	}

	return doAction;
}



bool
GDAudioDriver::IsPaused()
{
	bool value = false;
	
	HANDLE hIOMutex = CreateMutex( NULL, FALSE, NULL );
	if ( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );
		GDDA_CONTEXT * gddaContext = this->GetCurrentContext();
		value = ( gddaContext->fPaused && !gddaContext->fExiting && !gddaContext->fDonePlaying );
		ReleaseMutex( hIOMutex);
	}
#ifdef DEBUG
	else
	{
		DebugOutput(TEXT("IsPaused Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\n"), GetLastError());
	}
#endif

	return value;
}

bool
GDAudioDriver::IsPlaying()
{
	bool value = false;
	HANDLE hIOMutex = CreateMutex( NULL, FALSE, NULL );
	if( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );	

		GDDA_CONTEXT *gddaContext = this->GetCurrentContext();
		value = ( gddaContext->fPlayBackgroundSound && !gddaContext->fExiting && !gddaContext->fDonePlaying );	
#ifdef DEBUG
		DebugOutput(TEXT("IsPlaying: %s\n"), value ? TEXT("YES") : TEXT("NO"));
#endif
		ReleaseMutex( hIOMutex);	
	}
#ifdef DEBUG
	else
	{
		DebugOutput(TEXT("IsPaused Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\n"), GetLastError());
	}
#endif
	return value;
}

