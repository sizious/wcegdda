#include "gddadrv.hpp"
#include "error.hpp"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Function:

    PrepareForStreaming

Description:

    Prepares the specified buffer for streaming by setting up events
    to notify the app when the buffer has reached the halfway point or
    the end point - at either of those, it starts filling in one side of
    the buffer while playing the other side of the buffer (assuming it's
    repeating).
    
Arguments:

    IDirectSoundBuffer *pdsb        - The sound buffer to prepare

    DWORD dwBufferSize;             - Size of the buffer

    HANDLE *phEventNotify           - This event set when halfway or at end

    HANDLE *phEventDone             - This event set when sound is done playing

Return Value:

    TRUE on success, FALSE on failure.

-------------------------------------------------------------------*/
BOOL
GDAudioDriver::PrepareForStreaming( IDirectSoundBuffer *pdsb, DWORD dwBufferSize, HANDLE *phEventNotify )
{
	int currentContextIndex = this->gddaContextIndex;

#ifdef DEBUG
	DebugOutput(TEXT("[%d] PlayCommandThread: PlaySoundTrackIndex: PrepareForStreaming...\n"), currentContextIndex);
#endif

    IDirectSoundNotify	*pdsn;
	DSBPOSITIONNOTIFY	rgdsbpn[3];
	GDDA_CONTEXT		*gddaContext = this->GetCurrentContext();
	
    // Get a DirectSoundNotify interface so that we can set the events.
    gddaContext->errLast = pdsb->QueryInterface( IID_IDirectSoundNotify, (void **)&pdsn );
    if ( CheckError(TEXT(DSDEBUG_QUERY_INTERFACE)) )
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
    if ( CheckError(TEXT(DSDEBUG_SET_NOTIFICATION_POSITIONS)) )
	{
        return FALSE;
	}

    // No longer need the DirectSoundNotify interface, so release it
    pdsn->Release();

    return TRUE;
}
