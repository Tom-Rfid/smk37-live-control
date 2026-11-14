# Compile le firmware SMK37 Open + CONTROL MAP pour le SMK-37 Elite (test materiel).
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
# A toolchain extracted next to this script (toolchain-pi32.zip) needs no installer.
$localZip = Join-Path $root 'toolchain-pi32.zip'
$localTool = Join-Path $root 'toolchain\pi32'
if (-not (Test-Path -LiteralPath ($localTool + '\bin\clang.exe')) -and (Test-Path -LiteralPath $localZip)) {
    Write-Host 'Extraction de la chaine JieLi (toolchain-pi32.zip)...'
    Expand-Archive -LiteralPath $localZip -DestinationPath (Join-Path $root 'toolchain') -Force
}
$candidates = @($env:FM1_PI32_TOOLCHAIN, $localTool, 'C:\JL\pi32') | Where-Object { $_ }
# Plain string paths: Join-Path fails on drive letters that do not exist.
$toolchain = $candidates | Where-Object { Test-Path -LiteralPath ($_.TrimEnd('\') + '\bin\clang.exe') -ErrorAction SilentlyContinue } | Select-Object -First 1
if (-not $toolchain) {
    Write-Host ''
    Write-Host 'Chaine JieLi 2.5.3 introuvable (C:\JL\pi32\bin\clang.exe).' -ForegroundColor Yellow
    Write-Host 'Installez-la avec le programme qui va s''ouvrir (dossier par defaut C:\JL), puis relancez build-elite.bat.'
    Start-Process 'https://jl-update.oss-cn-shenzhen.aliyuncs.com/2.5.3.exe'
    exit 1
}
Write-Host "Chaine JieLi : $toolchain"
& (Join-Path $root 'sdk\Build.ps1') -Model elite -FirmwareOnly -Toolchain $toolchain
$built = Get-ChildItem -Path (Join-Path $root 'build') -Recurse -Filter 'SMK37_ELITE_OPEN.fwsc' |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $built) { throw 'Compilation terminee mais SMK37_ELITE_OPEN.fwsc introuvable dans build\.' }
$out = Join-Path $root 'SMK37_ELITE_OPEN_MAP.fwsc'
Copy-Item -LiteralPath $built.FullName -Destination $out -Force
Write-Host ''
Write-Host "OK : $out" -ForegroundColor Green
Write-Host 'Faites une sauvegarde complete de la flash avant de l''installer (Device update du workbench).'
Start-Process explorer.exe "/select,`"$out`""
