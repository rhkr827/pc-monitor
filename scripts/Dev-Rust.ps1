<#
.SYNOPSIS
    Start Rust backend in development mode
.DESCRIPTION
    Starts the Rust backend using Cargo Tauri in development mode
.EXAMPLE
    .\Dev-Rust.ps1
#>

param()

# Set encoding for Korean text support
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "Starting Rust backend in development mode..." -ForegroundColor Cyan

# Change to backend-rust directory
$rustDir = "backend-rust"
if (-not (Test-Path $rustDir)) {
    Write-Host "Error: Rust backend directory not found: $rustDir" -ForegroundColor Red
    exit 1
}

Push-Location $rustDir

try {
    Write-Host "Running cargo tauri dev..." -ForegroundColor Green
    & cargo tauri dev
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Rust development server failed with exit code: $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}