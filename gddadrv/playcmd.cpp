#include "gddadrv.hpp"
#include "error.hpp"

// PlayCommandThread
DWORD WINAPI
GDAudioDriver::PlayCommandThreadProc( LPVOID lpParameter )
{
	GDAudioDriver *gdda = (GDAudioDriver*) lpParameter;
	int currentContextIndex = gdda->gddaContextIndex;
	unsigned long repeatIndex = 0, soundIndex = 0, repeatCount, soundStartIndex, soundEndIndex;
	GDDA_CONTEXT *gddaContext = gdda->GetCurrentContext();

#ifdef DEBUG
	DebugOutput(TEXT("[%d] PlayCommandThread: Start, context slot: #%d, repeat count: %d\r\n"), currentContextIndex, gdda->gddaContextIndex, gddaContext->playRepeatCount);
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
		DebugOutput(TEXT("[%d] PlayCommandThread: Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\r\n"), currentContextIndex, GetLastError());
	}
#endif

	// Playing the parsed SEGACD_PLAYTRACK
	while( !gddaContext->fExiting && repeatIndex < repeatCount )
	{

#ifdef DEBUG
		DebugOutput(TEXT("[%d] PlayCommandThread: Executing SEGACD_PLAYTRACK sequence, track start: #%d, track end: #%d, repeat: #%d\r\n"), currentContextIndex, soundStartIndex, soundEndIndex, repeatCount);
#endif

		soundIndex = soundStartIndex;
		while( !gddaContext->fExiting && soundIndex < soundEndIndex )
		{

#ifdef DEBUG
			DebugOutput(TEXT("[%d] PlayCommandThread: Launching the play of the track #%d (%d of %d)...\r\n"), currentContextIndex, soundIndex, repeatIndex, repeatCount);
#endif
			// Playing the requested track index
			HANDLE hStreamThread = gdda->PlaySoundTrackIndex( soundIndex );			
			if ( hStreamThread )
			{

#ifdef DEBUG
				DebugOutput(TEXT("[%d] PlayCommandThread: Waiting for StreamThread...\r\n"), currentContextIndex);
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
	DebugOutput(TEXT("[%d] PlayCommandThread: Sound sequence play finished!\r\n"), currentContextIndex);
	DebugOutput(TEXT("[%d] PlayCommandThread: Done!\r\n"), currentContextIndex);
#endif

	return 0;
}
