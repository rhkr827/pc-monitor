<#
.SYNOPSIS
    Build all PC Monitor backends
.DESCRIPTION
    Builds all backend implementations (Frontend, Rust, Python, TypeScript, C#, C++)
.PARAMETER Release
    Build in release mode (default: debug)
.PARAMETER SkipFrontend
    Skip frontend build
.EXAMPLE
    .\Build-All.ps1 -Release
#>

param(
    [switch]$Release = $false,
    [switch]$SkipFrontend = $false
)

# Set encoding for Korean text support
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$ErrorActionPreference = "Stop"

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "   PC Monitor - Build All Backends" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

$buildMode = if ($Release) { "Release" } else { "Debug" }
Write-Host "Build Mode: $buildMode" -ForegroundColor Yellow
Write-Host ""

$startTime = Get-Date
$successCount = 0
$totalBuilds = if ($SkipFrontend) { 5 } else { 6 }

# Frontend
if (-not $SkipFrontend) {
    Write-Host "[1/6] Building Frontend..." -ForegroundColor Cyan
    try {
        Push-Location "frontend"
        & npm run build
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✅ Frontend build successful" -ForegroundColor Green
            $successCount++
        }
        else {
            throw "Frontend build failed"
        }
    }
    catch {
        Write-Host "❌ Frontend build failed: $($_.Exception.Message)" -ForegroundColor Red
    }
    finally {
        Pop-Location
    }
    Write-Host ""
}

# Rust
Write-Host "[$($SkipFrontend ? 1 : 2)/$totalBuilds] Building Rust backend..." -ForegroundColor Cyan
try {
    $buildArgs = @()
    if ($Release) { $buildArgs += "-Release" }
    & .\scripts\Build-Rust.ps1 @buildArgs
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Rust build successful" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "Rust build failed"
    }
}
catch {
    Write-Host "❌ Rust build failed: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# Python
Write-Host "[$($SkipFrontend ? 2 : 3)/$totalBuilds] Building Python backend..." -ForegroundColor Cyan
try {
    Push-Location "backend-python"
    & npm run build
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Python build successful" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "Python build failed"
    }
}
catch {
    Write-Host "❌ Python build failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# TypeScript
Write-Host "[$($SkipFrontend ? 3 : 4)/$totalBuilds] Building TypeScript backend..." -ForegroundColor Cyan
try {
    Push-Location "backend-typescript"
    & npm run build
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ TypeScript build successful" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "TypeScript build failed"
    }
}
catch {
    Write-Host "❌ TypeScript build failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# C#
Write-Host "[$($SkipFrontend ? 4 : 5)/$totalBuilds] Building C# backend..." -ForegroundColor Cyan
try {
    Push-Location "backend-csharp"
    & dotnet build
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ C# build successful" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "C# build failed"
    }
}
catch {
    Write-Host "❌ C# build failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
}
Write-Host ""

# C++
Write-Host "[$($SkipFrontend ? 5 : 6)/$totalBuilds] Building C++ backend..." -ForegroundColor Cyan
try {
    Push-Location "backend-cpp"
    
    if (-not (Test-Path "build")) {
        New-Item -ItemType Directory -Path "build" -Force | Out-Null
    }
    
    Push-Location "build"
    & cmake -A x64 ..
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }
    
    $config = if ($Release) { "Release" } else { "Debug" }
    & cmake --build . --config $config
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ C++ build successful" -ForegroundColor Green
        $successCount++
    }
    else {
        throw "C++ build failed"
    }
}
catch {
    Write-Host "❌ C++ build failed: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    Pop-Location
    Pop-Location
}

# Summary
$duration = (Get-Date) - $startTime
Write-Host ""
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "Build Summary:" -ForegroundColor White
Write-Host "  Successful: $successCount/$totalBuilds" -ForegroundColor $(if ($successCount -eq $totalBuilds) { "Green" } else { "Yellow" })
Write-Host "  Duration: $($duration.ToString('mm\:ss'))" -ForegroundColor White
Write-Host "====================================" -ForegroundColor Cyan

if ($successCount -eq $totalBuilds) {
    Write-Host "All builds completed successfully! 🎉" -ForegroundColor Green
    exit 0
}
else {
    Write-Host "Some builds failed. Check the output above." -ForegroundColor Red
    exit 1
}