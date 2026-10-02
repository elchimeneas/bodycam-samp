param([Parameter(Mandatory=$true)][string]$PackageDir)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$packageSource=(Resolve-Path -LiteralPath $PackageDir).Path
$temporaryRoot=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
$fixture=Join-Path $temporaryRoot ('BodycamInstallerTest-'+[guid]::NewGuid().ToString('N'))
$game=Join-Path $fixture 'game'
$package=Join-Path $fixture 'package'
$global:BodycamTestGameRunning=$false
$script:checks=0
function Assert-True($condition,[string]$description) {
    $script:checks++
    if (-not $condition) { throw "FAIL: $description" }
}
function Assert-Rejected([scriptblock]$operation,[string]$expected) {
    $caught=$false
    try { & $operation | Out-Null } catch {
        $caught=$true
        Assert-True ($_.Exception.Message -like "*$expected*") "Rejection reason: $expected"
    }
    Assert-True $caught 'Operation rejected'
}
# Mock only in this test process. Never close or interact with a real game.
function Get-Process {
    [CmdletBinding()]param([string]$Name)
    if ($Name -ne 'gta_sa') { throw 'Unexpected process query in installer test.' }
    if ($global:BodycamTestGameRunning) { [pscustomobject]@{Id=123;ProcessName='gta_sa'} }
}
function Fingerprint([string]$path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
try {
    New-Item -ItemType Directory -Path $game | Out-Null
    Copy-Item -LiteralPath $packageSource -Destination $package -Recurse
    # Minimal synthetic PE fields consumed by Assert-GameVersion. Never executable.
    $bytes=New-Object byte[] 8192
    [BitConverter]::GetBytes([int]0x80).CopyTo($bytes,60)
    [BitConverter]::GetBytes([uint32]0x4550).CopyTo($bytes,0x80)
    [BitConverter]::GetBytes([uint16]0x14c).CopyTo($bytes,0x84)
    [BitConverter]::GetBytes([uint16]1).CopyTo($bytes,0x86)
    [BitConverter]::GetBytes([uint16]0xe0).CopyTo($bytes,0x94)
    [BitConverter]::GetBytes([uint32]0x400000).CopyTo($bytes,0xb4)
    [BitConverter]::GetBytes([uint32]0x13e000).CopyTo($bytes,0x184)
    [BitConverter]::GetBytes([uint32]0x1000).CopyTo($bytes,0x188)
    [BitConverter]::GetBytes([uint32]0x200).CopyTo($bytes,0x18c)
    [byte[]]$signature=0xe8,0xdc,0x15,0x05,0,0xe8,0x57,0x31,0x1e,0,0xb9,0x88,0x17,0xba,0
    $signature.CopyTo($bytes,0x6ff)
    [IO.File]::WriteAllBytes((Join-Path $game 'gta_sa.exe'),$bytes)
    [IO.File]::WriteAllText((Join-Path $game 'samp.dll'),'synthetic fixture - never loaded')
    $asi=Join-Path $game 'Bodycam.asi';$ini=Join-Path $game 'Bodycam.ini'
    [IO.File]::WriteAllText($asi,'old plugin fixture - never loaded')
    [IO.File]::WriteAllText($ini,'old user configuration')
    $originalAsi=Fingerprint $asi;$originalIni=Fingerprint $ini
    $installer=Join-Path $package 'Gestionar.ps1'
    & $installer -Action Check -GameDir $game | Out-Null
    Assert-True ((Fingerprint $asi) -eq $originalAsi -and (Fingerprint $ini) -eq $originalIni) 'Check does not modify originals'
    $global:BodycamTestGameRunning=$true
    Assert-Rejected { & $installer -Action Install -GameDir $game } 'Cierra GTA'
    $global:BodycamTestGameRunning=$false
    Assert-True ((Fingerprint $asi) -eq $originalAsi) 'Running game guard leaves files unchanged'
    $packageIni=Join-Path $package 'Bodycam.ini'
    $cleanIni=[IO.File]::ReadAllBytes($packageIni)
    [IO.File]::AppendAllText($packageIni,"`nOfficer=MODIFIED")
    Assert-Rejected { & $installer -Action Install -GameDir $game } 'verificacion'
    [IO.File]::WriteAllBytes($packageIni,$cleanIni)
    $bytes[0x6ff]=0
    [IO.File]::WriteAllBytes((Join-Path $game 'gta_sa.exe'),$bytes)
    Assert-Rejected { & $installer -Action Install -GameDir $game } 'firma del HUD'
    $bytes[0x6ff]=0xe8
    [IO.File]::WriteAllBytes((Join-Path $game 'gta_sa.exe'),$bytes)
    & $installer -Action Install -GameDir $game | Out-Null
    Assert-True ((Fingerprint $asi) -eq (Fingerprint (Join-Path $package 'Bodycam.asi'))) 'Installed ASI matches package'
    Assert-True ((Fingerprint $ini) -eq (Fingerprint $packageIni)) 'Installed INI matches package'
    $state=Join-Path $game 'Bodycam.estado.json'
    Assert-True (Test-Path -LiteralPath $state) 'Install state exists'
    Assert-Rejected { & $installer -Action Install -GameDir $game } 'ya esta registrado'
    [IO.File]::AppendAllText($ini,"`nOfficer=PLAYER")
    $customIni=Fingerprint $ini
    $global:BodycamTestGameRunning=$true
    Assert-Rejected { & $installer -Action Restore -GameDir $game } 'Cierra GTA'
    $global:BodycamTestGameRunning=$false
    & $installer -Action Restore -GameDir $game | Out-Null
    Assert-True ((Fingerprint $asi) -eq $originalAsi -and (Fingerprint $ini) -eq $originalIni) 'Restore recovers original hashes'
    Assert-True (-not (Test-Path -LiteralPath $state)) 'Restore removes state'
    $saved=Get-ChildItem -LiteralPath (Join-Path $game 'Bodycam_Backups') -Directory | Where-Object Name -Like 'retirada-*' | Select-Object -First 1
    Assert-True ((Fingerprint (Join-Path $saved.FullName 'Bodycam.ini')) -eq $customIni) 'Player changes retained in removal backup'
    Remove-Item -LiteralPath $asi,$ini
    & $installer -Action Install -GameDir $game | Out-Null
    & $installer -Action Restore -GameDir $game | Out-Null
    Assert-True (-not (Test-Path -LiteralPath $asi) -and -not (Test-Path -LiteralPath $ini)) 'Fresh install removes only its files on restore'
    Assert-True (Test-Path -LiteralPath (Join-Path $game 'samp.dll')) 'Other game files preserved'
    Write-Output "PASS installer: $script:checks checks in an isolated synthetic fixture."
} finally {
    $resolved=[IO.Path]::GetFullPath($fixture)
    if ($resolved.StartsWith($temporaryRoot+'\',[StringComparison]::OrdinalIgnoreCase) -and (Split-Path $resolved -Leaf) -like 'BodycamInstallerTest-*') {
        if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
    } else { throw 'Refusing cleanup outside the temporary fixture.' }
    Remove-Variable -Name BodycamTestGameRunning -Scope Global -ErrorAction SilentlyContinue
}
