#include "gddadrv.hpp"
#include "error.hpp"
#include "utils.hpp"

bool 
GDAudioDriver::DiscoverTrackFiles()
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];	
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0xDEADBEEF;
	
	_stprintf( szDir, TEXT("\\CD-ROM\\GDDA\\TRACK??.WAV") );
	this->_audiodb.SetSourceDirectory( TEXT("\\CD-ROM\\GDDA\\") );

#ifdef DEBUG
	_stprintf( szDir, TEXT("\\PC\\Applications\\GDDA\\TRACK??.WAV") );
	this->_audiodb.SetSourceDirectory( TEXT("\\PC\\Applications\\GDDA\\") );
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
				this->_audiodb.Register( ffd.cFileName );
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
	for(size_t i = 0; i < this->_audiodb.Count(); i++)
	{
		DebugOutput( TEXT("  trackNumber: %d\n"), this->_audiodb.GetItems( i )->nTrackNumber );
	}

#endif

	return (dwError == 0);
}
