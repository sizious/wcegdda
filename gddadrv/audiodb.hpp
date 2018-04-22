#ifndef __ARRAY_HPP__
#define __ARRAY_HPP__

#include <windows.h>
#include <dsound.h>
#include "utils.hpp"

#define AUDIO_TRACK_DATABASE_INITIAL_SIZE 3

// Arbitrary nice large number (must be divisible by 2)
#define BUFFERSIZE 262144

typedef struct _DIRECTSOUND_BUFFER {
	int nSamplesPerSec;
	WORD wBitsPerSample;
	IDirectSoundBuffer * pSoundBuffer;
} DIRECTSOUND_BUFFER, *PDIRECTSOUND_BUFFER;

typedef struct _AUDIO_TRACK {
	int nTrackNumber;
	int nSamplesPerSec;
	WORD wBitsPerSample;
	DWORD dwSoundDataOffset;
	HANDLE hTrackFile;
} AUDIO_TRACK, *PAUDIO_TRACK;

typedef struct _AUDIO_TRACK_CONTEXT {
	HANDLE hTrackFile;
	IDirectSoundBuffer * pSoundBuffer;
	DWORD dwSoundDataOffset;
} AUDIO_TRACK_CONTEXT, *PAUDIO_TRACK_CONTEXT;

class GDAudioTrackDatabase
{
private:
	bool _isAudioTracksIndexed;	
	int * hashtable;
	int maxTrackNumber;

	DIRECTSOUND_BUFFER ** _soundBufferList;
	size_t _soundBufferListUsed;
	size_t _soundBufferListSize;
	bool _isSoundBuffersCreated;
	
	AUDIO_TRACK ** _audioTrackList;	
	size_t _audioTrackListUsed;
	size_t _audioTrackListSize;
	
	LPDIRECTSOUND pds;

	TCHAR szDirectory[ MAX_PATH ];
	
	void ClearSoundBuffers();
	void InitializeSoundBuffers();
	IDirectSoundBuffer * CreateSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample, DWORD dwBufferSize );
	void ClearAudioTrackIndexes();
	void CreateAudioTrackIndexes();

	void CreateSoundBuffers();
	void RegisterSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample );
	int FindSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample );

	HANDLE GetTrackFileHandle( const int audioTrackIndex );
	IDirectSoundBuffer * GetTrackSoundBuffer( const int audioTrackIndex );

public:
	GDAudioTrackDatabase();
	~GDAudioTrackDatabase();
	
	size_t Count();
	void Clear();
	int FindAudioTrack( const int trackNumber );
	PAUDIO_TRACK GetItems( const int itemIndex );

	bool GetAudioTrackContext( const int trackNumber, AUDIO_TRACK_CONTEXT * audioTrackContext );
	
	void Initialize( LPDIRECTSOUND pds );

	TCHAR * GetSourceDirectory();
	void SetSourceDirectory( TCHAR * szSourceDirectory );

	void Register( TCHAR * szWaveFileName );
};

#endif /* __ARRAY_HPP__ */