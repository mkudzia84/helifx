# ScaleFX Studio Configuration Editor

Windows GUI application for editing ScaleFX Hub `config.yaml` files.

## Features

- Tree view navigation of configuration sections
- Property grid for editing values with descriptions
- Save/Load YAML configuration files
- Reset to defaults
- Validates configuration structure

## Building

### Prerequisites

- [.NET 8 SDK](https://dotnet.microsoft.com/download/dotnet/8.0)
- Windows 10/11

### Build Commands

```powershell
# Navigate to project directory
cd app\win32\ScaleFXStudio

# Build debug version
dotnet build

# Build release version
dotnet build -c Release

# Publish single-file executable (self-contained, no .NET runtime needed)
dotnet publish -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true

# Output will be in: bin\Release\net8.0-windows\win-x64\publish\ScaleFXStudio.exe
```

### Quick Build Script

```powershell
# From project root
dotnet publish app\win32\ScaleFXStudio -c Release -o dist\win32
```

## Usage

1. **Open**: File → Open, select a `config.yaml` file
2. **Navigate**: Click sections in the tree view on the left
3. **Edit**: Modify properties in the property grid on the right
4. **Save**: File → Save or Save As

## Configuration Sections

- **Engine FX**: Engine sound configuration (start, run, stop sounds)
- **Gun FX**: Gun trigger, smoke, turret servos, and firing rates

## Deployment

The published executable is completely self-contained:
- Single `.exe` file (~15-25 MB)
- No .NET runtime installation required
- Just copy and run

## Development

The application uses:
- **WinForms** for the GUI
- **YamlDotNet** for YAML parsing/serialization
- **PropertyGrid** for auto-generated property editors
