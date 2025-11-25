# OpenBlue Stack Local Build Guide

This project is an independent module extracted from the Zephyr Bluetooth subsystem, built using GNU Make. It currently enables mbed TLS by default (CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS=y) and supports "system-first, fallback-to-local" dependency handling to ensure stable compilation of demos even in clean environments.

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
cmake --build build -j
```

**One-liner:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

### Notes

- If a `.config` file is not found in the repository root on the first run, need to run `make --build build --target genconfig` to generate a default configuration.
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

## Build with Make (Linux Optional)
Execute in the repository root directory:

### Kconfig Configuration
if `.config` is not found, need to run `genconfig` to generate `include/generated/autoconf.h`
```bash
make genconfig
```
### Open Kconfig Menu
Launch the Kconfig menuconfig UI, edit the configuration, and save it.
```bash
make menuconfig
```

### Build the Project
```bash
make clean; make all
```

This will generate:
- `libopenblue.a`
- `samples/demo/demo`
- `samples/cmds/btcmd`

The first execution will automatically install `kconfiglib` and generate `include/generated/autoconf.h`. When mbed TLS is not installed in the system, it will automatically fetch and build the local `third_party/mbedtls` (fixed version).

### Common Targets
- `make all`: Build everything (default target)
- `make demo`: Build only the sample program
- `make test`: Build and run unit tests
- `make clean`: Clean build artifacts (does not clean `third_party/mbedtls` source code and libraries)
- `make menuconfig`: Open Kconfig menu and update `.config`

### Running the Sample Programs
After building, run the demo program:
```bash
sudo ./samples/demo/demo
```

run btcmd to start the command line interface:
```bash
sudo ./samples/cmds/btcmd
```

### Cross Compilation
Toolchain can be overridden via environment variables:
`CROSS_COMPILE=aarch64-linux-gnu- make`

### Building the Bluetooth Module for External Projects with make
The Bluetooth module has been separated into `bluetooth/module.mk` and `bluetooth/Makefile`. It can be directly included by external projects or built as a standalone static library within this repository.

- Key Variables:
  - `BT_PLATFORM`: Selects the platform (supports `linux`, `nuttx`; `freertos` is reserved).
    - `linux`: Uses `osdep/posix/os.c` and `drivers/hci_sock.c`.
    - `nuttx`: Uses `drivers/h4.c` (currently still uses `osdep/posix/os.c` to ensure minimum viability).
  - `BT_AUTOCONF_H`: Path to the auto-generated configuration header (defaults to `$(BT_ROOT)/include/generated/autoconf.h`). The module is not responsible for generating this file; it must be generated by the parent project using the Kconfig toolchain.
  - `BT_SRCS` / `BT_CPPFLAGS`: The list of source files and compile include paths exposed by the module. It already includes `-include $(BT_AUTOCONF_H)` and `-include shim/include/shim.h`.

## Troubleshooting
- Unable to fetch `third_party/mbedtls` from network:
  - Ensure current environment can access GitHub, or install system packages instead (e.g., `sudo apt install libmbedtls-dev`, and ensure `pkg-config mbedcrypto` is available) then retry.
- `pkg-config` not found:
  - Please install `pkg-config` (e.g., `sudo apt install pkg-config`), or rely on local fallback (automatically fetch source code).
- Linking stage reports `mbedcrypto` not found:
  - Indicates system library is unavailable and fallback build was not successful, please check the above two items.