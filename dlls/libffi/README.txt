This folder is for Windows libffi DLLs needed by CFFI under Wine/Windows.

Place ONE of these in this folder (x64):
- libffi-8.dll
- libffi-7.dll
- libffi-6.dll
- libffi-5.dll
- libffi.dll

`build.bat` copies `dlls/**/*.dll` into `dist/` and adds `dist/` to PATH while building.
