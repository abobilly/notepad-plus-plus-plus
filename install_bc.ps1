$pluginDir = "C:\Program Files\Notepad++\plugins"
# Install BracketsCheck
$bcDir = Join-Path $pluginDir "BracketsCheck"
New-Item -ItemType Directory -Path $bcDir -Force | Out-Null
Copy-Item "c:\tbar_flip\plugin_temp\BracketsCheck\BracketsCheck.dll" -Destination $bcDir -Force
if (Test-Path (Join-Path $bcDir "BracketsCheck.dll")) { Write-Output "BracketsCheck installed OK" } else { Write-Output "BracketsCheck FAILED" }
