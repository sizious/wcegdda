#include "error.hpp"

// Print debug messages in the console
void DebugOutput( TCHAR *tszErr, ... )
{
#ifdef DEBUG
    TCHAR tszErrOut[256];

    va_list valist;

    va_start (valist,tszErr);
    wvsprintf (tszErrOut, tszErr, valist);
    OutputDebugString (tszErrOut);

    va_end (valist);
#endif
}
