# The tester's standard check for a story (Codex L-01 step 7): configure, build, count warnings,
# run every test, and save the results as evidence.
#
# Who tests what (owner, 2026-10-01, D-46): this local check builds and tests Debug (with
# AddressSanitizer, the configuration that finds memory bugs); CI builds and tests Release on qa,
# and both on main. -Config Release or -Config Both runs the other configuration here too, for
# example to reproduce a CI failure.
#
# Usage:  pwsh tools/verify.ps1 -Story US-011 [-Config Debug|Release|Both]
# Exit code 0 only when every build has zero warning lines and every test passes.
param(
    [Parameter(Mandatory = $true)][string]$Story,
    [ValidateSet('Debug', 'Release', 'Both')][string]$Config = 'Debug'
)
$configs = if ($Config -eq 'Both') { @('debug', 'release') } else { @($Config.ToLower()) }

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

foreach ($config in $configs) {
    $log = Join-Path $logs "build-$config.log"
    cmake --build --preset "windows-x64-$config" *> $log
    $warnings = @(Select-String -Path $log -Pattern 'warning').Count
    Write-Output "build $config exit $LASTEXITCODE, warning lines $warnings"
    if ($LASTEXITCODE -ne 0 -or $warnings -ne 0) {
        $ok = $false
        Select-String -Path $log -Pattern 'error|warning' | Select-Object -First 15 | ForEach-Object { '  ' + $_.Line.Trim() }
    }
}

foreach ($config in $configs) {
    $log = Join-Path $logs "ctest-$config.log"
    ctest --preset "windows-x64-$config" --timeout 600 -LE soak *> $log
    $result = $LASTEXITCODE
    Write-Output "ctest $config exit $result"
    Get-Content $log | Select-String 'tests passed|\*\*\*' | ForEach-Object { '  ' + $_.Line.Trim() }
    if ($result -ne 0) { $ok = $false }
    $header = "$Story Windows ctest, $config ($(Get-Date -Format 'yyyy-MM-dd')). Build: $(if ($ok) { '0 warning lines' } else { 'see build logs' })."
    ($header, '', (Get-Content $log)) | Set-Content -Encoding utf8 (Join-Path $evidence "windows-$config.txt")
}

if ($ok) { Write-Output "VERIFIED $Story"; exit 0 } else { Write-Output "NOT VERIFIED $Story"; exit 1 }
