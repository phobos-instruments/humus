@echo off
rem Humus on Windows - a small wrapper around CMake. Run `make` to see the
rem targets. This is the counterpart to the Makefile beside it:
rem the same target names, doing the same things, for a shell that has no make.
rem
rem Nothing here is required. `cmake -S engine -B engine\build -A x64` and
rem `cmake --build engine\build --config Release` do the same job by hand.
setlocal EnableDelayedExpansion

set "REPO=%~dp0"
if "%REPO:~-1%"=="\" set "REPO=%REPO:~0,-1%"
set "ENGINE_DIR=%REPO%\engine"
set "BUILD_DIR=%ENGINE_DIR%\build"
if not defined BUILD_TYPE set "BUILD_TYPE=Release"

set "GUI=%BUILD_DIR%\hum_gui_artefacts\%BUILD_TYPE%\Humus.exe"
set "CLI=%BUILD_DIR%\%BUILD_TYPE%\hum.exe"

rem Every exit check below is `neq 0`, never `if errorlevel 1`: "errorlevel N"
rem means "N or higher", and a Windows crash exits NEGATIVE - an access
rem violation is 0xC0000005, i.e. -1073741819 signed - so `if errorlevel 1`
rem reads a crashed build as a pass.
rem
rem And every `if` body is braced. `if COND cmd & other` runs `other`
rem unconditionally - the `&` is not part of the if - so an unbraced dispatch
rem table stops at its first non-matching line.

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=help"

if /i "%TARGET%"=="help"         goto usage
if /i "%TARGET%"=="-h"           goto usage
if /i "%TARGET%"=="--help"       goto usage
if /i "%TARGET%"=="/?"           goto usage
if /i "%TARGET%"=="configure"    goto configure
if /i "%TARGET%"=="build"        goto build
if /i "%TARGET%"=="run"          goto run
if /i "%TARGET%"=="pack"         goto pack
if /i "%TARGET%"=="install-pack" goto installpack
if /i "%TARGET%"=="clean"        goto clean

echo make: no target "%TARGET%".
echo.
goto usage

:configure
if exist "%BUILD_DIR%\CMakeCache.txt" (
  echo already configured in "%BUILD_DIR%" - delete it or run `make clean` to start over
  exit /b 0
)
echo configuring ^(fetches JUCE on first run - needs a network^)...
cmake -S "%ENGINE_DIR%" -B "%BUILD_DIR%" -A x64
if !ERRORLEVEL! neq 0 (echo. & echo ERROR: configure failed. & exit /b 1)
exit /b 0

:build
call :ensure || exit /b 1
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if !ERRORLEVEL! neq 0 (echo. & echo ERROR: build failed. & exit /b 1)
echo.
echo the command-line tool is built alongside the app:
echo   "%CLI%" info    patch.hum
echo   "%CLI%" render  patch.hum out.wav --seconds 10
exit /b 0

rem hum_gui_bundle, not hum_gui: linking leaves the executable unstaged, so the
rem app would run against whatever packs an older build left beside it.
:run
call :ensure || exit /b 1
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --target hum_gui_bundle
if !ERRORLEVEL! neq 0 (echo. & echo ERROR: build failed. & exit /b 1)
if not exist "%GUI%" (echo ERROR: "%GUI%" is missing after a successful build. & exit /b 1)
"%GUI%" %2 %3 %4 %5 %6 %7 %8 %9
exit /b !ERRORLEVEL!

:pack
call :ensure || exit /b 1
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --target humpacks
if !ERRORLEVEL! neq 0 (echo. & echo ERROR: build failed. & exit /b 1)
echo packs are in "%BUILD_DIR%\humpacks"
exit /b 0

rem A .humpack is a zip. tar ships with Windows 10 1803 and later and reads it
rem whatever the extension says, which Expand-Archive will not. Clear the
rem target first: extracting over a folder never deletes, so a file the new
rem bundle dropped would survive.
:installpack
rem Both spellings, so the Makefile's `PACK=<id>` works here too.
set "PACK=%~2"
if /i "!PACK:~0,5!"=="PACK=" set "PACK=!PACK:~5!"
set "PACKS_DIR=%APPDATA%\Humus\packs"
if "%PACK%"=="" goto packusage
if not exist "%BUILD_DIR%\humpacks\%PACK%.humpack" goto packusage
where tar >nul 2>nul
if !ERRORLEVEL! neq 0 (echo ERROR: no tar on PATH - it ships with Windows 10 1803 and later. & exit /b 1)
if exist "%PACKS_DIR%\%PACK%" rmdir /s /q "%PACKS_DIR%\%PACK%"
mkdir "%PACKS_DIR%\%PACK%" 2>nul
tar -xf "%BUILD_DIR%\humpacks\%PACK%.humpack" -C "%PACKS_DIR%\%PACK%"
if !ERRORLEVEL! neq 0 (echo ERROR: could not unpack "%PACK%". & exit /b 1)
echo installed %PACK% to "%PACKS_DIR%\%PACK%" - restart the app to see it
exit /b 0

:packusage
echo usage: make install-pack ^<id^>   ^(or PACK=^<id^>^)
if exist "%BUILD_DIR%\humpacks" (
  echo built packs:
  for %%f in ("%BUILD_DIR%\humpacks\*.humpack") do echo   %%~nf
) else (
  echo   none built yet - run `make pack` first
)
exit /b 1

:clean
if exist "%BUILD_DIR%" (
  echo removing "%BUILD_DIR%"...
  rmdir /s /q "%BUILD_DIR%"
  if !ERRORLEVEL! neq 0 (echo ERROR: could not remove it - is the app still running? & exit /b 1)
)
exit /b 0

:ensure
if not exist "%BUILD_DIR%\CMakeCache.txt" (
  echo configuring ^(fetches JUCE on first run - needs a network^)...
  cmake -S "%ENGINE_DIR%" -B "%BUILD_DIR%" -A x64
  if !ERRORLEVEL! neq 0 (echo. & echo ERROR: configure failed. & exit /b 1)
)
exit /b 0

:usage
echo Humus - make targets:
echo.
echo   configure      configure the build ^(fetches JUCE on first run^)
echo   build          build the app and the command-line tool
echo   run            build it, then launch it in this window
echo   pack           build the .humpack bundles
echo   install-pack   install one built pack: make install-pack ^<id^>
echo   clean          delete the build directory
echo.
echo Set BUILD_TYPE=Debug in the environment for a debug build.
echo.
echo It only wraps CMake, so this works just as well:
echo   cmake -S engine -B engine\build -A x64
echo   cmake --build engine\build --config Release
exit /b 0
