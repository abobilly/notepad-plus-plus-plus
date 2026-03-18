$pluginsBase = "C:\Program Files\Notepad++\plugins"
$stage = "c:\tbar_flip\plugin_stage"

# Copy each staged plugin to Notepad++
Get-ChildItem $stage -Directory | ForEach-Object {
    $dest = "$pluginsBase\$($_.Name)"
    New-Item -ItemType Directory -Path $dest -Force | Out-Null
    Copy-Item "$($_.FullName)\*" $dest -Force -Recurse
    if (Test-Path "$dest\$($_.Name).dll") {
        Write-Host "[OK] $($_.Name)"
    } else {
        Write-Host "[??] $($_.Name) - checking..."
        Get-ChildItem $dest -Filter "*.dll" | ForEach-Object { Write-Host "     Found: $($_.Name)" }
    }
}
