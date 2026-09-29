# The tester's standard check for a story (Codex L-01 step 7): configure, build Debug and
# Release, count warnings, run every test in both, and save the results as evidence.
#
# Usage:  pwsh tools/verify.ps1 -Story US-011
# Exit code 0 only when both builds have zero warning lines and every test passes.
param(
    [Parameter(Mandatory = $true)][string]$Story
)

$ErrorActionPreference = 'Continue'
$repo = Split-Path -Parent $PSScriptRoot
Set-Location $repo
$env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')
if (-not $env:VCPKG_ROOT) { $env:VCPKG_ROOT = [Environment]::GetEnvironmentVariable('VCPKG_ROOT', 'User') }

$evidence = Join-Path $repo "docs/evidence/$Story"
$logs = Join-Path $repo 'build/verify-logs'
New-Item -ItemType Directory -Force $evidence, $logs | Out-Null
$ok = $true

cmake --preset windows-x64-debug *> (Join-Path $logs 'configure.log')
if ($LASTEXITCODE -ne 0) { Write-Output 'configure FAILED'; Get-Content (Join-Path $logs 'configure.log') -Tail 20; exit 1 }

foreach ($config in 'debug', 'release') {
    $log = Join-Path $logs "build-$config.log"
    cmake --build --preset "windows-x64-$config" *> $log
    $warnings = @(Select-String -Path $log -Pattern 'warning').Count
    Write-Output "build $config exit $LASTEXITCODE, warning lines $warnings"
    if ($LASTEXITCODE -ne 0 -or $warnings -ne 0) {
        $ok = $false
        Select-String -Path $log -Pattern 'error|warning' | Select-Object -First 15 | ForEach-Object { '  ' + $_.Line.Trim() }
    }
}

foreach ($config in 'debug', 'release') {
    $log = Join-Path $logs "ctest-$config.log"
    ctest --preset "windows-x64-$config" --timeout 600 *> $log
    $result = $LASTEXITCODE
    Write-Output "ctest $config exit $result"
    Get-Content $log | Select-String 'tests passed|\*\*\*' | ForEach-Object { '  ' + $_.Line.Trim() }
    if ($result -ne 0) { $ok = $false }
    $header = "$Story Windows ctest, $config ($(Get-Date -Format 'yyyy-MM-dd')). Build: $(if ($ok) { '0 warning lines' } else { 'see build logs' })."
    ($header, '', (Get-Content $log)) | Set-Content -Encoding utf8 (Join-Path $evidence "windows-$config.txt")
}

if ($ok) { Write-Output "VERIFIED $Story"; exit 0 } else { Write-Output "NOT VERIFIED $Story"; exit 1 }
