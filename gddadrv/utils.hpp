#ifndef __UTILS_HPP__
#define __UTILS_HPP__

void GetSoundFilePath( int playTrackIndex, TCHAR szWaveFile[] );
bool ParseWaveFile( void *pvWaveFile, WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData, DWORD *pcbWaveSize );
bool PathFileExists( LPCTSTR pszPath );

#endif /* __UTILS_HPP__ */