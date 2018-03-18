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
PrintCommand( int nCommandNumber, int nCommandCount, int nCommand )
{
#ifdef DEBUG
	TCHAR * tszCommand;
	switch(nCommand)
	{	
		case STOP:
			tszCommand = TEXT("STOP");
			break;

		case PAUSE:
			tszCommand = TEXT("PAUSE");
			break;

		case RESUME:
			tszCommand = TEXT("RESUME");
			break;
		
		case PLAY:
		default:
			tszCommand = TEXT("PLAY");
			break;
	}

	DebugOutput(TEXT("\n\n>>> COMMAND %d of %d: %s <<<\n"), (nCommandNumber + 1), nCommandCount, tszCommand);
#endif
}

int
GetRandomNumber(int nMinimal, int nMaximal)
{
	return Random() % (nMaximal - nMinimal) + nMinimal;
}

void
ExecuteRandomStressTest()
{
	int nOperationsCount = GetRandomNumber(1, 50);

#ifdef DEBUG
	DebugOutput(TEXT("\nOperations count: %d...\n"), nOperationsCount);
#endif

	for(int i = 0; i < nOperationsCount; i++)
	{		
		int nOperation = GetRandomNumber(1, 48) % 4;

		PrintCommand( i, nOperationsCount, nOperation );

		switch(nOperation)
		{		
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

			default:
			case PLAY:
			{
				int nTrackNumber = GetRandomNumber(4, 8);
				int nRepeatTimes = GetRandomNumber(1, 4);
				PlayTrack( nTrackNumber, nTrackNumber, nRepeatTimes );
				break;
			}

		}

		int nWaitTime = GetRandomNumber(1, 3);
#ifdef DEBUG
		DebugOutput(TEXT("Waiting %d second(s)...\n\n"), nWaitTime);
#endif
		Sleep(1000 * nWaitTime);
	}
}

void
ExecutePauseQuickStressTest()
{
	PlayTrack( 8, 8, 1 );
	Sleep(1000);
	for(int i = 0; i < 5; i++)
	{
		int r = GetRandomNumber(1, 18) % 3;
		gdda.Pause();
		Sleep(1000 * r);	
	}
}

extern "C" int APIENTRY 
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{	
#ifdef DEBUG
	DebugOutput(TEXT("\n\n++++++ GDDA Debug Start ++++++\n"));
#endif

	if( Initialize() )
	{	
		ExecuteRandomStressTest();

		Finalize();
	}	

#ifdef DEBUG
	DebugOutput(TEXT("\n\n++++++ GDDA Debug End ++++++\n\n\n"));
#endif

    return 0;
}
