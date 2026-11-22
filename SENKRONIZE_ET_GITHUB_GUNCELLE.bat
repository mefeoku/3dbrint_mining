@echo off
chcp 65001 >nul
title 3dbrint Farm - GitHub Senkronizasyon ve Derleme
color 0b
python "%~dp0sync_github.py"
pause
