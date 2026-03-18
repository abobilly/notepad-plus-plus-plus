@echo off
REM ============================================================
REM BACKUP then REMOVE context menu entries
REM This script requires administrator privileges (UAC prompt)
REM ============================================================

echo.
echo ===== STEP 1: BACKING UP CURRENT ENTRIES =====
echo.

set BACKUP=c:\tbar_flip\ctx_menu_backup.reg

echo Windows Registry Editor Version 5.00 > "%BACKUP%"
echo. >> "%BACKUP%"

reg export "HKCR\Folder\shell\TreeSize Free" "%TEMP%\_ctx1.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx1.reg" type "%TEMP%\_ctx1.reg" >> "%BACKUP%" & del "%TEMP%\_ctx1.reg"

reg export "HKCR\Directory\shell\git_gui" "%TEMP%\_ctx2.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx2.reg" type "%TEMP%\_ctx2.reg" >> "%BACKUP%" & del "%TEMP%\_ctx2.reg"

reg export "HKCR\Directory\shell\git_shell" "%TEMP%\_ctx3.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx3.reg" type "%TEMP%\_ctx3.reg" >> "%BACKUP%" & del "%TEMP%\_ctx3.reg"

reg export "HKCR\Directory\Background\shell\git_gui" "%TEMP%\_ctx4.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx4.reg" type "%TEMP%\_ctx4.reg" >> "%BACKUP%" & del "%TEMP%\_ctx4.reg"

reg export "HKCR\Directory\Background\shell\git_shell" "%TEMP%\_ctx5.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx5.reg" type "%TEMP%\_ctx5.reg" >> "%BACKUP%" & del "%TEMP%\_ctx5.reg"

reg export "HKCR\Directory\shell\AddToPlaylistVLC" "%TEMP%\_ctx6.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx6.reg" type "%TEMP%\_ctx6.reg" >> "%BACKUP%" & del "%TEMP%\_ctx6.reg"

reg export "HKCR\Directory\shell\PlayWithVLC" "%TEMP%\_ctx7.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx7.reg" type "%TEMP%\_ctx7.reg" >> "%BACKUP%" & del "%TEMP%\_ctx7.reg"

reg export "HKCR\*\shellex\ContextMenuHandlers\DropboxExt" "%TEMP%\_ctx8.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx8.reg" type "%TEMP%\_ctx8.reg" >> "%BACKUP%" & del "%TEMP%\_ctx8.reg"

reg export "HKCR\Directory\shellex\ContextMenuHandlers\DropboxExt" "%TEMP%\_ctx9.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx9.reg" type "%TEMP%\_ctx9.reg" >> "%BACKUP%" & del "%TEMP%\_ctx9.reg"

reg export "HKCR\Directory\Background\shellex\ContextMenuHandlers\DropboxExt" "%TEMP%\_ctx10.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx10.reg" type "%TEMP%\_ctx10.reg" >> "%BACKUP%" & del "%TEMP%\_ctx10.reg"

reg export "HKCR\Directory\Background\shellex\ContextMenuHandlers\PowerRenameExt" "%TEMP%\_ctx11.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx11.reg" type "%TEMP%\_ctx11.reg" >> "%BACKUP%" & del "%TEMP%\_ctx11.reg"

reg export "HKCR\AllFilesystemObjects\shellex\ContextMenuHandlers\PowerRenameExt" "%TEMP%\_ctx12.reg" /y >nul 2>nul
if exist "%TEMP%\_ctx12.reg" type "%TEMP%\_ctx12.reg" >> "%BACKUP%" & del "%TEMP%\_ctx12.reg"

echo Backup saved to: %BACKUP%
echo.

echo ===== STEP 2: REMOVING CONTEXT MENU ENTRIES =====
echo.

echo Removing TreeSize Free...
reg delete "HKCR\Folder\shell\TreeSize Free" /f >nul 2>nul && echo   OK || echo   not found

echo Removing Git GUI (Directory)...
reg delete "HKCR\Directory\shell\git_gui" /f >nul 2>nul && echo   OK || echo   not found

echo Removing Git Shell (Directory)...
reg delete "HKCR\Directory\shell\git_shell" /f >nul 2>nul && echo   OK || echo   not found

echo Removing Git GUI (Background)...
reg delete "HKCR\Directory\Background\shell\git_gui" /f >nul 2>nul && echo   OK || echo   not found

echo Removing Git Shell (Background)...
reg delete "HKCR\Directory\Background\shell\git_shell" /f >nul 2>nul && echo   OK || echo   not found

echo Removing VLC AddToPlaylist...
reg delete "HKCR\Directory\shell\AddToPlaylistVLC" /f >nul 2>nul && echo   OK || echo   not found

echo Removing VLC PlayWith...
reg delete "HKCR\Directory\shell\PlayWithVLC" /f >nul 2>nul && echo   OK || echo   not found

echo Removing DropboxExt (files)...
reg delete "HKCR\*\shellex\ContextMenuHandlers\DropboxExt" /f >nul 2>nul && echo   OK || echo   not found

echo Removing DropboxExt (Directory)...
reg delete "HKCR\Directory\shellex\ContextMenuHandlers\DropboxExt" /f >nul 2>nul && echo   OK || echo   not found

echo Removing DropboxExt (Background)...
reg delete "HKCR\Directory\Background\shellex\ContextMenuHandlers\DropboxExt" /f >nul 2>nul && echo   OK || echo   not found

echo Removing PowerRenameExt (Background)...
reg delete "HKCR\Directory\Background\shellex\ContextMenuHandlers\PowerRenameExt" /f >nul 2>nul && echo   OK || echo   not found

echo Removing PowerRenameExt (AllFilesystemObjects)...
reg delete "HKCR\AllFilesystemObjects\shellex\ContextMenuHandlers\PowerRenameExt" /f >nul 2>nul && echo   OK || echo   not found

echo.
echo ===== DONE =====
echo All entries removed. Right-click context menu should be clean now.
echo To restore, double-click: c:\tbar_flip\ctx_menu_backup.reg
echo.
pause
