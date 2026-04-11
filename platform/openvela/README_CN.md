## OpenVela 平台

OpenVela 的接入布局与 Nemos 的接入方式一致：在 `external/openblue/` 下放置一层
平台包装文件，真正的 OpenBlue 源码树位于
`external/openblue/openblue/` 子目录中。

### 目录布局

接入后的目录结构应如下所示：

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

2. 将 OpenBlue 源码树克隆到 `openblue/` 子目录：

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

4. 在目标 OpenVela 配置中启用以下选项：

   - `CONFIG_LIB_OPENBLUE=y`
   - `CONFIG_OPENBLUE_PLATFORM_NUTTX=y`
   - `CONFIG_OPENBLUE_SAMPLES=y`
   - 如果启用了 `btcmd`，还必须同时启用 `CONFIG_BT_SHELL=y`。

5. 如有需要，启用以下应用：

   - `CONFIG_OPENBLUE_APP_BTCMD=y`
   - `CONFIG_OPENBLUE_APP_DEMO=y`
   - `CONFIG_OPENBLUE_APP_TEST=y`

6. 如需测试命令和加密支持，请确保仓库配置中已经启用：

   - `mbedtls` 相关选项
   - `cmocka` 相关选项

### 说明

- `platform/openvela/` 只包含 OpenVela/NuttX 侧的包装文件，不会修改 OpenBlue
  源码树布局。
- 包装文件默认假设源码树位于 `external/openblue/openblue/`。不要将源码树直接铺平到
  `external/openblue/` 根目录下。
- `btcmd` 依赖 `BT_SHELL`，因此在 `BT_SHELL` 未启用时，不要单独启用
  `OPENBLUE_BT_CMD`。
