#ifndef __EXTRAPI_HPP__
#define __EXTRAPI_HPP__

#include <windows.h>

// Tracks numbers are from 01 to 99
#define GDDA_TRACK_NUMBER_LENGTH 2

// Extra API missing from Windows CE

BOOL PathFileExists( LPCTSTR pszPath );
PTSTR StrCat( PTSTR psz1, PCTSTR psz2 );
LPTSTR StrStr( LPCTSTR lpFirst, LPCTSTR lpSrch );

#endif /* __EXTRAPI_HPP__ */
