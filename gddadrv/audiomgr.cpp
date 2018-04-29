#include "audiomgr.hpp"

// Thanks to casablanca
// https://stackoverflow.com/a/3536261/3726096

GDAudioTrackManager::GDAudioTrackManager()
{
	this->_audioTrackListUsed = 0;
	this->_audioTrackList = (AUDIO_TRACK **) malloc( AUDIO_TRACK_DATABASE_INITIAL_SIZE * sizeof( AUDIO_TRACK * ) );	
	this->_audioTrackListSize = AUDIO_TRACK_DATABASE_INITIAL_SIZE;

	this->_soundBufferListUsed = 0;
	this->_soundBufferList = (DIRECTSOUND_BUFFER_CONTEXT **) malloc( AUDIO_TRACK_DATABASE_INITIAL_SIZE * sizeof( DIRECTSOUND_BUFFER_CONTEXT *) );
	this->_soundBufferListSize = AUDIO_TRACK_DATABASE_INITIAL_SIZE;
	this->_isSoundBuffersCreated = false;

	this->_isAudioTracksIndexed = false;
	this->hashtable = NULL;

	this->maxTrackNumber = 0;
}

GDAudioTrackManager::~GDAudioTrackManager()
{
	this->ClearSoundBuffers( true );
	
	this->Clear();
	
	this->ClearAudioTrackIndexes();

	// Cleaning Audio Track List
	free( this->_audioTrackList );
	this->_audioTrackList = NULL;
	this->_audioTrackListUsed = 0;
	this->_audioTrackListSize = AUDIO_TRACK_DATABASE_INITIAL_SIZE;

	// Cleaning Sound Buffer List
	free( this->_soundBufferList );
	this->_soundBufferList = NULL;
	this->_soundBufferListUsed = 0;
	this->_soundBufferListSize = AUDIO_TRACK_DATABASE_INITIAL_SIZE;
}

void
GDAudioTrackManager::Clear()
{
	for(size_t i = 0; i < this->Count(); i++)
	{
		HANDLE hTrackFile = this->_audioTrackList[ i ]->hTrackFile;
		if ( hTrackFile )
		{
			CloseHandle( hTrackFile );		
		}
		free( this->_audioTrackList[ i ] );
		this->_audioTrackList[ i ] = NULL;
	}
	this->_audioTrackListUsed = 0;	
	this->maxTrackNumber = 0;
}

void
GDAudioTrackManager::ClearSoundBuffers( bool isObjectDestroying )
{
	for( size_t i = 0; i < this->_soundBufferListUsed; i++ )
	{
		if ( this->_soundBufferList[ i ] )
		{
			IDirectSoundBuffer * pSoundBuffer = this->_soundBufferList[ i ]->pSoundBuffer;
			if ( pSoundBuffer && !isObjectDestroying ) 
			{
				// This is implemented but seems to be unstable on Windows CE for Dreamcast...
				// So when this method is executed from the destructor, an error is thrown.
				pSoundBuffer->Release();			
			}
			this->_soundBufferList[ i ] = NULL;
		}
	}
	this->_soundBufferListUsed = 0;
	this->_isSoundBuffersCreated = false;
}

void 
GDAudioTrackManager::Register( TCHAR * szWaveFileName )
{
	this->_isAudioTracksIndexed = false;

	if ( this->_audioTrackListUsed == this->_audioTrackListSize ) 
	{
		this->_audioTrackListSize *= 2;
		this->_audioTrackList = (AUDIO_TRACK **) realloc( this->_audioTrackList, this->_audioTrackListSize * sizeof( AUDIO_TRACK * ) );		
	}

	PAUDIO_TRACK audioTrackElement = (AUDIO_TRACK *) malloc( sizeof( AUDIO_TRACK ) );

	// Handle the Track Number
	int trackNumber = ExtractTrackNumberFromFileName( szWaveFileName );

	audioTrackElement->nTrackNumber = trackNumber;
	if (trackNumber > this->maxTrackNumber)
	{
		this->maxTrackNumber = trackNumber;
	}

	// Compute the full wave file path
	TCHAR szWaveFullPathName[ MAX_PATH ] = TEXT("\0");
	StrCat( szWaveFullPathName, this->GetSourceDirectory() );
	StrCat( szWaveFullPathName, szWaveFileName );

	// Open the file handle and store it
	audioTrackElement->hTrackFile = CreateFile( szWaveFullPathName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );

	if ( audioTrackElement->hTrackFile == INVALID_HANDLE_VALUE )
	{		
#ifdef DEBUG
		DebugOutput( TEXT("Sorry, %s was not properly opened...\n"), szWaveFullPathName );
#endif
	}
	else
	{
		// Read the first 256 bytes to get file header
		ULONG cbRead;
		BYTE byTemp[256], *pbyData;
		ReadFile( audioTrackElement->hTrackFile, byTemp, sizeof(byTemp), &cbRead, NULL );		

		// Parse the header information to get information.
		DWORD dwSize;
		WAVEFORMATEX * pwfx;
		ParseWaveFile( (void*)byTemp, &pwfx, &pbyData, &dwSize );

		// Store the important information about the wave file.
		// This will be used with the CreateSoundBuffer function.
		audioTrackElement->nSamplesPerSec = pwfx->nSamplesPerSec;
		audioTrackElement->wBitsPerSample = pwfx->wBitsPerSample;

		// Store the location of wave data in the file
		audioTrackElement->dwSoundDataOffset = (DWORD)(pbyData - byTemp);

		this->RegisterSoundBuffer( pwfx->nSamplesPerSec, pwfx->wBitsPerSample );

		// Add the object to the list
		this->_audioTrackList[this->_audioTrackListUsed++] = audioTrackElement;
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Function:

    CreateSoundBuffer

Description:

    Creates an empty DirectSoundBuffer
    
Arguments:

    int     nSamplesPerSec      - Recording resolution (ex: 11025)

    WORD    wBitsPerSample      - Recording resoultion (ex: 16)

    DWORD   dwBufferSize        - length of recording time (ex: 16 * 1024)

Return Value:

    Pointer to the created buffer (NULL on failure)

-------------------------------------------------------------------*/
IDirectSoundBuffer *
GDAudioTrackManager::CreateSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample, DWORD dwBufferSize )
{
    IDirectSoundBuffer	*pdsb			= NULL;
    DSBUFFERDESC		dsbd			= {0};
    WAVEFORMATEX		waveformatex	= {0};

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

    int errorCode = this->pds->CreateSoundBuffer( &dsbd, &pdsb, NULL );
    if ( errorCode != 0 )
	{
        return NULL;
	}
	
    return pdsb;
}

DIRECTSOUND_BUFFER_CONTEXT *
GDAudioTrackManager::GetDirectSoundBufferContext( const int audioTrackIndex )
{
	DIRECTSOUND_BUFFER_CONTEXT * result = NULL;

	if ( audioTrackIndex != -1 )
	{
		PAUDIO_TRACK audioTrack = this->_audioTrackList[ audioTrackIndex ];
		int soundBufferIndex = this->FindSoundBuffer( audioTrack->nSamplesPerSec, audioTrack->wBitsPerSample );
		if ( soundBufferIndex != -1 )
		{			
			this->CreateSoundBuffers();
			result = this->_soundBufferList[ soundBufferIndex ];
		}
	}

	return result;
}

size_t
GDAudioTrackManager::Count()
{
	return this->_audioTrackListUsed;
}

int
GDAudioTrackManager::FindAudioTrack( const int trackNumber )
{
	int result = -1;
	
	if ( !this->_isAudioTracksIndexed )
	{
		this->CreateAudioTrackIndexes();
	}

	int listIndex = this->hashtable[ trackNumber ];
	if ( this->Count() > 0 && (size_t) listIndex < this->Count() )
	{
		result = listIndex;
	}

	return result;
}

HANDLE
GDAudioTrackManager::GetTrackFileHandle( const int audioTrackIndex )
{
	HANDLE hFileResult = NULL;
	
	if ( audioTrackIndex != -1 )
	{
		hFileResult = this->_audioTrackList[ audioTrackIndex ]->hTrackFile;
		if ( hFileResult && hFileResult != INVALID_HANDLE_VALUE )
		{
			SetFilePointer( hFileResult, 0, NULL, FILE_BEGIN );
		}
	}
	return hFileResult;
}

PAUDIO_TRACK
GDAudioTrackManager::GetItems( const int itemIndex )
{
	return this->_audioTrackList[ itemIndex ];
}

TCHAR * 
GDAudioTrackManager::GetSourceDirectory()
{
	return this->szDirectory;
}

void
GDAudioTrackManager::SetSourceDirectory( TCHAR * szSourceDirectory )
{
	memcpy( this->szDirectory, szSourceDirectory, _tcslen( szSourceDirectory ) * sizeof(TCHAR) );
}

void
GDAudioTrackManager::ClearAudioTrackIndexes()
{
	this->_isAudioTracksIndexed = false;
	if ( this->hashtable ) 
	{
		free( this->hashtable );
	}
	this->hashtable = NULL;
}

void
GDAudioTrackManager::CreateAudioTrackIndexes()
{
	this->ClearAudioTrackIndexes();
	this->hashtable = (int *) malloc( (this->maxTrackNumber + 1) * sizeof( int ) );
	
	for(size_t i = 0; i < this->Count(); i++)
	{
		int trackNumber = ( this->_audioTrackList[ i ]->nTrackNumber );
		this->hashtable[ trackNumber ] = i;
	}

	this->_isAudioTracksIndexed = true;
}

void
GDAudioTrackManager::RegisterSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample )
{
	if ( this->FindSoundBuffer( nSamplesPerSec, wBitsPerSample ) == -1 )
	{
		if ( this->_soundBufferListUsed == this->_soundBufferListSize ) 
		{
			this->_soundBufferListSize *= 2;
			this->_soundBufferList = (DIRECTSOUND_BUFFER_CONTEXT **) realloc( this->_soundBufferList, this->_soundBufferListSize * sizeof( DIRECTSOUND_BUFFER_CONTEXT * ) );		
		}

		PDIRECTSOUND_BUFFER_CONTEXT soundBufferElement = (DIRECTSOUND_BUFFER_CONTEXT *) malloc( sizeof( DIRECTSOUND_BUFFER_CONTEXT ) );
		
		soundBufferElement->nSamplesPerSec = nSamplesPerSec;
		soundBufferElement->wBitsPerSample = wBitsPerSample;
		soundBufferElement->pSoundBuffer = NULL;

		// Add the object to the list
		this->_soundBufferList[this->_soundBufferListUsed++] = soundBufferElement;
	}
}

int
GDAudioTrackManager::FindSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample )
{
	for(size_t i = 0; i < this->_soundBufferListUsed; i++)
	{
		PDIRECTSOUND_BUFFER_CONTEXT soundBuffer = this->_soundBufferList[ i ];
		if ( soundBuffer->nSamplesPerSec == nSamplesPerSec && soundBuffer->wBitsPerSample )
		{
			return i;
		}
	}

	return -1;
}

void
GDAudioTrackManager::Initialize( LPDIRECTSOUND pds )
{
	this->pds = pds;
}

void
GDAudioTrackManager::CreateSoundBuffers()
{
	if ( !this->_isSoundBuffersCreated )
	{

#ifdef DEBUG
		if ( !this->pds )
		{
			DebugOutput( TEXT("ERROR: GDAudioTrackManager::Initialize() WAS NOT CALLED!\n") );
			return;
		}
#endif

		for(size_t i = 0; i < this->_soundBufferListUsed; i++)
		{
			PDIRECTSOUND_BUFFER_CONTEXT soundBufferContext = this->_soundBufferList[ i ];
			if ( soundBufferContext->pSoundBuffer == NULL )
			{
				// Create the sound buffer
				IDirectSoundBuffer * pSoundBuffer = this->CreateSoundBuffer( soundBufferContext->nSamplesPerSec, soundBufferContext->wBitsPerSample, BUFFERSIZE );
				
				// Create the event
				this->PrepareForStreaming( pSoundBuffer, BUFFERSIZE, &soundBufferContext->hEventNotify );

				// Store the result...
				this->_soundBufferList[ i ]->pSoundBuffer = pSoundBuffer;
			}
		}
		this->_isSoundBuffersCreated = true;
	}
}

bool
GDAudioTrackManager::GetAudioTrackContext( const int trackNumber, AUDIO_TRACK_CONTEXT * audioTrackContext )
{	
	bool result = false;

	int audioTrackIndex = this->FindAudioTrack( trackNumber );
	if ( audioTrackIndex != -1 && audioTrackContext != NULL )
	{
		HANDLE hAudioTrackFile = this->GetTrackFileHandle( audioTrackIndex );
		DIRECTSOUND_BUFFER_CONTEXT * pDirectSoundBufferContext = this->GetDirectSoundBufferContext( audioTrackIndex );

		audioTrackContext->nTrackNumber = trackNumber;
		audioTrackContext->hTrackFile = hAudioTrackFile;	
		audioTrackContext->dwSoundDataOffset = this->_audioTrackList[ audioTrackIndex ]->dwSoundDataOffset;

		audioTrackContext->pSoundBuffer = pDirectSoundBufferContext->pSoundBuffer;
		audioTrackContext->hEventNotify = pDirectSoundBufferContext->hEventNotify;

		result = true;
	}
	else
	{
#ifdef DEBUG
		DebugOutput( TEXT("GetAudioTrackContext: Unable to find the track %d...\n"), trackNumber );
#endif
		audioTrackContext = NULL;
	}

	return result;
}

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

    true on success, false on failure.

-------------------------------------------------------------------*/
bool
GDAudioTrackManager::PrepareForStreaming( IDirectSoundBuffer *pdsb, DWORD dwBufferSize, HANDLE *phEventNotify )
{
	bool				result = false;
	IDirectSoundNotify	*pdsn;
	DSBPOSITIONNOTIFY	rgdsbpn[3];
	
    // Get a DirectSoundNotify interface so that we can set the events.
    int errLast = pdsb->QueryInterface( IID_IDirectSoundNotify, (void **)&pdsn );
    if ( errLast == 0 )
	{
		*phEventNotify = CreateEvent( NULL, FALSE, FALSE, NULL );

		rgdsbpn[0].hEventNotify = *phEventNotify;
		rgdsbpn[1].hEventNotify = *phEventNotify;
		rgdsbpn[2].hEventNotify = *phEventNotify;
		rgdsbpn[0].dwOffset     = dwBufferSize / 2;
		rgdsbpn[1].dwOffset     = dwBufferSize - 2;
		rgdsbpn[2].dwOffset     = DSBPN_OFFSETSTOP;

		errLast = pdsn->SetNotificationPositions( 3, rgdsbpn );
		if ( errLast == 0 )
		{
			// No longer need the DirectSoundNotify interface, so release it
			pdsn->Release();

			// Success...
			result = true;
		}
#ifdef DEBUG
		else
		{
			DebugOutput( TEXT("PrepareForStreaming: SetNotificationPositions failed (0x%x)\n"), errLast );
		}
#endif
	}
#ifdef DEBUG
	else
	{
		DebugOutput( TEXT("PrepareForStreaming: QueryInterface failed (0x%x)\n"), errLast );
	}
#endif

	return result;
}
