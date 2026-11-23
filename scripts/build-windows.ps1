param(
    [Parameter(Mandatory = $true)]
    [string]$QtRoot
)

$ErrorActionPreference = "Stop"

if ($env:OS -ne "Windows_NT") {
    throw "Este script debe ejecutarse en Windows con Visual Studio 2022."
}

foreach ($tool in @("cmake", "git")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "No se encontró '$tool' en PATH."
    }
}

$rootDir = Split-Path -Parent $PSScriptRoot
$qtDirectory = (Resolve-Path -LiteralPath $QtRoot).Path
$windeployQt = Join-Path $qtDirectory "bin\windeployqt.exe"
if (-not (Test-Path -LiteralPath $windeployQt)) {
    throw "No se encontró windeployqt.exe en '$qtDirectory\bin'."
}

$vcpkgDir = Join-Path $rootDir ".vcpkg"
$bootstrapScript = Join-Path $PSScriptRoot "bootstrap-vcpkg.ps1"
if (-not (Test-Path -LiteralPath (Join-Path $vcpkgDir "scripts\buildsystems\vcpkg.cmake"))
    -or -not (Test-Path -LiteralPath (Join-Path $vcpkgDir "vcpkg.exe"))) {
    & $bootstrapScript
    if ($LASTEXITCODE -ne 0) {
        throw "No se pudo preparar vcpkg."
    }
}

$env:VCPKG_ROOT = $vcpkgDir
$buildDir = Join-Path $rootDir "build\windows-release"
$packageRoot = Join-Path $rootDir "build\package"
$stageDir = Join-Path $packageRoot "PropiedadHorizontalPH-windows-x64"
$zipPath = Join-Path $packageRoot "PropiedadHorizontalPH-windows-x64.zip"
$toolchainFile = Join-Path $vcpkgDir "scripts\buildsystems\vcpkg.cmake"

New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null
if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

& cmake -S $rootDir -B $buildDir `
    -G "Visual Studio 17 2022" -A x64 `
    "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile" `
    "-DCMAKE_PREFIX_PATH=$qtDirectory" `
    "-DVCPKG_TARGET_TRIPLET=x64-windows"
if ($LASTEXITCODE -ne 0) {
    throw "Falló la configuración de CMake."
}

& cmake --build $buildDir --config Release --target PropiedadHorizontalPH
if ($LASTEXITCODE -ne 0) {
    throw "Falló la compilación Release."
}

$executable = Join-Path $buildDir "Release\PropiedadHorizontalPH.exe"
if (-not (Test-Path -LiteralPath $executable)) {
    throw "No se generó PropiedadHorizontalPH.exe."
}

New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Copy-Item -LiteralPath $executable -Destination $stageDir
Copy-Item -LiteralPath (Join-Path $rootDir "datos.env.example") -Destination $stageDir
Copy-Item -LiteralPath (Join-Path $rootDir "LICENSE") -Destination $stageDir
Copy-Item -LiteralPath (Join-Path $rootDir "COPYING") -Destination $stageDir

$vcpkgBinDir = Join-Path $buildDir "vcpkg_installed\x64-windows\bin"
if (-not (Test-Path -LiteralPath $vcpkgBinDir)) {
    throw "No se encontró el directorio de dependencias de vcpkg: '$vcpkgBinDir'."
}
$vcpkgDlls = Get-ChildItem -LiteralPath $vcpkgBinDir -Filter "*.dll" -File
if (-not $vcpkgDlls) {
    throw "vcpkg no produjo las DLL esperadas en '$vcpkgBinDir'."
}
Copy-Item -Path $vcpkgDlls.FullName -Destination $stageDir -Force

$packagedExecutable = Join-Path $stageDir "PropiedadHorizontalPH.exe"
& $windeployQt --release --compiler-runtime --no-translations --dir $stageDir $packagedExecutable
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt no pudo preparar las dependencias de Qt."
}

Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipPath -Force
Write-Host "Paquete generado: $zipPath"
