#include "gddadrv.hpp"
#include "error.hpp"

// StreamThread
DWORD WINAPI
GDAudioDriver::StreamThreadProc( LPVOID lpParameter ) 
{	
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;
	
	DWORD dwThreadId = GetCurrentThreadId();
	int currentContextIndex = gdda->gddaContextIndex;

#ifdef DEBUG
	DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Start!\n"), currentContextIndex, dwThreadId );
#endif
	
	DWORD fPlaying;
    BOOL  fSilence = FALSE;
    BYTE  *pbyBlock1;
    BYTE  *pbyBlock2;
    DWORD ibRead, ibWrite, cbToLock = BUFFERSIZE / 2, cbRead;
    DWORD nBytes1, nBytes2;
	
	GDDA_CONTEXT *gddaContext = gdda->GetCurrentContext();
		
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
#ifdef DEBUG
				DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Playing in progress...\n"), currentContextIndex, dwThreadId );
#endif				
                // Find out where we currently are playing, and where we last wrote
                gddaContext->errLast = gddaContext->pdsbBackground->GetCurrentPosition( &ibRead, &ibWrite );
                if ( gdda->CheckError(TEXT(DSDEBUG_GET_CURRENT_POSITION)) )
				{
                    return 0;
				}
#ifdef DEBUG
				else
				{
					DebugOutput( TEXT("StreamThread: (%d) [0x%08x] GetCurrentPosition (ibRead: %d, ibWrite: %d)\n"), currentContextIndex, dwThreadId, ibRead, ibWrite );
				}
#endif
        
                // Lock the next half of the sound buffer
                gddaContext->errLast = gddaContext->pdsbBackground->Lock( ibWrite, cbToLock, (void **)&pbyBlock1, &nBytes1, (void **)&pbyBlock2, &nBytes2, 0 );
                if ( gdda->CheckError(TEXT(DSDEBUG_LOCK)) )
				{
                    return 0;
				}
#ifdef DEBUG
				else
				{
					DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Lock (ibWrite: %d, cbToLock: %d, nBytes1: %d, nBytes2: %d)\n"), 
						currentContextIndex, dwThreadId, ibWrite, cbToLock, nBytes1, nBytes2);
				}
#endif
                // fSilence is true if we hit the end of the sound file on the
                // last pass through here.  In that case, fill both blocks with "silence"
                // and stop the sound from playing.
                if ( fSilence )
                {
#ifdef DEBUG
					DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Silence is needed...\n"), currentContextIndex, dwThreadId);			
#endif				
                    memset(pbyBlock1, SILENCE_BYTE, nBytes1);
                    memset(pbyBlock2, SILENCE_BYTE, nBytes2);

                    // After filling it, stop playing
                    gddaContext->errLast = gddaContext->pdsbBackground->Stop();
                }
                else
                {
                    // Read the next chunk of bits from the sound file
                    ReadFile( gddaContext->hSoundFile, pbyBlock1, nBytes1, &cbRead, NULL );
#ifdef DEBUG
					DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Reading next chunk for Block 1 (nBytes1: %d, cbRead: %1)...\n"), currentContextIndex, dwThreadId, nBytes1, cbRead);			
#endif
                    if ( nBytes1 != cbRead )
                    {
                        // The file has less data than the size of the first block, so fill the
                        // remainder with silence - also fill the second block with silence
                        memset(pbyBlock1 + cbRead, SILENCE_BYTE, nBytes1 - cbRead);
                        memset(pbyBlock2, SILENCE_BYTE, nBytes2);

                        // Next time through, just play silence.
                        fSilence = TRUE;
#ifdef DEBUG
						DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Silence will be necessary next time (from block 1)...\n"), currentContextIndex, dwThreadId);			
#endif						
                    }
                    else
                    {
                        // If there is a second block, then read more of the sound file into that block.
                        if ( nBytes2 )
                        {
                            ReadFile(gddaContext->hSoundFile, pbyBlock2, nBytes2, &cbRead, NULL);
#ifdef DEBUG
							DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Reading chunk for block 2 (nBytes2: %d, cbRead: %1)...\n"), currentContextIndex, dwThreadId, nBytes2, cbRead);			
#endif
                            if (nBytes2 != cbRead)
                            {
                                // The file has less data than the size of the second block, so
                                // fill the remainder with silence.
                                memset(pbyBlock2 + cbRead, SILENCE_BYTE, nBytes2 - cbRead);

                                // next time through, just play silence.
                                fSilence = TRUE;
#ifdef DEBUG
								DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Silence will be necessary next time (from block 2)...\n"), currentContextIndex, dwThreadId);
#endif
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
#ifdef DEBUG
				else
				{
					DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Unlock (nBytes1: %d, nBytes2: %d)\n"), currentContextIndex, dwThreadId, nBytes1, nBytes2);
				}
#endif				
            }
            else
            {
#ifdef DEBUG
				DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Playing is NOT in progress!\n"), currentContextIndex, dwThreadId );
#endif					
				// Test if we are still playing the sound
				if ( gddaContext->fPlayBackgroundSound )
				{
					// Break out of this while() loop and exit the thread.
					break;
				}
            }
        }
#ifdef DEBUG
		else
		{
			DebugOutput( TEXT("StreamThread: (%d) [0x%08x] PlayBackgroundSound is false... nothing to play!\n"), currentContextIndex, dwThreadId );
		}
#endif			

    } // while

	// If we are playing the sound, then we completed it.
#ifdef DEBUG
	DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Stopping playback...\n"), currentContextIndex, dwThreadId );
#endif
	if ( gddaContext->pdsbBackground )
	{
		gddaContext->pdsbBackground->Stop();
	}

	// This flag tell us if the thread is terminated as well.
	gddaContext->fDonePlaying = true;

#ifdef DEBUG
	DebugOutput( TEXT("StreamThread: (%d) [0x%08x] Done!\n"), currentContextIndex, dwThreadId );
#endif

//	LeaveCriticalSection( &g_csStreamLoaderThread );
	return 0;
}
