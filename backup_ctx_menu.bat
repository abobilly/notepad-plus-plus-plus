@echo off
REM ============================================================
REM BACKUP context menu registry keys before removal
REM Run this BEFORE running remove_ctx_menu.reg
REM Creates: c:\tbar_flip\ctx_menu_backup.reg
REM ============================================================

set BACKUP=c:\tbar_flip\ctx_menu_backup.reg

echo Windows Registry Editor Version 5.00 > "%BACKUP%"
echo. >> "%BACKUP%"
echo ; Context menu backup - created %date% %time% >> "%BACKUP%"
echo. >> "%BACKUP%"

echo Exporting TreeSize Free...
reg export "HKCR\Folder\shell\TreeSize Free" "%TEMP%\ts.reg" /y >nul 2>nul
if exist "%TEMP%\ts.reg" type "%TEMP%\ts.reg" >> "%BACKUP%"

echo Exporting Git GUI (Directory)...
reg export "HKCR\Directory\shell\git_gui" "%TEMP%\gg1.reg" /y >nul 2>nul
if exist "%TEMP%\gg1.reg" type "%TEMP%\gg1.reg" >> "%BACKUP%"

echo Exporting Git Shell (Directory)...
reg export "HKCR\Directory\shell\git_shell" "%TEMP%\gs1.reg" /y >nul 2>nul
if exist "%TEMP%\gs1.reg" type "%TEMP%\gs1.reg" >> "%BACKUP%"

echo Exporting Git GUI (Background)...
reg export "HKCR\Directory\Background\shell\git_gui" "%TEMP%\gg2.reg" /y >nul 2>nul
if exist "%TEMP%\gg2.reg" type "%TEMP%\gg2.reg" >> "%BACKUP%"

echo Exporting Git Shell (Background)...
reg export "HKCR\Directory\Background\shell\git_shell" "%TEMP%\gs2.reg" /y >nul 2>nul
if exist "%TEMP%\gs2.reg" type "%TEMP%\gs2.reg" >> "%BACKUP%"

echo Exporting VLC AddToPlaylist...
reg export "HKCR\Directory\shell\AddToPlaylistVLC" "%TEMP%\vlc1.reg" /y >nul 2>nul
if exist "%TEMP%\vlc1.reg" type "%TEMP%\vlc1.reg" >> "%BACKUP%"

echo Exporting VLC PlayWith...
reg export "HKCR\Directory\shell\PlayWithVLC" "%TEMP%\vlc2.reg" /y >nul 2>nul
if exist "%TEMP%\vlc2.reg" type "%TEMP%\vlc2.reg" >> "%BACKUP%"

echo Exporting DropboxExt (files)...
reg export "HKCR\*\shellex\ContextMenuHandlers\DropboxExt" "%TEMP%\db1.reg" /y >nul 2>nul
if exist "%TEMP%\db1.reg" type "%TEMP%\db1.reg" >> "%BACKUP%"

echo Exporting DropboxExt (Directory)...
reg export "HKCR\Directory\shellex\ContextMenuHandlers\DropboxExt" "%TEMP%\db2.reg" /y >nul 2>nul
if exist "%TEMP%\db2.reg" type "%TEMP%\db2.reg" >> "%BACKUP%"

echo Exporting DropboxExt (Background)...
reg export "HKCR\Directory\Background\shellex\ContextMenuHandlers\DropboxExt" "%TEMP%\db3.reg" /y >nul 2>nul
if exist "%TEMP%\db3.reg" type "%TEMP%\db3.reg" >> "%BACKUP%"

echo Exporting PowerRenameExt (Background)...
reg export "HKCR\Directory\Background\shellex\ContextMenuHandlers\PowerRenameExt" "%TEMP%\pr1.reg" /y >nul 2>nul
if exist "%TEMP%\pr1.reg" type "%TEMP%\pr1.reg" >> "%BACKUP%"

echo Exporting PowerRenameExt (AllFilesystemObjects)...
reg export "HKCR\AllFilesystemObjects\shellex\ContextMenuHandlers\PowerRenameExt" "%TEMP%\pr2.reg" /y >nul 2>nul
if exist "%TEMP%\pr2.reg" type "%TEMP%\pr2.reg" >> "%BACKUP%"

echo.
echo Backup saved to: %BACKUP%
echo Now you can safely run remove_ctx_menu.reg
