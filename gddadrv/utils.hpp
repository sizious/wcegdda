#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <windows.h>
#include <dsound.h>
#include "error.hpp"
#include "extrapi.hpp"

int ExtractTrackNumberFromFileName( TCHAR * szWaveFile );
bool ParseWaveFile( void *pvWaveFile, WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData, DWORD *pcbWaveSize );

#endif /* __UTILS_HPP__ */