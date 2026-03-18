$out = @()

# Shell keys (TreeSize, Git, VLC)
$shellPaths = @(
    'Registry::HKEY_CLASSES_ROOT\Directory\shell',
    'Registry::HKEY_CLASSES_ROOT\Directory\Background\shell',
    'Registry::HKEY_CLASSES_ROOT\Drive\shell',
    'Registry::HKEY_CLASSES_ROOT\Folder\shell'
)
foreach ($p in $shellPaths) {
    if (Test-Path $p) {
        Get-ChildItem $p -ErrorAction SilentlyContinue | ForEach-Object {
            $n = $_.PSChildName
            if ($n -match 'TreeSize|git_gui|git_shell|AddToPlaylist|PlayWith') {
                $out += "SHELL: $($_.Name)"
            }
        }
    }
}

# File-level shell keys (VLC on files)
$filePaths = @(
    'Registry::HKEY_CLASSES_ROOT\*\shell'
)
foreach ($p in $filePaths) {
    if (Test-Path $p) {
        Get-ChildItem $p -ErrorAction SilentlyContinue | ForEach-Object {
            $n = $_.PSChildName
            if ($n -match 'AddToPlaylist|PlayWith') {
                $out += "FILE_SHELL: $($_.Name)"
            }
        }
    }
}

# Shell extension handlers (Dropbox, PowerRename)
$shellexPaths = @(
    'Registry::HKEY_CLASSES_ROOT\*\shellex\ContextMenuHandlers',
    'Registry::HKEY_CLASSES_ROOT\Directory\shellex\ContextMenuHandlers',
    'Registry::HKEY_CLASSES_ROOT\Directory\Background\shellex\ContextMenuHandlers',
    'Registry::HKEY_CLASSES_ROOT\Folder\shellex\ContextMenuHandlers',
    'Registry::HKEY_CLASSES_ROOT\AllFilesystemObjects\shellex\ContextMenuHandlers'
)
foreach ($p in $shellexPaths) {
    if (Test-Path $p) {
        Get-ChildItem $p -ErrorAction SilentlyContinue | ForEach-Object {
            $n = $_.PSChildName
            if ($n -match 'Dropbox|PowerRename') {
                $out += "SHELLEX: $($_.Name)"
            }
        }
    }
}

# Also check HKLM equivalents
$hklmShellPaths = @(
    'HKLM:\SOFTWARE\Classes\Directory\shell',
    'HKLM:\SOFTWARE\Classes\Directory\Background\shell',
    'HKLM:\SOFTWARE\Classes\Drive\shell',
    'HKLM:\SOFTWARE\Classes\Folder\shell'
)
foreach ($p in $hklmShellPaths) {
    if (Test-Path $p) {
        Get-ChildItem $p -ErrorAction SilentlyContinue | ForEach-Object {
            $n = $_.PSChildName
            if ($n -match 'TreeSize|git_gui|git_shell|AddToPlaylist|PlayWith') {
                $out += "HKLM_SHELL: $($_.Name)"
            }
        }
    }
}

# HKLM shell extensions
$hklmShellexPaths = @(
    'HKLM:\SOFTWARE\Classes\*\shellex\ContextMenuHandlers',
    'HKLM:\SOFTWARE\Classes\Directory\shellex\ContextMenuHandlers',
    'HKLM:\SOFTWARE\Classes\Directory\Background\shellex\ContextMenuHandlers',
    'HKLM:\SOFTWARE\Classes\AllFilesystemObjects\shellex\ContextMenuHandlers'
)
foreach ($p in $hklmShellexPaths) {
    if (Test-Path $p) {
        Get-ChildItem $p -ErrorAction SilentlyContinue | ForEach-Object {
            $n = $_.PSChildName
            if ($n -match 'Dropbox|PowerRename') {
                $out += "HKLM_SHELLEX: $($_.Name)"
            }
        }
    }
}

# Also check for Dropbox context menu via CLSID approach 
# Known Dropbox CLSIDs
$dropboxClsids = @(
    'Registry::HKEY_CLASSES_ROOT\CLSID\{ECD97DE5-3C8F-4ACB-AEEE-CCAB78F7711C}',
    'Registry::HKEY_CLASSES_ROOT\CLSID\{FB314ED9-A251-47B7-93E1-CDD82E34AF8B}',
    'Registry::HKEY_CLASSES_ROOT\CLSID\{FB314EDA-A251-47B7-93E1-CDD82E34AF8B}',
    'Registry::HKEY_CLASSES_ROOT\CLSID\{FB314EDB-A251-47B7-93E1-CDD82E34AF8B}'
)
foreach ($c in $dropboxClsids) {
    if (Test-Path $c) {
        $out += "DROPBOX_CLSID: $c"
    }
}

$out | Set-Content 'c:\tbar_flip\ctx_found.txt'
Write-Host "Done. Found $($out.Count) entries."
