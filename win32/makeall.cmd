:: makeall.cmd 	
:: Gabriele Sirri (8-dic-2003)
::
:: Build FEDRA for WindowsNT/2000/XP/2003

@ECHO OFF
	IF /I '%1'=='clean'	GOTO CLEAN
	IF /I '%1'=='chktgt'	GOTO CHECKTARGET
	IF /I '%1'=='archive'	GOTO ARCHIVE
	IF NOT '%1'=='' IF NOT '%1'=='-vc7' IF NOT '%1'=='-novars' IF NOT '%1'=='clean' IF NOT '%1'=='chktgt' IF NOT '%1'=='archive'  GOTO HELP
	GOTO MAKEALL
GOTO END

::----------------------------------------------------------------------
:MAKEALL
	SETLOCAL
	IF '%1'=='-vc7'    SET compilerver=-vc7
	IF '%1'=='-novars' SET compilerver=-novars
	SET CURRENTDIR=%CD%

	:: Archive old fedra binaries
	IF EXIST lib\*.* CALL %0 ARCHIVE

	ECHO Check IF ROOT is installed 
		IF NOT DEFINED ROOTSYS GOTO ROOTSYS_NOT_DEFINED
		IF NOT EXIST %ROOTSYS%\bin\root.exe GOTO ROOT_DONT_EXIST
		ECHO ok!

	ECHO Check the version of ROOT
      	dir %ROOTSYS%\bin |find ".dll" > rootver.temp
      	IF EXIST workspace\obj\rootver.txt fc workspace\obj\rootver.txt rootver.temp > temp.temp
      
	IF %ERRORLEVEL%==0 GOTO ROOT_NOT_CHANGED
	:ROOT_CHANGED
		ECHO The version of ROOT has been changed, I will remove FEDRA binaries...
		CALL %0 clean
		GOTO ROOT_END
	:ROOT_NOT_CHANGED
		ECHO ROOT ok!
		GOTO ROOT_END
	:ROOT_END
		IF EXIST temp.temp del temp.temp
		IF EXIST rootver.temp del rootver.temp
   ::-----------------------------------------------------------------------------
   :BUILDFEDRA
	IF NOT EXIST workspace\obj mkdir workspace\obj 
	dir %ROOTSYS%\bin |find ".dll" > workspace\obj\rootver.txt

	ECHO Remove old FEDRA environment variables
	ECHO.
	cd tools
	CALL setfedra -u -d
	cd %CURRENTDIR%

	ECHO Build FEDRA
	ECHO -----------
	cd workspace
	CALL make.cmd	libEdb	%compilerver%
	CALL make.cmd	libEmath	%compilerver%
	CALL make.cmd	libEdr	%compilerver%
	CALL make.cmd	libEdd	%compilerver%
	CALL make.cmd	rwc2edb	%compilerver%
	CALL make.cmd	recset	%compilerver%
	echo FOR %%1 %%%%F IN (*.rwc) DO rwc2edb %%%%F %%%%F.root > ..\bin\convertall.cmd
	cd %CURRENTDIR%
	ECHO.

	ECHO SET the environment variables
	ECHO -----------------------------
	ECHO.
	cd tools
	CALL setfedra -u
	cd %CURRENTDIR%
	SET FEDRA=%CURRENTDIR%
	SET path=%CURRENTDIR%\bin;%CURRENTDIR%\lib;%PATH% 
	ECHO Environment variables have been changed only for the current user.
	ECHO Other users should SET their own variables using setfedra utility.
	ECHO.

	ECHO Check Targets:
	ECHO --------------
	CALL %0 chktgt lib\libEdb.lib
	CALL %0 chktgt lib\libEdb.dll
	CALL %0 chktgt lib\libEmath.lib
	CALL %0 chktgt lib\libEmath.dll
	CALL %0 chktgt lib\libEdr.lib
	CALL %0 chktgt lib\libEdr.dll
	CALL %0 chktgt lib\libEdd.lib
	CALL %0 chktgt lib\libEdd.dll
	CALL %0 chktgt bin\rwc2edb.exe
	CALL %0 chktgt bin\recset.exe

	ECHO.
	ECHO NOTES:
	ECHO Example: load libEdb library in ROOT/CINT:
	ECHO          Root[0] gSystem.Load("%%FEDRA%%\\lib\\libEdb.dll");
	ECHO.
	
	pause
	endlocal

GOTO END

::----------------------------------------------------------------------
:ROOTSYS_NOT_DEFINED
	ECHO.
        ECHO ERROR: the ROOTSYS environment variable has not been set. Please read 
	ECHO        the installation note of ROOT to run on the Windows command prompt 
	ECHO. 
	GOTO END
::----------------------------------------------------------------------
:ROOT_DONT_EXIST 
	ECHO.
        ECHO ERROR: root.exe doesn't EXIST in %%ROOTSYS%%\bin\root.exe
	ECHO. 
	GOTO END
::----------------------------------------------------------------------
:CLEAN
	ECHO Deleting FEDRA binaries...
	IF EXIST bin RD /S/Q bin
	IF EXIST lib RD /S/Q lib
	IF EXIST workspace\obj RD /S/Q workspace\obj
	IF EXIST workspace\rootcompare.txt DEL workspace\rootcompare.txt
	ECHO ok!
	GOTO END
::----------------------------------------------------------------------
:CHECKTARGET
        IF EXIST     %2 ECHO 	%2...ok!
        IF NOT EXIST %2 ECHO 	%2...ERROR
	  GOTO END
::----------------------------------------------------------------------
:ARCHIVE
	IF NOT EXIST lib\*.dll IF NOT EXIST lib\*.lib IF NOT EXIST bin\*.lib GOTO END
   :LOOP
	SET Choice=
	SET /P Choice=Previous FEDRA binaries EXIST, do you want to archive these files (Y/N)? 
	IF NOT '%Choice%'=='' SET Choice=%Choice:~0,1%
	ECHO.
	IF /I '%Choice%'=='Y' GOTO SAVEARCHIVE
	IF /I '%Choice%'=='N' GOTO END
  GOTO LOOP
	
   :SAVEARCHIVE
   ECHO.
   ECHO The files will be saved in %cd%\old\"folder"
	SET Choice=
	SET /P Choice=Insert the folder name:
	ECHO.
	mkdir old
	mkdir old\%Choice%
	mkdir old\%Choice%\bin
	mkdir old\%Choice%\lib
	mkdir old\%Choice%\tools
	move  bin\*.*   old\%Choice%\bin
	move  lib\*.*   old\%Choice%\lib
	copy  tools\*.* old\%Choice%\tools 
	ECHO Files saved. IF you want to use these files, change the environment variables
	ECHO using %CD%\old\%Choice%\tools\setfedra -u
	echo.
	pause
	echo.
	GOTO END
::----------------------------------------------------------------------
:HELP
	ECHO.
	ECHO usage: makeall [clean,-vc7, archive]
	ECHO.	
	ECHO Build FEDRA for WindowsNT/2000/XP/2003 with MS Visual Studio 6 /.NET/.NET 2003.
	ECHO The script automatically cleans the files when ROOT version has been changed. 
	ECHO N.B. Clean the files by yourself if you want to change the compiler version. 
	ECHO. 
	ECHO examples: makeall            build (MS Visual Studio 6)
	ECHO           makeall -vc7       build using MS Visual Studio .NET (2003)
	ECHO           makeall -novars    build without defining VC variables (for develop.)
	ECHO           makeall clean      remove targets and intermediate files
	ECHO           makeall archive    move current binaries in a separated folder
	ECHO           makeall -? (/?,-h) this help
	ECHO.
	GOTO END
::----------------------------------------------------------------------
:END
