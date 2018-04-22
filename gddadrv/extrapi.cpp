#include "extrapi.hpp"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Function:

    StrStr

Description:

    Finds the first occurrence of a substring within a string.
	The comparison is case sensitive.

Arguments:

    LPCTSTR			lpFirst     -	Address of the string being searched.

	LPCTSTR			lpSrch		-	Substring to search for. 

Return Value:

	Returns the address of the first occurrence of the matching substring
	if successful, or NULL otherwise.

-------------------------------------------------------------------*/

// Thanks Techie Delight
// http://www.techiedelight.com/implement-strstr-function-c-iterative-recursive/

LPTSTR
StrStr( LPCTSTR lpFirst, LPCTSTR lpSrch )
{
	size_t n = _tcslen( lpSrch );

	while( *lpFirst )
	{
		if (!memcmp(lpFirst, lpSrch, n))
			return const_cast <LPTSTR> (lpFirst);

		lpFirst++;
	}

	return NULL;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Function:

    StrCat

Description:

    Appends one string to another.

Arguments:

    PTSTR			psz1    -	A pointer to a null-terminated string. When this function returns 
								successfully, this string contains its original content with the 
								string psz2 appended. This buffer must be large enough to hold 
								both strings and the terminating null character.

	PCTSTR			psz2	-	A pointer to a null-terminated string to be appended to psz1.

Return Value:

	Returns a pointer to psz1, which holds the combined strings.

-------------------------------------------------------------------*/

PTSTR StrCat( PTSTR psz1, PCTSTR psz2 )
{
	size_t nLength1 = _tcslen( psz1 );
	size_t nLength2 = _tcslen( psz2 );
		
	memcpy( psz1 + nLength1, psz2, nLength2 * sizeof(TCHAR) );
	
	size_t nFinalLength = nLength1 + nLength2 + 1;
	psz1[ nFinalLength ] = '\0';
	
	return psz1;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Function:

    PathFileExists

Description:

    Determines whether a path to a file system object such as a file or folder is valid.
    
	This function is a rewrite of the PathFileExists from Shlwapi, which is not available
	on Windows CE, at least for the Sega Dreamcast.

Arguments:

    LPCTSTR      pszPath     -  A pointer to a null-terminated string of maximum length 
								MAX_PATH that contains the full path of the object to verify.

Return Value:

    true if the file exists; otherwise, false.
	Call GetLastError for extended error information.

-------------------------------------------------------------------*/

BOOL 
PathFileExists( LPCTSTR pszPath ) 
{
	BOOL result = FALSE;

	HANDLE hFile = CreateFile( pszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
	
	result = (hFile == INVALID_HANDLE_VALUE);
	
	if (!result)
	{
		CloseHandle( hFile );
	}

	return result;
}
