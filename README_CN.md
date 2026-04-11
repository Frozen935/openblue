# OpenBlue 协议栈本地构建指南

该项目是从 Zephyr 蓝牙子系统中提取的独立模块，使用 CMake 构建。目前默认启用 mbed TLS (CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS=y)，并支持“系统优先，本地回退”的依赖处理方式，以确保即使在干净的环境中也能稳定编译演示程序。

修改记录见 `CHANGELOG_OPENBLUE.md`。

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
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j4
```

**GDB 调试构建:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j4
```

### 注意事项

- 如果在首次运行时在仓库根目录中找不到 `.config` 文件，需要运行 `cmake --build build --target genconfig` 来生成默认配置。
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

## 故障排除
- 无法从网络获取 `third_party/mbedtls`:
  - 确保当前环境可以访问 GitHub，或者改为安装系统包 (例如 `sudo apt install libmbedtls-dev`，并确保 `pkg-config mbedcrypto` 可用) 然后重试。
- `pkg-config` 未找到:
  - 请安装 `pkg-config` (例如 `sudo apt install pkg-config`)，或依赖本地回退 (自动获取源代码)。
- 链接阶段报告 `mbedcrypto` 未找到:
  - 表明系统库不可用且回退构建未成功，请检查以上两项。
