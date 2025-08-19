# Restore GUI

A Qt6-based application for managing Git-based checkpoints and restoring functionality. Provides a user-friendly graphical interface to create snapshots and restore files using Git's underlying functionality without requiring Git knowledge.

[![latest packaged version(s)](https://repology.org/badge/latest-versions/restore-gui.svg)](https://repology.org/project/restore-gui/versions)
[![build result](https://build.opensuse.org/projects/home:mx-packaging/packages/restore-gui/badge.svg?type=default)](https://software.opensuse.org//download.html?project=home%3Amx-packaging&package=restore-gui)

## Features

- **Git-based Snapshots**: Create and manage file checkpoints using Git
- **Simple Interface**: User-friendly Qt6-based GUI requiring no Git knowledge
- **Directory Tracking**: Monitor and track changes within any directory
- **Easy Restoration**: Roll back to previous states with a single click
- **Privilege Escalation**: Automatic privilege elevation when needed
- **Internationalization**: Multi-language support with 10+ translations
- **Cross-platform**: Works on Linux and other Unix-like systems

## Installation

### Prerequisites

- Qt6 (Core, Gui, Widgets, LinguistTools)
- C++20 compatible compiler (GCC 12+ or Clang)
- CMake 3.16+
- Ninja build system (recommended)
- Git (for underlying functionality)

### Building from Source

#### Quick Build
```bash
./build.sh           # Release build
./build.sh --debug   # Debug build
./build.sh --clean   # Clean rebuild
./build.sh --debian  # Build Debian package
```

#### Manual CMake Build
```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
sudo cmake --install build
```

#### Debian Package Build
```bash
./build.sh --debian
# Or manually:
debuild -us -uc
```

### Package Installation

On MX Linux and compatible Debian-based systems:
```bash
sudo apt update
sudo apt install restore-gui
```

## Usage

1. Launch the application from the application menu or run `restore-gui` in terminal
2. Select or navigate to the directory you want to track
3. The application will initialize Git tracking if not already present
4. Use the interface to:
   - **Create Checkpoint**: Save the current state of files
   - **View History**: Browse previous checkpoints
   - **Restore Files**: Roll back to a previous state
   - **Compare Changes**: See what has changed between checkpoints

**Note**: If you need more advanced Git options, use Git directly or other Git GUI programs.

## Technical Details

- **Language**: C++20
- **Framework**: Qt6
- **Build System**: CMake with Ninja generator
- **License**: GPL v3
- **Dependencies**: Git, Qt6, polkit (for privilege escalation)

## Development

### Code Style
- Modern C++20 with Qt6 framework
- Header guards using `#pragma once`
- Member variables use camelCase

### Contributing

1. Fork the repository
2. Create a feature branch
3. Follow the existing code style
4. Test your changes thoroughly
5. Submit a pull request

### Translation Contributions

- Please join Translation Forum: https://forum.mxlinux.org/viewforum.php?f=96
- Please register on Transifex: https://forum.mxlinux.org/viewtopic.php?t=38671
- Choose your language and start translating: https://app.transifex.com/anticapitalista/antix-development

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See [LICENSE](LICENSE) for the full license text.

## Authors

- **Adrian** <adrian@mxlinux.org>
- **MX Linux Team** <http://mxlinux.org>

## Links

- [MX Linux](http://mxlinux.org)
- [Source Repository](https://github.com/AdrianTM/restore-gui)
- [Issue Tracker](https://github.com/AdrianTM/restore-gui/issues)
