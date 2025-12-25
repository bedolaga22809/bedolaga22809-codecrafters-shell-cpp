[![progress-banner](https://backend.codecrafters.io/progress/shell/bcdceccf-0c5b-4e41-b8ac-4f5ee536eeb4)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

# kubsh — POSIX Shell Implementation in C++17

A custom shell with FUSE-based virtual filesystem support.

## Installation

### From DEB package
```bash
sudo apt-get update
sudo apt-get install -y ./kubsh.deb
```

The package automatically installs its dependency: `libfuse3-3` or `libfuse3-4`

### From source
```bash
# Install build dependencies
sudo apt-get install -y build-essential libfuse3-dev

# Build and test
make build
make deb
sudo dpkg -i kubsh.deb
```

## Quick Test

```bash
kubsh
$ echo "Hello, World!"
Hello, World!
$ \q
```

## Building

```bash
make build    # Compile the shell
make run      # Run interactively
make deb      # Create DEB package
make clean    # Remove build artifacts
```

## Features

- REPL shell with command history (`~/kubsh_history`)
- Built-in commands: `echo`, `debug`, `\e $VAR`, `\l /dev/device`, `history`, `\q`
- Full external command execution support
- FUSE virtual filesystem at `/users/{username}/{id|home|shell}`
- Signal handling (SIGHUP)
- Single-quote string parsing

## Project Structure

```
src/
├── main.cpp     # Shell REPL implementation
└── vfs.cpp      # FUSE virtual filesystem

kubsh-package/  # DEB package files
```

## Dependencies

- **Runtime**: `libfuse3-3` or `libfuse3-4`
- **Build**: `g++` (C++17), `make`, `libfuse3-dev`

---

**Original Challenge**: [Build Your Own Shell](https://app.codecrafters.io/courses/shell/overview)
