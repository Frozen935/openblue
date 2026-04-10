# OpenBlue 修改记录

本文档用于固化 `openblue` 仓库内的重要修改记录，尤其是：
- 与 Zephyr 主线同步/对齐相关的改动
- 构建系统、Kconfig、平台适配、公共接口等基础设施改动
- 影响范围较大、后续回溯成本较高的行为调整

## 维护规则

建议每次完成一组可独立说明的修改后，追加一条记录；至少在 `git commit` 前后同步补齐。每条记录建议包含以下字段：

```md
## YYYY-MM-DD - 标题
- Commit: `提交 SHA`（未提交可写 `working tree`）
- 类型: `sync` / `build` / `kconfig` / `feature` / `fix` / `refactor`
- 范围: `涉及目录或模块`
- 背景: 为什么要改
- 修改: 做了什么
- 影响: 对构建、接口、行为、依赖的影响
- 验证: 已执行的验证命令或检查结果
- 关联上游: `Zephyr SHA`（若无可省略）
```

## 当前记录

## 2026-04-10 - Debug 构建补齐 `CONFIG_BT_SETTINGS` 条件保护
- Commit: `working tree`
- 类型: `fix`
- 范围: `bluetooth/host/gatt.c`
- 背景: 开启 `-DCMAKE_BUILD_TYPE=Debug` 后，`static void sc_store(struct gatt_sc_cfg *cfg)` 在 `CONFIG_BT_SETTINGS` 未启用的情况下仍调用 `bt_settings_store_sc(...)`，导致编译失败。
- 修改: 在 `sc_store()` 中为 `bt_settings_store_sc(...)` 及相关日志补上 `#if defined(CONFIG_BT_SETTINGS)` 条件保护。
- 影响: Debug 构建在未启用 `BT_SETTINGS` 时不再因为 `sc_store()` 引用缺失符号而失败。
- 验证: 用 Debug 构建场景复现问题后，通过增加条件编译保护规避该编译错误。

## 2026-04-10 - CMake 分层接入 sample 与 native tests
- Commit: `202e4bd`
- 类型: `build`
- 范围: `CMakeLists.txt`、`samples/`、`tests/`
- 背景: 根 `CMakeLists.txt` 中样例与测试入口逐渐堆积，不利于按目录层级维护。
- 修改: 新增 `samples/CMakeLists.txt`、`tests/CMakeLists.txt`、`tests/base/CMakeLists.txt`、`tests/osdep/CMakeLists.txt`，把样例与非 `tests/bluetooth` 的 native tests 下沉到各自层级管理。
- 影响: 后续新增 sample/test 时只需在对应目录维护，根构建脚本保持精简。
- 验证: `cmake -S . -B build`；抽样编译 `btcmd`、`test_bt_atomic`、`test_os`。

## 2026-04-10 - menuconfig 解耦兼容修正
- Commit: `working tree`
- 类型: `kconfig`
- 范围: `Kconfig`、`bluetooth/common/Kconfig`、`bluetooth/host/Kconfig`、`bluetooth/mesh/Kconfig`、`bluetooth/host/monitor.c`
- 背景: `cmake --build build --target menuconfig` 依赖的独立 Kconfig 树缺少 Zephyr 全局符号，出现未定义 range/default 警告。
- 修改: 删除 `BT_DEBUG_MONITOR_RTT` 及其 RTT buffer 配置；新增 `NUM_PREEMPT_PRIORITIES` 默认值与 `OPENBLUE_SYSTEM_WORKQUEUE_STACK_SIZE`；把原先依赖 `SYSTEM_WORKQUEUE_STACK_SIZE` 的位置改为依赖新符号；清理 monitor 中已无配置入口的 RTT 分支。
- 影响: OpenBlue 独立 `menuconfig` 不再依赖 Zephyr 的 RTT / system workqueue 全局符号定义。
- 验证: `cmake --build build --target menuconfig`（Kconfig warning 消失，剩余为非交互环境下 curses 退出异常）；`cmake --build build --target genconfig`。

## 2026-04-10 - 头文件映射继续对齐 Zephyr 目录布局
- Commit: `bfef6d0`
- 类型: `sync`
- 范围: `bluetooth/`、`include/bluetooth/`
- 背景: OpenBlue 头文件与 Zephyr 基线仍存在布局与注释差异，影响后续对齐与审计。
- 修改: 继续整理 `bluetooth/**` 与 `include/bluetooth/**` 的 include 对应关系，恢复误删注释，保持 byteorder/buffer 映射与上游一致。
- 影响: 降低后续同步主线时的目录级 diff 噪音。
- 验证: 以提交内容为准。

## 2026-04-09 - 清理剩余时间辅助接口差异
- Commit: `28ba4a3`
- 类型: `sync`
- 范围: `bluetooth/`
- 背景: 部分时间获取/换算辅助接口仍保留 Zephyr 口径，影响 openblue 侧统一运行时抽象。
- 修改: 继续把剩余 timing helper 用法收敛到 openblue 运行时接口。
- 影响: 时钟/延时相关适配口径更统一。
- 验证: 以提交内容为准。

## 2026-04-09 - mesh 与 services 构建链路对齐 openblue runtime
- Commit: `473c82a`
- 类型: `build`
- 范围: `bluetooth/mesh`、`bluetooth/services`
- 背景: mesh/services 的构建组织与 openblue runtime 侧配置口径仍有分叉。
- 修改: 调整 mesh 与 services 相关构建接入，保持与 openblue runtime 一致。
- 影响: 减少构建分支差异，便于后续继续同步 Zephyr 主线。
- 验证: 以提交内容为准。

## 2026-04-09 - audio 构建收敛到 config-only runtime
- Commit: `ad5699d`
- 类型: `build`
- 范围: `bluetooth/audio`
- 背景: audio 子系统构建仍混有 Zephyr 侧特定接入方式。
- 修改: 将 audio 构建继续收敛到仅依赖配置驱动的 openblue runtime 方式。
- 影响: audio 侧构建路径更统一。
- 验证: 以提交内容为准。

## 2026-04-09 - LE 能力构建扩展
- Commit: `15dfae9`
- 类型: `feature`
- 范围: `bluetooth/host`
- 背景: 广播、扫描、动态 GATT DB 等能力需要继续打开以贴近上游 host 基线。
- 修改: 启用 advertising、scan 与 dynamic gatt db 对应构建链路。
- 影响: LE host 能力覆盖更完整。
- 验证: 以提交内容为准。

## 2026-04-09 - classic host 构建对齐与 demo 扩展
- Commit: `4c0057a`、`a17486e`
- 类型: `build`
- 范围: `bluetooth/host/classic`、`samples/demo`
- 背景: classic profile 的构建接入和 demo 能力不完整。
- 修改: 先对齐 classic host 构建，再在 demo 中启用扩展 profile。
- 影响: classic 栈与 demo 的可见能力增强。
- 验证: 以提交内容为准。

## 2026-04-07 - 合并 Zephyr 主线基线
- Commit: `cc0a588`
- 类型: `sync`
- 范围: `bluetooth/**`、`include/bluetooth/**`
- 背景: 需要将 openblue 基线推进到新的 Zephyr 主线对齐点。
- 修改: 合并当时的 Zephyr main 基线并引入对应目录下的上游变更。
- 影响: 后续所有对齐工作均基于该主线基线继续推进。
- 验证: 以提交内容为准。
- 关联上游: `1863204edabbafacef0935237f232dc21154abd4`
