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

#### MSVC (Visual Studio)
Use [vcpkg](https://github.com/microsoft/vcpkg):

```
git clone https://github.com/microsoft/vcpkg
cd vcpkg && bootstrap-vcpkg.bat
vcpkg\vcpkg install libgit2
```

Ensure the resulting `installed` folder is on your `LIB` and `INCLUDE`
paths when compiling with `compile-cl.bat`.

#### MinGW
Run `install_libgit2_mingw.bat` to build libgit2 natively with MinGW. The
script installs the static library and headers under `libgit2\_install`.

`compile-cl.bat` expects a vcpkg installation while `compile.bat` uses the
library produced by `install_libgit2_mingw.bat` and will call it
automatically if `libgit2\_install` is missing.

## Building
### Using the provided scripts
Run `make` (Linux/macOS), `compile.bat` (MinGW) or `compile-cl.bat` (MSVC) to
build the project. `compile.bat` invokes `install_libgit2_mingw.bat` when
`libgit2` is missing. The binary is produced as `autogitpull` (or
`autogitpull.exe` on Windows).

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
