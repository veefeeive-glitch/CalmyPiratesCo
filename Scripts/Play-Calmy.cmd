@echo off
pwsh.exe -NoProfile -File "%~dp0start_game.ps1" %*
if errorlevel 1 pause
