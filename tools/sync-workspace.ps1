# Mirrors the project documents from the Google Drive master folder into docs/project/.
#
# Dominus and Anima write the requirements, backlog and diagrams on Google Drive; the repo keeps
# copies so everything lives on GitHub too. Claude Code runs this at session start (see
# .claude/settings.json). It copies every Drive file whose content changed, reports Drive files
# it does not know yet, and tells Claude to commit. Codex.md is handled by sync-codex.ps1.
#
# Usage:  pwsh tools/sync-workspace.ps1
#         pwsh tools/sync-workspace.ps1 -Source <Drive project folder>
param(
    [string]$Source = $(if ($env:ODYSSEUS_WORKSPACE_SOURCE) { $env:ODYSSEUS_WORKSPACE_SOURCE }
                        else { 'G:\My Drive\~gamerrr\Project Odysseus' })
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$docs = Join-Path $repoRoot 'docs\project'

# Drive file name -> folder under docs/project/. Add new documents here.
$map = [ordered]@{
    'Project Odyssey.docx'                       = 'requirements'
    'Project Odyssey - MVP Backlog.xlsx'         = 'requirements'
    'Project Odyssey - Mraw Build Codex.docx'    = 'codex'
    'Anima Prompt Catalog (pending upload).html' = 'codex'
    'anima-SKILL (upload to claude.ai).md'       = 'codex'
    'Amek Workflow.png'                          = 'diagrams'
    'Odysseus - Architecture Diagram.png'        = 'diagrams'
    'Odysseus - MVP Timeline.png'                = 'diagrams'
    'Odysseus - Overview.md'                     = 'archive'
    'Odysseus - Council Review.xlsx'             = 'archive'
    'Project Odysseus - Requirements.xlsx'       = 'archive'
    'Top_50_Professions.xlsx'                    = 'archive'
}
# Handled elsewhere or not a real file: the Codex (sync-codex.ps1), Google-native shortcuts.
$ignored = @('Codex.md')

# No Drive on this machine (CI, another PC): nothing to mirror, stay silent.
if (-not (Test-Path -LiteralPath $Source)) { exit 0 }

$updated = @()
foreach ($name in $map.Keys) {
    $from = Join-Path $Source $name
    if (-not (Test-Path -LiteralPath $from)) { continue }
    $toDir = Join-Path $docs $map[$name]
    $to = Join-Path $toDir $name
    # Compare content, not dates: Drive sync touches timestamps without changing files.
    if ((Test-Path -LiteralPath $to) -and
        (Get-FileHash -LiteralPath $from).Hash -eq (Get-FileHash -LiteralPath $to).Hash) { continue }
    New-Item -ItemType Directory -Force -Path $toDir | Out-Null
    Copy-Item -LiteralPath $from -Destination $to -Force
    $updated += "docs/project/$($map[$name])/$name"
}

$unknown = Get-ChildItem -LiteralPath $Source -File |
    Where-Object { -not $map.Contains($_.Name) -and $ignored -notcontains $_.Name -and $_.Extension -notmatch '^\.g(sheet|doc|slides)$' } |
    ForEach-Object { $_.Name }

if (-not $updated -and -not $unknown) { exit 0 }

$notes = @()
if ($updated) { $notes += "updated $($updated.Count) file(s): $($updated -join ', ')" }
if ($unknown) { $notes += "new on Drive, not mirrored yet: $($unknown -join ', ')" }
$context = "Workspace sync from Google Drive: $($notes -join '; '). "
if ($updated) { $context += "Commit the updated files on main as 'Docs: sync workspace files from Drive' and push, and mention it in the assembly report. " }
if ($unknown) { $context += "For each new file, ask the owner which docs/project/ folder it belongs in, add it to the map in tools/sync-workspace.ps1 and to docs/project/README.md." }

@{
    systemMessage      = "Workspace sync: $($notes -join '; ')."
    hookSpecificOutput = @{ hookEventName = 'SessionStart'; additionalContext = $context }
} | ConvertTo-Json -Compress
