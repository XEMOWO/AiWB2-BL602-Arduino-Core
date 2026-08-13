# install_windows.ps1 — install the Ai-Thinker Ai-WB2-12F Arduino core into
# Arduino IDE 2.x (manual install; Boards Manager publishing comes later).
#
#  1. Unzip aiwb2-arduino-0.1.0.zip
#  2. Right-click install_windows.bat -> Run as administrator is NOT needed
#  3. Enter the path to your Ai-Thinker WB2 SDK on this Windows machine.
#
# The SDK provides:
#   - the RISC-V toolchain (toolchain\riscv\MSYS) — this is COPIED into the
#     package as tools\riscv-msys, because a gcc driver spawned from a UNC path
#     cannot execute its sub-tools (cc1/cc1plus/as); the toolchain must be on a
#     Windows-local disk. The package's platform.txt already points
#     toolchain.path at tools\riscv-msys.
#   - the C headers and linker script, referenced live via sdk.path.
#
# The SDK does not need to be on Windows: if it lives in WSL2 (as it does for
# this project, /root/wb2-12f-desktop-clock), point sdk.path at the
# \\wsl.localhost\... UNC path and the IDE reaches headers + ld script over the
# 9P share (plain file reads) — zero copy, always in sync with the Linux tree.

$ErrorActionPreference = "Stop"

Write-Host "=== Ai-Thinker Ai-WB2-12F Arduino core install ===" -ForegroundColor Cyan

# --- Auto-detect the WSL2-hosted SDK (default) ---
$autoSdk = $null
foreach ($candidate in @(
    "\\wsl.localhost\Ubuntu\root\wb2-12f-desktop-clock",
    "\\wsl$\Ubuntu\root\wb2-12f-desktop-clock"
)) {
    if (Test-Path "$candidate\toolchain\riscv\MSYS\bin\riscv64-unknown-elf-gcc.exe") {
        $autoSdk = $candidate
        break
    }
}
if ($autoSdk) {
    # Default to the auto-detected WSL2 SDK without prompting (zero-copy, non-interactive).
    $sdk = $autoSdk
    Write-Host "Detected WSL2-hosted SDK at: $sdk" -ForegroundColor Yellow
} else {
    $sdk = Read-Host "Enter your WB2 SDK root (e.g. C:\Users\you\wb2-12f-desktop-clock)"
    $sdk = $sdk.Trim('"').Trim(' ').TrimEnd('\')
}

if (-not (Test-Path "$sdk\toolchain\riscv\MSYS\bin\riscv64-unknown-elf-gcc.exe")) {
    Write-Host "ERROR: no toolchain at $sdk\toolchain\riscv\MSYS\bin\riscv64-unknown-elf-gcc.exe" -ForegroundColor Red
    Write-Host "The SDK must contain toolchain\riscv\MSYS (the Windows RISC-V gcc)."
    Write-Host "If your SDK lives in WSL2, get its path with:  wsl wslpath -w /root/wb2-12f-desktop-clock"
    exit 1
}
if (-not (Test-Path "$sdk\components\platform\soc\bl602")) {
    Write-Host "ERROR: no components\platform\soc\bl602 under $sdk — does not look like the WB2 SDK." -ForegroundColor Red
    exit 1
}

$arduino15 = Join-Path $env:LOCALAPPDATA "Arduino15"
$dst = Join-Path $arduino15 "packages\aithinker\hardware\wb2\0.1.3"
$src = $PSScriptRoot

Write-Host "Installing to: $dst"
New-Item -ItemType Directory -Force -Path $dst | Out-Null
# Examples must live under libraries/<Name>/examples — arduino-cli indexes examples
# only from libraries, NOT from a top-level examples/ dir in the platform package.
foreach ($d in @("cores", "variants", "lib", "tools", "libraries")) {
    Copy-Item -Recurse -Force (Join-Path $src $d) $dst
}
# Drop a stale top-level examples/ from older installs (superseded by libraries/).
if (Test-Path (Join-Path $dst "examples")) {
    Remove-Item -Recurse -Force (Join-Path $dst "examples")
    Write-Host "Removed stale top-level examples/ (superseded by libraries/AiWB2)."
}
Copy-Item -Force (Join-Path $src "boards.txt"), (Join-Path $src "platform.txt") $dst

# Point platform.txt at the user's SDK.
$plat = Join-Path $dst "platform.txt"
(Get-Content $plat) -replace '^sdk\.path=.*', "sdk.path=$sdk" | Set-Content $plat
Write-Host "platform.txt sdk.path set to: $sdk"

# --- Provision the RISC-V toolchain (Windows-local copy; ~1GB from the SDK) ---
$tcSrc = Join-Path $sdk "toolchain\riscv\MSYS"
$tcDst = Join-Path $dst "tools\riscv-msys"
if (Test-Path (Join-Path $tcDst "bin\riscv64-unknown-elf-gcc.exe")) {
    Write-Host "tools\riscv-msys already present — skipping toolchain copy."
} elseif (Test-Path (Join-Path $tcSrc "bin\riscv64-unknown-elf-gcc.exe")) {
    Write-Host "Copying RISC-V toolchain from SDK (this is ~1GB, one-time)..." -ForegroundColor Yellow
    foreach ($d in @("bin", "lib", "libexec", "riscv64-unknown-elf")) {
        robocopy (Join-Path $tcSrc $d) (Join-Path $tcDst $d) /E /XD "*rv64*" /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) {
            Write-Host "ERROR: robocopy failed copying $d (exit $LASTEXITCODE)" -ForegroundColor Red
            exit 1
        }
    }
    Write-Host "Toolchain ready at tools\riscv-msys."
} else {
    Write-Host "WARN: no toolchain\riscv\MSYS under $sdk — install a riscv64-unknown-elf gcc 10.2.0 into tools\riscv-msys manually." -ForegroundColor Red
}

Write-Host ""
Write-Host "Done." -ForegroundColor Green
Write-Host "Restart Arduino IDE 2.x, then:"
Write-Host "  File > Examples > Ai-Thinker Ai-WB2-12F > 02.Serial > SerialEcho"
Write-Host "  Tools > Board > Ai-Thinker > Ai-Thinker Ai-WB2-12F"
Write-Host "  Upload (board must be on a COM port; console runs at 115200 baud)."
