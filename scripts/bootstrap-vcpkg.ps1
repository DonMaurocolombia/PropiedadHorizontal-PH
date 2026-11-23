$ErrorActionPreference = "Stop"

$rootDir = Split-Path -Parent $PSScriptRoot
$vcpkgDir = Join-Path $rootDir ".vcpkg"
$vcpkgCommit = "93ad765c7b66ab9ce97469ec23a90b15c3a14ee5"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git is required to download vcpkg."
}

if (-not (Test-Path (Join-Path $vcpkgDir ".git"))) {
    git clone https://github.com/microsoft/vcpkg.git $vcpkgDir
}

git -C $vcpkgDir checkout $vcpkgCommit
& (Join-Path $vcpkgDir "bootstrap-vcpkg.bat") -disableMetrics
