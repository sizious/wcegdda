#include "gddadrv.hpp"
#include "error.hpp"
#include "utils.hpp"

void
GDAudioDriver::Play( SEGACD_PLAYTRACK playtrack )
{
#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command Start: Play\n"), this->gddaContextIndex);
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
	// Select an empty and ready-to-use slot
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
	DebugOutput(TEXT("[%d] Context slot is now chosen!\n"), this->gddaContextIndex);
#endif

	// Resetting the chosen slot.
	this->Reset();

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
	
	// Notify the object, the music playback is started
	gddaContext->fStarted = true;

	// Executing the Watcher Thread (it will run the streaming process)
    gddaContext->hPlayCommandThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PlayCommandThreadProc, this, 0, &dwThreadId);	

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command End: Play\n"), this->gddaContextIndex);
#endif
}

// PlayCommandThread
DWORD WINAPI
GDAudioDriver::PlayCommandThreadProc( LPVOID lpParameter )
{
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;

	unsigned long repeatIndex = 0, soundIndex = 0, repeatCount, soundStartIndex, soundEndIndex;
	int currentContextIndex = gdda->gddaContextIndex;	
	GDDA_CONTEXT *gddaContext = gdda->GetCurrentContext();

#ifdef DEBUG
	DebugOutput(TEXT("[%d] PlayCommandThread: Start, context slot: #%d, repeat count: %d\n"), currentContextIndex, gdda->gddaContextIndex, gddaContext->playRepeatCount);
#endif

	// Get parsed SEGACD_PLAYTRACK data
	HANDLE hIOMutex = CreateMutex( NULL, FALSE, NULL );
	if ( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );
		
		repeatCount = gddaContext->playRepeatCount;
		soundStartIndex = gddaContext->playSoundStartIndex;
		soundEndIndex = gddaContext->playSoundEndIndex;

		ReleaseMutex( hIOMutex );
	}
#ifdef DEBUG
	else
	{
		DebugOutput(TEXT("[%d] PlayCommandThread: Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\n"), currentContextIndex, GetLastError());
	}
#endif


	// Playing the parsed SEGACD_PLAYTRACK
	while( !gddaContext->fExiting && repeatIndex < repeatCount )
	{

#ifdef DEBUG
		DebugOutput(TEXT("[%d] PlayCommandThread: Executing SEGACD_PLAYTRACK sequence, track start: #%d, track end: #%d, repeat: #%d\n"), currentContextIndex, soundStartIndex, soundEndIndex, repeatCount);
#endif

		soundIndex = soundStartIndex;
		while( !gddaContext->fExiting && soundIndex < soundEndIndex )
		{

#ifdef DEBUG
			DebugOutput(TEXT("[%d] PlayCommandThread: Launching the play of the track #%d (%d of %d)...\n"), currentContextIndex, soundIndex, repeatIndex, repeatCount);
#endif
			// Playing the requested track index
			HANDLE hStreamThread = gdda->PlaySoundTrackIndex( soundIndex );			
			if ( hStreamThread )
			{

#ifdef DEBUG
				DebugOutput(TEXT("[%d] PlayCommandThread: Waiting for StreamThread...\n"), currentContextIndex);
#endif

				WaitForSingleObject( hStreamThread, INFINITE );
				CloseHandle( hStreamThread );
			}

			if ( !gdda->IsPaused() )
			{
				soundIndex++;
			}
		}

		if ( !gdda->IsPaused() )
		{
			repeatIndex++;
		}
	}

#ifdef DEBUG
	DebugOutput(TEXT("[%d] PlayCommandThread: Sound sequence play finished!\n"), currentContextIndex);
	DebugOutput(TEXT("[%d] PlayCommandThread: Done!\n"), currentContextIndex);
#endif

	return 0;
}

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
		ReadFile( gddaContext->hSoundFile, byTemp, 256, &cbRead, NULL );		

		// Parse the header information to get information.
		ParseWaveFile( (void*)byTemp, &pwfx, &pbyData, &dwSize );
    
		// Set file pointer to point to start of data
		SetFilePointer( gddaContext->hSoundFile, (int)(pbyData - byTemp), NULL, FILE_BEGIN );

		// Create the sound buffer
#ifdef DEBUG
		DebugOutput(TEXT("[%d] PlayCommandThread: PlaySoundTrackIndex: Creating the sound buffer for \"%s\"...\n"), currentContextIndex, szWaveFile);
#endif			
		gddaContext->pdsbBackground = CreateSoundBuffer( pwfx->nSamplesPerSec, pwfx->wBitsPerSample, BUFFERSIZE );
		if ( !gddaContext->pdsbBackground )
		{
			goto end;
		}

		// Prepare the wav file for streaming (set up event notifications)
		if ( !PrepareForStreaming( gddaContext->pdsbBackground, BUFFERSIZE, &gddaContext->hSoundNotifyEvent ) )
		{
			goto end;
		}

		// Sound resume event
		gddaContext->hSoundResumeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		if( gddaContext->pdsbBackground )
		{
			// Fill the sound buffer with the start of the wav file.
			gddaContext->pdsbBackground->Lock( 0, BUFFERSIZE, (void **)&pbyBlock1, &nBytes1, (void **)&pbyBlock2, &nBytes2, 0 );
			ReadFile( gddaContext->hSoundFile, pbyBlock1, nBytes1, &cbRead, NULL );
			gddaContext->pdsbBackground->Unlock( pbyBlock1, nBytes1, pbyBlock2, nBytes2 );

			// Create a separate thread which will handling streaming the sound
			hStreamThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) StreamThreadProc, this, 0, &dwThreadId );
			
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
				DebugOutput(TEXT("[%d] StreamThread Created (dwThreadId=0x%x) for \"%s\"...\n"), currentContextIndex, dwThreadId, szWaveFile);
			}
#endif

			// Start the sound playing
			gddaContext->pdsbBackground->Play( 0, 0, DSBPLAY_LOOPING );

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
