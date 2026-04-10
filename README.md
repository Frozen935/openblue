# OpenBlue Stack Local Build Guide

This project is an independent module extracted from the Zephyr Bluetooth subsystem, built using GNU Make. It currently enables mbed TLS by default (CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS=y) and supports "system-first, fallback-to-local" dependency handling to ensure stable compilation of demos even in clean environments.

Change history is tracked in `CHANGELOG_OPENBLUE.md`.

## Environment Dependencies
- gcc (recommended) or clang
- python3 (for generating configuration header files from Kconfig)
- pip (kconfiglib will be automatically installed on first build)
- git (used to fetch local mbed TLS source code when system dependencies are not available)
- Linux/Unix environment (examples depend on pthread, rt)
- CMake (required for building)

install dependencies:
```bash
sudo apt install libcmocka-dev libmbedtls-dev git cmake
```

## Dependency Handling Strategy (System-First + Local Fallback)
When `CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS=y`:
- Prioritize detecting system-installed mbed TLS via `pkg-config`:
  - Prefer `mbedcrypto`, fallback to `mbedtls` if not found
  - Automatically obtain compilation include paths (CFLAGS) and link libraries (LIBS)
- If not provided by system (`pkg-config` not found or package not found), the build process will automatically:
  - Fetch and fix to specified version: `third_party/mbedtls` (default `v2.28.8` LTS)
  - Use its `library/Makefile` to generate `libmbedcrypto.a`
  - Add `third_party/mbedtls/include` to compilation include paths

Note: The demo links at least `libmbedcrypto`; if the system's `pkg-config mbedtls` returns additional libraries (such as `mbedx509`, `mbedtls`), they will also be linked without affecting functionality and stability.

## Build with CMake (Linux)

### Prerequisites

- `cmake` >= 3.16
- `pkg-config`
- `gcc` or `clang`
- `python3`
- `git` (for mbed TLS fallback)

### Create Build Directory
```bash
mkdir -p build
```

### Configure via Kconfig (menuconfig)

Requires Python3; if `kconfiglib` is missing, the custom `menuconfig` target will install it via `pip --user` automatically.

```bash
# open the menuconfig UI and edit .config
cmake --build build --target menuconfig

# optionally: regenerate autoconf.h without reconfiguring
cmake --build build --target genconfig
```

Note: After editing `.config`, either rerun `cmake -S . -B build` to reload configuration, or run `genconfig` to update `${build}/include/generated/autoconf.h`.

### Build the Project

You can use either the step-by-step commands or the one-liner.

**Step-by-step:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

**One-liner:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

### Notes

- If a `.config` file is not found in the repository root on the first run, run `cmake --build build --target genconfig` to generate a default configuration.
- The build artifacts will be located in the `build/` directory:
  - `build/libopenblue.a`
  - `build/samples/demo/demo`
  - `build/samples/cmds/btcmd`
- If the system does not provide mbed TLS (i.e., it's not found by `pkg-config`), the build system will automatically fetch version `v2.28.8` and statically link it as `libmbedcrypto.a`. System-provided libraries are always preferred.

### Running the Samples

After a successful build, you can run the sample applications:
```bash
sudo ./build/samples/demo/demo
```
```bash
sudo ./build/samples/cmds/btcmd
```

## Troubleshooting
- Unable to fetch `third_party/mbedtls` from network:
  - Ensure current environment can access GitHub, or install system packages instead (e.g., `sudo apt install libmbedtls-dev`, and ensure `pkg-config mbedcrypto` is available) then retry.
- `pkg-config` not found:
  - Please install `pkg-config` (e.g., `sudo apt install pkg-config`), or rely on local fallback (automatically fetch source code).
- Linking stage reports `mbedcrypto` not found:
  - Indicates system library is unavailable and fallback build was not successful, please check the above two items.
