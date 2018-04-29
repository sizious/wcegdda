#include "gddadrv.hpp"

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

		GDDA_CONTEXT * gddaContext = this->GetCurrentContext();

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

		ReleaseMutex( hIOMutex );
	}
#ifdef DEBUG
	else
	{
		DebugOutput(TEXT("[%d] Stop Mutex: Failed to get a valid mutex handle! (Error # = 0x%08x).\n"), currentContextIndex, GetLastError());
	}

	DebugOutput(TEXT("[%d] Command End: Stop\n"), currentContextIndex);
#endif
}

void
GDAudioDriver::CleanUp()
{
	int currentContextIndex = this->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command Start: Cleanup\n"), currentContextIndex);
#endif

	if ( this->hCleanerThread != NULL ) 
	{
		if ( this->isCleaningFinished )
		{
#ifdef DEBUG
			DebugOutput(TEXT("[%d] CleanerThread was successfully finished, closing handle...\n"), currentContextIndex);
#endif
			if( hCleanerThread )
			{
				CloseHandle( hCleanerThread );
				hCleanerThread = NULL;
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
	this->hCleanerThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) CleanerThreadProc, this, NULL, &dwThreadId );				

#ifdef DEBUG
	DebugOutput(TEXT("[%d] Command End: Cleanup\n"), currentContextIndex);
#endif
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
