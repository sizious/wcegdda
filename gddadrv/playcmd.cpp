#include "gddadrv.hpp"
#include "error.hpp"
#include "utils.hpp"

void
GDAudioDriver::Play( SEGACD_PLAYTRACK playtrack )
{
#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command Start: Play\n"), this->gddaContextIndex);
#endif

	GDDA_CONTEXT * gddaContext;

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
	gddaContext = this->SetContext( playtrack );

	// Executing the Watcher Thread (it will run the streaming process) 
	DWORD dwThreadId;
    gddaContext->hPlayCommandThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) PlayCommandThreadProc, this, NULL, &dwThreadId );
	IsThreadCreated( gddaContext->hPlayCommandThread, dwThreadId );
	
#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command End: Play\n"), this->gddaContextIndex);
#endif
}

// PlayCommandThread
DWORD WINAPI
GDAudioDriver::PlayCommandThreadProc( LPVOID lpParameter )
{
	GDAudioDriver * gdda = (GDAudioDriver*) lpParameter;
	
	DWORD dwThreadId = GetCurrentThreadId();
	int currentContextIndex = gdda->gddaContextIndex;
	GDDA_CONTEXT * gddaContext = gdda->GetCurrentContext();

	unsigned long repeatIndex = 0, soundIndex = 0, repeatCount, soundStartIndex, soundEndIndex;

#ifdef DEBUG
	DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] Start, context slot: #%d, repeat count: %d\n"), currentContextIndex, dwThreadId, gdda->gddaContextIndex, gddaContext->playRepeatCount);
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
		DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\n"), currentContextIndex, dwThreadId, GetLastError());
	}
#endif


	// Playing the parsed SEGACD_PLAYTRACK
	while( !gddaContext->fExiting && repeatIndex < repeatCount )
	{

#ifdef DEBUG
		DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] Executing SEGACD_PLAYTRACK sequence, track start: #%d, track end: #%d, repeat: #%d\n"), currentContextIndex, dwThreadId, soundStartIndex, soundEndIndex, repeatCount);
#endif

		soundIndex = soundStartIndex;
		while( !gddaContext->fExiting && soundIndex < soundEndIndex )
		{

			// Playing the requested track index
			gdda->PlaySoundTrackIndex( soundIndex, dwThreadId );			
			if ( gddaContext->hStreamThread )
			{
#ifdef DEBUG
				DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] Waiting for StreamThread...\n"), currentContextIndex, dwThreadId);
#endif
				if ( gddaContext->hStreamThread && !gdda->isInstanceDestroyed )
				{
					WaitForSingleObject( gddaContext->hStreamThread, INFINITE );
					gddaContext->hStreamThread = NULL;
				}
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
	DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] Sound sequence play finished!\n"), currentContextIndex, dwThreadId);
	DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] Done!\n"), currentContextIndex, dwThreadId);
#endif

	return 0;
}

HANDLE
GDAudioDriver::PlaySoundTrackIndex( int playTrackIndex, DWORD dwThreadId )
{
	HANDLE hStreamThreadCreated = NULL;
	int currentContextIndex = this->gddaContextIndex;

	HANDLE hIOMutex = CreateMutex( NULL, FALSE, NULL );
	if ( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );
     
		ULONG			nBytes1, nBytes2, cbRead;
		BYTE			*pbyBlock1, *pbyBlock2;
	
		
		GDDA_CONTEXT * gddaContext = this->GetCurrentContext();

		// Check if the driver is ready
		if ( !this->IsReady() )
		{
#ifdef DEBUG
			DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] PlaySoundTrackIndex: Driver is NOT ready!\n"), currentContextIndex, dwThreadId);
#endif			
			goto end;
		}

#ifdef DEBUG
		DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] PlaySoundTrackIndex: Launching playback for \"%d\"...\n"), currentContextIndex, dwThreadId, playTrackIndex);
#endif

		// Loading audio context!
		AUDIO_TRACK_CONTEXT audioTrackContext;
		if ( !this->_audiomgr.GetAudioTrackContext( playTrackIndex, &audioTrackContext ) )
		{		
#ifdef DEBUG
			DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] PlaySoundTrackIndex: Sorry, GetAudioTrackContext FAILED!!!\n"), currentContextIndex, dwThreadId);
#endif
			goto end;
		}
		
		// Initializing important variables
		gddaContext->hSoundNotifyEvent = audioTrackContext.hEventNotify;
		gddaContext->hSoundFile = audioTrackContext.hTrackFile;
		gddaContext->pdsbBackground = audioTrackContext.pSoundBuffer;

		// Set file pointer to point to start of data
		SetFilePointer( gddaContext->hSoundFile, audioTrackContext.dwSoundDataOffset, NULL, FILE_BEGIN );

		// Sound resume event
		gddaContext->hSoundResumeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		if( gddaContext->pdsbBackground )
		{
			// Fill the sound buffer with the start of the wav file.
			gddaContext->pdsbBackground->Lock( 0, BUFFERSIZE, (void **)&pbyBlock1, &nBytes1, (void **)&pbyBlock2, &nBytes2, 0 );
			ReadFile( gddaContext->hSoundFile, pbyBlock1, nBytes1, &cbRead, NULL );
			gddaContext->pdsbBackground->Unlock( pbyBlock1, nBytes1, pbyBlock2, nBytes2 );

			// Start the sound playing
			gddaContext->pdsbBackground->Play( 0, 0, DSBPLAY_LOOPING );


			DWORD dwThreadId;

			// Create a separate thread which will handling streaming the sound
			gddaContext->hStreamThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) StreamThreadProc, this, NULL, &dwThreadId );
			if ( this->IsThreadCreated( gddaContext->hStreamThread, dwThreadId ) )
			{
				hStreamThreadCreated = gddaContext->hStreamThread;
				gddaContext->fPlayBackgroundSound = true;
			}						
		}
end:
		ReleaseMutex( hIOMutex );
	}
#ifdef DEBUG
	else
	{
		DebugOutput(TEXT("PlayCommandThread: (%d) [0x%08x] PlaySoundTrackIndex Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\n"), currentContextIndex, dwThreadId, GetLastError());
	}
#endif

	return hStreamThreadCreated;
}
