#include "gddadrv.hpp"
#include "error.hpp"
#include "utils.hpp"

// PlayCommandThread
DWORD WINAPI
GDAudioDriver::PlayCommandThreadProc( LPVOID lpParameter )
{
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;
	int currentContextIndex = gdda->gddaContextIndex;
	unsigned long repeatIndex = 0, soundIndex = 0, repeatCount, soundStartIndex, soundEndIndex;
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
		if ( !PrepareForStreaming(gddaContext->pdsbBackground, BUFFERSIZE, &gddaContext->hSoundNotifyEvent) )
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
