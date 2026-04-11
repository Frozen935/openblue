## OpenVela Platform

The OpenVela integration layout is the same as the Nemos integration: place one
layer of platform wrapper files under `external/openblue/`, and keep the actual
OpenBlue source tree in the `external/openblue/openblue/` subdirectory.

### Directory Layout

The integrated directory layout should look like this:

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

### Integration Steps

1. Create the platform wrapper directory:

   ```bash
   mkdir -p $(ROOTDIR)/external/openblue
   ```

2. Clone the OpenBlue source tree into the `openblue/` subdirectory:

   ```bash
   git clone git@github.com:Frozen935/openblue.git \
     $(ROOTDIR)/external/openblue/openblue
   ```

3. Copy the platform wrapper files from `platform/openvela/` into
   `$(ROOTDIR)/external/openblue/`:

   ```bash
   cp platform/openvela/CMakeLists.txt $(ROOTDIR)/external/openblue/
   cp platform/openvela/Kconfig $(ROOTDIR)/external/openblue/
   cp platform/openvela/Make.defs $(ROOTDIR)/external/openblue/
   cp platform/openvela/Makefile $(ROOTDIR)/external/openblue/
   ```

4. Enable the following options in the target OpenVela configuration:

   - `CONFIG_LIB_OPENBLUE=y`
   - `CONFIG_OPENBLUE_PLATFORM_NUTTX=y`
   - `CONFIG_OPENBLUE_SAMPLES=y`
   - If `btcmd` is enabled, `CONFIG_BT_SHELL=y` must also be enabled.

5. Enable the following applications if needed:

   - `CONFIG_OPENBLUE_APP_BTCMD=y`
   - `CONFIG_OPENBLUE_APP_DEMO=y`
   - `CONFIG_OPENBLUE_APP_TEST=y`

6. For test commands and crypto support, make sure the repository configuration
   already enables:

   - `mbedtls` related options
   - `cmocka` related options

### Notes

- `platform/openvela/` only contains OpenVela/NuttX-side wrapper files and does
  not change the OpenBlue source tree layout.
- The wrapper files assume the source tree is located at
  `external/openblue/openblue/`. Do not flatten the source tree directly into
  the `external/openblue/` root directory.
- `btcmd` depends on `BT_SHELL`, so do not enable `OPENBLUE_BT_CMD` alone when
  `BT_SHELL` is disabled.
