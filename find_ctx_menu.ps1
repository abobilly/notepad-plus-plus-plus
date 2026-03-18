$out = @()

# TreeSize
$treeSizeKeys = @(
    'HKLM:\SOFTWARE\Classes\Directory\shell\TreeSizeFree',
    'HKLM:\SOFTWARE\Classes\Directory\Background\shell\TreeSizeFree',
    'HKLM:\SOFTWARE\Classes\Drive\shell\TreeSizeFree',
    'HKLM:\SOFTWARE\Classes\Folder\shell\TreeSizeFree'
)
foreach ($k in $treeSizeKeys) { if (Test-Path $k) { $out += "TREE: $k" } }

# Git  
$gitKeys = @(
    'HKLM:\SOFTWARE\Classes\Directory\shell\git_gui',
    'HKLM:\SOFTWARE\Classes\Directory\shell\git_shell',
    'HKLM:\SOFTWARE\Classes\Directory\Background\shell\git_gui',
    'HKLM:\SOFTWARE\Classes\Directory\Background\shell\git_shell'
)
foreach ($k in $gitKeys) { if (Test-Path $k) { $out += "GIT: $k" } }

# VLC
$vlcKeys = @(
    'HKLM:\SOFTWARE\Classes\Directory\shell\AddtoPlaylistVLC',
    'HKLM:\SOFTWARE\Classes\Directory\shell\PlayWithVLC'
)
foreach ($k in $vlcKeys) { if (Test-Path $k) { $out += "VLC: $k" } }

# Dropbox shell extensions
$dbKeys = @(
    'HKLM:\SOFTWARE\Classes\*\shellex\ContextMenuHandlers\DropboxExt',
    'HKLM:\SOFTWARE\Classes\Directory\shellex\ContextMenuHandlers\DropboxExt',
    'HKLM:\SOFTWARE\Classes\Folder\shellex\ContextMenuHandlers\DropboxExt',
    'HKLM:\SOFTWARE\Classes\Directory\Background\shellex\ContextMenuHandlers\DropboxExt'
)
foreach ($k in $dbKeys) { if (Test-Path $k) { $out += "DROPBOX: $k" } }

# PowerRename (PowerToys)
$prKeys = @(
    'HKLM:\SOFTWARE\Classes\*\shellex\ContextMenuHandlers\PowerRenameExt',
    'HKLM:\SOFTWARE\Classes\Directory\shellex\ContextMenuHandlers\PowerRenameExt',
    'HKLM:\SOFTWARE\Classes\Folder\shellex\ContextMenuHandlers\PowerRenameExt',
    'HKLM:\SOFTWARE\Classes\AllFilesystemObjects\shellex\ContextMenuHandlers\PowerRenameExt'
)
foreach ($k in $prKeys) { if (Test-Path $k) { $out += "POWERRENAME: $k" } }

# Also check CLSID-based for modern Windows 11 context menu
$clsidKeys = @(
    'HKCR:\Directory\shell\git_gui',
    'HKCR:\Directory\shell\git_shell',
    'HKCR:\Directory\Background\shell\git_gui',
    'HKCR:\Directory\Background\shell\git_shell'
)

# Check via registry provider with HKEY_CLASSES_ROOT
$hkcrPath = 'Registry::HKEY_CLASSES_ROOT'
$morePaths = @(
    "$hkcrPath\Directory\shell\git_gui",
    "$hkcrPath\Directory\shell\git_shell",
    "$hkcrPath\Directory\Background\shell\git_gui",
    "$hkcrPath\Directory\Background\shell\git_shell",
    "$hkcrPath\Directory\shell\AddtoPlaylistVLC",
    "$hkcrPath\Directory\shell\PlayWithVLC",
    "$hkcrPath\Directory\shell\TreeSizeFree",
    "$hkcrPath\Directory\Background\shell\TreeSizeFree",
    "$hkcrPath\Drive\shell\TreeSizeFree",
    "$hkcrPath\Folder\shell\TreeSizeFree"
)
foreach ($k in $morePaths) { if (Test-Path $k) { $out += "HKCR: $k" } }

# Dropbox via HKCR
$dbHkcr = @(
    "$hkcrPath\*\shellex\ContextMenuHandlers\DropboxExt",
    "$hkcrPath\Directory\shellex\ContextMenuHandlers\DropboxExt",
    "$hkcrPath\Folder\shellex\ContextMenuHandlers\DropboxExt",
    "$hkcrPath\Directory\Background\shellex\ContextMenuHandlers\DropboxExt"
)
foreach ($k in $dbHkcr) { if (Test-Path $k) { $out += "DROPBOX-HKCR: $k" } }

# PowerRename via HKCR
$prHkcr = @(
    "$hkcrPath\*\shellex\ContextMenuHandlers\PowerRenameExt",
    "$hkcrPath\Directory\shellex\ContextMenuHandlers\PowerRenameExt",
    "$hkcrPath\Folder\shellex\ContextMenuHandlers\PowerRenameExt",
    "$hkcrPath\AllFilesystemObjects\shellex\ContextMenuHandlers\PowerRenameExt"
)
foreach ($k in $prHkcr) { if (Test-Path $k) { $out += "RENAME-HKCR: $k" } }

# VLC via file-level context menus
$vlcFile = @(
    "$hkcrPath\*\shell\AddtoPlaylistVLC",
    "$hkcrPath\*\shell\PlayWithVLC"
)
foreach ($k in $vlcFile) { if (Test-Path $k) { $out += "VLC-FILE: $k" } }

$out | Out-File "c:\tbar_flip\ctx_menu_results.txt" -Encoding UTF8
"Found $($out.Count) entries" | Out-File "c:\tbar_flip\ctx_menu_results.txt" -Append -Encoding UTF8
