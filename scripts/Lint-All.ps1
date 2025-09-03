<#
.SYNOPSIS
    Run linting for all PC Monitor backends
.DESCRIPTION
    Runs linting for Frontend and Python backends
.PARAMETER Fix
    Automatically fix linting issues where possible
.EXAMPLE
    .\Lint-All.ps1 -Fix
#>

param(
    [switch]$Fix = $false
)

# Set encoding for Korean text support
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$ErrorActionPreference = "Continue"

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "   PC Monitor - Lint All Backends" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

if ($Fix) {
    Write-Host "Mode: Fix issues automatically" -ForegroundColor Yellow
}
else {
    Write-Host "Mode: Check only" -ForegroundColor Yellow
}
Write-Host ""

$startTime = Get-Date
$successCount = 0
$totalLints = 2

# Frontend Lint
Write-Host "[1/2] Linting Frontend..." -ForegroundColor Cyan
try {
    Push-Location "frontend"
    $lintCommand = if ($Fix) { "lint:fix" } else { "lint" }
    & npm run $lintCommand
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Frontend lint passed" -ForegroundColor Green
        $successCount++
    }
    else {
        Write-Host "❌ Frontend lint failed" -ForegroundColor Red
    }
}
catch {
    Write-Host "❌ Frontend lint execution failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# Python Lint
Write-Host "[2/2] Linting Python backend..." -ForegroundColor Cyan
try {
    Push-Location "backend-python"
    if ($Fix) {
        Write-Host "  Running Black formatter..." -ForegroundColor Yellow
        & uv run black .
        Write-Host "  Running Ruff with fix..." -ForegroundColor Yellow
        & uv run ruff check . --fix
    }
    else {
        & uv run ruff check .
    }
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Python lint passed" -ForegroundColor Green
        $successCount++
    }
    else {
        Write-Host "❌ Python lint failed" -ForegroundColor Red
    }
}
catch {
    Write-Host "❌ Python lint execution failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}

# Summary
$duration = (Get-Date) - $startTime
Write-Host ""
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "Lint Summary:" -ForegroundColor White
Write-Host "  Passed: $successCount/$totalLints" -ForegroundColor $(if ($successCount -eq $totalLints) { "Green" } else { "Yellow" })
Write-Host "  Duration: $($duration.ToString('mm\:ss'))" -ForegroundColor White
Write-Host "====================================" -ForegroundColor Cyan

if ($successCount -eq $totalLints) {
    Write-Host "All linting passed! 🎉" -ForegroundColor Green
    exit 0
}
else {
    Write-Host "Some linting failed. Check the output above." -ForegroundColor Red
    exit 1
}