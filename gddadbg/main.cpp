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
#ifdef DEBUG
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
#endif
}

int
GetRandomNumber(int nMinimal, int nMaximal)
{
	return Random() % (nMaximal - nMinimal) + nMinimal;
}

extern "C" int APIENTRY 
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{	
#ifdef DEBUG
	DebugOutput(TEXT("\n\n++++++ GDDA Debug Start ++++++\n"));
#endif

	Initialize();	
	
	int nOperationsCount = GetRandomNumber(10, 20);

#ifdef DEBUG
		DebugOutput(TEXT("Operations count: %d..."), nOperationsCount);
#endif

	for(int i = 0; i < nOperationsCount; i++)
	{
		int nOperation = GetRandomNumber(1, 4);
		
		PrintCommand( i, nOperation );

		switch(nOperation)
		{
			case PLAY:
			{
				int nTrackNumber = GetRandomNumber(4, 8);
				int nRepeatTimes = GetRandomNumber(1, 4);
				PlayTrack( nTrackNumber, nTrackNumber, nRepeatTimes );
				break;
			}

			case STOP:
			{
				gdda.Stop();
				break;
			}

			case PAUSE:
			{
				gdda.Pause();
				break;
			}

			case RESUME:
			{
				gdda.Resume();
				break;
			}
		}

		int nWaitTime = GetRandomNumber(1, 120);
#ifdef DEBUG
		DebugOutput(TEXT("Waiting %d second(s)..."), nWaitTime);
#endif
		Sleep(1000 * nWaitTime);
	}

	Finalize();

#ifdef DEBUG
	DebugOutput(TEXT("\n\n++++++ GDDA Debug End ++++++\n\n\n"));
#endif

    return 0;
}
