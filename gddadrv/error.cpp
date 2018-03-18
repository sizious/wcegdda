#include "gddadrv.hpp"
#include "error.hpp"

BOOL
GDAudioDriver::CheckError(TCHAR *tszErr)
{
	GDDA_CONTEXT *gddaContext = this->GetCurrentContext();
	BOOL isError = ( gddaContext->errLast != 0 );

#ifdef DEBUG
    if ( isError )
    {
        DebugOutput(TEXT("CheckError: %s failed (ErrorCode=0x%08x).\n"), tszErr, gddaContext->errLast);        
    }	
#endif

    return ( isError );
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Function:

    DebugOutput

Description:

    Simple Debug Output mechanism.  If DEBUG is TRUE, then the function
    outputs the passed-in String and variables to the debug output
    Stream.  Otherwise, the function does nothing

Arguments:

    TCHAR *tszDest       - TEXT String to output if an error occurred

    ... (variable arg)   - List of variable arguments to embed in output
                           (similar to printf format)

Return Value:

    None

-------------------------------------------------------------------*/
void
DebugOutput(TCHAR *tszErr, ...)
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
