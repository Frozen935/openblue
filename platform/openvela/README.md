## OpenVela Platform

OpenVela 的接入方式和 Nemos 一致：在 `external/openblue/` 下放一层平台包装文件，
真正的 OpenBlue 源码放在 `external/openblue/openblue/` 子目录中。

### 目录布局

接入后的目录应为：

```text
$(ROOTDIR)/external/openblue/
├── CMakeLists.txt
├── Kconfig
├── Make.defs
├── Makefile
└── openblue/
    ├── CMakeLists.txt
    ├── Kconfig
    ├── bluetooth/
    ├── base/
    ├── core/
    ├── drivers/
    └── ...
```

### 接入步骤

1. 创建平台包装目录：

   ```bash
   mkdir -p $(ROOTDIR)/external/openblue
   ```

2. 将 OpenBlue 源码克隆到 `openblue/` 子目录：

   ```bash
   git clone git@github.com:Frozen935/openblue.git \
     $(ROOTDIR)/external/openblue/openblue
   ```

3. 将 `platform/openvela/` 下的平台包装文件复制到
   `$(ROOTDIR)/external/openblue/`：

   ```bash
   cp platform/openvela/CMakeLists.txt $(ROOTDIR)/external/openblue/
   cp platform/openvela/Kconfig $(ROOTDIR)/external/openblue/
   cp platform/openvela/Make.defs $(ROOTDIR)/external/openblue/
   cp platform/openvela/Makefile $(ROOTDIR)/external/openblue/
   ```

4. 在目标 OpenVela 配置中启用：

   - `CONFIG_LIB_OPENBLUE=y`
   - `CONFIG_OPENBLUE_PLATFORM_NUTTX=y`
   - `CONFIG_OPENBLUE_SAMPLES=y`
   - 若启用 `btcmd`，必须同时启用 `CONFIG_BT_SHELL=y`

5. 如需启用命令：

   - `CONFIG_OPENBLUE_APP_BTCMD=y`
   - `CONFIG_OPENBLUE_APP_DEMO=y`
   - `CONFIG_OPENBLUE_APP_TEST=y`

6. 如需测试命令与加密支持，请确保仓内已打开：

   - `mbedtls` 相关配置
   - `cmocka` 相关配置

### 说明

- `platform/openvela/` 只放 OpenVela/NuttX 侧包装文件，不修改 OpenBlue 源码目录结构。
- 包装文件默认假设源码位于 `external/openblue/openblue/`，不要直接把源码铺平到
  `external/openblue/` 根目录。
- `btcmd` 依赖 `BT_SHELL`，因此不开 `BT_SHELL` 时不要单独启用 `OPENBLUE_BT_CMD`。
