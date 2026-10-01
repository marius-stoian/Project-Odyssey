# Kept for reruns: measures the sheets with Sheet.cs (load.ps1 reads a PNG) and writes content-cuts.json.
$ErrorActionPreference = 'Stop'
# Measures the seven M2d sheets and writes assets/sprites/content-cuts.json (US-130).
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$sp = (Resolve-Path (Join-Path $here '..\..\assets\sprites')).Path
$cuts = New-Object System.Collections.Generic.List[object]
$used = @{}
function Unique([string]$name) { $n = $name.ToLower(); $k = $n; $i = 2; while ($used.ContainsKey($k)) { $k = "$n $i"; $i++ }; $used[$k] = 1; $k }
function Cut($name, $page, $sheet, $rect, $key, $extra) {
  $c = [ordered]@{ name = $name; page = $page; sheet = $sheet; rect = @($rect); key = $key }
  if ($extra) { foreach ($k in $extra.Keys) { $c[$k] = $extra[$k] } }
  $cuts.Add($c)
}
function Inset($x1, $y1, $x2, $y2, $d) { @(($x1 + $d), ($y1 + $d), ($x2 - $x1 - 2 * $d), ($y2 - $y1 - 2 * $d)) }

# ---- Names (reading order on each sheet) ----
$fantasyWeapons = @(
 'iron sword','flame sword','venom sword','blood greatsword','frost sword','bone scimitar','golden rapier','void katana','crimson cleaver','shadow twin blades',
 'steel battle axe','crimson axe','frost axe','war hammer','iron maul','sun mace','venom flail','frost morning star','spiked mace','thunder hammer',
 'iron spear','lightning spear','golden halberd','blood spear','golden glaive','void scythe','bone scythe','venom scythe','frost halberd','fire trident',
 'wooden longbow','void bow','golden bow','crossbow','frost bow','venom recurve','storm bow','wooden crossbow','void crossbow','blood bow',
 'throwing knives','frost shuriken','golden chakram','blood ring blade','golden boomerang','frost chakram','void chakram','blood chakram','venom shuriken','steel star',
 'fire whip','spiked flail','lightning chain','venom chain','golden kusarigama','void chain','blood chain sickle','frost nunchaku','iron flail','fire flail',
 'nature staff','frost staff','void staff','sun staff','fire staff','shadow staff','arcane orb staff','frost wand','venom staff','angel staff',
 'flintlock pistol','revolver','frost blaster','red blaster','void blaster','steel revolver','venom pistol','blood pistol','frost ray pistol','plasma pistol',
 'hunting rifle','assault rifle','void rifle','golden rifle','venom rifle','red carbine','sniper rifle','gatling cannon','void sniper','plasma cannon',
 'rocket launcher','frost cannon','red minigun','ice beam gun','void pulse gun','flamethrower','drone launcher','black hole gun','venom launcher','bone beast gun')
$sciWeapons = @(
 'knight sword','katana','rapier','dark greatsword','scimitar','dagger','twin daggers','battle axe','red war axe','heavy war hammer',
 'morning star','long spear','halberd','trident','druid staff','crystal wand','recurve bow','heavy crossbow','ring blade','thorn whip',
 'claw gauntlet','ball and chain','sun sword','blood sword','ice crystal sword','violet energy blade','red energy blade','plasma axe','energy spear','void lance',
 'storm staff','plasma blaster','magnum','venom blaster','shotgun','battle rifle','long rifle','ion cannon','void cannon','gatling gun',
 'flame projector','drone blaster','sonic blaster','frost blaster rifle','shock rifle','bio cannon','laser crossbow','void disc','power fist','gravity hammer')
$elementalFx = @(
 'gold spark','blue star','ember sparks','smoke puff still','fire wisp','ice shards','void orb','poison drip','light streaks','bubbles',
 'red star','twin stars','holy ring','smoke cloud','ember burst','ice shatter','arcane crescent','poison splash','rock debris still','lightning bolt still',
 'fire comet','ice comet','void comet','poison comet','wind comet','fireball','ice ball','void vortex','light arrow','poison cloud still',
 'fire burst','ice burst','void burst','nature burst','earth burst','holy burst','water burst','wind swirl','void ring','ice crystal',
 'fire slash','ice slash','void slash','poison slash','wind slash','holy slash','water slash','shadow slash still','thorn ring','lightning arc',
 'fire orb','frost orb','arcane orb still','healing orb','holy orb','water circle','void circle','nature circle','small tornado','ice spikes',
 'fire pillar','ice pillar','void pillar','nature pillar','holy pillar','water geyser','shadow pillar','earth spikes','wind tornado','poison fog',
 'fire explosion','ice eruption','void dome','nature ring','holy rays','tidal wave','black hole still','rock spires','lightning strike','frost storm',
 'fire storm','ice spire','void spiral','poison cloud large','holy beam','great wave','void eye','stone spires','great tornado','waterfall',
 'inferno','glacier','void galaxy','toxic skull','divine light','tsunami','dark portal','earthquake','thunderstorm','ice beam')
$animatedFx = @('spark','fire spark','ember','flame','big fire','fire nova','fire ball','fire trail','explosion small','explosion large',
 'smoke puff','smoke trail','dust','sand burst','dirt explosion','rock debris','ground crack','earth spike','rock burst','sand tornado',
 'water drop','water splash','water wave','water ring','water jet','bubble','whirlpool','water explosion','ice spike','ice explosion',
 'snow','snow burst','ice shard','frost nova','blizzard','wind gust','tornado','air slash','wind trail','feather',
 'lightning spark','lightning bolt','chain lightning','electric burst','electric ring','energy beams','electric ball','thunder strike','shockwave','plasma explosion',
 'poison cloud','poison burst','acid splash','acid trail','toxic bubble','healing glow','holy light','divine burst','magic circle','teleport',
 'arcane orb','arcane burst','magic missile','magic trail','spell cast','mana explosion','portal open','portal idle','portal close','teleport trail',
 'death aura','soul release','shadow slash','dark explosion','dark mist','curse','life steal','blood splash','blood trail','shadow portal',
 'nature leaf','leaf swirl','petal burst','vines','thorn spike','nature healing','tree growth','nature explosion','fireflies','butterfly swarm',
 'meteors','comet trail','asteroid shower','star burst','galaxy spiral','time warp','space rift','black hole','meteor explosion','reality distortion')
$weather = @('light drizzle','steady rain','heavy rain','slanted rain','storm rain','thunderstorm weather','distant lightning','close lightning','chain lightning weather','rolling clouds',
 'light snow','snow flurry','heavy snowfall','blizzard weather','sleet','hail','ice pellets','freezing rain','frost gust','ground blizzard',
 'whiteout','fog','dense fog','drifting mist','cold breath haze','low cloud bank','dark storm clouds','fast clouds','overcast bands','clearing clouds',
 'sunrise haze','heat shimmer','heat wave','mirage shimmer','dust breeze','dust storm','sand gust','sandstorm','desert whirlwind','leaf drift',
 'leaf swirl weather','autumn leaf storm','pollen drift','floating spores','ash fall','ash storm','volcanic smoke','ember drift','acid rain','toxic smog',
 'green gas front','swamp haze','steam vent','geothermal steam','monsoon burst','tropical squall','hurricane bands','cyclone spiral','tornado funnel','waterspout',
 'microburst','gust front','cold front sweep','aurora shimmer','starry frost','moonlit mist','rainbow after rain','sun shower','sunbeams','ice fog',
 'rime frost','black ice sheen','mud splash','puddle ripple','wave spray gale','sea storm spray','ocean mist','coastal storm','snow dust devil','crystal snow',
 'electric storm','plasma storm','magic rainbows','purple storm','blood rain','meteor shower','comet tails','eclipse gloom','twilight haze','dawn dew sparkle',
 'morning fog lift','evening mist','locust swarm','firestorm wind','solar flare sky','radioactive rain','bioluminescent rain','floating lanterns','meteoric snow','alien sky')
$animals = @('grey wolf','fox','bear','deer','stag','doe','elk','moose','boar','wild pig','cougar','lynx','leopard','jaguar','cheetah','lion','tiger','snow leopard','hyena','jackal',
 'elephant','rhino','hippopotamus','giraffe','zebra','buffalo','bison','camel','llama','alpaca','cow','bull','horse','donkey','goat','sheep','pig','reindeer','yak','water buffalo',
 'dog','cat','rabbit','squirrel','raccoon','badger','beaver','otter','skunk','porcupine')
$plants = @('grass','tall grass','bush','clover','fern','moss','ivy','vine','palm leaf','banana plant','bamboo','cactus','agave','aloe','yucca','succulent','desert shrub','golden lavender','lavender','rose','sunflower',
 'tulip','daffodil','lily','orchid','lotus','water lily','hibiscus','cherry blossom','magnolia','peony','dahlia','marigold','carnation','poppy','bluebell','hydrangea','iris','camellia','azalea','rhododendron',
 'nightshade','moonflower','bird of paradise','heliconia','bromeliad','calla lily','snapdragon','foxglove','wisteria','white peony','chrysanthemum','geranium','begonia','fuchsia','petunia','pansy','yellow hibiscus','red hibiscus','plumeria','blue lotus',
 'purple lotus','pink lotus','water hyacinth','cattail','reed','papyrus','wheat','rice','corn','small sunflower','cotton','sugarcane','carrot top','onion grass','garlic','potato plant','tomato plant','pepper plant','strawberry','blueberry bush',
 'raspberry bush','blackberry bush','cranberry','grape vine','olive tree','lemon tree','orange tree','apple tree','pear tree','peach tree','plum tree','cherry tree','fig tree','date palm','coconut palm','maple tree','oak tree','pine tree','spruce tree','birch tree',
 'willow tree','cedar tree','acacia tree','baobab tree','redwood','sequoia','ash tree','elm tree','poplar tree','chestnut tree','linden tree','jacaranda tree','dogwood tree','magnolia tree','ginkgo tree','cypress tree','bonsai tree','dead tree','autumn tree','snowy tree',
 'swamp tree','mangrove','jungle tree','rainforest tree','red fantasy tree','blue fantasy tree','purple fantasy tree','crystal tree','glowing tree','mushroom tree','alien tree','spore tree','coral tree','bone tree','void tree','electric tree','ice tree','fire tree','spirit tree','tech tree',
 'brown mushroom','red mushroom','blue mushroom','purple mushroom','glow mushroom','red morel','flat mushroom','tall mushroom','truffle','morel','dark morel','shelf fungus')

# ---- Grid sheets ----
$s1 = '100-Icon Fantasy Weapon Sprite Sheet.png'
$wc = 0,120,242,361,483,603,719,837,956,1084,1254; $wr = 0,125,250,376,506,628,754,883,994,1105,1254
$i = 0; for ($r = 0; $r -lt 10; $r++) { for ($c = 0; $c -lt 10; $c++) { Cut (Unique $fantasyWeapons[$i]) 'icons' $s1 (Inset $wc[$c] $wr[$r] $wc[$c+1] $wr[$r+1] 3) 'colour' @{ tolerance = 10 }; $i++ } }

$s2 = 'Fantasy Sci-Fi Weapon Sprite Sheet.png'
$sr = 0,178,354,525,690,878
$sc = @(@(0,170,342,508,676,843,993,1169,1344,1518,1774), @(0,169,342,508,676,843,993,1170,1345,1583,1774), @(0,169,341,507,675,842,997,1171,1363,1583,1774), @(0,169,336,506,673,840,996,1171,1363,1583,1774), @(0,192,364,515,671,838,1035,1237,1408,1583,1774))
$i = 0; for ($r = 0; $r -lt 5; $r++) { for ($c = 0; $c -lt 10; $c++) { Cut (Unique $sciWeapons[$i]) 'icons' $s2 (Inset $sc[$r][$c] $sr[$r] $sc[$r][$c+1] $sr[$r+1] 6) 'colour' @{ tolerance = 18 }; $i++ } }

$s3 = 'Pixel Art Elemental VFX Grid.png'
$ec = 0,126,251,377,502,626,752,877,1003,1128,1254; $er = 0,114,226,335,447,557,669,789,908,1060,1247
$i = 0; for ($r = 0; $r -lt 10; $r++) { for ($c = 0; $c -lt 10; $c++) { Cut (Unique $elementalFx[$i]) 'effects' $s3 (Inset $ec[$c] $er[$r] $ec[$c+1] $er[$r+1] 3) 'luma' $null; $i++ } }

$s4 = 'Pixel Weather Sprite Atlas.png'
$tc = 1,135,266,397,519,643,772,903,1028,1147,1253; $tr = 1,120,246,375,507,640,772,901,1022,1132,1250
$i = 0; for ($r = 0; $r -lt 10; $r++) { for ($c = 0; $c -lt 10; $c++) {
  $x1 = $tc[$c] + 10; $x2 = $tc[$c+1] - 9; $y1 = $tr[$r] + 35; $y2 = $tr[$r+1] - 28
  $fw = ($x2 - $x1) / 4.0; $frames = @(); for ($f = 0; $f -lt 4; $f++) { $a = [int]($x1 + $f * $fw); $b = [int]($x1 + ($f + 1) * $fw); $frames += ,@($a, $y1, ($b - $a), ($y2 - $y1)) }
  Cut (Unique $weather[$i]) 'weather' $s4 @($x1, $y1, ($x2 - $x1), ($y2 - $y1)) 'luma' @{ frameRects = $frames }; $i++ } }

# ---- Labelled sheets with real transparency ----
function RowsOfLabels($boxes, $gapY) {
  $sorted = $boxes | Sort-Object { $_[1] }
  $rows = @(); $cur = @(); $last = -1000
  foreach ($b in $sorted) { if ($b[1] - $last -gt $gapY -and $cur.Count) { $rows += ,@($cur); $cur = @() }; $cur += ,$b; $last = $b[1] }
  if ($cur.Count) { $rows += ,@($cur) }
  $rows | ForEach-Object { ,@($_ | Sort-Object { $_[0] }) }
}
function LabelledCuts($sheetName, $sh, $labels, $names, $pageOf, $splits) {
  $rows = @(RowsOfLabels $labels 40); $i = 0; $prevBottom = 0
  foreach ($row in $rows) {
    for ($k = 0; $k -lt $row.Count; $k++) {
      $b = $row[$k]
      $left = if ($k -gt 0) { $row[$k-1][0] + $row[$k-1][2] } else { 0 }
      $right = if ($k -lt $row.Count - 1) { $row[$k+1][0] } else { $sh.W }
      $fl = if ($k -gt 0) { [int](($row[$k-1][0] + $row[$k-1][2] / 2 + $b[0] + $b[2] / 2) / 2) } else { 0 }
      $fr = if ($k -lt $row.Count - 1) { [int](($b[0] + $b[2] / 2 + $row[$k+1][0] + $row[$k+1][2] / 2) / 2) } else { $sh.W }
      $top = $prevBottom + 4; $bottom = $b[1] - 4
      $name = $names[$i]
      if ($splits.ContainsKey($name)) { $fl = $splits[$name][0]; $fr = $splits[$name][1] }
      Cut (Unique $name) (& $pageOf $name) $sheetName @($left, $top, ($right - $left), ($bottom - $top)) 'alpha' @{ focus = @($fl, $top, ($fr - $fl), ($bottom - $top)) }
      $i++
      if ($name -eq 'glow mushroom') {   # the red morel has no label of its own
        $i++; Cut (Unique 'red morel') (& $pageOf 'red morel') $sheetName @(700, $top, 90, ($bottom - $top)) 'alpha' @{ focus = @(712, $top, 63, ($bottom - $top)) }
      }
    }
    $prevBottom = [int](($row | ForEach-Object { $_[1] + $_[3] } | Measure-Object -Maximum).Maximum)
  }
  if ($i -ne $names.Count) { throw "$sheetName : $i cuts for $($names.Count) names" }
}
$s5 = 'Pixel-Art Animal Sprite Sheet.png'; $sh = & "$here\load.ps1" "$sp\$s5"
LabelledCuts $s5 $sh @($sh.DarkBoxes(40, 50, 220, 14, 40, 0.6)) $animals { param($n) 'animals' } @{}

$treeWords = 'tree','palm','redwood','sequoia','mangrove'
$tallNames = 'bush','palm leaf','banana plant','bamboo','cactus','agave','sunflower','bird of paradise','heliconia','foxglove','wisteria','cattail','reed','papyrus','wheat','rice','corn','sugarcane','cotton','tomato plant','pepper plant','blueberry bush','raspberry bush','blackberry bush','cranberry','grape vine','desert shrub','yucca','hydrangea','rhododendron','azalea','camellia','calla lily','iris','lily','small sunflower'
$pageOfPlant = { param($n) if ($tallNames -contains $n) { 'plants-tall' } elseif ($treeWords | Where-Object { $n -match $_ }) { 'trees' } else { 'plants-small' } }
$s6 = 'Botanical Sprite Atlas_ 150 Nature Assets.png'; $sh = & "$here\load.ps1" "$sp\$s6"
LabelledCuts $s6 $sh @($sh.DarkBoxes(40, 30, 160, 10, 34, 0.5)) $plants $pageOfPlant @{ 'glow mushroom' = @(627, 712); 'flat mushroom' = @(775, 860) }

# VFX atlas: label on top of each cell, 3-4 frames below, their numbers under them.
$s7 = '100-Effect Pixel Art VFX Atlas.png'; $sh = & "$here\load.ps1" "$sp\$s7"
$rows = @(RowsOfLabels @($sh.DarkBoxes(45, 40, 200, 12, 30, 0.45)) 40); $i = 0
for ($r = 0; $r -lt $rows.Count; $r++) {
  $row = $rows[$r]; $nextTop = if ($r -lt $rows.Count - 1) { [int](($rows[$r+1] | ForEach-Object { $_[1] } | Measure-Object -Minimum).Minimum) } else { $sh.H + 2 }
  for ($k = 0; $k -lt $row.Count; $k++) {
    $b = $row[$k]; $x1 = $b[0]; $x2 = if ($k -lt $row.Count - 1) { $row[$k+1][0] - 2 } else { $sh.W - 1 }
    $y1 = $b[1] + $b[3] + 1; $y2 = $nextTop - 25
    $runs = @($sh.Occupancy($x1, $y1, $x2 - $x1, $y2 - $y1, 200, 2).Split(',') | Where-Object { $_ } | ForEach-Object { $p = $_.Split('-'); ,@([int]$p[0], [int]$p[1]) } | Where-Object { $_[1] - $_[0] -ge 2 })
    $frames = @()
    if ($runs.Count -ge 2 -and $runs.Count -le 4) {
      for ($f = 0; $f -lt $runs.Count; $f++) {
        $a = if ($f -eq 0) { $x1 } else { [int](($runs[$f-1][1] + $runs[$f][0]) / 2) }
        $e = if ($f -eq $runs.Count - 1) { $x2 } else { [int](($runs[$f][1] + $runs[$f+1][0]) / 2) }
        $frames += ,@($a, $y1, ($e - $a), ($y2 - $y1))
      }
    } else {
      $fw = ($x2 - $x1) / 4.0; for ($f = 0; $f -lt 4; $f++) { $a = [int]($x1 + $f * $fw); $e = [int]($x1 + ($f + 1) * $fw); $frames += ,@($a, $y1, ($e - $a), ($y2 - $y1)) }
    }
    Cut (Unique $animatedFx[$i]) 'effects' $s7 @($x1, $y1, ($x2 - $x1), ($y2 - $y1)) 'alpha-soft' @{ frameRects = $frames }; $i++
  }
}
if ($i -ne 100) { throw "vfx atlas: $i cells" }

$pages = [ordered]@{
  'icons' = [ordered]@{ cell = @(32, 32); fit = 'centre' }
  'plants-small' = [ordered]@{ cell = @(32, 32); fit = 'bottom' }
  'plants-tall' = [ordered]@{ cell = @(32, 64); fit = 'bottom' }
  'trees' = [ordered]@{ cell = @(64, 96); fit = 'bottom' }
  'animals' = [ordered]@{ cell = @(64, 48); fit = 'bottom' }
  'effects' = [ordered]@{ cell = @(48, 48); fit = 'centre' }
  'weather' = [ordered]@{ cell = @(32, 64); fit = 'centre'; trim = $false }
}
$doc = [ordered]@{ contentCutsVersion = 1; pages = $pages; cuts = $cuts }
$json = $doc | ConvertTo-Json -Depth 8 -Compress
# One cut per line: easy to read and to diff.
$json = $json.Replace('},{"name"', "},`n  {`"name`"").Replace('"cuts":[{', "`"cuts`":[`n  {").Replace('}},"cuts"', "}},`n`"cuts`"")
[IO.File]::WriteAllText("$sp\content-cuts.json", $json + "`n", (New-Object Text.UTF8Encoding($false)))
"cuts: $($cuts.Count)"; $cuts | Group-Object { $_.page } | ForEach-Object { "$($_.Name): $($_.Count)" }
