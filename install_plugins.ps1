$pluginsBase = "C:\Program Files\Notepad++\plugins"
$tempDir = "c:\tbar_flip\plugin_temp"

# Helper function
function Install-NppPlugin {
    param([string]$Repo, [string]$Pattern, [string]$PluginName, [string]$DllName)
    Write-Host "--- Installing $PluginName ---"
    
    # Clean previous downloads
    Remove-Item "$tempDir\*.zip" -Force -ErrorAction SilentlyContinue
    Remove-Item "$tempDir\extract" -Recurse -Force -ErrorAction SilentlyContinue
    
    # Download
    & gh release download --repo $Repo --pattern $Pattern --dir $tempDir 2>&1
    
    $zip = Get-ChildItem "$tempDir\*.zip" | Select-Object -First 1
    if (-not $zip) {
        Write-Host "  FAILED: No zip found for $PluginName"
        return
    }
    
    # Extract
    Expand-Archive $zip.FullName -DestinationPath "$tempDir\extract" -Force
    
    # Find the DLL
    $dll = Get-ChildItem "$tempDir\extract" -Recurse -Filter "$DllName" | Select-Object -First 1
    if (-not $dll) {
        Write-Host "  FAILED: DLL not found for $PluginName"
        Get-ChildItem "$tempDir\extract" -Recurse -File | Select-Object Name
        return
    }
    
    # Install
    $destDir = "$pluginsBase\$PluginName"
    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    Copy-Item $dll.FullName "$destDir\" -Force
    
    # Copy any extra DLLs from same dir (dependencies)
    $extraDlls = Get-ChildItem $dll.DirectoryName -Filter "*.dll" | Where-Object { $_.Name -ne $DllName }
    foreach ($ed in $extraDlls) {
        Copy-Item $ed.FullName "$destDir\" -Force
    }
    
    if (Test-Path "$destDir\$DllName") {
        Write-Host "  OK: $PluginName installed"
    } else {
        Write-Host "  FAILED: $PluginName"
    }
}

Write-Host "========== A-TIER PLUGINS =========="

# 1. NppFTP (already downloaded, just copy)
$destDir = "$pluginsBase\NppFTP"
New-Item -ItemType Directory -Path $destDir -Force | Out-Null
Copy-Item "$tempDir\NppFTP_extract\NppFTP.dll" "$destDir\" -Force
if (Test-Path "$destDir\NppFTP.dll") { Write-Host "  OK: NppFTP installed" } else { Write-Host "  FAILED: NppFTP" }

# 2. Compare
Install-NppPlugin -Repo "pnedev/compare-plugin" -Pattern "*x64*" -PluginName "ComparePlugin" -DllName "ComparePlugin.dll"

# 3. XML Tools
Install-NppPlugin -Repo "morbac/xmltools" -Pattern "*x64*" -PluginName "XMLTools" -DllName "XMLTools.dll"

# 4. JSTool
Install-NppPlugin -Repo "nicedoc/JSTool" -Pattern "*uni*" -PluginName "JSMinNPP" -DllName "JSMinNPP.dll"
# JSTool dll is sometimes named JSMinNPP - try both patterns
if (-not (Test-Path "$pluginsBase\JSMinNPP\JSMinNPP.dll")) {
    Remove-Item "$tempDir\*.zip","$tempDir\extract" -Recurse -Force -ErrorAction SilentlyContinue
    Install-NppPlugin -Repo "nicedoc/JSTool" -Pattern "*64*" -PluginName "JSMinNPP" -DllName "JSMinNPP.dll"
}

# 5. AutoSave - already installed!
Write-Host "  OK: AutoSave already installed"

# 6. Explorer
Install-NppPlugin -Repo "oviradoi/npp-explorer-plugin" -Pattern "*x64*" -PluginName "Explorer" -DllName "Explorer.dll"

Write-Host ""
Write-Host "========== B-TIER PLUGINS =========="

# 7. MarkdownViewer++
Install-NppPlugin -Repo "nea/MarkdownViewerPlusPlus" -Pattern "*x64*" -PluginName "MarkdownViewerPlusPlus" -DllName "MarkdownViewerPlusPlus.dll"

# 8. NppExec
Install-NppPlugin -Repo "d0vgan/nppexec" -Pattern "*x64*dll*" -PluginName "NppExec" -DllName "NppExec.dll"

# 9. EditorConfig
Install-NppPlugin -Repo "editorconfig/editorconfig-notepad-plus-plus" -Pattern "*x64*" -PluginName "NppEditorConfig" -DllName "NppEditorConfig.dll"

# 10. BracketsCheck - included in mimeTools or available separately
# Try downloading
Install-NppPlugin -Repo "nicedoc/JSTool" -Pattern "*uni*" -PluginName "BracketsCheck" -DllName "BracketsCheck.dll"

Write-Host ""
Write-Host "========== SUMMARY =========="
Get-ChildItem $pluginsBase -Directory | ForEach-Object {
    $dlls = Get-ChildItem $_.FullName -Filter "*.dll" -ErrorAction SilentlyContinue
    if ($dlls) { Write-Host "  [OK] $($_.Name)" } else { Write-Host "  [--] $($_.Name) (no DLL)" }
}
