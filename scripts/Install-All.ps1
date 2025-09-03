<#
.SYNOPSIS
    Install dependencies for all PC Monitor backends
.DESCRIPTION
    Installs dependencies for Frontend, Python, TypeScript, and C# backends
.EXAMPLE
    .\Install-All.ps1
#>

param()

# Set encoding for Korean text support
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$ErrorActionPreference = "Stop"

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "   PC Monitor - Install All Dependencies" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

$startTime = Get-Date
$successCount = 0
$totalInstalls = 4

# Frontend
Write-Host "[1/4] Installing Frontend dependencies..." -ForegroundColor Cyan
try {
    Push-Location "frontend"
    & npm install
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Frontend dependencies installed" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "Frontend npm install failed"
    }
}
catch {
    Write-Host "❌ Frontend install failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# Python
Write-Host "[2/4] Installing Python dependencies..." -ForegroundColor Cyan
try {
    Push-Location "backend-python"
    & npm install
    if ($LASTEXITCODE -ne 0) { throw "Python npm install failed" }
    & uv sync
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Python dependencies installed" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "Python uv sync failed"
    }
}
catch {
    Write-Host "❌ Python install failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# TypeScript
Write-Host "[3/4] Installing TypeScript dependencies..." -ForegroundColor Cyan
try {
    Push-Location "backend-typescript"
    & npm install
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ TypeScript dependencies installed" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "TypeScript npm install failed"
    }
}
catch {
    Write-Host "❌ TypeScript install failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# C#
Write-Host "[4/4] Installing C# dependencies..." -ForegroundColor Cyan
try {
    Push-Location "backend-csharp"
    & dotnet restore
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ C# dependencies restored" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "C# dotnet restore failed"
    }
}
catch {
    Write-Host "❌ C# install failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}

# Summary
$duration = (Get-Date) - $startTime
Write-Host ""
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "Install Summary:" -ForegroundColor White
Write-Host "  Successful: $successCount/$totalInstalls" -ForegroundColor $(if ($successCount -eq $totalInstalls) { "Green" } else { "Yellow" })
Write-Host "  Duration: $($duration.ToString('mm\:ss'))" -ForegroundColor White
Write-Host "====================================" -ForegroundColor Cyan

if ($successCount -eq $totalInstalls) {
    Write-Host "All dependencies installed successfully! 🎉" -ForegroundColor Green
    exit 0
}
else {
    Write-Host "Some installations failed. Check the output above." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Note: C++ dependencies are handled automatically during build" -ForegroundColor Gray