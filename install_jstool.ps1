$pluginDir = "C:\Program Files\Notepad++\plugins"
$jsDir = Join-Path $pluginDir "JSMinNPP"
New-Item -ItemType Directory -Path $jsDir -Force | Out-Null
Copy-Item "c:\tbar_flip\plugin_temp\JSMinNPP\JSMinNPP.dll" -Destination $jsDir -Force
if (Test-Path (Join-Path $jsDir "JSMinNPP.dll")) { Write-Output "JSTool installed OK" } else { Write-Output "JSTool FAILED" }
