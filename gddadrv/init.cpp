#include "gddadrv.hpp"
#include "error.hpp"
#include "utils.hpp"

GDAudioDriver::GDAudioDriver()
{
#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver initializing...\n"));
#endif

	this->isInstanceDestroyed = false;
	this->isCleaningFinished = false;
	this->pds = NULL;
	this->gddaContextIndex = 0;	
	
	memset( this->gddaContextStorage, 0, sizeof(this->gddaContextStorage) );
	for(int i = 0; i < MAX_GDDA_CONTEXT; i++)
	{
		this->gddaContextStorage[i].fCleaned = true;
		this->gddaContextStorage[i].pdsbBackground = NULL;
		this->gddaContextStorage[i].hPlayCommandThread = NULL;
		this->gddaContextStorage[i].hSoundFile = NULL;
		this->gddaContextStorage[i].hSoundNotifyEvent = NULL;
		this->gddaContextStorage[i].hSoundResumeEvent = NULL;		
	}

	InitializeCriticalSection( &csThread );

	this->DiscoverTrackFiles();

#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver initializing is done!\n"));
#endif
}

GDAudioDriver::~GDAudioDriver()
{
#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver finalizing...\n"));
#endif

	// Stop the current sound (if needed)
	this->Stop();

	this->isInstanceDestroyed = true;
	this->CleanUp();

	// Clear all thread
	for(int i = 0; i < MAX_GDDA_CONTEXT; i++)
	{
		GDDA_CONTEXT currentContext = this->gddaContextStorage[ i ];
		this->CleanThread( currentContext.hStreamThread );
		this->CleanThread( currentContext.hPlayCommandThread );
	}
	this->CleanThread( this->hCleanerThread );

#ifdef DEBUG
	DebugOutput(TEXT("GDAudioDriver finalizing is done!\n"));
#endif
}

void
GDAudioDriver::Initialize( LPDIRECTSOUND pds )
{
	this->pds = pds;
	this->_audiomgr.Initialize( pds );
}

bool 
GDAudioDriver::DiscoverTrackFiles()
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];	
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0xDEADBEEF;
	
	_stprintf( szDir, TEXT("\\CD-ROM\\GDDA\\TRACK??.WAV") );
	this->_audiomgr.SetSourceDirectory( TEXT("\\CD-ROM\\GDDA\\") );

#ifdef DEBUG
	_stprintf( szDir, TEXT("\\PC\\Applications\\GDDA\\TRACK??.WAV") );
	this->_audiomgr.SetSourceDirectory( TEXT("\\PC\\Applications\\GDDA\\") );
#endif

#ifdef DEBUG
	DebugOutput( TEXT("Discovering tracks...\n") );
#endif

	hFind = FindFirstFile( szDir, &ffd );

	if (hFind != INVALID_HANDLE_VALUE) 
	{
		do
		{
			if ( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{				
				this->_audiomgr.Register( ffd.cFileName );
#ifdef DEBUG
				filesize.LowPart = ffd.nFileSizeLow;
				filesize.HighPart = ffd.nFileSizeHigh;								
				int trackNumber = ExtractTrackNumberFromFileName( ffd.cFileName );
				DebugOutput(TEXT("  %s, size: %ld KB"), ffd.cFileName, filesize.QuadPart / 1024);
				DebugOutput(TEXT(", trackNumber: %d\n"), trackNumber);
#endif			
			}
		}
		while ( FindNextFile( hFind, &ffd ) != 0 );

		dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES) 
		{
			// There was an other error...
			return false;
		}

		FindClose(hFind);
	}

#if DEBUG
	DebugOutput( TEXT("Registered tracks...\n") );
	for(size_t i = 0; i < this->_audiomgr.Count(); i++)
	{
		DebugOutput( TEXT("  trackNumber: %d\n"), this->_audiomgr.GetItems( i )->nTrackNumber );
	}

#endif

	return (dwError == 0);
}
