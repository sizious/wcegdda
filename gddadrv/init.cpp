#include "gddadrv.hpp"
#include "error.hpp"

bool 
GDAudioDriver::DiscoverTrackFiles()
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];	
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0xDEADBEEF;
	TCHAR szTrackNumber[3];

	

#if SH4
	_stprintf( szDir, TEXT("\\CD-ROM\\GDDA\\TRACK??.WAV") );

#ifdef DEBUG
	_stprintf( szDir, TEXT("\\PC\\Applications\\GDDA\\TRACK??.WAV") );
#endif

#else
	_stprintf( szDir, TEXT("TRACK??.WAV") );
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
				filesize.LowPart = ffd.nFileSizeLow;
				filesize.HighPart = ffd.nFileSizeHigh;
				
				memcpy( szTrackNumber, &ffd.cFileName[5], 2 );
				szTrackNumber[2] = '\0';

#ifdef DEBUG
				DebugOutput(TEXT("  %s, size: %ld Bytes, number: %s\n"), ffd.cFileName, filesize.QuadPart, szTrackNumber);
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

   return (dwError == 0);
}
