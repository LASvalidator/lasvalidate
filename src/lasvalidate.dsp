# Microsoft Developer Studio Project File - Name="lasvalidate" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=lasvalidate - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lasvalidate.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lasvalidate.mak" CFG="lasvalidate - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lasvalidate - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "lasvalidate - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lasvalidate - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /w /W0 /GX /O2 /I "..\..\lasread\inc" /I "..\..\lascheck\inc" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "../src" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ../../lasread/lib/LASread.lib ../../lascheck/lib/LAScheck.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\lasvalidate.exe  ..\bin\lasvalidate.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "lasvalidate - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /w /W0 /Gm /GX /ZI /Od /I "..\..\lasread\inc" /I "..\..\lascheck\inc" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\src" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../../lasread/lib/LASreadD.lib ../../lascheck/lib/LAScheckD.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\lasvalidate.exe ..\bin\lasvalidate.exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "lasvalidate - Win32 Release"
# Name "lasvalidate - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\lasvalidate.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\LAScheck\inc\lascheck.hpp
# End Source File
# Begin Source File

SOURCE=..\..\lasread\inc\lasdefinitions.hpp
# End Source File
# Begin Source File

SOURCE=..\..\lasread\inc\lasheader.hpp
# End Source File
# Begin Source File

SOURCE=..\laslibrary\inc\lasheader.hpp
# End Source File
# Begin Source File

SOURCE=..\..\lasread\inc\laspoint.hpp
# End Source File
# Begin Source File

SOURCE=..\laslibrary\inc\laspoint.hpp
# End Source File
# Begin Source File

SOURCE=..\..\LASread\inc\lasreader.hpp
# End Source File
# Begin Source File

SOURCE=..\laslibrary\inc\lasreader.hpp
# End Source File
# Begin Source File

SOURCE=..\..\LASread\inc\lasreadopener.hpp
# End Source File
# Begin Source File

SOURCE=..\laslibrary\inc\lasreadopener.hpp
# End Source File
# Begin Source File

SOURCE=..\..\lasread\inc\lasutility.hpp
# End Source File
# Begin Source File

SOURCE=..\laslibrary\inc\lasutility.hpp
# End Source File
# Begin Source File

SOURCE=..\..\lasread\inc\laszip.hpp
# End Source File
# Begin Source File

SOURCE=..\laslibrary\inc\laszip.hpp
# End Source File
# Begin Source File

SOURCE=..\..\lasread\inc\mydefs.hpp
# End Source File
# Begin Source File

SOURCE=..\laslibrary\inc\mydefs.hpp
# End Source File
# Begin Source File

SOURCE=..\..\LAScheck\inc\xmlwriter.hpp
# End Source File
# Begin Source File

SOURCE=.\xmlwriter.hpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
