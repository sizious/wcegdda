#ifndef __GDDADRV_HPP__
#define __GDDADRV_HPP__
 
#include <tchar.h>
#include <dsound.h>

#ifdef SH4

// For Sega Dreamcast

#include <ceddcdrm.h>
#include <segagdrm.h>

#else

// For Microsoft Windows

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

#include <iostream>

using namespace std;

typedef struct {
    DWORD dwStartTrack;
	DWORD dwEndTrack;
	DWORD dwRepeat;
} SEGACD_PLAYTRACK;

typedef struct _VOLUME_CONTROL {
    UCHAR PortVolume[4];
} VOLUME_CONTROL, *PVOLUME_CONTROL;

#endif /* SH4 */

// StreamThread private info

#define MAX_GDDA_CONTEXT 8

typedef struct _GDDA_CONTEXT
{
	HANDLE hSoundFile;								// Handle of the background sound file to play
	IDirectSoundBuffer *pdsbBackground;				// The background sound buffer
	HANDLE hSoundNotifyEvent;						// Notify Event for the background sound buffer  		
//	HANDLE hSoundDoneEvent;							// End-Of-Playing Event for the background sound buffer	
	
	HANDLE hSoundResumeEvent;						// Notify Event for the resume action
	HANDLE hPlayCommandThread;						// Handle of the PlayCommandThread
	volatile bool fStarted;							// true if the sound process is started
	volatile bool fPaused;							// true if the sound was paused
	volatile bool fPlayBackgroundSound;				// true if the background sound should be played
	volatile bool fDonePlaying;						// Set to true to indicate completion of the thread
	volatile bool fExiting;							// true if the thread must exits
	int errLast;									// Error return code of last function

	// Parsed SEGACD_PLAYTRACK struct
	unsigned long playSoundStartIndex;				// Track Start Index
	unsigned long playSoundEndIndex;				// Track End Index (max index)
	unsigned long playRepeatCount;					// Repeat Count

	volatile bool fCleaned;

	DWORD dwPreviousStartTrack;
	DWORD dwPreviousEndTrack;
} GDDA_CONTEXT, *PGDDA_CONTEXT;

// Sound Stream Driver class definition
class GDAudioDriver
{
private:
	// Attributes	
	LPDIRECTSOUND pds;													// The main DirectSound object
	GDDA_CONTEXT gddaContextStorage[ MAX_GDDA_CONTEXT ];
	int gddaContextIndex;
	bool isInstanceDestroyed;
	HANDLE hGarbageCollectorThread;
	volatile bool isCleaningFinished;
	CRITICAL_SECTION csThread;

	// Private Methods
	void Reset();
	bool ChangePlayingStatus( bool allowPlaying );
	void CleanUp();
	
	static DWORD WINAPI CleanerThreadProc( LPVOID lpParameter );
	static DWORD WINAPI StreamThreadProc( LPVOID lpParameter );
	static DWORD WINAPI PlayCommandThreadProc( LPVOID lpParameter );

	IDirectSoundBuffer * CreateSoundBuffer( int nSamplesPerSec, WORD wBitsPerSample, DWORD dwBufferSize );
	BOOL PrepareForStreaming( IDirectSoundBuffer *pdsb, DWORD dwBufferSize, HANDLE *phEventNotify );

	BOOL CheckError( TCHAR *tszErr );
	HANDLE PlaySoundTrackIndex( int playTrackIndex );
	GDDA_CONTEXT * GetContext( int index );
	GDDA_CONTEXT * GetCurrentContext();

public:
	GDAudioDriver();
	~GDAudioDriver();
	void Initialize( LPDIRECTSOUND pds );
	bool Pause();
	void Play( SEGACD_PLAYTRACK playtrack );
	bool IsPaused();
	bool IsPlaying();
	bool IsReady();
	void Resume();
	VOLUME_CONTROL GetVolume();
	void SetVolume( VOLUME_CONTROL volume );
	void Stop();
};

#ifdef DEBUG

// For Stream Thread methods
#define DSDEBUG_GET_STATUS					"IDirectSoundBuffer::GetStatus"
#define DSDEBUG_GET_CURRENT_POSITION		"IDirectSoundBuffer::GetCurrentPosition"
#define DSDEBUG_LOCK						"IDirectSoundBuffer::Lock"
#define DSDEBUG_UNLOCK						"IDirectSoundBuffer::Unlock"
#define DSDEBUG_QUERY_INTERFACE				"IDirectSoundBuffer::QueryInterface"
#define DSDEBUG_SET_NOTIFICATION_POSITIONS	"Set Notification Positions"
#define DSDEBUG_CREATE_BUFFER				"Create DirectSound Buffer"

#else

#define DSDEBUG_GET_STATUS					""
#define DSDEBUG_GET_CURRENT_POSITION		""
#define DSDEBUG_LOCK						""
#define DSDEBUG_UNLOCK						""
#define DSDEBUG_QUERY_INTERFACE				""
#define DSDEBUG_SET_NOTIFICATION_POSITIONS	""
#define DSDEBUG_CREATE_BUFFER				""

#endif

// Arbitrary nice large number (must be divisible by 2)
#define BUFFERSIZE 524288

#ifndef SH4

// For Microsoft Windows Only

// On Windows, the DSBCAPS_CTRLPOSITIONNOTIFY is VERY important.
// If not present, the IID_IDirectSoundNotify interface won't work.
#define DSBCAPS_CTRLDEFAULT					DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPOSITIONNOTIFY

#endif

#endif /* __GDDADRV_HPP__ */
