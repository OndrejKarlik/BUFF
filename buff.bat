@echo off

REM Check if Node.js is installed by using the 'where' command
where node >nul 2>&1
if %errorlevel% neq 0 (
    echo Node.js is not installed! Get it here: https://nodejs.org/en/download/package-manager
    exit /b 1
)

REM Check if tsx node package is installed by using the 'where' command
where tsx >nul 2>&1
if %errorlevel% neq 0 (
    echo Installing tsx node package...
    call npm install --global tsx
    echo DONE!
)

tsx Buff/Scripts/Buff.ts %*