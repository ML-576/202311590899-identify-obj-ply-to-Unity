@echo off
rem ============================================================
rem build_windows.bat - Build Unity native plugin ModelReader.dll
rem
rem Usage:
rem   build_windows.bat          - try MSVC first, fallback to MinGW
rem   build_windows.bat msvc     - force MSVC (Visual Studio 2017/2019/2022
rem                                with C++ desktop workload, needs Windows SDK)
rem   build_windows.bat mingw    - force MinGW-w64 (g++ must be in PATH,
rem                                links winpthreads/libgcc/libstdc++ static
rem                                so the DLL is self-contained)
rem ============================================================
setlocal
cd /d "%~dp0"

set OUTDIR=UnityAssets\Plugins\x86_64
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

set MODE=%1
if "%MODE%"=="" set MODE=auto
if /i "%MODE%"=="msvc" goto :msvc
if /i "%MODE%"=="mingw" goto :mingw

rem ---- auto: MSVC first ----
where cl >nul 2>nul
if errorlevel 1 goto :msvc
goto :msvc

:msvc
rem --- locate Visual Studio via vswhere ---
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VSWHERE%" (
    if /i "%MODE%"=="auto" goto :mingw
    echo [ERROR] vswhere.exe not found. Please install Visual Studio.
    goto :fail
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSDIR=%%i
if not defined VSDIR (
    if /i "%MODE%"=="auto" goto :mingw
    echo [ERROR] Visual Studio with C++ tools not found.
    goto :fail
)
echo Using Visual Studio: %VSDIR%
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto :fail

echo.
echo [1/3] Building plugin DLL (MSVC) ...
cl /nologo /O2 /MD /EHsc /GR- /W3 ^
   /DNO_OPENGL_RENDER /DMODEL_BRIDGE_EXPORTS ^
   /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE ^
   /I. ^
   /LD OBJ.cpp glm.cpp hply.cpp ReadPly.cpp ModelBridge.cpp ^
   /Fe:"%OUTDIR%\ModelReader.dll"
if errorlevel 1 goto :fail

echo [2/3] Building test program ...
cl /nologo /O2 /MD /EHsc /GR- /W3 ^
   /DNO_OPENGL_RENDER ^
   /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE ^
   /I. ^
   test_bridge.cpp /Fe:test_bridge.exe /link "%OUTDIR%\ModelReader.lib"
if errorlevel 1 goto :fail

echo [3/3] Cleaning intermediates ...
del /q *.obj 2>nul
copy /y "%OUTDIR%\ModelReader.dll" ModelReader.dll >nul

goto :done

:mingw
where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ not found in PATH and MSVC unavailable.
    echo         Install Visual Studio C++ workload with Windows SDK,
    echo         or MinGW-w64 such as WinLibs, and add g++ to PATH.
    goto :fail
)
for /f "delims=" %%v in ('g++ -dumpfullversion 2^>nul') do set GCCVER=%%v
echo Using MinGW g++ %GCCVER%
echo.
echo [1/3] Building plugin DLL (MinGW, statically linked) ...
g++ -std=c++14 -O2 -shared -static -static-libgcc -static-libstdc++ ^
   -DNO_OPENGL_RENDER -DMODEL_BRIDGE_EXPORTS ^
   -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE ^
   -I. OBJ.cpp glm.cpp hply.cpp ReadPly.cpp ModelBridge.cpp ^
   -o "%OUTDIR%\ModelReader.dll" -s
if errorlevel 1 goto :fail

echo [2/3] Building test program ...
g++ -std=c++14 -O2 -municode -static -static-libgcc -static-libstdc++ ^
   -D_CRT_SECURE_NO_WARNINGS ^
   -I. test_bridge.cpp "%OUTDIR%\ModelReader.dll" -o test_bridge.exe -s
if errorlevel 1 goto :fail

echo [3/3] Copying DLL for local test run ...
copy /y "%OUTDIR%\ModelReader.dll" ModelReader.dll >nul

:done
echo.
echo ================= BUILD OK =================
echo   Plugin DLL : %OUTDIR%\ModelReader.dll
echo   Test tool  : test_bridge.exe
echo   Usage      : test_bridge.exe "model.obj" ["more files..."]
echo ============================================
exit /b 0

:fail
echo.
echo ================= BUILD FAILED =================
exit /b 1
