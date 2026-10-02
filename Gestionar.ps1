param(
    [ValidateSet('Install','Restore','Check')][string]$Action = 'Install',
    [string]$GameDir
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ([string]::IsNullOrWhiteSpace($GameDir)) { $GameDir = Read-Host 'Carpeta de GTA (donde esta gta_sa.exe)' }
$root = (Resolve-Path -LiteralPath $GameDir).Path.TrimEnd('\')
if (-not (Test-Path -LiteralPath (Join-Path $root 'gta_sa.exe') -PathType Leaf)) { throw 'No se encuentra gta_sa.exe en esa carpeta.' }
function Assert-Closed {
    if (Get-Process -Name gta_sa -ErrorAction SilentlyContinue) { throw 'Cierra GTA antes de instalar o retirar Bodycam.' }
}
function Hash([string]$p) {
    $stream=[IO.File]::OpenRead($p)
    $sha=[Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','').ToLowerInvariant() }
    finally { $sha.Dispose(); $stream.Dispose() }
}
function Save-Json($o,[string]$p) { $o | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $p -Encoding UTF8 }
function Safe-Child([string]$base,[string]$relative) {
    $parent = [IO.Path]::GetFullPath($base).TrimEnd('\')
    $p = [IO.Path]::GetFullPath((Join-Path $parent $relative))
    if (-not $p.StartsWith($parent+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Ruta fuera de la carpeta esperada.' }
    return $p
}
function New-Backup([string]$prefix) {
    $p=Safe-Child $root ('Bodycam_Backups\'+$prefix+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fffffff'))
    New-Item -ItemType Directory -Path $p | Out-Null
    return $p
}
function Assert-GameVersion {
    $b=[IO.File]::ReadAllBytes((Join-Path $root 'gta_sa.exe'))
    if ($b.Length -lt 1024) { throw 'Ejecutable no reconocido.' }
    $pe=[BitConverter]::ToInt32($b,60)
    if ($pe -lt 64 -or $pe+264 -gt $b.Length -or [BitConverter]::ToUInt32($b,$pe) -ne 0x4550) { throw 'Ejecutable PE no reconocido.' }
    if ([BitConverter]::ToUInt16($b,$pe+4) -ne 0x14c -or [BitConverter]::ToUInt32($b,$pe+52) -ne 0x400000) { throw 'Esta version requiere GTA SA US 1.0 x86.' }
    $count=[BitConverter]::ToUInt16($b,$pe+6)
    $start=$pe+24+[BitConverter]::ToUInt16($b,$pe+20)
    $rva=0x53e4ff-0x400000
    $found=$false
    for ($i=0;$i -lt $count;$i++) {
        $s=$start+40*$i
        if ($s+40 -gt $b.Length) { throw 'Tabla PE invalida.' }
        $va=[BitConverter]::ToUInt32($b,$s+12);$size=[BitConverter]::ToUInt32($b,$s+16);$raw=[BitConverter]::ToUInt32($b,$s+20)
        if ($rva -ge $va -and $rva+15 -le $va+$size) {
            $off=[int]($raw+$rva-$va)
            if ($off+15 -gt $b.Length) { throw 'Segmento PE invalido.' }
            $hex=[BitConverter]::ToString($b,$off,15)
            $found=$hex -eq 'E8-DC-15-05-00-E8-57-31-1E-00-B9-88-17-BA-00'
            break
        }
    }
    if (-not $found) { throw 'La firma del HUD no coincide con GTA SA US 1.0 compatible.' }
}
$statePath=Join-Path $root 'Bodycam.estado.json'
if ($Action -eq 'Restore') {
    Assert-Closed
    if (-not (Test-Path -LiteralPath $statePath)) { throw 'No hay instalacion registrada de Bodycam.' }
    $state=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    if (-not [string]::Equals($state.GameDir,$root,[StringComparison]::OrdinalIgnoreCase)) { throw 'El registro pertenece a otra carpeta.' }
    $original=Safe-Child $root $state.BackupRelative
    foreach ($f in $state.Files) {
        if ($f.Name -notin @('Bodycam.asi','Bodycam.ini')) { throw 'Nombre no permitido en registro.' }
        if ($f.Existed) {
            $b=Safe-Child $original $f.Name
            if ((Hash $b) -ne $f.BeforeHash) { throw 'El respaldo original no coincide con su hash.' }
        }
    }
    $saved=New-Backup 'retirada'
    foreach ($f in $state.Files) {
        $current=Safe-Child $root $f.Name
        if (Test-Path -LiteralPath $current) { Copy-Item -LiteralPath $current -Destination (Join-Path $saved $f.Name) }
    }
    Copy-Item -LiteralPath $statePath -Destination (Join-Path $saved 'Bodycam.estado.json')
    foreach ($f in $state.Files) {
        $current=Safe-Child $root $f.Name
        if ($f.Existed) { Copy-Item -LiteralPath (Safe-Child $original $f.Name) -Destination $current -Force }
        elseif (Test-Path -LiteralPath $current) { Remove-Item -LiteralPath $current }
    }
    $log=Safe-Child $root 'Bodycam.log'
    if (Test-Path -LiteralPath $log) { Move-Item -LiteralPath $log -Destination (Join-Path $saved 'Bodycam.log') }
    Remove-Item -LiteralPath $statePath
    Write-Output "Bodycam retirado. Configuracion actual conservada en: $saved"
    return
}
Assert-GameVersion
if (-not (Test-Path -LiteralPath (Join-Path $root 'samp.dll'))) { throw 'No se encuentra samp.dll en esa carpeta.' }
$manifest=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'manifest.json') -Raw | ConvertFrom-Json
foreach ($f in $manifest.Files) {
    if ($f.Name -notin @('Bodycam.asi','Bodycam.ini')) { throw 'Nombre no permitido en el paquete.' }
    if ((Hash (Safe-Child $PSScriptRoot $f.Name)) -ne $f.Hash) { throw "El paquete no supera la verificacion de $($f.Name)." }
}
if ($Action -eq 'Check') { Write-Output 'Ejecutable y paquete verificados. Comprobacion sin cambios.'; return }
Assert-Closed
if (Test-Path -LiteralPath $statePath) { throw 'Bodycam ya esta registrado. Retiralo antes de instalar otro paquete.' }
$backup=New-Backup 'instalacion'
$files=@()
foreach ($f in $manifest.Files) {
    $dest=Safe-Child $root $f.Name
    $existed=Test-Path -LiteralPath $dest -PathType Leaf
    $before=$null
    if ($existed) { $before=Hash $dest;Copy-Item -LiteralPath $dest -Destination (Join-Path $backup $f.Name) }
    $files+=@{Name=$f.Name;Existed=$existed;BeforeHash=$before;InstalledHash=$f.Hash}
}
$state=@{Version=$manifest.Version;GameDir=$root;BackupRelative=$backup.Substring($root.Length+1);Files=$files;InstalledAt=(Get-Date).ToString('o')}
Save-Json $state (Join-Path $backup 'installation.json')
try {
    foreach ($f in $manifest.Files) {
        Assert-Closed
        Copy-Item -LiteralPath (Safe-Child $PSScriptRoot $f.Name) -Destination (Safe-Child $root $f.Name) -Force
        if ((Hash (Safe-Child $root $f.Name)) -ne $f.Hash) { throw 'Fallo verificando la copia instalada.' }
    }
    Save-Json $state $statePath
} catch {
    foreach ($f in $files) {
        $dest=Safe-Child $root $f.Name
        if ($f.Existed) { Copy-Item -LiteralPath (Safe-Child $backup $f.Name) -Destination $dest -Force }
        elseif (Test-Path -LiteralPath $dest) { Remove-Item -LiteralPath $dest }
    }
    throw
}
Write-Output "Bodycam $($manifest.Version) instalado en: $root"
Write-Output "Respaldo y registro: $backup"
