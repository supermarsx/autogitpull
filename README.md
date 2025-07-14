# autogitpull
Automatic Git Puller & Monitor

## Dependencies
This tool relies on [libgit2](https://libgit2.org/). The helper scripts
`install_deps.sh` (Linux/macOS) and `install_deps.bat` (Windows) automatically
download and install `libgit2` when needed.  The project links against the
static version of the library so the final executable does not depend on a
separate `libgit2` DLL.  If you prefer to install the library yourself, follow
the instructions below.

### Installing libgit2 on Linux
```
sudo apt-get update && sudo apt-get install -y libgit2-dev     # Debian/Ubuntu
sudo yum install -y libgit2-devel                              # RHEL/Fedora
```

### Installing libgit2 on Windows
The easiest route on Windows is to use
[vcpkg](https://github.com/microsoft/vcpkg):

```
git clone https://github.com/microsoft/vcpkg
cd vcpkg && bootstrap-vcpkg.bat
vcpkg\vcpkg install libgit2
```

Ensure that the resulting `vcpkg` `installed` folder is on your `LIB` and
`INCLUDE` paths when compiling.

The Windows build scripts (`compile.bat` and `compile-cl.bat`) look for a
`vcpkg` directory in the project root or use the `VCPKG_ROOT` environment
variable. Make sure one of these is set so the headers (`git2.h`) and the
library can be found. vcpkg names its static libraries using the `.lib`
extension, and `compile.bat` links against `git2.lib` directly.

## Building
### Using the provided scripts
Run `make` (Linux/macOS) or `compile.bat` (MinGW) to build the project. These
scripts call `install_deps` when `libgit2` is missing. The binary is produced as
`autogitpull` (or `autogitpull.exe` on Windows).

Clean up intermediate files with `make clean`.

### Building with CMake
Alternatively, configure the project with CMake:

```bash
cmake -S . -B build
cmake --build build
```

The resulting executable will be in the `build` directory.

Usage: `autogitpull <root-folder> [--include-private] [--show-skipped] [--interval <seconds>] [--log-dir <path>] [--help]`

Available options:

* `--include-private` – include private or non-GitHub repositories in the scan.
* `--show-skipped` – display repositories that were skipped because they are non-GitHub or require authentication.
* `--interval <seconds>` – delay between automatic scans (default 60).
* `--log-dir <path>` – directory where pull logs will be written.
* `--help` – show the usage information and exit.

By default, repositories whose `origin` remote does not point to GitHub or require authentication are skipped during scanning. Use `--include-private` to include them. Skipped repositories are hidden from the TUI unless `--show-skipped` is also provided.

Provide `--log-dir <path>` to store pull logs for each repository. After every pull operation the log
is written to a timestamped file inside this directory and its location is shown in the TUI.

## Runtime requirements
* **Git** must be available in your `PATH` for libgit2 to interact with repositories.
* Network access is required to contact remote Git servers when pulling updates.
* The application prints ANSI color codes; on Windows run it in a terminal that
  supports color (e.g. Windows Terminal or recent PowerShell).

## Licensing
autogitpull is licensed under the MIT license (see `LICENSE`). The project
bundles the license for the statically linked libgit2 library in
`LIBGIT2_LICENSE`.
