param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$version='0.4.1'
$name="Bodycam-SA-MP-$version"
$binary=Join-Path $PSScriptRoot 'build\Release\Bodycam.asi'
if (-not (Test-Path -LiteralPath $binary)) { throw 'Compila Release con build.ps1 primero.' }
$versionInfo=(Get-Item -LiteralPath $binary).VersionInfo
if ($versionInfo.FileVersion -ne $version) { throw 'Version del binario distinta al paquete.' }
if ((Get-Item -LiteralPath (Join-Path $PSScriptRoot 'Bodycam.ini')).Length -ge 16384) { throw 'El INI supera el limite del lector.' }
$dist=Join-Path $PSScriptRoot 'dist'
New-Item -ItemType Directory -Path $dist -Force | Out-Null
# Every run uses a fresh stage; previous output is never mixed into a release.
$stage=Join-Path $dist ('stage-'+[guid]::NewGuid().ToString('N'))
$package=Join-Path $stage $name
New-Item -ItemType Directory -Path $package | Out-Null
Copy-Item -LiteralPath $binary -Destination (Join-Path $package 'Bodycam.asi')
foreach ($file in @('Bodycam.ini','INSTALAR.cmd','RETIRAR.cmd','Gestionar.ps1','LEEME.txt','README.md','CHANGELOG.md','LICENSE')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination (Join-Path $package $file)
}
$docs=Join-Path $package 'docs'
New-Item -ItemType Directory -Path $docs | Out-Null
foreach ($file in @('VALIDACION.md','CREDITOS.md','DESARROLLO.md')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "docs\$file") -Destination (Join-Path $docs $file)
}
$files=@('Bodycam.asi','Bodycam.ini') | ForEach-Object {
    @{Name=$_;Hash=(Get-FileHash -LiteralPath (Join-Path $package $_) -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$utf8=New-Object Text.UTF8Encoding($false)
$manifest=@{Version=$version;Files=@($files)} | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText((Join-Path $package 'manifest.json'),$manifest,$utf8)
$zip=Join-Path $dist "$name.zip"
Compress-Archive -LiteralPath $package -DestinationPath $zip -CompressionLevel Optimal -Force
$sum=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $dist 'SHA256SUMS.txt'),"$sum  $name.zip`n",$utf8)
Write-Output "Player ZIP: $zip"
Write-Output "Installer test input: $package"
