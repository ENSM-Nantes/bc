@echo off
setlocal

tasklist | findstr /I "bridgecommand-es.exe" > "%TEMP%\serverStatus.log"

for /f %%i in ('find /c /v "" ^< "%TEMP%\serverStatus.log"') do set COUNT=%%i

if %COUNT% == 0 (
	echo -------- Start EnetServer
    cd /d "C:\Program Files\Bridge Command SOMOS 3.3\bin\win"
    bridgecommand-es.exe
) else (
    echo -------- EnetServer is already running
)

del "%TEMP%\serverStatus.log"
