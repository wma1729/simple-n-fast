@ECHO OFF

REM Generates Makefile.constants

SETLOCAL ENABLEEXTENSIONS

SET ME=%~nx0
SET USAGE="Usage: %ME% [-cc <c++ compiler>] [-debug] [-install <install_path>] [-h]" 
SET CC=cl
SET DEBUG=/O2

:argparsingstart
IF -%1-==-- GOTO argparsingend

SET valid=0

IF %1==-cc (
	SET valid=1
	IF NOT -%2-==-- (
		SET CC=%2
		SHIFT
	) ELSE (
		ECHO %USAGE%
		EXIT /b 1
	)
)

IF %1==-debug (
	SET valid=1
	SET DEBUG=/Zi /D_DEBUG /MDd
)

IF %1==-install (
	SET valid=1
	IF NOT -%2-==-- (
		SET INSTPATH=%2
		SHIFT
	) ELSE (
		ECHO %USAGE%
		EXIT /b 1
	)
)

IF %valid% EQU 0 (
	ECHO %USAGE%
	EXIT /b 1
)

SHIFT
GOTO argparsingstart
:argparsingend

IF DEFINED Platform (
	SET HARDWARE=%Platform%
)

IF NOT DEFINED HARDWARE (
	IF DEFINED PROCESSOR_ARCHITECTURE (
		IF %PROCESSOR_ARCHITECTURE%==AMD64 (
			SET HARDWARE=x64
		)
	)
)

IF NOT DEFINED HARDWARE (
	SET HARDWARE=x86
)

SET BLDPLAT=Windows_%HARDWARE%
SET BLDDIR=%~dp0

ECHO P = %BLDPLAT% > Makefile.constants
ECHO M = %HARDWARE% >> Makefile.constants
ECHO INSTDIR = %INSTPATH% >> Makefile.constants
ECHO CC = %CC% /nologo >> Makefile.constants
ECHO CFLAGS = /c /TP /EHsc /W3 /WX /Gs >> Makefile.constants
ECHO DEFINES = /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS >> Makefile.constants
ECHO LD = link /dll /nologo >> Makefile.constants
ECHO LDFLAGS = /DEBUG:FULL /DYNAMICBASE >> Makefile.constants
ECHO AR = link /lib /nologo >> Makefile.constants
ECHO ARFLAGS = >> Makefile.constants
ECHO DBG = %DEBUG% >> Makefile.constants
ECHO INCLCOM = /I"%BLDDIR%libcom\include" >> Makefile.constants
ECHO INCLRDB = /I"%BLDDIR%librdb\include" >> Makefile.constants
ECHO INCLJSON = /I"%BLDDIR%libjson\include" >> Makefile.constants
ECHO INCLLOG = /I"%BLDDIR%liblog\include" >> Makefile.constants
ECHO INCLNET = /I"%BLDDIR%libnet\include" >> Makefile.constants
ECHO INCLTF = /I"%BLDDIR%tf" >> Makefile.constants
ECHO LIBCOM = "%BLDDIR%libcom\src\%BLDPLAT%\com.lib" >> Makefile.constants
ECHO LIBRDB = "%BLDDIR%librdb\src\%BLDPLAT%\rdb.lib" >> Makefile.constants
ECHO LIBJSON = "%BLDDIR%libjson\src\%BLDPLAT%\json.lib" >> Makefile.constants
ECHO LIBLOG = "%BLDDIR%liblog\src\%BLDPLAT%\log.lib" >> Makefile.constants
ECHO LIBNET = "%BLDDIR%libnet\src\%BLDPLAT%\net.lib" >> Makefile.constants

ECHO Makefile.constants generated successfully

EXIT /b 0
