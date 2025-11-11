# ByteBlue Stack 本地构建指南（启用 mbed TLS）

本项目是从 Zephyr Bluetooth 子系统抽取的独立模块，使用 GNU Make 构建。当前默认启用 mbed TLS（CONFIG_BYTEBLUE_CRYPTO_USE_MBEDTLS=y），支持“系统优先、缺省回退至本地”的依赖处理，以保证在干净环境也可稳定编译 demo。

## 环境依赖
- gcc（建议）或 clang
- python3（用于从 Kconfig 生成配置头文件）
- pip（首次构建会自动安装 `kconfiglib`）
- git（用于在无系统依赖时拉取本地 mbed TLS 源码）
- Linux/Unix 环境（示例依赖 pthread、rt）

## 依赖处理策略（系统优先 + 本地回退）
当 `CONFIG_BYTEBLUE_CRYPTO_USE_MBEDTLS=y` 时：
- 优先通过 `pkg-config` 检测系统安装的 mbed TLS：
  - 首选 `mbedcrypto`，若不存在则尝试 `mbedtls`
  - 自动获取编译包含路径（CFLAGS）与链接库（LIBS）
- 若系统未提供（`pkg-config` 不存在或未找到相应包），构建流程会自动：
  - 拉取并固定到指定版本：`third_party/mbedtls`（默认 `v2.28.8` LTS）
  - 使用其 `library/Makefile` 生成 `libmbedcrypto.a`
  - 将 `third_party/mbedtls/include` 加入编译包含路径

说明：demo 至少链接 `libmbedcrypto`；若系统的 `pkg-config mbedtls` 返回额外库（如 `mbedx509`, `mbedtls`），也会一并链接，不影响功能与稳定性。

## 一键构建
在仓库根目录执行：

```
make clean; make
```

将生成：
- `libzblue.a`
- `samples/demo/demo`

首次执行会自动安装 `kconfiglib` 并生成 `include/generated/autoconf.h`。当系统未安装 mbed TLS 时，会自动拉取并构建本地 `third_party/mbedtls`（固定版本）。

## 常用目标
- `make demo`：仅构建示例程序
- `make clean`：清理构建产物（不清理 `third_party/mbedtls` 源码与库）
- `make menuconfig`：打开 Kconfig 菜单并更新 `.config`

## 交叉编译
可通过环境变量覆盖工具链：

```
CROSS_COMPILE=aarch64-linux-gnu- make
```

## 故障排查
- 无法从网络拉取 `third_party/mbedtls`：
  - 请确保当前环境可访问 GitHub，或改为安装系统包（如 `sudo apt install libmbedtls-dev`，并确保 `pkg-config mbedcrypto` 可用）后重试。
- `pkg-config` 不存在：
  - 请安装 `pkg-config`（如 `sudo apt install pkg-config`），或依赖本地回退（自动拉取源码）。
- 链接阶段报找不到 `mbedcrypto`：
  - 表示系统库不可用且回退未成功构建，请检查上两项。

## 备注
- 示例程序位于 `samples/demo/main.c`，包含最小化的蓝牙初始化调用。
- 若需要启用更多特性，请在 `make menuconfig` 中调整配置后重新构建。
