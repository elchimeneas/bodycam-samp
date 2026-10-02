param(
    [ValidateSet('Release','RelWithDebInfo','Debug')][string]$Configuration='Release',
    [switch]$SkipGraphicsTests
)
$ErrorActionPreference='Stop'
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Instala Visual Studio 2022 o Build Tools con C++ x86, Windows SDK y CMake.' }
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Se necesitan las herramientas C++ x86 de Visual Studio.' }
$cmake=Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) {
    $cmake=(Get-Command cmake.exe -ErrorAction Stop).Source
}
$ctest=Join-Path (Split-Path $cmake) 'ctest.exe'
Push-Location $PSScriptRoot
try {
    & $cmake -S . -B build -A Win32
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de configuracion.' }
    & $cmake --build build --config $Configuration --parallel
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de compilacion.' }
    $arguments=@('--test-dir','build','-C',$Configuration,'--output-on-failure')
    if ($SkipGraphicsTests) { $arguments+=@('-R','bodycam_camera_validation') }
    & $ctest @arguments
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de validacion.' }
    if ($SkipGraphicsTests) { Write-Warning 'Pruebas graficas omitidas: ejecutarlas en un equipo con Direct3D 9 antes de publicar.' }
} finally { Pop-Location }
