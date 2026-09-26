# setup_xiaozhi_supermini.ps1
# Reproducible XiaoZhi ESP32-S3 SuperMini setup.
#
# Installs/checks:
#   - Git
#   - ESP-IDF Installation Manager (EIM)
#   - ESP-IDF v6.1 + required tools
#
# Then:
#   - clones upstream XiaoZhi
#   - checks out upstream commit 4632dc5
#   - clones the custom repository
#   - replaces upstream main/ with custom main/
#   - activates ESP-IDF 6.1
#   - resolves ESP-IDF Component Manager dependencies from main/idf_component.yml
#   - verifies that managed_components/ is generated
#
# If PowerShell blocks this script, run:
#   Set-ExecutionPolicy -Scope Process Bypass
#   .\setup_xiaozhi_supermini.ps1

$ErrorActionPreference = "Stop"

$BaseDir = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
$XiaoZhiDir = Join-Path $BaseDir "xiaozhi-esp32"
$MyRepoDir = Join-Path $BaseDir "xiaozhi-esp32-custom"

$UpstreamUrl = "https://github.com/78/xiaozhi-esp32.git"
$MyRepoUrl = "https://github.com/Aravinthkalai/xiaozhi-esp32.git"
$Commit = "4632dc5"

$IdfVersion = "v6.1"
$IdfPath = "C:\esp\v6.1\esp-idf"
$IdfToolsPath = "C:\Espressif\tools"
$IdfProfile = "C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"

function Test-CommandExists {
    param([string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Install-WithWinget {
    param(
        [Parameter(Mandatory=$true)][string]$Id,
        [string]$Name = $Id
    )

    if (-not (Test-CommandExists "winget")) {
        throw "WinGet is not available. Install/update App Installer from Microsoft Store, then run this script again. Required package: $Name"
    }

    Write-Host "Installing $Name..." -ForegroundColor Yellow
    winget install --id $Id -e --source winget `
        --accept-package-agreements `
        --accept-source-agreements
    if ($LASTEXITCODE -ne 0) {
        throw "WinGet failed to install $Name (exit code $LASTEXITCODE)."
    }
}

function Find-IdfProfile {
    $candidates = @(
        $IdfProfile,
        "C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    # EIM normally creates the profile under IDF_TOOLS_PATH.
    if (Test-Path $IdfToolsPath) {
        $found = Get-ChildItem $IdfToolsPath -Filter "Microsoft.*.PowerShell_profile.ps1" -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match "v6\.1" } |
            Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    return $null
}

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " XiaoZhi ESP32-S3 SuperMini Setup" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# ---------------------------------------------------------------------------
# 1. Git
# ---------------------------------------------------------------------------
Write-Host "`n[1/6] Checking Git..." -ForegroundColor Yellow

if (-not (Test-CommandExists "git")) {
    Write-Host "Git is not installed. Installing Git for Windows..." -ForegroundColor Yellow
    Install-WithWinget -Id "Git.Git" -Name "Git for Windows"

    # Refresh PATH for this process after winget installation.
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("Path", "User")

    if (-not (Test-CommandExists "git")) {
        throw "Git was installed but is not available in this PowerShell session. Close PowerShell, open a new one, and run the script again."
    }
}

git --version

# ---------------------------------------------------------------------------
# 2. ESP-IDF / EIM
# ---------------------------------------------------------------------------
Write-Host "`n[2/6] Checking ESP-IDF 6.1..." -ForegroundColor Yellow

$ExistingProfile = Find-IdfProfile

if (-not $ExistingProfile) {
    Write-Host "ESP-IDF 6.1 was not found. Checking ESP-IDF Installation Manager (EIM)..." -ForegroundColor Yellow

    if (-not (Test-CommandExists "eim")) {
        Write-Host "EIM is not installed. Installing EIM CLI with WinGet..." -ForegroundColor Yellow
        Install-WithWinget -Id "Espressif.EIM-CLI" -Name "Espressif Installation Manager CLI"

        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
                    [System.Environment]::GetEnvironmentVariable("Path", "User")
    }

    if (-not (Test-CommandExists "eim")) {
        throw "EIM was installed but is not available in this PowerShell session. Close PowerShell, open a new one, and run the script again."
    }

    Write-Host "Installing ESP-IDF $IdfVersion and its required tools..." -ForegroundColor Yellow
    Write-Host "This can take a while and requires Internet access and several GB of disk space." -ForegroundColor DarkYellow

    # EIM installs ESP-IDF and required tools. The command is non-interactive.
    eim install -i $IdfVersion -n true
    if ($LASTEXITCODE -ne 0) {
        throw "EIM failed to install ESP-IDF $IdfVersion (exit code $LASTEXITCODE)."
    }

    $ExistingProfile = Find-IdfProfile
}

if (-not $ExistingProfile) {
    throw "ESP-IDF 6.1 installation was not found after installation. Expected profile: $IdfProfile"
}

Write-Host "ESP-IDF profile: $ExistingProfile" -ForegroundColor Green

# ---------------------------------------------------------------------------
# 3. Activate ESP-IDF
# ---------------------------------------------------------------------------
Write-Host "`n[3/6] Activating ESP-IDF 6.1..." -ForegroundColor Yellow

# Dot-source the profile so its environment changes remain in this process.
. $ExistingProfile
$env:PYTHONUTF8 = "1"

$env:IDF_TARGET = "esp32s3"

if (-not (Test-CommandExists "idf.py")) {
    throw "idf.py is still unavailable after activating ESP-IDF."
}

$idfVersionOutput = idf.py --version
Write-Host $idfVersionOutput -ForegroundColor Green

if ($idfVersionOutput -notmatch "ESP-IDF v6\.1") {
    throw "Expected ESP-IDF v6.1, but detected: $idfVersionOutput"
}

# ---------------------------------------------------------------------------
# 4. Clone upstream at pinned commit
# ---------------------------------------------------------------------------
Write-Host "`n[4/6] Preparing upstream XiaoZhi..." -ForegroundColor Yellow

if (Test-Path $XiaoZhiDir) {
    Write-Host "Removing existing $XiaoZhiDir ..." -ForegroundColor DarkYellow
    Remove-Item -Recurse -Force $XiaoZhiDir
}

git clone $UpstreamUrl $XiaoZhiDir
if ($LASTEXITCODE -ne 0) {
    throw "Failed to clone upstream XiaoZhi."
}

Push-Location $XiaoZhiDir
try {
    git checkout $Commit
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to checkout upstream commit $Commit."
    }

    $actualCommit = (git rev-parse --short HEAD).Trim()
    if ($actualCommit -ne $Commit) {
        throw "Pinned commit mismatch. Expected $Commit but got $actualCommit."
    }
}
finally {
    Pop-Location
}

# ---------------------------------------------------------------------------
# 5. Clone custom repository and replace main/
# ---------------------------------------------------------------------------
Write-Host "`n[5/6] Applying your custom main folder..." -ForegroundColor Yellow

if (Test-Path $MyRepoDir) {
    Remove-Item -Recurse -Force $MyRepoDir
}

git clone $MyRepoUrl $MyRepoDir
if ($LASTEXITCODE -ne 0) {
    throw "Failed to clone $MyRepoUrl"
}

$CustomMain = Join-Path $MyRepoDir "main"
$TargetMain = Join-Path $XiaoZhiDir "main"

if (-not (Test-Path $CustomMain)) {
    throw "Your GitHub repository does not contain a main folder: $CustomMain"
}

if (Test-Path $TargetMain) {
    Remove-Item -Recurse -Force $TargetMain
}

Copy-Item -Recurse -Force $CustomMain $TargetMain

# Verify the custom SuperMini board files are present.
$BoardDir = Join-Path $TargetMain "boards\esp32s3-supermini"
$RequiredFiles = @(
    (Join-Path $BoardDir "config.h"),
    (Join-Path $BoardDir "config.json"),
    (Join-Path $BoardDir "esp32s3_supermini_board.cc")
)

foreach ($file in $RequiredFiles) {
    if (-not (Test-Path $file)) {
        throw "Required SuperMini file is missing: $file"
    }
}

# ---------------------------------------------------------------------------
# 6. Resolve ESP-IDF Component Manager dependencies
# ---------------------------------------------------------------------------
Write-Host "`n[6/7] Resolving ESP-IDF dependencies..." -ForegroundColor Yellow

$ManifestFile = Join-Path $TargetMain "idf_component.yml"

if (-not (Test-Path $ManifestFile)) {
    throw "Dependency manifest is missing: $ManifestFile"
}

Write-Host "Found dependency manifest:" -ForegroundColor Green
Write-Host "  $ManifestFile"

Write-Host "Running ESP-IDF configuration to resolve dependencies..." -ForegroundColor Yellow
Push-Location $XiaoZhiDir
try {
    # Component Manager reads main/idf_component.yml and creates/updates
    # managed_components automatically. Do not copy or commit managed_components.
    idf.py reconfigure
    if ($LASTEXITCODE -ne 0) {
        throw "ESP-IDF dependency resolution failed during 'idf.py reconfigure'."
    }
}
finally {
    Pop-Location
}

$ManagedComponentsDir = Join-Path $XiaoZhiDir "managed_components"

if (-not (Test-Path $ManagedComponentsDir)) {
    throw "ESP-IDF did not create managed_components: $ManagedComponentsDir"
}

$ManagedComponentCount = @(Get-ChildItem $ManagedComponentsDir -Directory -ErrorAction SilentlyContinue).Count

if ($ManagedComponentCount -eq 0) {
    throw "managed_components exists but contains no components. Dependency resolution may have failed."
}

$EspSrDir = Join-Path $ManagedComponentsDir "espressif__esp-sr"
if (-not (Test-Path $EspSrDir)) {
    throw "Required ESP-SR component was not resolved: $EspSrDir"
}

Write-Host "ESP-IDF dependencies resolved successfully." -ForegroundColor Green
Write-Host "  Managed components: $ManagedComponentCount"
Write-Host "  ESP-SR: found"

# ---------------------------------------------------------------------------
# 7. Finish
# ---------------------------------------------------------------------------
Write-Host "`n[7/7] Setup verification..." -ForegroundColor Yellow

Push-Location $XiaoZhiDir
try {
    $finalCommit = (git rev-parse --short HEAD).Trim()
    if ($finalCommit -ne $Commit) {
        throw "Final XiaoZhi commit is not $Commit. Found $finalCommit"
    }
}
finally {
    Pop-Location
}

Write-Host "`n============================================" -ForegroundColor Green
Write-Host " Setup complete!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green

Write-Host "`nProject:" -ForegroundColor Cyan
Write-Host "  $XiaoZhiDir"

Write-Host "`nESP-IDF:" -ForegroundColor Cyan
Write-Host "  $idfVersionOutput"
Write-Host "  Target: $env:IDF_TARGET"

Write-Host "`nUpstream commit:" -ForegroundColor Cyan
Write-Host "  $Commit"

Write-Host "`nDependencies:" -ForegroundColor Cyan
Write-Host "  Resolved from main\idf_component.yml"
Write-Host "  managed_components generated automatically"

Write-Host "`nNext commands:" -ForegroundColor Cyan
Write-Host "  cd `"$XiaoZhiDir`""
Write-Host '  python scripts\build.py esp32s3-supermini'
Write-Host '  idf.py -p COM8 flash'
Write-Host '  idf.py -p COM8 monitor'

Write-Host "`nNote: COM8 may be different on another PC." -ForegroundColor DarkYellow


Write-Host ""
Write-Host "============================================================"
Write-Host " CLEAN BUILD AND FLASH"
Write-Host "============================================================"
Write-Host ""
Write-Host "After setup, use the following commands for a clean first flash:"
Write-Host ""
Write-Host "  cd xiaozhi-esp32"
Write-Host "  python scripts\build.py esp32s3-supermini"
Write-Host "  idf.py -p COM8 erase-flash"
Write-Host "  idf.py -p COM8 flash"
Write-Host "  idf.py -p COM8 monitor"
Write-Host ""
Write-Host "IMPORTANT:"
Write-Host "  erase-flash erases the ENTIRE ESP32-S3 flash."
Write-Host "  This is recommended for the first installation or when"
Write-Host "  changing partition tables/assets."
Write-Host "  Replace COM8 with the actual ESP32-S3 port."
Write-Host ""

