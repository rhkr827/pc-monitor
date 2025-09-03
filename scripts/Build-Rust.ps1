<#
.SYNOPSIS
    Build Rust backend for PC Monitor
.DESCRIPTION
    Sets up MSVC environment and builds the Rust backend using Cargo
.EXAMPLE
    .\Build-Rust.ps1
#>

param(
    [switch]$Release = $false,
    [switch]$Verbose = $false
)

# Set encoding for Korean text support
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "Building Rust backend..." -ForegroundColor Cyan

# Set up Visual Studio environment
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    Write-Host "Error: Visual Studio Build Tools not found at: $vcvars" -ForegroundColor Red
    exit 1
}

# Change to backend-rust directory
$rustDir = "backend-rust"
if (-not (Test-Path $rustDir)) {
    Write-Host "Error: Rust backend directory not found: $rustDir" -ForegroundColor Red
    exit 1
}

Push-Location $rustDir

try {
    # Set up MSVC environment
    Write-Host "Setting up MSVC environment..." -ForegroundColor Yellow
    cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
        if ($_ -match "^([^=]+)=(.*)$") {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }

    # Build with Cargo
    $buildArgs = @("build")
    if ($Release) {
        $buildArgs += "--release"
        Write-Host "Building in release mode..." -ForegroundColor Green
    }
    else {
        Write-Host "Building in debug mode..." -ForegroundColor Green
    }
    
    if ($Verbose) {
        $buildArgs += "--verbose"
    }

    & cargo @buildArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Rust build completed successfully!" -ForegroundColor Green
    }
    else {
        Write-Host "Rust build failed with exit code: $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}