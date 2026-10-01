@echo off
setlocal DisableDelayedExpansion

set "SCRIPT=%~dp0prepare-ios-data.ps1"
if not exist "%SCRIPT%" (
    echo Could not find prepare-ios-data.ps1 beside this launcher.
    pause
    exit /b 1
)
set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%POWERSHELL%" (
    echo Windows PowerShell was not found.
    pause
    exit /b 1
)
set "UTILITY_MODULE=%SystemRoot%\System32\WindowsPowerShell\v1.0\Modules\Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1"
if not exist "%UTILITY_MODULE%" (
    echo The Windows PowerShell utility module was not found.
    pause
    exit /b 1
)

echo CnC TS Public Test 055 data preparation
echo Paste or drag the installed Tiberian Sun folder and downloaded IPA below.
echo.

set /p "SOURCE_DIR=Tiberian Sun / Firestorm installation folder: "
set "SOURCE_DIR=%SOURCE_DIR:"=%"
if not defined SOURCE_DIR (
    echo No game installation folder was provided.
    pause
    exit /b 1
)

set /p "IPA_PATH=OpenTS-unsigned.ipa file: "
set "IPA_PATH=%IPA_PATH:"=%"
if not defined IPA_PATH (
    echo No IPA file was provided.
    pause
    exit /b 1
)

set "OUTPUT_PARENT=%USERPROFILE%\Documents"
set /p "OUTPUT_PARENT=Output parent folder [Enter for %OUTPUT_PARENT%]: "
set "OUTPUT_PARENT=%OUTPUT_PARENT:"=%"
if not defined OUTPUT_PARENT set "OUTPUT_PARENT=%USERPROFILE%\Documents"
set "OUTPUT_DIR=%OUTPUT_PARENT%\CnC-TS-iOS-Ready"

echo.
echo Creating "%OUTPUT_DIR%\OpenTS" ...
"%POWERSHELL%" -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "Import-Module $env:UTILITY_MODULE -ErrorAction Stop; & $env:SCRIPT -SourceDir $env:SOURCE_DIR -Ipa $env:IPA_PATH -OutputDir $env:OUTPUT_DIR"
set "RESULT=%ERRORLEVEL%"

if "%RESULT%"=="0" (
    echo.
    echo Preparation complete. Copy OpenTS into the CnC TS Documents folder.
) else (
    echo.
    echo Preparation failed with exit code %RESULT%. No IPA or source files were changed.
)

pause
exit /b %RESULT%
