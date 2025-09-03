<#
.SYNOPSIS
    Run all tests for PC Monitor backends
.DESCRIPTION
    Runs tests for Python, TypeScript, and C# backends
.EXAMPLE
    .\Test-All.ps1
#>

param()

# Set encoding for Korean text support
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$ErrorActionPreference = "Continue"

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "   PC Monitor - Run All Tests" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

$startTime = Get-Date
$successCount = 0
$totalTests = 3

# Python Tests
Write-Host "[1/3] Running Python tests..." -ForegroundColor Cyan
try {
    Push-Location "backend-python"
    & uv run pytest
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Python tests passed" -ForegroundColor Green
        $successCount++
    }
    else {
        Write-Host "❌ Python tests failed" -ForegroundColor Red
    }
}
catch {
    Write-Host "❌ Python test execution failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# TypeScript Tests
Write-Host "[2/3] Running TypeScript tests..." -ForegroundColor Cyan
try {
    Push-Location "backend-typescript"
    & npm test
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ TypeScript tests passed" -ForegroundColor Green
        $successCount++
    }
    else {
        Write-Host "❌ TypeScript tests failed" -ForegroundColor Red
    }
}
catch {
    Write-Host "❌ TypeScript test execution failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# C# Tests
Write-Host "[3/3] Running C# tests..." -ForegroundColor Cyan
try {
    Push-Location "backend-csharp"
    & dotnet test
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ C# tests passed" -ForegroundColor Green
        $successCount++
    }
    else {
        Write-Host "❌ C# tests failed" -ForegroundColor Red
    }
}
catch {
    Write-Host "❌ C# test execution failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}

# Summary
$duration = (Get-Date) - $startTime
Write-Host ""
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "Test Summary:" -ForegroundColor White
Write-Host "  Passed: $successCount/$totalTests" -ForegroundColor $(if ($successCount -eq $totalTests) { "Green" } else { "Yellow" })
Write-Host "  Duration: $($duration.ToString('mm\:ss'))" -ForegroundColor White
Write-Host "====================================" -ForegroundColor Cyan

if ($successCount -eq $totalTests) {
    Write-Host "All tests passed! 🎉" -ForegroundColor Green
    exit 0
}
else {
    Write-Host "Some tests failed. Check the output above." -ForegroundColor Red
    exit 1
}