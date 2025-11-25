# OpenBlue 协议栈本地构建指南

该项目是从 Zephyr 蓝牙子系统中提取的独立模块，使用 GNU Make 构建。目前默认启用 mbed TLS (CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS=y)，并支持“系统优先，本地回退”的依赖处理方式，以确保即使在干净的环境中也能稳定编译演示程序。

## 环境依赖
- gcc (推荐) 或 clang
- python3 (用于从 Kconfig 生成配置头文件)
- pip (kconfiglib 将在首次构建时自动安装)
- git (当系统依赖不可用时，用于获取本地 mbed TLS 源代码)
- Linux/Unix 环境 (示例依赖 pthread, rt)
- CMake (构建所需)

安装依赖:
```bash
sudo apt install libcmocka-dev libmbedtls-dev git cmake
```

## 依赖处理策略 (系统优先 + 本地回退)
当 `CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS=y` 时:
- 优先通过 `pkg-config` 检测系统安装的 mbed TLS:
  - 优先使用 `mbedcrypto`，如果未找到则回退到 `mbedtls`
  - 自动获取编译包含路径 (CFLAGS) 和链接库 (LIBS)
- 如果系统未提供 (`pkg-config` 未找到或包未找到)，构建过程将自动:
  - 获取并固定到指定版本: `third_party/mbedtls` (默认为 `v2.28.8` LTS)
  - 使用其 `library/Makefile` 生成 `libmbedcrypto.a`
  - 将 `third_party/mbedtls/include` 添加到编译包含路径

注意: 演示程序至少链接 `libmbedcrypto`；如果系统的 `pkg-config mbedtls` 返回额外的库 (例如 `mbedx509`, `mbedtls`)，它们也将被链接，而不会影响功能和稳定性。

## 使用 CMake 构建 (Linux)

### 先决条件

- `cmake` >= 3.16
- `pkg-config`
- `gcc` 或 `clang`
- `python3`
- `git` (用于 mbed TLS 回退)

### 创建构建目录
```bash
mkdir -p build
```

### 通过 Kconfig 配置 (menuconfig)

需要 Python3；如果缺少 `kconfiglib`，自定义的 `menuconfig` 目标将通过 `pip --user` 自动安装它。

```bash
# 打开 menuconfig 界面并编辑 .config
cmake --build build --target menuconfig

# 可选: 在不重新配置的情况下重新生成 autoconf.h
cmake --build build --target genconfig
```

注意: 编辑 `.config` 后，要么重新运行 `cmake -S . -B build` 以重新加载配置，要么运行 `genconfig` 来更新 `${build}/include/generated/autoconf.h`。

### 构建项目

您可以使用分步命令或单行命令。

**分步:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

**单行:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

### 注意事项

- 如果在首次运行时在仓库根目录中找不到 `.config` 文件，需要运行 `make --build build --target genconfig` 来生成默认配置。
- 构建产物将位于 `build/` 目录中:
  - `build/libopenblue.a`
  - `build/samples/demo/demo`
  - `build/samples/cmds/btcmd`
- 如果系统不提供 mbed TLS (即 `pkg-config` 找不到它)，构建系统将自动获取 `v2.28.8` 版本并将其静态链接为 `libmbedcrypto.a`。总是优先使用系统提供的库。

### 运行示例

成功构建后，您可以运行示例应用程序:
```bash
sudo ./build/samples/demo/demo
```
```bash
sudo ./build/samples/cmds/btcmd
```

## 使用 Make 构建 (Linux 可选)
在仓库根目录中执行:

### Kconfig 配置
如果找不到 `.config`，需要运行 `genconfig` 来生成 `include/generated/autoconf.h`
```bash
make genconfig
```
### 打开 Kconfig 菜单
启动 Kconfig menuconfig 界面，编辑配置并保存。
```bash
make menuconfig
```

### 构建项目
```bash
make clean; make all -j4
```

这将生成:
- `libopenblue.a`
- `samples/demo/demo`
- `samples/cmds/btcmd`

首次执行将自动安装 `kconfiglib` 并生成 `include/generated/autoconf.h`。当系统中未安装 mbed TLS 时，它将自动获取并构建本地的 `third_party/mbedtls` (固定版本)。

### 常用目标
- `make all`: 构建所有内容 (默认目标)
- `make demo`: 仅构建示例程序
- `make test`: 构建并运行单元测试
- `make clean`: 清理构建产物 (不清理 `third_party/mbedtls` 源代码和库)
- `make menuconfig`: 打开 Kconfig 菜单并更新 `.config`

### 运行示例程序
构建后，运行演示程序:
```bash
sudo ./samples/demo/demo
```

运行 btcmd 启动命令行界面:
```bash
sudo ./samples/cmds/btcmd
```

### 交叉编译
可以通过环境变量覆盖工具链:
`CROSS_COMPILE=aarch64-linux-gnu- make`

### 使用 make 为外部项目构建蓝牙模块
蓝牙模块已被分离到 `bluetooth/module.mk` 和 `bluetooth/Makefile` 中。它可以被外部项目直接包含，也可以在此仓库中作为独立的静态库构建。

- 关键变量:
  - `BT_PLATFORM`: 选择平台 (支持 `linux`, `nuttx`; `freertos` 为保留项)。
    - `linux`: 使用 `osdep/posix/os.c` 和 `drivers/hci_sock.c`。
    - `nuttx`: 使用 `drivers/h4.c` (目前仍使用 `osdep/posix/os.c` 以确保最小可行性)。
  - `BT_AUTOCONF_H`: 自动生成的配置头文件的路径 (默认为 `$(BT_ROOT)/include/generated/autoconf.h`)。该模块不负责生成此文件；它必须由父项目使用 Kconfig 工具链生成。
  - `BT_SRCS` / `BT_CPPFLAGS`: 模块公开的源文件列表和编译包含路径。它已经包含了 `-include $(BT_AUTOCONF_H)` 和 `-include shim/include/shim.h`。

## 故障排除
- 无法从网络获取 `third_party/mbedtls`:
  - 确保当前环境可以访问 GitHub，或者改为安装系统包 (例如 `sudo apt install libmbedtls-dev`，并确保 `pkg-config mbedcrypto` 可用) 然后重试。
- `pkg-config` 未找到:
  - 请安装 `pkg-config` (例如 `sudo apt install pkg-config`)，或依赖本地回退 (自动获取源代码)。
- 链接阶段报告 `mbedcrypto` 未找到:
  - 表明系统库不可用且回退构建未成功，请检查以上两项。