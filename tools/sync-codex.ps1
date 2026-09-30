# Keeps the repo's copy of Anima's Codex in step with the master copy on Google Drive.
#
# Anima publishes new Codex versions to Google Drive. Run this (the AI coding session also
# runs it automatically at session start, see .claude/settings.json) and it will:
#   - copy a newer Codex into docs/Codex.md,
#   - regenerate the files P-000 derives from it (CLAUDE.md from the Charter C-01,
#     .claude/agents/*.md from the roles R-01..R-07),
#   - tell the session to commit the sync and push.
# It never overwrites the repo with an older or unversioned change; it reports those instead.
#
# Usage:  pwsh tools/sync-codex.ps1            (source defaults to the Drive master copy)
#         pwsh tools/sync-codex.ps1 -Source <path to Codex.md>
param(
    [string]$Source = $(if ($env:ODYSSEUS_CODEX_SOURCE) { $env:ODYSSEUS_CODEX_SOURCE }
                        else { 'G:\My Drive\~gamerrr\Project Odysseus\Codex.md' })
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$target = Join-Path $repoRoot 'docs\Codex.md'

function Write-HookResult([string]$userMessage, [string]$agentContext) {
    # SessionStart hooks talk back as JSON: systemMessage is shown to the owner,
    # additionalContext is read by the AI agent.
    @{
        systemMessage      = $userMessage
        hookSpecificOutput = @{ hookEventName = 'SessionStart'; additionalContext = $agentContext }
    } | ConvertTo-Json -Compress
}

function Get-CodexVersion([string]$text) {
    if ($text -match '(?m)^# Project Odyssey Codex v(\d+)\.(\d+)') {
        return [version]"$($Matches[1]).$($Matches[2])"
    }
    return $null
}

# No Drive on this machine (CI, another PC): nothing to sync, stay silent.
if (-not (Test-Path -LiteralPath $Source)) { exit 0 }

# Compare with LF line endings: Git on Windows may check files out with CRLF.
$sourceText = [IO.File]::ReadAllText($Source) -replace "`r`n", "`n"
$targetText = if (Test-Path -LiteralPath $target) { [IO.File]::ReadAllText($target) -replace "`r`n", "`n" } else { '' }
if ($sourceText -ceq $targetText) { exit 0 }   # already in sync

$sourceVersion = Get-CodexVersion $sourceText
$targetVersion = Get-CodexVersion $targetText
if (-not $sourceVersion) {
    Write-HookResult "Codex sync skipped: $Source has no 'Codex vX.Y' title line." `
        "Codex sync skipped: the Drive Codex has no version line. Tell the owner; do not edit docs/Codex.md by hand."
    exit 0
}
if ($targetVersion -and $sourceVersion -lt $targetVersion) {
    Write-HookResult "Codex sync: repo has v$targetVersion, Drive still has v$sourceVersion. Copy docs/Codex.md to Drive to update the master." `
        "The repo Codex (v$targetVersion) is newer than the Drive master (v$sourceVersion). Keep using docs/Codex.md and remind the owner to update the Drive copy."
    exit 0
}
if ($targetVersion -and $sourceVersion -eq $targetVersion) {
    Write-HookResult "Codex sync skipped: the Drive Codex changed but is still v$sourceVersion. Anima must bump the version." `
        "The Drive Codex differs from docs/Codex.md but both say v$sourceVersion. Do not sync; raise a codex issue asking Anima to bump the version."
    exit 0
}

# Newer version: take it, then regenerate what P-000 derives from it, verbatim.
[IO.File]::WriteAllText($target, $sourceText)
$regenerated = @()

# Writes a derived file only when its text changed, and remembers which ones did.
function Update-DerivedFile([string]$relativePath, [string]$text) {
    $path = Join-Path $repoRoot $relativePath
    $current = if (Test-Path -LiteralPath $path) { [IO.File]::ReadAllText($path) -replace "`r`n", "`n" } else { $null }
    if ($current -cne $text) {
        [IO.File]::WriteAllText($path, $text)
        $script:regenerated += $relativePath
    }
}

$charter = [regex]::Match($sourceText, '(?s)## 2\. Charter \(C-01\)\n.*?```markdown\n(.*?)\n```\n')
if ($charter.Success) { Update-DerivedFile 'CLAUDE.md' ($charter.Groups[1].Value + "`n") }
$roles = [regex]::Matches($sourceText, '(?s)### (R-\d+) (mraw-[a-z]+) .*?\n```markdown\n(.*?)\n```\n')
foreach ($role in $roles) {
    Update-DerivedFile ".claude/agents/$($role.Groups[2].Value).md" ($role.Groups[3].Value + "`n")
}

$from = if ($targetVersion) { "v$targetVersion" } else { 'none' }
$files = ($regenerated + 'docs/Codex.md') -join ', '
Write-HookResult "Codex synced from Anima: $from -> v$sourceVersion ($files)." `
    ("Anima published Codex v$sourceVersion (was $from). tools/sync-codex.ps1 updated: $files. " +
     "Before any other work: read the new amendment log in docs/Codex.md, commit these files on main as " +
     "'Codex v${sourceVersion}: sync from Anima', push, and mention the sync in the assembly report. " +
     "If new prompts were added, add them to docs/status.md in Codex order.")
