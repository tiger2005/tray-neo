@echo off
setlocal

rem build.bat - Build TrayNeo plugin (x86 and x64 Release)

set "REPO_ROOT=%~dp0"
set "REPO_ROOT=%REPO_ROOT:~0,-1%"

rem Find vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Error: vswhere not found. Is Visual Studio installed?
    exit /b 1
)

rem Find MSBuild (write to temp file to avoid quoting issues)
set "TMPFILE=%TEMP%\vswhere_output.txt"
"%VSWHERE%" -latest -find "MSBuild\**\Bin\MSBuild.exe" > "%TMPFILE%"
set /p MSBUILD=<"%TMPFILE%"
del "%TMPFILE%" 2>nul

if not defined MSBUILD (
    echo Error: MSBuild not found
    exit /b 1
)
echo MSBuild: %MSBUILD%

set "VCPROJ=%REPO_ROOT%\src\TrayNeo.vcxproj"

rem Build Win32 Release
echo.
echo ========== Build Win32 Release ==========
"%MSBUILD%" "%VCPROJ%" /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /p:SolutionDir="%REPO_ROOT%\\" /m /v:minimal
if errorlevel 1 (
    echo Build failed: Win32 Release
    exit /b 1
)
echo Build OK: Win32 Release

rem Build x64 Release
echo.
echo ========== Build x64 Release ==========
"%MSBUILD%" "%VCPROJ%" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%REPO_ROOT%\\" /m /v:minimal
if errorlevel 1 (
    echo Build failed: x64 Release
    exit /b 1
)
echo Build OK: x64 Release

echo.
echo All builds complete
echo Output:
echo   x86: %REPO_ROOT%\Bin\Release\plugins\
echo   x64: %REPO_ROOT%\Bin\x64\Release\plugins\
exit /b 0
