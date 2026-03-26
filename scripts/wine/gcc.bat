@echo off
setlocal enableextensions

REM gcc shim for Wine builds.
REM Converts Windows paths to Unix paths (via `winepath -u`) and calls the Linux MinGW cross-compiler.

set "ROOT=%~dp0.."

REM Prefer bash from the host (Wine maps / to Z:\ by default).
set "BASH=Z:\usr\bin\bash"
if exist "Z:\usr\bin\bash.exe" set "BASH=Z:\usr\bin\bash.exe"
if not exist "%BASH%" (
  set "BASH=Z:\bin\bash"
)
if exist "Z:\bin\bash.exe" set "BASH=Z:\bin\bash.exe"

if not exist "%BASH%" (
  echo ERROR: bash not found at %BASH%
  exit /b 1
)

set "WRAP=%ROOT%\scripts\wine\gcc-wrap.sh"
if not exist "%WRAP%" (
  echo ERROR: gcc wrapper not found: %WRAP%
  exit /b 1
)

"%BASH%" "%WRAP%" %*
exit /b %errorlevel%
