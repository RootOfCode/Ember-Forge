@echo off
setlocal enableextensions enabledelayedexpansion

REM Build Ember Forge on Windows (or under Wine).
REM Requirements:
REM   - sbcl.exe on PATH
REM   - Quicklisp setup at %USERPROFILE%\quicklisp\setup.lisp (or QUICKLISP_SETUP env var)
REM   - SDL2 DLLs available at runtime (see vendor\windows\x64\README.md)

set "ROOT_DIR=%~dp0"
pushd "%ROOT_DIR%"

if not exist "dist" mkdir "dist"
if exist "dist\assets" rmdir /s /q "dist\assets"
xcopy /e /i /y "assets" "dist\assets" >nul

REM Always add dist to PATH so DLLs can be found while Quicklisp systems load.
set "PATH=%CD%\dist;%PATH%"

REM Optional: use host MinGW toolchain via wrappers when building under Wine.
REM build-win.sh sets EMBER_FORGE_WINE_TOOLS=1.
if defined EMBER_FORGE_WINE_TOOLS (
  copy /y "scripts\wine\gcc.bat" "dist\gcc.bat" >nul
  REM cffi-toolchain picks the compiler from %CC% (defaults to "gcc"). Force it to our shim.
  set "CC=%CD%\dist\gcc.bat"
)

REM --- SDL2 DLL discovery ---
REM Preferred: vendor\windows\x64\*.dll (flat)
REM Fallback:  dlls\**\*.dll (recursive, flattened into dist\)
set "VENDOR_DLL_DIR=vendor\windows\x64"
set "FALLBACK_DLLS_DIR=dlls"

set "DLL_SOURCE="
if exist "%VENDOR_DLL_DIR%\SDL2.dll" (
  set "DLL_SOURCE=%VENDOR_DLL_DIR%"
) else if exist "%FALLBACK_DLLS_DIR%\sdl2\SDL2.dll" (
  set "DLL_SOURCE=%FALLBACK_DLLS_DIR%"
)

if defined DLL_SOURCE (
  if /i "%DLL_SOURCE%"=="%VENDOR_DLL_DIR%" (
    copy /y "%VENDOR_DLL_DIR%\*.dll" "dist\" >nul
  ) else (
    for /r "%FALLBACK_DLLS_DIR%" %%F in (*.dll) do (
      copy /y "%%F" "dist\" >nul
    )
  )
) else (
  echo NOTE: No Windows SDL2 DLLs found.
  echo       Put x64 DLLs in %VENDOR_DLL_DIR%  OR  use the repo's dlls\ folder.
)

REM If using the host MinGW cross toolchain, ensure its runtime DLLs are on PATH so grovel executables can run.
if defined EMBER_FORGE_WINE_TOOLS (
  if exist "Z:\usr\x86_64-w64-mingw32\bin\libgcc_s_seh-1.dll" (
    set "PATH=Z:\usr\x86_64-w64-mingw32\bin;%PATH%"
  )
)

REM Required DLLs for a Windows/Wine build.
if not exist "dist\SDL2.dll" (
  echo ERROR: dist\SDL2.dll not found.
  echo        Add SDL2.dll under %VENDOR_DLL_DIR% or %FALLBACK_DLLS_DIR%\sdl2\ then re-run build.bat
  popd
  exit /b 1
)

REM CFFI needs libffi on Windows.
if not exist "dist\libffi-8.dll" if not exist "dist\libffi-7.dll" if not exist "dist\libffi-6.dll" if not exist "dist\libffi-5.dll" if not exist "dist\libffi.dll" (
  echo ERROR: libffi DLL not found in dist\.
  echo        Put libffi-8.dll ^(or libffi-7/6/5.dll, or libffi.dll^) under %VENDOR_DLL_DIR% or %FALLBACK_DLLS_DIR%\ so it gets copied.
  popd
  exit /b 1
)

if not defined QUICKLISP_SETUP (
  set "QUICKLISP_SETUP=%USERPROFILE%\quicklisp\setup.lisp"
)

if not exist "%QUICKLISP_SETUP%" (
  echo Quicklisp not found at: %QUICKLISP_SETUP%
  echo Set QUICKLISP_SETUP to your setup.lisp path, then re-run build.bat
  popd
  exit /b 1
)

where sbcl >nul 2>nul
if errorlevel 1 (
  echo SBCL not found on PATH.
  echo Install SBCL for Windows and ensure sbcl.exe is on PATH, then re-run build.bat
  popd
  exit /b 1
)

set "EMBER_FORGE_OUT=dist/ember-forge.exe"
set "XDG_CACHE_HOME=%CD%\.cache"
if not exist "%XDG_CACHE_HOME%" mkdir "%XDG_CACHE_HOME%"

sbcl --noinform --non-interactive --load "scripts\build-release.lisp"
if errorlevel 1 (
  echo Build failed.
  popd
  exit /b 1
)

echo Built: dist\ember-forge.exe
popd
exit /b 0
