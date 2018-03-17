#include "gddadrv.hpp"
#include "error.hpp"
#include "utils.hpp"

HANDLE
GDAudioDriver::PlaySoundTrackIndex( int playTrackIndex )
{
	int currentContextIndex = this->gddaContextIndex;
	HANDLE hStreamThread = NULL;
	HANDLE hIOMutex = CreateMutex( NULL, FALSE, NULL );
	if ( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );

		WAVEFORMATEX	*pwfx;         
		DWORD			dwThreadId, dwSize;
		ULONG			nBytes1, nBytes2, cbRead;
		BYTE			byTemp[256], *pbyBlock1, *pbyBlock2, *pbyData;
		TCHAR			szWaveFile[MAX_PATH];
		GDDA_CONTEXT	*gddaContext = this->GetCurrentContext();

		// Check if the driver is ready
		if ( !this->IsReady() )
		{
#ifdef DEBUG
			DebugOutput(TEXT("[%d] PlayCommandThread: PlaySoundTrackIndex: Driver is NOT ready!\n"), currentContextIndex);
#endif			
			goto end;
		}

		// Getting the WAV file name to load
		GetSoundFilePath( playTrackIndex, szWaveFile );

#ifdef DEBUG
		DebugOutput(TEXT("[%d] PlayCommandThread: PlaySoundTrackIndex: Launching playback for \"%s\"...\n"), currentContextIndex, szWaveFile);
#endif

		// Load the wav file that we want to stream in the background.
		gddaContext->hSoundFile = CreateFile( szWaveFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
		if ( gddaContext->hSoundFile == INVALID_HANDLE_VALUE )
		{		
#ifdef DEBUG
			DebugOutput(TEXT("[%d] PlayCommandThread: PlaySoundTrackIndex: Sorry, file \"%s\" doesn't exists...\n"), currentContextIndex, szWaveFile);
#endif
			goto end;
		}

		// Read the first 256 bytes to get file header
#ifdef DEBUG
		DebugOutput(TEXT("[%d] PlayCommandThread: PlaySoundTrackIndex: Getting the file header for \"%s\"...\n"), currentContextIndex, szWaveFile);
#endif		
		ReadFile(gddaContext->hSoundFile, byTemp, 256, &cbRead, NULL);

		// Parse the header information to get information.
		ParseWaveFile((void*)byTemp, &pwfx, &pbyData, &dwSize);
    
		// Set file pointer to point to start of data
		SetFilePointer(gddaContext->hSoundFile, (int)(pbyData - byTemp), NULL, FILE_BEGIN);

		// Create the sound buffer
#ifdef DEBUG
		DebugOutput(TEXT("[%d] PlayCommandThread: PlaySoundTrackIndex: Creating the sound buffer for \"%s\"...\n"), currentContextIndex, szWaveFile);
#endif			
		gddaContext->pdsbBackground = CreateSoundBuffer(pwfx->nSamplesPerSec, pwfx->wBitsPerSample, BUFFERSIZE);
		if (!gddaContext->pdsbBackground)
		{
			goto end;
		}

		// Prepare the wav file for streaming (set up event notifications)
		if (!PrepareForStreaming(gddaContext->pdsbBackground, BUFFERSIZE, &gddaContext->hSoundNotifyEvent))
		{
			goto end;
		}

		// Sound resume event
		gddaContext->hSoundResumeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		if( gddaContext->pdsbBackground )
		{
			// Fill the sound buffer with the start of the wav file.
			gddaContext->pdsbBackground->Lock(0, BUFFERSIZE, (void **)&pbyBlock1, &nBytes1, (void **)&pbyBlock2, &nBytes2, 0);
			ReadFile(gddaContext->hSoundFile, pbyBlock1, nBytes1, &cbRead, NULL);
			gddaContext->pdsbBackground->Unlock(pbyBlock1, nBytes1, pbyBlock2, nBytes2);

			// Create a separate thread which will handling streaming the sound
			hStreamThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) StreamThreadProc, this, 0, &dwThreadId);
			if (!hStreamThread)
			{
#ifdef DEBUG
				DebugOutput(TEXT("[%d] Error calling CreateThread for StreamThread!\n"), currentContextIndex);
#endif
				goto end;
			}
#ifdef DEBUG
			else
			{
				DebugOutput(TEXT("[%d] StreamThread Created (0x%x) for \"%s\"...\n"), currentContextIndex, dwThreadId, szWaveFile);
			}
#endif

			// Start the sound playing
			gddaContext->pdsbBackground->Play(0, 0, DSBPLAY_LOOPING);

			gddaContext->fPlayBackgroundSound = true;    
		}
end:
		ReleaseMutex( hIOMutex );
	}
#ifdef DEBUG
	else
	{
		DebugOutput(TEXT("[%d] PlayCommandThread: PlaySoundTrackIndex Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\n"), currentContextIndex, GetLastError());
	}
#endif

	return hStreamThread;
}


////////////////////////////////////////////////////////////////////////////////
// GDAudioDriver Constructor/Destructor
////////////////////////////////////////////////////////////////////////////////

GDAudioDriver::GDAudioDriver()
{
#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver initializing...\n"));
#endif

	this->isInstanceDestroyed = false;
	this->isCleaningFinished = false;
	this->pds = NULL;
	this->gddaContextIndex = 0;	
	
	memset(this->gddaContextStorage, 0, sizeof(this->gddaContextStorage));
	for(int i = 0; i < MAX_GDDA_CONTEXT; i++)
	{
		this->gddaContextStorage[i].fCleaned = true;
		this->gddaContextStorage[i].pdsbBackground = NULL;
		this->gddaContextStorage[i].hPlayCommandThread = NULL;
		this->gddaContextStorage[i].hSoundFile = NULL;
		this->gddaContextStorage[i].hSoundNotifyEvent = NULL;
		this->gddaContextStorage[i].hSoundResumeEvent = NULL;		
	}

	InitializeCriticalSection( &csThread );

#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver initializing is done!\n"));
#endif
}

GDAudioDriver::~GDAudioDriver()
{
#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver finalizing...\n"));
#endif

	// Stop the current sound (if needed)
	this->Stop();

	this->isInstanceDestroyed = true;
	this->CleanUp();

#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver finalizing is done!\n"));
#endif
}

GDDA_CONTEXT *
GDAudioDriver::GetContext(int index)
{
	return &(this->gddaContextStorage[index]);
}

GDDA_CONTEXT *
GDAudioDriver::GetCurrentContext()
{
	return this->GetContext( this->gddaContextIndex );
}

void
GDAudioDriver::Initialize( LPDIRECTSOUND pds )
{
	this->pds = pds;
}

bool
GDAudioDriver::IsReady()
{
	return ( this->pds != NULL );
}

void
GDAudioDriver::CleanUp()
{
	int currentContextIndex = this->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command Start: Cleanup\n"), currentContextIndex);
#endif

	if ( this->hGarbageCollectorThread != NULL ) 
	{
		if ( this->isCleaningFinished )
		{
#ifdef DEBUG
			DebugOutput(TEXT("[%d] CleanerThread was successfully finished, closing handle...\n"), currentContextIndex);
#endif
			if( hGarbageCollectorThread )
			{
				CloseHandle( hGarbageCollectorThread );
				hGarbageCollectorThread = NULL;
			}
		}
		else
		{
#ifdef DEBUG
			DebugOutput(TEXT("[%d] Cleanup command is still running...\n"), currentContextIndex);
#endif
			// Stop here!
			return;
		}
	}

	// Run the Garbage Collector Thread
	this->isCleaningFinished = false;
	DWORD dwThreadId;	
	this->hGarbageCollectorThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) CleanerThreadProc, this, NULL, &dwThreadId );				

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command End: Cleanup\n"), currentContextIndex);
#endif
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
GDAudioDriver::Pause()
{
	this->ChangePlayingStatus(false);
	return this->IsPaused();
}

void
GDAudioDriver::Resume()
{
	this->ChangePlayingStatus(true);
}

void
GDAudioDriver::Reset()
{
	int currentContextIndex = this->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command Start: Reset\n"), currentContextIndex);
#endif

	GDDA_CONTEXT *gddaContext = this->GetCurrentContext();
	
	gddaContext->fDonePlaying = false;
	gddaContext->fStarted = false;
	gddaContext->fPlayBackgroundSound = false;	
	gddaContext->fExiting = false;
	gddaContext->fPaused = false;
	gddaContext->errLast = 0;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command End: Reset\n"), currentContextIndex);
#endif
}

void
GDAudioDriver::Play( SEGACD_PLAYTRACK playtrack )
{
#ifdef DEBUG
	DebugOutput(TEXT("[?] Command Start: Play\n"));
#endif

	DWORD			dwThreadId;
	GDDA_CONTEXT	*gddaContext;

	// If we are in PAUSE state, try to resume the sound instead of launching a new PLAY process
	gddaContext = this->GetCurrentContext();
	bool isSamePlaytrack = (playtrack.dwStartTrack == gddaContext->dwPreviousStartTrack) && (playtrack.dwEndTrack == gddaContext->dwPreviousEndTrack);
	if( this->IsPaused() && isSamePlaytrack )
	{
		this->Resume();
		return;
	}

	// Stop the current playing thread
	this->Stop();	

	// Clean up the current objects (we do nothing with the current slot, because the thread still be running)
	this->CleanUp();

	// Start a new GDDA context
	do
	{
		this->gddaContextIndex++; // this next slot was cleaned by the CleanUp method above. the only one slot that wasn't cleaned is the previous slot (see above)
		if ( this->gddaContextIndex >= MAX_GDDA_CONTEXT )
		{
			this->gddaContextIndex = 0;
		}
		gddaContext = this->GetCurrentContext();
	}
	while( !gddaContext->fCleaned );

#ifdef DEBUG
	DebugOutput(TEXT("[?] Chosen Play context slot: %d\n"), this->gddaContextIndex);
#endif

	// Resetting the chosen slot.
	this->Reset();

	// Notify the object, the music playback is started
	gddaContext->fStarted = true;

	// Saving the current playtrack valuable data		
	gddaContext->dwPreviousStartTrack = playtrack.dwStartTrack;
	gddaContext->dwPreviousEndTrack = playtrack.dwEndTrack;

	// Initializing sound parameters
	gddaContext->playSoundStartIndex = playtrack.dwStartTrack;
	gddaContext->playSoundEndIndex = playtrack.dwEndTrack + 1;	

	// Get the repeat count number
	gddaContext->playRepeatCount = playtrack.dwRepeat + 1;
	
	// Infinite loop requested?
	gddaContext->playRepeatCount = (gddaContext->playRepeatCount > 16) ? INT_MAX : gddaContext->playRepeatCount;
	
	// Executing the Watcher Thread (it will run the streaming process)
    gddaContext->hPlayCommandThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PlayCommandThreadProc, this, 0, &dwThreadId);	

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command End: Play\n"), this->gddaContextIndex);
#endif
}

void
GDAudioDriver::Stop()
{
	int currentContextIndex = this->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command Start: Stop\n"), currentContextIndex);
#endif

	// We are stopping the current playing sound
	HANDLE hIOMutex = CreateMutex( NULL, FALSE, NULL );
	if ( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );	

		GDDA_CONTEXT *gddaContext = this->GetCurrentContext();

		if ( gddaContext->fStarted )
		{
#ifdef DEBUG
			DebugOutput(TEXT("[%d] Performing stop command...\n"), currentContextIndex);
#endif

			gddaContext->fExiting = true;
			gddaContext->fStarted = false;

			if ( gddaContext->pdsbBackground )
			{
				gddaContext->pdsbBackground->Stop();
			}	
		}

		ReleaseMutex( hIOMutex);
	}
#ifdef DEBUG
	else
	{
		DebugOutput(TEXT("[%d] Stop Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\n"), currentContextIndex, GetLastError());
	}

	DebugOutput(TEXT("[%d] Command End: Stop\n"), currentContextIndex);
#endif
}

bool
GDAudioDriver::IsPaused()
{
	bool value = false;
	
	HANDLE hIOMutex = CreateMutex (NULL, FALSE, NULL);
	if ( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );
		GDDA_CONTEXT *gddaContext = this->GetCurrentContext();
		value = ( gddaContext->fPaused && !gddaContext->fExiting && !gddaContext->fDonePlaying );

#ifdef DEBUG
//		DebugOutput(TEXT("IsPaused: %s\n"), value ? TEXT("YES") : TEXT("NO"));
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

VOLUME_CONTROL
GDAudioDriver::GetVolume()
{
	VOLUME_CONTROL volume;

	volume.PortVolume[0] = 0;
	volume.PortVolume[1] = 0;

	return volume;
}

void
GDAudioDriver::SetVolume( VOLUME_CONTROL volume )
{
	UCHAR ucLeftVolume  = volume.PortVolume[0];
	UCHAR ucRightVolume = volume.PortVolume[1];

	GDDA_CONTEXT *gddaContext = this->GetCurrentContext();

//	gddaContext->pdsbBackground->SetPan( xxx );

//	GetPan
}

BOOL
GDAudioDriver::CheckError(TCHAR *tszErr)
{
	GDDA_CONTEXT *gddaContext = this->GetCurrentContext();
	BOOL isError = (gddaContext->errLast != 0);

#ifdef DEBUG
    if ( isError )
    {
        DebugOutput(TEXT("CheckError: %s failed (Error # = 0x%08x).\n"), tszErr, gddaContext->errLast);        
    }	
#endif

    return ( isError );
}

