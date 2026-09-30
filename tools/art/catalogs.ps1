# Writes the M2d catalogs in assets/data/ (weapons, plants, animals, effects, weather) from the
# item names in assets/sprites/content-cuts.json (US-130). First numbers, meant to be tuned by
# hand in the JSON afterwards; run again only to start over.
$ErrorActionPreference = 'Stop'
$root = Resolve-Path "$PSScriptRoot\..\.."
$cuts = (Get-Content "$root\assets\sprites\content-cuts.json" -Raw | ConvertFrom-Json).cuts
function Names($page) { @($cuts | Where-Object { $_.page -eq $page } | ForEach-Object { $_.name }) }
function Has($name, $words) { foreach ($w in $words) { if ($name.Contains($w)) { return $true } }; $false }
function Write-Json($file, $key, $items) {
  $lines = $items | ForEach-Object { '    ' + ($_ | ConvertTo-Json -Compress) }
  $text = "{`n  `"$key`": [`n" + ($lines -join ",`n") + "`n  ]`n}`n"
  [IO.File]::WriteAllText("$root\assets\data\$file", $text, (New-Object Text.UTF8Encoding($false)))
}

# ---- Weapons ----
$classes = [ordered]@{
  sword = @{ damage = 5; speed = 2.0; range = 1.5 }; axe = @{ damage = 9; speed = 1.0; range = 1.4 }
  spear = @{ damage = 6; speed = 1.5; range = 2.2 }; bow = @{ damage = 5; speed = 1.2; range = 10.0 }
  thrown = @{ damage = 4; speed = 2.0; range = 7.0 }; whip = @{ damage = 4; speed = 1.8; range = 2.8 }
  staff = @{ damage = 6; speed = 1.2; range = 8.0 }; gun = @{ damage = 7; speed = 2.5; range = 12.0 } }
$starters = 'iron sword','flame sword','steel battle axe','frost axe','iron spear','lightning spear','wooden longbow','venom recurve',
            'throwing knives','void chakram','iron flail','fire whip','nature staff','void staff','flintlock pistol','venom pistol'
$weapons = foreach ($n in (Names 'icons')) {
  $class = if (Has $n 'bow','recurve') { 'bow' }
    elseif (Has $n 'morning star','ball and chain','whip','flail','chain','kusarigama','nunchaku') { 'whip' }
    elseif (Has $n 'knives','shuriken','chakram','ring blade','boomerang','star','disc') { 'thrown' }
    elseif (Has $n 'staff','wand') { 'staff' }
    elseif (Has $n 'spear','halberd','glaive','scythe','trident','lance') { 'spear' }
    elseif (Has $n 'axe','hammer','maul','mace','fist') { 'axe' }
    elseif (Has $n 'sword','blade','katana','rapier','scimitar','cleaver','dagger','claw') { 'sword' }
    else { 'gun' }
  $element = if (Has $n 'flame','fire','inferno') { 'fire' } elseif (Has $n 'frost','ice') { 'ice' }
    elseif (Has $n 'lightning','storm','thunder','shock') { 'lightning' } elseif (Has $n 'venom','poison','bio') { 'poison' }
    elseif (Has $n 'void','shadow','black hole','dark') { 'void' } else { 'none' }
  $era = if ($class -eq 'gun' -or (Has $n 'energy','plasma','laser','ion','sonic','drone','gravity','power fist')) { 'future' } else { 'fantasy' }
  $s = $classes[$class]
  [ordered]@{ name = $n; frame = $n; class = $class; element = $element; era = $era; starter = ($starters -contains $n); damage = $s.damage; speed = $s.speed; range = $s.range }
}
$missing = $starters | Where-Object { ($weapons | ForEach-Object { $_.name }) -notcontains $_ }
if ($missing) { throw "starters not in the sheets: $missing" }
Write-Json 'weapons.json' 'weapons' $weapons

# ---- Plants ----
$blocking = 'bush','bamboo','cactus','raspberry bush','blackberry bush','blueberry bush','grape vine'
$edible = 'olive tree','lemon tree','orange tree','apple tree','pear tree','peach tree','plum tree','cherry tree','fig tree','date palm','coconut palm',
          'wheat','rice','corn','carrot top','onion grass','garlic','potato plant','tomato plant','pepper plant','strawberry','sugarcane',
          'raspberry bush','blackberry bush','cranberry','blueberry bush','grape vine','brown mushroom','tall mushroom','truffle','morel'
$plants = foreach ($c in ($cuts | Where-Object { $_.page -match 'plants|trees' })) {
  $n = $c.name; $isTree = $c.page -eq 'trees'
  $text = if ($edible -contains $n) { "Something to eat grows here." } elseif ($n -match 'mushroom|morel|fungus') { "Better not eat this one." }
    elseif ($isTree) { "A sturdy tree. Its wood could be useful one day." } elseif ($n -match 'grass|moss|clover|fern|ivy|vine|reed|cattail|papyrus') { "Green and quiet." }
    else { "Its flowers smell sweet." }
  [ordered]@{ name = $n; frame = $n; size = ($c.page -replace 'plants-', '' -replace 'trees', 'tree'); blocks = ($isTree -or $blocking -contains $n); edible = ($edible -contains $n); inspect = $text }
}
Write-Json 'plants.json' 'plants' $plants

# ---- Animals ----
$big = 'bear','lion','tiger','rhino','hippopotamus','elephant','bull','buffalo','water buffalo','bison','moose','giraffe','camel','yak','horse','cow','elk'
$small = 'fox','cat','rabbit','squirrel','raccoon','badger','beaver','otter','skunk','porcupine','dog','jackal','lynx'
$enemies = 'grey wolf','fox','bear','cougar','lynx','leopard','jaguar','cheetah','lion','tiger','snow leopard','hyena','jackal','boar','wild pig','rhino','hippopotamus','bull','buffalo','water buffalo'
$animals = foreach ($n in (Names 'animals')) {
  $hp = if ($big -contains $n) { 150 } elseif ($small -contains $n) { 40 } else { 80 }
  $hit = if ($big -contains $n) { 18 } elseif ($small -contains $n) { 6 } else { 10 }
  $enemy = $enemies -contains $n
  [ordered]@{ name = $n; frame = $n; hp = $hp; enemy = $enemy; strikeDamage = $(if ($enemy) { $hit } else { 0 }); reach = 1.5 }
}
Write-Json 'animals.json' 'animals' $animals

# ---- Effects ----
$loops = 'flame','big fire','whirlpool','electric ring','poison cloud','toxic bubble','magic circle','portal idle','death aura','dark mist','fireflies','butterfly swarm','galaxy spiral','time warp','space rift','black hole','reality distortion'
$effects = foreach ($c in ($cuts | Where-Object { $_.page -eq 'effects' })) {
  $count = if ($c.frameRects) { @($c.frameRects).Count } else { 1 }
  [ordered]@{ name = $c.name; frames = $count; ticksPerFrame = 3; loop = ($loops -contains $c.name) }
}
Write-Json 'effects.json' 'effects' $effects

# ---- Weather ----
$soft = 'fog','mist','haze','cloud','smog','smoke','steam','overcast','gloom','whiteout','sky','front','bank'
$weather = @([ordered]@{ name = 'clear'; frames = 0; ticksPerFrame = 4; weight = 50; blend = 'alpha' })
$weather += foreach ($n in (Names 'weather')) { [ordered]@{ name = $n; frames = 4; ticksPerFrame = 4; weight = 1; blend = $(if (Has $n $soft) { 'alpha' } else { 'add' }) } }
Write-Json 'weather.json' 'weather' $weather

"weapons $(@($weapons).Count) (starters $(@($weapons | Where-Object { $_.starter }).Count)), plants $(@($plants).Count), animals $(@($animals).Count), effects $(@($effects).Count), weather $(@($weather).Count)"
$weapons | Group-Object { $_.class } | ForEach-Object { "$($_.Name) $($_.Count)" }
