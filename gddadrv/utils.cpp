#include "utils.hpp"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Function:

    ExtractTrackNumberFromFileName

Description:

    Extract the track number from the passed wave file name.
	The filename should be like TRACKxx.WAV, where xx is a number
	between 01 and 99.
    
Arguments:

    TCHAR        *szWaveFile     -  Pointer to the wav file name to parse

Return Value:

    An int value corresponding to the szWaveFile passed in parameter.
	The value can be between 1 and 99.
	0 is returned if the track number was not successfully parsed.

-------------------------------------------------------------------*/

int
ExtractTrackNumberFromFileName( TCHAR * szWaveFile )
{
	int result = 0;
	TCHAR szTrackNumber[ GDDA_TRACK_NUMBER_LENGTH + 1 ];
	TCHAR * szTrackNumberPosition;	

	szTrackNumberPosition = StrStr( CharLower( szWaveFile ), TEXT(".wav\0") );
	if( szTrackNumberPosition != NULL ) {
		memcpy( &szTrackNumber, &szTrackNumberPosition[ -GDDA_TRACK_NUMBER_LENGTH ], GDDA_TRACK_NUMBER_LENGTH * sizeof(TCHAR) );
		szTrackNumber[ GDDA_TRACK_NUMBER_LENGTH ] = '\0';
		result = _ttoi( szTrackNumber );
	}

	return result;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Function:

    ParseWaveFile

Description:

    Get the Wave File header, size, and data pointer...
    
Arguments:

    void         *pvWaveFile     -  Pointer to the wav file to parse

    WAVEFORMATEX **ppWaveHeader  -  Fill this with pointer to wave header

    BYTE         **ppbWaveData   -  Fill this with pointer to wave data

    DWORD        **pcbWaveSize   -  Fill this with wave data size.

Return Value:

    TRUE on success, FALSE on failure.

-------------------------------------------------------------------*/

bool
ParseWaveFile( void *pvWaveFile, WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData, DWORD *pcbWaveSize )
{
    DWORD *pdw;
    DWORD *pdwEnd;
    DWORD dwRiff;
    DWORD dwType;
    DWORD dwLength;

    if (ppWaveHeader)
        *ppWaveHeader = NULL;

    if (ppbWaveData)
        *ppbWaveData = NULL;

    if (pcbWaveSize)
        *pcbWaveSize = 0;

    pdw = (DWORD *)pvWaveFile;
    dwRiff   = *pdw++;
    dwLength = *pdw++;
    dwType   = *pdw++;

    // Check if it's a WAV format file
    if (dwType != mmioFOURCC('W', 'A', 'V', 'E'))
        return false;

    // Check if it's a RIFF format file
    if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F'))
        return false;

    pdwEnd = (DWORD *)((BYTE *)pdw + dwLength-4);

    while (pdw < pdwEnd)
    {
        dwType = *pdw++;
        dwLength = *pdw++;

        switch (dwType)
        {
        case mmioFOURCC('f', 'm', 't', ' '):
            if (ppWaveHeader && !*ppWaveHeader)
            {
                if (dwLength < sizeof(WAVEFORMAT))
                    return false;

                *ppWaveHeader = (WAVEFORMATEX *)pdw;

                if ((!ppbWaveData || *ppbWaveData) && (!pcbWaveSize || *pcbWaveSize))
                    return true;
            }
            break;

        case mmioFOURCC('d', 'a', 't', 'a'):
            if ((ppbWaveData && !*ppbWaveData) || (pcbWaveSize && !*pcbWaveSize))
            {
                if (ppbWaveData)
                    *ppbWaveData = (LPBYTE)pdw;

                if (pcbWaveSize)
                    *pcbWaveSize = dwLength;

                if (!ppWaveHeader || *ppWaveHeader)
                    return true;
            }
            break;
        }

        pdw = (DWORD *)((BYTE *)pdw + ((dwLength+1)&~1));
    }

    return false;
}

