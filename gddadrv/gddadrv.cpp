#include "gddadrv.hpp"
#include "error.hpp"
#include "utils.hpp"

// Critical Section for Threads
CRITICAL_SECTION g_csCleanerThread;
CRITICAL_SECTION g_csStreamLoaderThread;
CRITICAL_SECTION g_csPlayCommandThread;

// CleanerThread
DWORD WINAPI GDAudioDriver::CleanerThreadProc( LPVOID lpParameter )
{
	GDAudioDriver *gdda = (GDAudioDriver*)lpParameter;
//	EnterCriticalSection( &g_csCleanerThread );
	
#ifdef DEBUG
	DebugOutput(TEXT("CleanerThread: Start!\r\n"));
#endif

	for( int contextIndex = 0; contextIndex < MAX_GDDA_CONTEXT; contextIndex++ )
	{
		if ( contextIndex != gdda->gddaContextIndex || gdda->isInstanceDestroyed ) // do nothing with current context if the app is running.
		{
			GDDA_CONTEXT* gddaContext = gdda->GetContext( contextIndex );
			bool isContextShouldBeCleaned = ( !gddaContext->fStarted || ( gddaContext->fDonePlaying && !gddaContext->fCleaned ) );
			if ( isContextShouldBeCleaned )
			{
#ifdef DEBUG
				DebugOutput(TEXT("CleanerThread: Slot #%d will be cleaned up...\r\n"), contextIndex);
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

				if (gddaContext->pdsbBackground)
				{
					gddaContext->pdsbBackground->Stop();
					gddaContext->pdsbBackground->Release();
					gddaContext->pdsbBackground = NULL;
				}

				gddaContext->fCleaned = true;
				
#ifdef DEBUG
				DebugOutput(TEXT("CleanerThread: Slot #%d has been cleaned up!\r\n"), contextIndex);
#endif				
			}
		}
	}

	gdda->isCleaningFinished = true;

#ifdef DEBUG
	DebugOutput(TEXT("CleanerThread: Done!\r\n"));
#endif

//	LeaveCriticalSection( &g_csCleanerThread );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// PlayCommandThread Implementation
////////////////////////////////////////////////////////////////////////////////

BOOL GDAudioDriver::PrepareForStreaming( IDirectSoundBuffer *pdsb, DWORD dwBufferSize, HANDLE *phEventNotify )
{
#ifdef DEBUG
	DebugOutput(TEXT("PlayCommandThread: PlaySoundTrackIndex: PrepareForStreaming...\r\n"));
#endif

    IDirectSoundNotify	*pdsn;
	DSBPOSITIONNOTIFY	rgdsbpn[3];
	GDDA_CONTEXT		*gddaContext = this->GetCurrentContext();
	
    // Get a DirectSoundNotify interface so that we can set the events.
    gddaContext->errLast = pdsb->QueryInterface(IID_IDirectSoundNotify, (void **)&pdsn);
    if (CheckError(TEXT(DSDEBUG_QUERY_INTERFACE)))
	{
        return FALSE;
	}

    *phEventNotify = CreateEvent( NULL, FALSE, FALSE, NULL );

    rgdsbpn[0].hEventNotify = *phEventNotify;
    rgdsbpn[1].hEventNotify = *phEventNotify;
    rgdsbpn[2].hEventNotify = *phEventNotify;
    rgdsbpn[0].dwOffset     = dwBufferSize / 2;
    rgdsbpn[1].dwOffset     = dwBufferSize - 2;
    rgdsbpn[2].dwOffset     = DSBPN_OFFSETSTOP;

    gddaContext->errLast = pdsn->SetNotificationPositions(3, rgdsbpn);
    if (CheckError(TEXT(DSDEBUG_SET_NOTIFICATION_POSITIONS)))
	{
        return FALSE;
	}

    // No longer need the DirectSoundNotify interface, so release it
    pdsn->Release();

    return TRUE;
}

// StreamThread
DWORD WINAPI GDAudioDriver::StreamThreadProc( LPVOID lpParameter ) 
{
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;
//	EnterCriticalSection( &g_csStreamLoaderThread );

#ifdef DEBUG
	DebugOutput(TEXT("StreamThread: Start!\r\n"));
#endif
	
	DWORD fPlaying;
    BYTE  bySilence = 0x80;
    BOOL  fSilence = FALSE;
    BYTE  *pbyBlock1;
    BYTE  *pbyBlock2;
    DWORD ibRead, ibWrite, cbToLock = BUFFERSIZE/2, cbRead;
    DWORD nBytes1, nBytes2;
	
	GDDA_CONTEXT *gddaContext = gdda->GetCurrentContext();
	
#ifdef DEBUG
	DebugOutput(TEXT("StreamThread: Context Slot: #%d!\r\n"), gdda->gddaContextIndex );
#endif
	
    while ( !gddaContext->fExiting )
    {
        // Check if there is a sound to play in the background (streamed).
        if ( gddaContext->fPlayBackgroundSound )
        {
			// The background sound buffer (pdsbBackground) is a 1 second long buffer.
            // It works by playing 500 ms of sound while loading the other 500 ms, and
            // then swapping sides.  As such, we will wait here until one of the sound
            // buffer's predefined events (hit halfway mark, hit end of buffer) occurs.
            WaitForSingleObject( gddaContext->hSoundNotifyEvent, INFINITE );                       

            // Check to see if it's current state is "playing"
            gddaContext->errLast = gddaContext->pdsbBackground->GetStatus( &fPlaying );
            if ( gdda->CheckError( TEXT(DSDEBUG_GET_STATUS) ) )
			{
                return 0;
			}

            if ( fPlaying )
            {
                // Find out where we currently are playing, and where we last wrote
                gddaContext->errLast = gddaContext->pdsbBackground->GetCurrentPosition( &ibRead, &ibWrite );
                if ( gdda->CheckError(TEXT(DSDEBUG_GET_CURRENT_POSITION)) )
				{
                    return 0;
				}
        
                // Lock the next half of the sound buffer
                gddaContext->errLast = gddaContext->pdsbBackground->Lock( ibWrite, cbToLock, (void **)&pbyBlock1, &nBytes1, (void **)&pbyBlock2, &nBytes2, 0 );
                if ( gdda->CheckError(TEXT(DSDEBUG_LOCK)) )
				{
                    return 0;
				}

                // fSilence is true if we hit the end of the sound file on the
                // last pass through here.  In that case, fill both blocks with "silence"
                // and stop the sound from playing.
                if ( fSilence )
                {
                    memset(pbyBlock1, bySilence, nBytes1);
                    memset(pbyBlock2, bySilence, nBytes2);

                    // After filling it, stop playing
                    gddaContext->errLast = gddaContext->pdsbBackground->Stop();
                }
                else
                {
                    // Read the next chunk of bits from the sound file
                    ReadFile( gddaContext->hSoundFile, pbyBlock1, nBytes1, &cbRead, NULL );
                    if ( nBytes1 != cbRead )
                    {
                        // The file has less data than the size of the first block, so fill the
                        // remainder with silence - also fill the second block with silence
                        memset(pbyBlock1 + cbRead, bySilence, nBytes1 - cbRead);
                        memset(pbyBlock2, bySilence, nBytes2);

                        // Next time through, just play silence.
                        fSilence = TRUE;
                    }
                    else
                    {
                        // If there is a second block, then read more of the sound file into that block.
                        if ( nBytes2 )
                        {
                            ReadFile(gddaContext->hSoundFile, pbyBlock2, nBytes2, &cbRead, NULL);
                            if (nBytes2 != cbRead)
                            {
                                // The file has less data than the size of the second block, so
                                // fill the remainder with silence.
                                memset(pbyBlock2 + cbRead, bySilence, nBytes2 - cbRead);

                                // next time through, just play silence
                                fSilence = TRUE;
                            }
                        }
                    }
                }

                // Done with the sound buffer, so unlock it.
                gddaContext->errLast = gddaContext->pdsbBackground->Unlock( pbyBlock1, nBytes1, pbyBlock2, nBytes2 );
                if ( gdda->CheckError(TEXT(DSDEBUG_UNLOCK)) )
				{
                    return 0;
				}
            }
            else
            {
				// Test if we are still playing the sound
				if ( gddaContext->fPlayBackgroundSound )
				{
					// Break out of this while() loop and exit the thread.
					break;
				}
            }
        }
        else
        {
            // App doesn't have a sound to play - don't do anything
			// This is happening when the stream is paused
			if ( gdda->IsPaused() )
			{
#ifdef DEBUG
				DebugOutput(TEXT("StreamThread is waiting to resume the process...\r\n"));
#endif
				if ( gddaContext->pdsbBackground )
				{
					gddaContext->pdsbBackground->Stop();
					WaitForSingleObject( gddaContext->hSoundResumeEvent, INFINITE );
				}
#ifdef DEBUG			
				DebugOutput(TEXT("StreamThread was waked up!\r\n"));
#endif
			}
        }
    } // while

	// If we are playing the sound, then we completed it.
	if ( gddaContext->pdsbBackground )
	{
		gddaContext->pdsbBackground->Stop();
	}

	// This flag tell us if the thread is terminated as well.
	gddaContext->fDonePlaying = true;

#ifdef DEBUG
	DebugOutput(TEXT("StreamThread: Done!\r\n"));	
#endif

//	LeaveCriticalSection( &g_csStreamLoaderThread );
	return 0;
}



HANDLE GDAudioDriver::PlaySoundTrackIndex( int playTrackIndex )
{
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
			DebugOutput(TEXT("PlayCommandThread: PlaySoundTrackIndex: Driver is NOT ready!\r\n"));
#endif			
			goto end;
		}

		// Getting the WAV file name to load
#if SH4
		_stprintf( szWaveFile, TEXT("\\CD-ROM\\GDDA\\TRACK%02d.WAV"), playTrackIndex );

#ifdef DEBUG
		if ( !PathFileExists( szWaveFile ) )
		{
			_stprintf( szWaveFile, TEXT("\\PC\\Applications\\GDDA\\TRACK%02d.WAV"), playTrackIndex );
		}
#endif

#else
		_stprintf( szWaveFile, TEXT("TRACK%02d.WAV"), playTrackIndex );
#endif	

#ifdef DEBUG
		DebugOutput(TEXT("PlayCommandThread: PlaySoundTrackIndex: Launching playback for %s...\r\n"), szWaveFile);
#endif

		// Load the wav file that we want to stream in the background.
		gddaContext->hSoundFile = CreateFile( szWaveFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
		if ( gddaContext->hSoundFile == INVALID_HANDLE_VALUE )
		{		
#ifdef DEBUG
			DebugOutput(TEXT("PlayCommandThread: PlaySoundTrackIndex: Sorry, file %s doesn't exists...\r\n"), szWaveFile);
#endif
			goto end;
		}

		// Read the first 256 bytes to get file header
#ifdef DEBUG
		DebugOutput(TEXT("PlayCommandThread: PlaySoundTrackIndex: Getting the file header...\r\n"));
#endif		
		ReadFile(gddaContext->hSoundFile, byTemp, 256, &cbRead, NULL);

		// Parse the header information to get information.
		ParseWaveFile((void*)byTemp, &pwfx, &pbyData, &dwSize);
    
		// Set file pointer to point to start of data
		SetFilePointer(gddaContext->hSoundFile, (int)(pbyData - byTemp), NULL, FILE_BEGIN);

		// Create the sound buffer
#ifdef DEBUG
		DebugOutput(TEXT("PlayCommandThread: PlaySoundTrackIndex: Creating the sound buffer...\r\n"));
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
				DebugOutput(TEXT("Error calling CreateThread for StreamThread!\r\n"));
#endif
				goto end;
			}
#ifdef DEBUG
			else
			{
				DebugOutput(TEXT("StreamThread Created (0x%x)...\r\n"), dwThreadId);
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
		DebugOutput(TEXT("PlayCommandThread: PlaySoundTrackIndex Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\r\n"), GetLastError());
	}
#endif

	return hStreamThread;
}

// PlayCommandThread
DWORD WINAPI GDAudioDriver::PlayCommandThreadProc( LPVOID lpParameter )
{
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;
//	EnterCriticalSection( &g_csPlayCommandThread );
	
	unsigned long repeatIndex = 0, soundIndex = 0, repeatCount, soundStartIndex, soundEndIndex;
	GDDA_CONTEXT *gddaContext = gdda->GetCurrentContext();

#ifdef DEBUG
	DebugOutput(TEXT("PlayCommandThread: Start, context slot: #%d, repeat count: %d\r\n"), gdda->gddaContextIndex, gddaContext->playRepeatCount);
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
		DebugOutput(TEXT("PlayCommandThread: Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\r\n"), GetLastError());
	}
#endif

	// Playing the parsed SEGACD_PLAYTRACK
	while( !gddaContext->fExiting && repeatIndex < repeatCount )
	{

#ifdef DEBUG
		DebugOutput(TEXT("PlayCommandThread: Executing SEGACD_PLAYTRACK sequence, track start: #%d, track end: #%d, repeat: #%d\r\n"), soundStartIndex, soundEndIndex - 1, repeatCount);
#endif

		soundIndex = soundStartIndex;
		while( !gddaContext->fExiting && soundIndex < soundEndIndex )
		{

#ifdef DEBUG
			DebugOutput(TEXT("PlayCommandThread: Launching the play of the track #%d (%d of %d)...\r\n"), soundIndex, repeatIndex + 1, repeatCount);
#endif
			// Playing the requested track index
			HANDLE hStreamThread = gdda->PlaySoundTrackIndex( soundIndex );			
			if ( hStreamThread )
			{

#ifdef DEBUG
				DebugOutput(TEXT("PlayCommandThread: Waiting for StreamThread...\r\n"));
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
	DebugOutput(TEXT("PlayCommandThread: Sound sequence play finished!\r\nPlayCommandThread: Done!\r\n"));
#endif

//	LeaveCriticalSection( &g_csPlayCommandThread );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// GDAudioDriver Constructor/Destructor
////////////////////////////////////////////////////////////////////////////////

GDAudioDriver::GDAudioDriver()
{
#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver initializing...\r\n"));
#endif

	this->isInstanceDestroyed = false;
	this->isCleaningFinished = false;
	this->pds = NULL;
	this->gddaContextIndex = 0;	
	
	memset(this->gddaContextStorage, 0, sizeof(this->gddaContextStorage));
	for(int i = 0; i < MAX_GDDA_CONTEXT; i++)
	{
		this->gddaContextStorage[i].fCleaned = true;
	}

	InitializeCriticalSection( &g_csCleanerThread );
	InitializeCriticalSection( &g_csStreamLoaderThread );
	InitializeCriticalSection( &g_csPlayCommandThread );

#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver initializing is done!\r\n"));
#endif
}

GDAudioDriver::~GDAudioDriver()
{
#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver finalizing...\r\n"));
#endif

	// Stop the current sound (if needed)
	this->Stop();

	this->isInstanceDestroyed = true;
	this->CleanUp();

#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver finalizing is done!\r\n"));
#endif
}

GDDA_CONTEXT* GDAudioDriver::GetContext(int index)
{
	return &(this->gddaContextStorage[index]);
}

GDDA_CONTEXT* GDAudioDriver::GetCurrentContext()
{
	return this->GetContext( this->gddaContextIndex );
}

void GDAudioDriver::Initialize( LPDIRECTSOUND pds )
{
	this->pds = pds;
}

bool GDAudioDriver::IsReady()
{
	return ( this->pds != NULL );
}

void GDAudioDriver::CleanUp()
{
#ifdef DEBUG
	DebugOutput(TEXT("Command Start: Cleanup\r\n"));
#endif

	if ( this->hGarbageCollectorThread != NULL ) 
	{
		if ( this->isCleaningFinished )
		{
#ifdef DEBUG
			DebugOutput(TEXT("CleanerThread was successfully finished, closing handle...\r\n"));
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
			DebugOutput(TEXT("Cleanup command is still running...\r\n"));
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
	DebugOutput(TEXT("Command End: Cleanup\r\n"));
#endif
}

bool GDAudioDriver::ChangePlayingStatus( bool allowPlaying )
{

#ifdef DEBUG
	DebugOutput(TEXT("ChangePlayingStatus request: %s\r\n"), allowPlaying ? TEXT("RESUME") : TEXT("PAUSE"));
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
				DebugOutput(TEXT("ChangePlayingStatus was IGNORED!\r\n"));
			}
#endif			
		}

		ReleaseMutex( hIOMutex );
	}

	// Send the Sound Resume event!
	if ( doAction && allowPlaying && gddaContext->hSoundResumeEvent )
	{
#ifdef DEBUG
		DebugOutput(TEXT("Sound Resume Event sent!\r\n"));
#endif
		SetEvent( gddaContext->hSoundResumeEvent );
	}

	return doAction;
}

bool GDAudioDriver::Pause()
{
	this->ChangePlayingStatus(false);
	return this->IsPaused();
}

void GDAudioDriver::Resume()
{
	this->ChangePlayingStatus(true);
}

void GDAudioDriver::Reset()
{
#ifdef DEBUG
	DebugOutput(TEXT("Reset called on gdda context #%d\r\n"), this->gddaContextIndex);
#endif

	GDDA_CONTEXT *gddaContext = this->GetCurrentContext();
	gddaContext->hSoundNotifyEvent = NULL;
	gddaContext->hSoundResumeEvent = NULL;
	gddaContext->hPlayCommandThread = NULL;
	gddaContext->hSoundFile = NULL;	
	gddaContext->pdsbBackground = NULL;
	gddaContext->fDonePlaying = false;
	gddaContext->fStarted = false;
	gddaContext->fPlayBackgroundSound = false;	
	gddaContext->fExiting = false;
	gddaContext->fPaused = false;
	gddaContext->errLast = 0;
}

void GDAudioDriver::Play( SEGACD_PLAYTRACK playtrack )
{
#ifdef DEBUG
	DebugOutput(TEXT("Command Start: Play\r\n"));
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
	this->Reset();

#ifdef DEBUG
	DebugOutput(TEXT("  Context slot: #%d...\r\n"), this->gddaContextIndex);
#endif

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
	DebugOutput(TEXT("Command End: Play\r\n"));
#endif
}

void GDAudioDriver::Stop()
{
#ifdef DEBUG
	DebugOutput(TEXT("Command Start: Stop\r\n"));
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
			DebugOutput(TEXT("Performing stop command...\n"));
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
		DebugOutput(TEXT("Stop Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\r\n"), GetLastError());
	}

	DebugOutput(TEXT("Command End: Stop\r\n"));
#endif
}

bool GDAudioDriver::IsPaused()
{
	bool value = false;
	
	HANDLE hIOMutex = CreateMutex (NULL, FALSE, NULL);
	if ( hIOMutex )
	{
		WaitForSingleObject( hIOMutex, INFINITE );
		GDDA_CONTEXT *gddaContext = this->GetCurrentContext();
		value = ( gddaContext->fPaused && !gddaContext->fExiting && !gddaContext->fDonePlaying );

#ifdef DEBUG
		DebugOutput(TEXT("IsPaused: %s\n"), value ? TEXT("YES") : TEXT("NO"));
#endif

		ReleaseMutex( hIOMutex);
	}
#ifdef DEBUG
	else
	{
		DebugOutput(TEXT("IsPaused Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\r\n"), GetLastError());
	}
#endif

	return value;
}

bool GDAudioDriver::IsPlaying()
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
		DebugOutput(TEXT("IsPaused Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\r\n"), GetLastError());
	}
#endif
	return value;
}

VOLUME_CONTROL GDAudioDriver::GetVolume()
{
	VOLUME_CONTROL volume;

	volume.PortVolume[0] = 0;
	volume.PortVolume[1] = 0;

	return volume;
}

void GDAudioDriver::SetVolume( VOLUME_CONTROL volume )
{
	UCHAR ucLeftVolume  = volume.PortVolume[0];
	UCHAR ucRightVolume = volume.PortVolume[1];

	GDDA_CONTEXT *gddaContext = this->GetCurrentContext();

//	gddaContext->pdsbBackground->SetPan( xxx );

//	GetPan
}

BOOL GDAudioDriver::CheckError(TCHAR *tszErr)
{
	GDDA_CONTEXT	*gddaContext = this->GetCurrentContext();

#ifdef DEBUG
    if (gddaContext->errLast != 0)
    {
        DebugOutput(TEXT("CheckError: %s failed (Error # = 0x%08x).\r\n"), tszErr, gddaContext->errLast);        
    }	
#endif

    return (gddaContext->errLast != 0);
}

IDirectSoundBuffer * GDAudioDriver::CreateSoundBuffer(int nSamplesPerSec, WORD wBitsPerSample, DWORD dwBufferSize)
{
    IDirectSoundBuffer	*pdsb			= NULL;
    DSBUFFERDESC		dsbd			= {0};
    WAVEFORMATEX		waveformatex	= {0};
	GDDA_CONTEXT		*gddaContext	= this->GetCurrentContext();

    // Set up the Wave format description
    waveformatex.wFormatTag      = WAVE_FORMAT_PCM;
    waveformatex.nChannels       = 1;
    waveformatex.nSamplesPerSec  = nSamplesPerSec;
    waveformatex.wBitsPerSample  = wBitsPerSample;
    waveformatex.nBlockAlign     = (waveformatex.nChannels * waveformatex.wBitsPerSample) / 8;
    waveformatex.nAvgBytesPerSec = waveformatex.nSamplesPerSec * waveformatex.nBlockAlign;
    waveformatex.cbSize          = 0;
    dsbd.dwSize                  = sizeof(dsbd);
    dsbd.dwBufferBytes           = dwBufferSize;
    dsbd.dwFlags                 = DSBCAPS_CTRLDEFAULT | DSBCAPS_LOCSOFTWARE;
    dsbd.lpwfxFormat             = &waveformatex;

    gddaContext->errLast = pds->CreateSoundBuffer(&dsbd, &pdsb, NULL);
    if (CheckError(TEXT(DSDEBUG_CREATE_BUFFER)))
	{
        return NULL;
	}

    return pdsb;
}
