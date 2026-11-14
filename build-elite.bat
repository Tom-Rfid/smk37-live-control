@echo off
rem Compile le firmware SMK37 Open + CONTROL MAP pour le SMK-37 Elite.
rem Double-cliquez ce fichier. Resultat : SMK37_ELITE_OPEN_MAP.fwsc dans ce dossier.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-elite.ps1"
pause
