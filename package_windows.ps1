<#
.SYNOPSIS
  Create a distributable folder containing an .exe and its DLLs for Windows.

#>
param(
    [Parameter(Mandatory=$true)]
    [string]$ExePath,
    [string]$OutDir = "dist"
)

if (-not (Test-Path $ExePath)) {
    Write-Error "Executable not found: $ExePath"
    exit 1
}

New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
Copy-Item -Path $ExePath -Destination $OutDir -Force

function Get-Dependencies-DumpBin {
    # Use dumpbin if available (Visual Studio) to list dependents
    $dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
    if ($null -eq $dumpbin) { return @() }
    $out = & dumpbin /DEPENDENTS $ExePath 2>$null
    return $out | ForEach-Object { $_.Trim() } | Where-Object { $_ -match '\.dll$' }
}

$deps = Get-Dependencies-DumpBin
if ($deps.Count -eq 0) {
    Write-Output "No dumpbin available or no deps found. You may need to run this from MSYS2 for MinGW builds."
}

foreach ($d in $deps) {
    # Try to find in PATH or common locations
    $bn = Split-Path $d -Leaf
    $candidates = @("$env:ProgramFiles\mingw-w64\bin\$bn", "$env:ProgramFiles(x86)\mingw-w64\bin\$bn", "C:\msys64\mingw64\bin\$bn")
    $found = $null
    foreach ($c in $candidates) { if (Test-Path $c) { $found = $c; break } }
    if (-not $found) { $found = (Get-Command $bn -ErrorAction SilentlyContinue)?.Source }
    if ($found) { Copy-Item -Path $found -Destination $OutDir -Force }
    else { Write-Warning "Could not locate $bn; it may be a system DLL" }
}

Write-Output "Files in $OutDir:"
Get-ChildItem $OutDir | ForEach-Object { Write-Output " - $($_.Name)" }

try {
    $zipfile = "$OutDir.zip"
    Compress-Archive -Path "$OutDir\*" -DestinationPath $zipfile -Force
    Write-Output "Created $zipfile"
} catch {
    Write-Warning "Failed to create zip via Compress-Archive; files left in $OutDir"
}
