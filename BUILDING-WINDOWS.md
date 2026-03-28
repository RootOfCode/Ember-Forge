# Building Ember Forge on Windows

Step-by-step guide to compiling Ember Forge from source on a fresh Windows machine.

---

## Prerequisites

| Tool | Version Used | Purpose |
|---|---|---|
| [SBCL](https://www.sbcl.org/) | 2.6.2 | Common Lisp compiler |
| [Quicklisp](https://www.quicklisp.org/) | latest | CL package manager (fetches dependencies) |
| [WinLibs MinGW GCC](https://winlibs.com/) | 15.2.0 | C compiler for CFFI grovel stubs |
| [pkg-config-lite](https://sourceforge.net/projects/pkgconfiglite/) | 0.28-1 | Locates C library headers/flags |
| SDL2 runtime DLLs | 2.32.4 | Windowing & rendering |
| SDL2_image runtime DLLs | 2.8.x | Image loading |
| SDL2_ttf runtime DLLs | 2.24.0 | Font rendering |
| libffi DLL | 3.4.x (libffi-8.dll) | Required by CFFI |

---

## 1. Install SBCL

```powershell
winget install SBCL.SBCL --accept-package-agreements
```

Restart your shell afterward so `sbcl` is on PATH. Verify:

```powershell
sbcl --version
# Should print: SBCL 2.6.2
```

## 2. Install Quicklisp

```powershell
Invoke-WebRequest -Uri "https://beta.quicklisp.org/quicklisp.lisp" -OutFile "$env:TEMP\quicklisp.lisp"

sbcl --noinform --non-interactive `
  --load "$env:TEMP\quicklisp.lisp" `
  --eval "(quicklisp-quickstart:install)" `
  --eval "(ql:add-to-init-file)" `
  --quit
```

> **Note:** You may be prompted to "Press Enter to continue" — do so.

This installs Quicklisp to `%USERPROFILE%\quicklisp\` and adds it to SBCL's init file (`~/.sbclrc`).

## 3. Install MinGW GCC

CFFI's grovel system compiles small C stubs at build time, which requires a C compiler.

```powershell
winget install BrechtSanders.WinLibs.POSIX.UCRT --accept-package-agreements
```

Restart your shell. Verify:

```powershell
gcc --version
```

## 4. Install pkg-config

CFFI grovel uses `pkg-config` to locate libffi headers.

```powershell
winget install bloodrock.pkg-config-lite --accept-package-agreements
```

## 5. Set Up libffi Development Files

WinLibs GCC does **not** ship with libffi headers. You need to manually provide `ffi.h`, `ffitarget.h`, and `libffi.pc`.

### a. Download libffi source (for headers only)

```powershell
Invoke-WebRequest -Uri "https://github.com/libffi/libffi/releases/download/v3.4.6/libffi-3.4.6.tar.gz" `
  -OutFile "$env:TEMP\libffi-3.4.6.tar.gz"

tar -xzf "$env:TEMP\libffi-3.4.6.tar.gz" -C "$env:TEMP"
```

### b. Find your WinLibs install root

```powershell
$gccPath = (Get-Command gcc).Source
$winlibsRoot = Split-Path (Split-Path $gccPath)
Write-Output $winlibsRoot
```

### c. Create ffi.h from the template

```powershell
$ffiHIn = Get-Content "$env:TEMP\libffi-3.4.6\include\ffi.h.in" -Raw
$ffiH = $ffiHIn `
  -replace '@TARGET@','X86_WIN64' `
  -replace '@HAVE_LONG_DOUBLE@','0' `
  -replace '@HAVE_LONG_DOUBLE_VARIANT@','0' `
  -replace '@FFI_EXEC_TRAMPOLINE_TABLE@','0'

Set-Content -Path "$winlibsRoot\include\ffi.h" -Value $ffiH
```

### d. Copy ffitarget.h

```powershell
Copy-Item "$env:TEMP\libffi-3.4.6\src\x86\ffitarget.h" `
  -Destination "$winlibsRoot\include\ffitarget.h"
```

### e. Create libffi.pc

```powershell
$pkgconfigDir = "$winlibsRoot\lib\pkgconfig"
New-Item -ItemType Directory -Force -Path $pkgconfigDir | Out-Null

$prefix = $winlibsRoot -replace '\\','/'
@"
prefix=$prefix
exec_prefix=`${prefix}
libdir=`${exec_prefix}/lib
includedir=`${prefix}/include

Name: libffi
Description: Library supporting Foreign Function Interfaces
Version: 3.4.6
Libs: -L`${libdir} -lffi
Cflags: -I`${includedir}
"@ | Set-Content "$pkgconfigDir\libffi.pc"
```

## 6. Obtain SDL2 DLLs

Download the **SDL2** (not SDL3!) runtime zips from GitHub:

| Library | Download |
|---|---|
| SDL2 | [SDL2-2.32.4-win32-x64.zip](https://github.com/libsdl-org/SDL/releases/download/release-2.32.4/SDL2-2.32.4-win32-x64.zip) |
| SDL2_image | [SDL2_image-2.8.8-win32-x64.zip](https://github.com/libsdl-org/SDL_image/releases) *(check latest 2.x)* |
| SDL2_ttf | [SDL2_ttf-2.24.0-win32-x64.zip](https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.24.0/SDL2_ttf-2.24.0-win32-x64.zip) |

Extract all `.dll` files into `vendor\windows\x64\` in the project root:

```
vendor\windows\x64\
├── SDL2.dll
├── SDL2_image.dll
├── SDL2_ttf.dll
├── libavif-16.dll      (from SDL2_image)
├── libtiff-5.dll       (from SDL2_image)
├── libwebp-7.dll       (from SDL2_image)
└── libwebpdemux-2.dll  (from SDL2_image)
```

## 7. Obtain libffi-8.dll

If you have **Git for Windows** installed, copy it from there:

```powershell
Copy-Item "C:\Program Files\Git\mingw64\bin\libffi-8.dll" `
  -Destination "vendor\windows\x64\"
```

Otherwise, install [MSYS2](https://www.msys2.org/) and run `pacman -S mingw-w64-ucrt-x86_64-libffi`, then copy from its `bin\` directory.

## 8. Build

### Option A: Using build.bat (with env vars)

```powershell
$env:PKG_CONFIG_PATH = "$winlibsRoot\lib\pkgconfig" -replace '\\','/'
$env:EMBER_FORGE_COMPRESSION = "0"

cmd /c build.bat
```

### Option B: Direct SBCL invocation

```powershell
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
$gccPath = (Get-Command gcc).Source
$winlibsRoot = Split-Path (Split-Path $gccPath)
$env:PKG_CONFIG_PATH = ($winlibsRoot + "\lib\pkgconfig") -replace '\\','/'
$env:EMBER_FORGE_COMPRESSION = "0"
$env:EMBER_FORGE_OUT = "dist/ember-forge.exe"

# Prepare dist directory
if (-not (Test-Path "dist")) { mkdir dist }
Remove-Item "dist\assets" -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "assets" -Destination "dist\assets" -Recurse
Copy-Item "vendor\windows\x64\*.dll" -Destination "dist\" -Force
$env:Path = (Resolve-Path "dist").Path + ";" + $env:Path

sbcl --noinform --non-interactive --load "scripts\build-release.lisp"
```

### Output

```
dist\
├── ember-forge.exe    (~54 MB uncompressed)
├── assets\            (game assets)
├── SDL2.dll
├── SDL2_image.dll
├── SDL2_ttf.dll
├── libffi-8.dll
└── ... (other DLLs)
```

Run with: `dist\ember-forge.exe`

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `npm.ps1 cannot be loaded` | Use `cmd /c` prefix, or set execution policy: `Set-ExecutionPolicy RemoteSigned -Scope CurrentUser` |
| `pkg-config: libffi not found` | Ensure `PKG_CONFIG_PATH` points to the directory containing `libffi.pc` |
| `ffi.h: No such file or directory` | Follow step 5 to install libffi headers into WinLibs include dir |
| `Unable to save compressed core` | Set `EMBER_FORGE_COMPRESSION=0` — the winget SBCL build lacks zlib support |
| `SDL2.dll not found` | Ensure DLLs are in `vendor\windows\x64\` (build.bat copies them to `dist\`) |
