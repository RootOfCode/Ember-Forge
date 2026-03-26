# Wine Build Helpers

`build-win.sh` sets `EMBER_FORGE_WINE_TOOLS=1`, which makes `build.bat` copy a `gcc` shim into `dist/`.

This is needed because some Quicklisp dependencies (notably `cffi-libffi`) run `cffi-grovel`, which requires a C compiler during the build.

The shim (`scripts/wine/gcc.bat` + `scripts/wine/gcc-wrap.sh`) uses the host Linux MinGW-w64 cross compiler (`x86_64-w64-mingw32-gcc`) and converts Windows paths to Unix paths via `winepath -u`.

