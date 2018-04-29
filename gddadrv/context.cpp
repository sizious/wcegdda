#include "gddadrv.hpp"

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

GDDA_CONTEXT *
GDAudioDriver::SetContext( SEGACD_PLAYTRACK playtrack )
{
	GDDA_CONTEXT * gddaContext;

	// Select an empty and ready-to-use slot
	do
	{
		this->gddaContextIndex++;
		if ( this->gddaContextIndex >= MAX_GDDA_CONTEXT )
		{
			this->gddaContextIndex = 0;
		}
		gddaContext = this->GetCurrentContext();
	}
	while( !gddaContext->fCleaned );

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

#ifdef DEBUG
	DebugOutput( TEXT("[%d] Context slot is now chosen!\n"), this->gddaContextIndex );
#endif

	return gddaContext;
}
