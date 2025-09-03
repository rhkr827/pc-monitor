<#
.SYNOPSIS
    Clean all build artifacts for PC Monitor
.DESCRIPTION
    Removes all build directories and artifacts from all backends
.PARAMETER Force
    Force removal without confirmation
.EXAMPLE
    .\Clean-All.ps1 -Force
#>

param(
    [switch]$Force = $false
)

# Set encoding for Korean text support
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "   PC Monitor - Clean All Builds" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

if (-not $Force) {
    $confirm = Read-Host "This will delete all build artifacts. Continue? (y/N)"
    if ($confirm -ne "y" -and $confirm -ne "Y") {
        Write-Host "Cancelled by user." -ForegroundColor Yellow
        exit 0
    }
}

$cleanDirs = @(
    "frontend\dist",
    "backend-rust\target",
    "backend-python\dist",
    "backend-typescript\dist",
    "backend-csharp\bin",
    "backend-csharp\obj",
    "backend-cpp\build",
    "dist"
)

$removedCount = 0

foreach ($dir in $cleanDirs) {
    if (Test-Path $dir) {
        try {
            Write-Host "Removing: $dir" -ForegroundColor Yellow
            Remove-Item $dir -Recurse -Force
            Write-Host "✅ Removed: $dir" -ForegroundColor Green
            $removedCount++
        }
        catch {
            Write-Host "❌ Failed to remove: $dir - $($_.Exception.Message)" -ForegroundColor Red
        }
    }
    else {
        Write-Host "⏭️  Not found: $dir" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "Clean Summary:" -ForegroundColor White
Write-Host "  Directories removed: $removedCount/$($cleanDirs.Count)" -ForegroundColor Green
Write-Host "====================================" -ForegroundColor Cyan

Write-Host "Clean operation completed! 🧹" -ForegroundColor Green