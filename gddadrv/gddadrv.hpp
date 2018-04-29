#ifndef __GDDADRV_HPP__
#define __GDDADRV_HPP__
 
#include <tchar.h>
#include <dsound.h>
#include <ceddcdrm.h>
#include <segagdrm.h>
#include "audiomgr.hpp"

#define MAX_GDDA_CONTEXT 4

#define SILENCE_BYTE 0x80

typedef struct _GDDA_CONTEXT
{
//	int contextIndex;
	HANDLE hSoundFile;								// Handle of the background sound file to play
	IDirectSoundBuffer *pdsbBackground;				// The background sound buffer

	// Events
	HANDLE hSoundNotifyEvent;						// Notify Event for the background sound buffer  			
	HANDLE hSoundResumeEvent;						// Notify Event for the resume action

	volatile bool fStarted;							// true if the sound process is started
	volatile bool fPaused;							// true if the sound was paused
	volatile bool fPlayBackgroundSound;				// true if the background sound should be played
	volatile bool fDonePlaying;						// Set to true to indicate completion of the thread
	volatile bool fExiting;							// true if the thread must exits
	volatile bool fCleaned;							// true if the context was cleaned up
	int errLast;									// Error return code of last function

	// Thread objects
	HANDLE hStreamThread;							// Streaming Sound thread handle
	HANDLE hPlayCommandThread;						// Handle of the PlayCommandThread

	// Parsed SEGACD_PLAYTRACK struct
	unsigned long playSoundStartIndex;				// Track Start Index
	unsigned long playSoundEndIndex;				// Track End Index (max index)
	unsigned long playRepeatCount;					// Repeat Count

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
		
	volatile bool isCleaningFinished;
	CRITICAL_SECTION csThread;
	GDAudioTrackManager _audiomgr;

	HANDLE hCleanerThread;							// Garbage Collector (Cleaner) thread handle

	// Private Methods
	void Reset();
	bool ChangePlayingStatus( bool allowPlaying );
	void CleanUp();

	static DWORD WINAPI CleanerThreadProc( LPVOID lpParameter );
	static DWORD WINAPI StreamThreadProc( LPVOID lpParameter );
	static DWORD WINAPI PlayCommandThreadProc( LPVOID lpParameter );

	BOOL CheckError( TCHAR *tszErr );
	HANDLE PlaySoundTrackIndex( int playTrackIndex, DWORD dwThreadId );

	GDDA_CONTEXT * GetContext( int index );
	GDDA_CONTEXT * GetCurrentContext();
	GDDA_CONTEXT * SetContext( SEGACD_PLAYTRACK playtrack );

	bool DiscoverTrackFiles();
	void CleanThread( HANDLE hThread );
	bool IsThreadCreated( HANDLE hThread, DWORD dwThreadId );

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

#endif /* __GDDADRV_HPP__ */
