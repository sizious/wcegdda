#include <windows.h>
#include <dsound.h>
#include "gddadrv.hpp"

GDAudioDriver gdda;
LPDIRECTSOUND pds = NULL;

bool Initialize()
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

void Finalize()
{
	if ( pds )
	{
		pds->Release();
		pds = NULL;
	}
}

void PlayTrack(DWORD startTrack, DWORD endTrack, DWORD repeatCount)
{	
	SEGACD_PLAYTRACK playtrack;

	playtrack.dwStartTrack = startTrack;
	playtrack.dwEndTrack = endTrack;
	playtrack.dwRepeat = repeatCount;

	gdda.Play( playtrack );
}

extern "C" int APIENTRY 
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{	
	Initialize();	
	
	PlayTrack( 4, 4, 1 );
	Sleep(10000);

	PlayTrack( 4, 4, 1 );
	Sleep(2000);

	PlayTrack( 5, 5, 1 );
	Sleep(3000);

	Finalize();
    return 0;
}
