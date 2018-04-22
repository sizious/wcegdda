# Microsoft Developer Studio Project File - Name="gddadrv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE SH4) Static Library" 0x8604

CFG=gddadrv - Win32 (WCE SH4) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gddadrv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gddadrv.mak" CFG="gddadrv - Win32 (WCE SH4) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gddadrv - Win32 (WCE SH4) Release" (based on "Win32 (WCE SH4) Static Library")
!MESSAGE "gddadrv - Win32 (WCE SH4) Debug" (based on "Win32 (WCE SH4) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Dreamcast"
# PROP WCE_FormatVersion "6.0"
CPP=shcl.exe

!IF  "$(CFG)" == "gddadrv - Win32 (WCE SH4) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESH4Rel"
# PROP BASE Intermediate_Dir "WCESH4Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCESH4Rel"
# PROP Intermediate_Dir "WCESH4Rel"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MC /W3 /Zi /Ox /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
# ADD CPP /nologo /MC /W3 /Zi /Ox /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /Qgvp /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "gddadrv - Win32 (WCE SH4) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESH4Dbg"
# PROP BASE Intermediate_Dir "WCESH4Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCESH4Dbg"
# PROP Intermediate_Dir "WCESH4Dbg"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /c
# ADD CPP /nologo /MC /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH4" /D "_SH4_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_UNICODE" /YX /Qsh4r7 /Qs /Qfast /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "gddadrv - Win32 (WCE SH4) Release"
# Name "gddadrv - Win32 (WCE SH4) Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\audiodb.cpp
# End Source File
# Begin Source File

SOURCE=.\clean.cpp
DEP_CPP_CLEAN=\
	".\error.hpp"\
	".\gddadrv.hpp"\
	
NODEP_CPP_CLEAN=\
	".\eddcdrm.h"\
	".\eddstor.h"\
	".\egagdrm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\dsutils.cpp
DEP_CPP_DSUTI=\
	".\error.hpp"\
	".\gddadrv.hpp"\
	
NODEP_CPP_DSUTI=\
	".\eddcdrm.h"\
	".\eddstor.h"\
	".\egagdrm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\error.cpp
DEP_CPP_ERROR=\
	".\error.hpp"\
	".\gddadrv.hpp"\
	
NODEP_CPP_ERROR=\
	".\eddcdrm.h"\
	".\eddstor.h"\
	".\egagdrm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\extrapi.cpp
DEP_CPP_EXTRA=\
	".\extrapi.hpp"\
	
# End Source File
# Begin Source File

SOURCE=.\init.cpp
DEP_CPP_INIT_=\
	".\error.hpp"\
	".\extrapi.hpp"\
	".\gddadrv.hpp"\
	".\utils.hpp"\
	
NODEP_CPP_INIT_=\
	".\eddcdrm.h"\
	".\eddstor.h"\
	".\egagdrm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\main.cpp
DEP_CPP_MAIN_=\
	".\error.hpp"\
	".\extrapi.hpp"\
	".\gddadrv.hpp"\
	".\utils.hpp"\
	
NODEP_CPP_MAIN_=\
	".\eddcdrm.h"\
	".\eddstor.h"\
	".\egagdrm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\playcmd.cpp
DEP_CPP_PLAYC=\
	".\error.hpp"\
	".\extrapi.hpp"\
	".\gddadrv.hpp"\
	".\utils.hpp"\
	
NODEP_CPP_PLAYC=\
	".\eddcdrm.h"\
	".\eddstor.h"\
	".\egagdrm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\stream.cpp
DEP_CPP_STREA=\
	".\error.hpp"\
	".\gddadrv.hpp"\
	
NODEP_CPP_STREA=\
	".\eddcdrm.h"\
	".\eddstor.h"\
	".\egagdrm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\utils.cpp
DEP_CPP_UTILS=\
	".\error.hpp"\
	".\extrapi.hpp"\
	".\utils.hpp"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\audiodb.hpp
# End Source File
# Begin Source File

SOURCE=.\error.hpp
# End Source File
# Begin Source File

SOURCE=.\extrapi.hpp
# End Source File
# Begin Source File

SOURCE=.\gddadrv.hpp
# End Source File
# Begin Source File

SOURCE=.\utils.hpp
# End Source File
# End Group
# End Target
# End Project
