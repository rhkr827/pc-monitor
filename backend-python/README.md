# PC Monitor Python Backend (uv)

Python backend implementation using uv package manager for faster dependency management.

## Prerequisites

### Install uv (if not already installed)

**Windows (PowerShell):**
```powershell
powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
```

**Alternative methods:**
```bash
# Via pip
pip install uv

# Via pipx  
pipx install uv

# Via conda
conda install -c conda-forge uv
```

## Quick Start

### 1. Install Dependencies
```bash
# Install Python dependencies with uv
uv sync

# Install Node.js dependencies for Electron
npm install
```

### 2. Development
```bash
# Run Python server only
uv run python monitor.py

# Run full Electron app (Python + Electron)
npm run dev

# Alternative development commands
npm run python:dev     # Python server only
npm run python:lint    # Lint Python code
npm run python:format  # Format Python code
npm run python:test    # Run Python tests
```

### 3. Build
```bash
npm run build
```

## uv Commands

### Dependency Management
```bash
# Add new dependency
uv add psutil

# Add development dependency  
uv add --dev pytest

# Remove dependency
uv remove package-name

# Update all dependencies
uv sync --upgrade

# Show dependency tree
uv tree
```

### Development Tools
```bash
# Run with uv
uv run python monitor.py

# Run specific tool
uv run ruff check .     # Lint
uv run black .          # Format
uv run pytest          # Test
uv run mypy .           # Type check
```

### Virtual Environment
```bash
# Activate virtual environment
uv shell

# Run command in environment
uv run <command>

# Check Python version
uv python --version
```

## Migration from pip

The following files are now managed by uv:
- `pyproject.toml` - Project configuration and dependencies
- `.python-version` - Python version specification
- `uv.lock` - Lock file (auto-generated, like package-lock.json)

**Old files (can be removed after migration):**
- `requirements.txt` - Replaced by pyproject.toml
- Any virtual environment folders

## Port Configuration

- **Python API Server**: http://localhost:8080
- **WebSocket**: ws://localhost:8080/ws/stats
- **Electron App**: Managed by Electron main process

## Performance Benefits

uv provides significant performance improvements:
- **10-100x faster** dependency resolution
- **Faster installs** compared to pip
- **Better caching** and dependency management
- **Built-in virtual environment** management