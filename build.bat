@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
setlocal enabledelayedexpansion

cd %1

mkdir build

rem Set the file extension to look for (without the dot)
set "EXT=c"
set "FILE_LIST="

for %%F in (*.%EXT%) do (
    echo %%~nF | findstr /i "test" >nul
    if errorlevel 1 (
        set "FILE_LIST=!FILE_LIST! %%F"
    )
)

echo %FILE_LIST%

cl /ZI %FILE_LIST% /Fobuild\ /Fe:build/nanoserv.exe %~2