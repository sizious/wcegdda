#include <windows.h>
#include <dsound.h>
#include "gddadrv.hpp"
#include "error.hpp"

GDAudioDriver gdda;
LPDIRECTSOUND pds = NULL;

bool
Initialize()
{	
	HWND hwnd = NULL;
	bool result = false;

	if ( DirectSoundCreate( NULL, &pds, NULL ) == 0 )
	{
		pds->SetCooperativeLevel( hwnd, DSSCL_NORMAL );
		gdda.Initialize( pds );
		result = true;
	}   
	
	return result;
}

void
Finalize()
{
	if ( pds )
	{
		pds->Release();
		pds = NULL;
	}
}

void
PlayTrack(DWORD startTrack, DWORD endTrack, DWORD repeatCount)
{	
	SEGACD_PLAYTRACK playtrack;

	playtrack.dwStartTrack = startTrack;
	playtrack.dwEndTrack = endTrack;
	playtrack.dwRepeat = repeatCount;

	gdda.Play( playtrack );
}

#define PLAY	1
#define STOP	2
#define PAUSE	3
#define RESUME	4

void
PrintCommand( int nCommandNumber, int nCommand )
{
	TCHAR * tszCommand;
	switch(nCommand)
	{
		case PLAY:
			tszCommand = TEXT("PLAY");
			break;

		case STOP:
			tszCommand = TEXT("STOP");
			break;

		case PAUSE:
			tszCommand = TEXT("PAUSE");
			break;

		case RESUME:
			tszCommand = TEXT("RESUME");
			break;
	}
	DebugOutput(TEXT("\n\n>>> COMMAND %d: %s <<<\n"), nCommandNumber, tszCommand);
}

extern "C" int APIENTRY 
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{	
	DebugOutput(TEXT("\n\n++++++ GDDA Debug Start ++++++\n"));

	Initialize();	
	
	PrintCommand(1, PLAY);
	PlayTrack( 8, 8, 1 );
	Sleep(1000);

	PrintCommand(2, PLAY);
	PlayTrack( 4, 4, 1 );
	Sleep(2000);

	PrintCommand(3, PLAY);
	PlayTrack( 5, 5, 1 );
	Sleep(3000);

	PrintCommand(4, PLAY);
	PlayTrack( 6, 6, 2 );
	Sleep(10000);

	PrintCommand(5, PLAY);
	PlayTrack( 7, 7, 2 );
	Sleep(10000);

	PrintCommand(6, STOP);
	gdda.Stop();
	Sleep(5000);

	PrintCommand(7, PLAY);
	PlayTrack( 8, 8, 1 );
	Sleep(1000);

	PrintCommand(8, PAUSE);
	gdda.Pause();
	Sleep(1000);

	PrintCommand(9, STOP);
	gdda.Stop();
	Sleep(1000);

	PrintCommand(10, PLAY);
	PlayTrack( 4, 4, 1 );
	Sleep(2000);

	Finalize();

	DebugOutput(TEXT("\n\n++++++ GDDA Debug End ++++++\n\n\n"));

    return 0;
}
