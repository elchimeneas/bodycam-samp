@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Gestionar.ps1" -Action Restore -GameDir "%~1"
pause
