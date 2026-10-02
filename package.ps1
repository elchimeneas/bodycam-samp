param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$version='0.4.3'
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
$package=$stage
New-Item -ItemType Directory -Path $package | Out-Null
Copy-Item -LiteralPath $binary -Destination (Join-Path $package 'Bodycam.asi')
foreach ($file in @('Bodycam.ini','README.md')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination (Join-Path $package $file)
}
$utf8=New-Object Text.UTF8Encoding($false)
$zip=Join-Path $dist "$name.zip"
$files=@('Bodycam.asi','Bodycam.ini','README.md')
$paths=@($files | ForEach-Object { Join-Path $package $_ })
Compress-Archive -LiteralPath $paths -DestinationPath $zip -CompressionLevel Optimal -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead($zip)
try {
    $entries=@($archive.Entries | ForEach-Object { $_.FullName })
    if ($entries.Count -ne 3 -or (Compare-Object $files $entries)) {
        throw 'El ZIP debe contener solo Bodycam.asi, Bodycam.ini y README.md en la raiz.'
    }
} finally { $archive.Dispose() }
$sum=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $dist 'SHA256SUMS.txt'),"$sum  $name.zip`n",$utf8)
Write-Output "Player ZIP: $zip"
Write-Output 'Contenido verificado: Bodycam.asi, Bodycam.ini, README.md'
