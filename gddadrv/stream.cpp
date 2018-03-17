#include "gddadrv.hpp"
#include "error.hpp"

// StreamThread
DWORD WINAPI
GDAudioDriver::StreamThreadProc( LPVOID lpParameter ) 
{
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;
	int currentContextIndex = gdda->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] StreamThread: Start!\n"), currentContextIndex);
#endif
	
	DWORD fPlaying;
    BYTE  bySilence = 0x80;
    BOOL  fSilence = FALSE;
    BYTE  *pbyBlock1;
    BYTE  *pbyBlock2;
    DWORD ibRead, ibWrite, cbToLock = BUFFERSIZE/2, cbRead;
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
				DebugOutput(TEXT("[%d] StreamThread is waiting to resume the process...\n"), currentContextIndex);
#endif
				if ( gddaContext->pdsbBackground )
				{
					gddaContext->pdsbBackground->Stop();
					WaitForSingleObject( gddaContext->hSoundResumeEvent, INFINITE );
				}
#ifdef DEBUG			
				DebugOutput(TEXT("[%d] StreamThread was waked up!\n"), currentContextIndex);
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
	DebugOutput(TEXT("[%d] StreamThread: Done!\n"), currentContextIndex);	
#endif

//	LeaveCriticalSection( &g_csStreamLoaderThread );
	return 0;
}
