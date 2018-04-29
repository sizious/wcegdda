#ifndef __ARRAY_HPP__
#define __ARRAY_HPP__

#include <windows.h>
#include <dsound.h>
#include "utils.hpp"

#define AUDIO_TRACK_DATABASE_INITIAL_SIZE 3

// Arbitrary nice large number (must be divisible by 2)
#define BUFFERSIZE 262144

typedef struct _DIRECTSOUND_BUFFER_CONTEXT {
	int nSamplesPerSec;
	WORD wBitsPerSample;
	IDirectSoundBuffer * pSoundBuffer;
	HANDLE hEventNotify;
} DIRECTSOUND_BUFFER_CONTEXT, *PDIRECTSOUND_BUFFER_CONTEXT;

typedef struct _AUDIO_TRACK {
	int nTrackNumber;
	int nSamplesPerSec;
	WORD wBitsPerSample;
	DWORD dwSoundDataOffset;
	HANDLE hTrackFile;
} AUDIO_TRACK, *PAUDIO_TRACK;

typedef struct _AUDIO_TRACK_CONTEXT {
	int nTrackNumber;
	HANDLE hTrackFile;
	DWORD dwSoundDataOffset;
	HANDLE hEventNotify;
	IDirectSoundBuffer * pSoundBuffer;	
} AUDIO_TRACK_CONTEXT, *PAUDIO_TRACK_CONTEXT;

class GDAudioTrackManager
{
private:
	bool _isAudioTracksIndexed;	
	int * hashtable;
	int maxTrackNumber;

	DIRECTSOUND_BUFFER_CONTEXT ** _soundBufferList;
	size_t _soundBufferListUsed;
	size_t _soundBufferListSize;
	bool _isSoundBuffersCreated;
	
	AUDIO_TRACK ** _audioTrackList;	
	size_t _audioTrackListUsed;
	size_t _audioTrackListSize;
	
	LPDIRECTSOUND pds;

	TCHAR szDirectory[ MAX_PATH ];
	
	void ClearSoundBuffers( bool isObjectDestroying );
	void InitializeSoundBuffers();
	IDirectSoundBuffer * CreateSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample, DWORD dwBufferSize );
	void ClearAudioTrackIndexes();
	void CreateAudioTrackIndexes();

	void CreateSoundBuffers();
	void RegisterSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample );
	int FindSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample );

	HANDLE GetTrackFileHandle( const int audioTrackIndex );
	DIRECTSOUND_BUFFER_CONTEXT * GetDirectSoundBufferContext( const int audioTrackIndex );

	int FindAudioTrack( const int trackNumber );

	bool PrepareForStreaming( IDirectSoundBuffer *pdsb, DWORD dwBufferSize, HANDLE *phEventNotify );

public:
	GDAudioTrackManager();
	~GDAudioTrackManager();
	
	size_t Count();
	void Clear();
	
	bool GetAudioTrackContext( const int trackNumber, AUDIO_TRACK_CONTEXT * audioTrackContext );


	
	void Initialize( LPDIRECTSOUND pds );

	TCHAR * GetSourceDirectory();
	void SetSourceDirectory( TCHAR * szSourceDirectory );

	void Register( TCHAR * szWaveFileName );

#ifdef DEBUG
	PAUDIO_TRACK GetItems( const int itemIndex );
#endif
};

#endif /* __ARRAY_HPP__ */
