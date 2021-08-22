@echo off

:: Set APP_SOURCE_DIR root directory
setlocal enabledelayedexpansion
if "%APP_SOURCE_DIR%"=="" (
  :: Delayed expansion is required within parentheses https://superuser.com/questions/78496/variables-in-batch-file-not-being-set-when-inside-if
  cd /d "%~dp0"
  cd ..\..
  set APP_SOURCE_DIR=!CD!
  cd ..\..
  echo APP_SOURCE_DIR set to '!APP_SOURCE_DIR!'.
)
endlocal & set APP_SOURCE_DIR=%APP_SOURCE_DIR%

:: Set build configuration parameters
set CONFIGURATION=Release
set PLATFORM=x64
set PLATFORMTOOLSET=""
echo Building for Windows in %CONFIGURATION%, %Platform% configuration...
echo.

:: Return back to scripts folder
cd /d "%~dp0"

:: Call windows scripts.
call "%APP_SOURCE_DIR%\ci\windows\build.bat"
if %errorlevel% neq 0 pause && exit /b %errorlevel%

:: Press a key to continue
pause
