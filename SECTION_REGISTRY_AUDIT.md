# OpenBlue Section 机制审计与重设计方案

## 背景

OpenBlue 从 Zephyr 蓝牙子系统解耦后，逐步移除了对 linker section / iterable section 的原生依赖，但代码里仍然保留了大量基于 section 的静态注册设计。当前现象已经暴露出这一问题：

- `CONFIG_BT_GATT_GAP_SVC_VALIDATE=y` 时，`gatt_gap_svc_validate()` 中 `gap_svc_count != 1`
- 若默认 GAP service 未被最终注册，启动阶段会在 `bt_conn_init()` 返回 `-EINVAL`
- 一些 `STACK_INIT(...)`、静态 GATT service、静态 callback、静态 fixed channel、静态内存池初始化并不会真正生效

本文件用于：

1. 盘点代码中所有仍依赖 section 的设计与代码路径
2. 标记已经出问题或高风险的点
3. 给出替代设计方案
4. 形成后续逐项整改的基线文档

---

## 一、当前总体现状

### 1. section 兼容层已经不是“兼容”，而是“语法保留、语义移除”

- `base/utils.h:127`：`STRUCT_SECTION_ITERABLE(...)` 目前只会定义普通静态对象
- `base/utils.h:133`：`STRUCT_SECTION_FOREACH(...)` 目前展开后是空循环

这意味着：

- 代码可以继续编译
- 但所有依赖 iterable section 聚合的数据，在运行时都不会被遍历到

### 2. `STACK_INIT` 仍保留注册宏，但执行器不再扫描 section

- `include/bt_stack_init.h:41`：`STACK_INIT(fn, level, prio)` 仍把对象放入 `.stack_init`
- `core/stack_init.c:14`：`bt_stack_init_once()` 只手工调用有限几个初始化函数

当前只显式调用了：

- `bt_work_main_work_init()`
- `bt_driver_userchan_init()`
- `bt_driver_h4_init()`

所以其余 `STACK_INIT(...)` 注册点基本不会执行。

### 3. 即使恢复 section，当前静态库链接方式仍然有“对象不被拉入最终可执行文件”的风险

- `CMakeLists.txt:166`：`openblue` 当前被构建成 `STATIC` 库
- `samples/demo/CMakeLists.txt:8`、`samples/cmds/CMakeLists.txt:8`：最终程序普通链接 `openblue`

对“仅靠静态对象存在即可生效”的注册机制来说，这意味着：

- 对象文件可能已编进 `libopenblue.a`
- 但如果没有显式符号引用，最终链接器可能不会把该对象文件拉入 `demo`/`btcmd`

这和 section 语义本身是两个不同层面的风险，二者当前同时存在。

---

## 二、section 相关机制盘点

下面按模块分类列出仍保留 section 设计的关键代码。

### A. 基础机制层

#### 1) iterable section 兼容宏

- `base/utils.h:127`：`STRUCT_SECTION_ITERABLE`
- `base/utils.h:133`：`STRUCT_SECTION_FOREACH`

用途：承载所有 Zephyr 风格的静态注册对象与遍历逻辑。

当前问题：

- 注册对象不会进入可遍历集合
- 遍历逻辑永远拿不到任何元素

#### 2) stack init 注册宏

- `include/bt_stack_init.h:31`：`struct stack_init_entry`
- `include/bt_stack_init.h:41`：`STACK_INIT`
- `core/stack_init.c:14`：`bt_stack_init_once`

用途：按 level / prio 收集并执行初始化函数。

当前问题：

- 注册宏还在
- 统一调度器已不存在
- 绝大多数注册点不会执行

---

### B. 基础资源与运行时组件

#### 1) 静态内存池 registry

- 定义宏：`base/bt_mem_pool.h:31`
- 遍历初始化：`base/bt_mem_pool.c:49`
- 初始化挂接：`base/bt_mem_pool.c:128`

代表实例：

- `bluetooth/audio/shell/bap.c:2519`
- `bluetooth/mesh/net.c:113`
- `bluetooth/mesh/transport.c:113`

用途：建立 pool/slab free list。

风险：若 `mem_pool_list_init()` 不执行，运行时内存池初始化不完整。

#### 2) 主工作队列初始化

- `base/bt_work.c:864`：`STACK_INIT(main_work_init, STACK_BASE_INIT, 1)`

当前情况：

- 由于 `core/stack_init.c` 手工调用了 `bt_work_main_work_init()`，所以这一项暂时“未靠 section 也能活”
- 但该成功是白名单特判，不是机制恢复

---

### C. Host 连接回调总线（`BT_CONN_CB_DEFINE`）

#### 宏与遍历

- 定义：`include/bluetooth/conn.h:2556`
- 遍历：`bluetooth/host/conn.c:1634`、`1877`、`1954`、`2024`、`2089`、`2479`、`2570`、`3015`、`3158`、`3227`、`3527`、`3576`、`3645`

#### 典型静态回调实例

- `bluetooth/audio/aics_client.c:694`
- `bluetooth/audio/ascs.c:1309`
- `bluetooth/audio/bap_broadcast_assistant.c:978`
- `bluetooth/audio/bap_scan_delegator.c:356`
- `bluetooth/audio/bap_unicast_client.c:4734`
- `bluetooth/audio/cap_common.c:269`
- `bluetooth/audio/ccp_call_control_client.c:112`
- `bluetooth/audio/csip_set_coordinator.c:1373`
- `bluetooth/audio/csip_set_member.c:647`
- `bluetooth/audio/gmap_client.c:89`
- `bluetooth/audio/has.c:412`
- `bluetooth/audio/has_client.c:1000`
- `bluetooth/audio/mcc.c:1355`
- `bluetooth/audio/mcs.c:82`
- `bluetooth/audio/media_proxy.c:687`
- `bluetooth/audio/micp_mic_ctlr.c:511`
- `bluetooth/audio/pacs.c:1354`
- `bluetooth/audio/tbs.c:393`
- `bluetooth/audio/vcp_vol_ctlr.c:872`
- `bluetooth/host/att.c:3760`
- `bluetooth/host/cs.c:50`
- `bluetooth/host/gatt.c:1363`
- `bluetooth/host/scan.c:1428`
- `bluetooth/host/shell/bt.c:1245`
- `bluetooth/host/shell/l2cap.c:188`
- `bluetooth/mesh/gatt_cli.c:354`
- `bluetooth/mesh/pb_gatt_srv.c:310`
- `bluetooth/mesh/proxy_srv.c:1208`
- `bluetooth/services/ias/ias.c:125`

#### 现有替代路径

- `bluetooth/host/conn.c:2666`：`bt_conn_cb_register()`
- `bluetooth/host/conn.c:2679`：`bt_conn_cb_unregister()`

结论：

- 静态 callback 机制仍大面积存在
- 但 host 已有可用的显式注册 API
- 这是后续最适合去-section化的一类点

---

### D. GATT 静态服务（`BT_GATT_SERVICE_DEFINE`）

#### 宏与遍历

- 定义：`include/bluetooth/gatt.h:852`
- 遍历：`bluetooth/host/gatt.c:1373`、`1759`、`1959`

#### 静态服务实例

- `bluetooth/host/gatt.c:1038`：`_1_gatt_svc`
- `bluetooth/services/ans.c:247`
- `bluetooth/services/bas/bas.c:69`
- `bluetooth/services/cts.c:245`
- `bluetooth/services/dis.c:243`
- `bluetooth/services/ets.c:247`
- `bluetooth/services/ets.c:255`
- `bluetooth/services/gap_svc.c:145`
- `bluetooth/services/hrs.c:109`
- `bluetooth/services/ias/ias.c:130`
- `bluetooth/services/tps.c:44`

#### 已有显式注册路径（更安全）

- `bluetooth/audio/aics.c:616`
- `bluetooth/audio/ascs.c:3174`
- `bluetooth/audio/bap_scan_delegator.c:1354`
- `bluetooth/audio/cap_acceptor.c:67`
- `bluetooth/audio/csip_set_member.c:1074`
- `bluetooth/audio/gmap_server.c:357`
- `bluetooth/audio/has.c:1794`
- `bluetooth/audio/mcs.c:1343`
- `bluetooth/audio/micp_mic_dev.c:204`
- `bluetooth/audio/pacs.c:866`
- `bluetooth/audio/tbs.c:2477`
- `bluetooth/audio/tmap.c:220`
- `bluetooth/audio/vcp_vol_rend.c:474`
- `bluetooth/audio/vocs.c:452`
- `bluetooth/mesh/pb_gatt_srv.c:181`
- `bluetooth/mesh/proxy_srv.c:972`
- `bluetooth/services/ots/ots.c:502`

结论：

- `BT_GATT_SERVICE_DEFINE` 是当前最直接的故障源之一
- 纯静态服务会受 section 空遍历 + 静态库对象未拉入 两层影响
- 显式 `bt_gatt_service_register()` 的服务更适合保留

---

### E. L2CAP 固定信道

#### 1) LE fixed channel

- 定义：`include/bluetooth/l2cap.h:399`
- 遍历：`bluetooth/host/l2cap.c:413`

实例：

- `bluetooth/host/att.c:3558`
- `bluetooth/host/l2cap.c:2958`
- `bluetooth/host/smp.c:6399`
- `bluetooth/host/smp_null.c:101`

关键问题：

- `BT_L2CAP_FIXED_CHANNEL_DEFINE` 现在只定义静态对象
- 它自己就没有再挂进任何 section
- 即使以后把 `STRUCT_SECTION_FOREACH` 恢复，这里依然不完整

#### 2) BR fixed channel

- 定义：`bluetooth/host/classic/l2cap_br_interface.h:16`
- 遍历：`bluetooth/host/classic/l2cap_br.c:1790`、`1871`
- 实例：`bluetooth/host/classic/l2cap_br.c:6184`、`6530`，`bluetooth/host/smp.c:6404`

结论：

- L2CAP fixed channel 机制需要单独重审
- 尤其 LE fixed channel 当前是“接口定义与遍历协议已经脱节”

---

### F. SCO / Classic callback

#### 宏与遍历

- `bluetooth/host/classic/sco_internal.h:225`：`BT_SCO_CONN_CB_DEFINE`
- `bluetooth/host/classic/sco_internal.h:297`：`BT_SCO_HCI_CB_DEFINE`
- `bluetooth/host/classic/sco.c:87`、`104`、`121`、`138`

#### 已有显式注册 API

- `bluetooth/host/classic/sco.c:481`：`bt_sco_conn_cb_register()`
- `bluetooth/host/classic/sco.c:509`：`bt_sco_hci_cb_register()`

结论：

- SCO 也仍保留静态 section 风格
- 但代码中已经存在显式注册 API，可作为迁移样板

---

### G. Mesh callback buses

#### 宏定义位置

- `bluetooth/mesh/app_keys.h:26`：`BT_MESH_APP_KEY_CB_DEFINE`
- `bluetooth/mesh/subnet.h:103`：`BT_MESH_SUBNET_CB_DEFINE`
- `include/bluetooth/mesh/heartbeat.h:123`：`BT_MESH_HB_CB_DEFINE`
- `include/bluetooth/mesh/proxy.h:54`：`BT_MESH_PROXY_CB_DEFINE`
- `include/bluetooth/mesh/main.h:744`：`BT_MESH_LPN_CB_DEFINE`
- `include/bluetooth/mesh/main.h:796`：`BT_MESH_FRIEND_CB_DEFINE`
- `include/bluetooth/mesh/main.h:858`：`BT_MESH_BEACON_CB_DEFINE`

#### 遍历逻辑位置

- `bluetooth/mesh/app_keys.c:167`
- `bluetooth/mesh/subnet.c:66`
- `bluetooth/mesh/heartbeat.c:36`、`73`、`84`
- `bluetooth/mesh/proxy_srv.c:374`、`416`
- `bluetooth/mesh/lpn.c:326`、`454`、`1142`
- `bluetooth/mesh/friend.c:190`、`756`、`766`
- `bluetooth/mesh/beacon.c:557`、`597`

代表实例：

- `bluetooth/mesh/cdb.c:1294`
- `bluetooth/mesh/cfg_srv.c:350`
- `bluetooth/mesh/beacon.c:774`
- `bluetooth/mesh/brg_cfg.c:205`
- `bluetooth/mesh/friend.c:1339`
- `bluetooth/mesh/lpn.c:1182`
- `bluetooth/mesh/proxy_cli.c:388`
- `bluetooth/mesh/proxy_srv.c:906`
- `bluetooth/mesh/rpr_srv.c:433`
- `bluetooth/mesh/shell/shell.c:309`

结论：

- 这是仅次于 GATT 静态服务的第二大高风险集中区
- 一旦 section 总线失效，mesh 的 key/subnet/lpn/friend/proxy/beacon 事件广播会静默失效

---

### H. IAS callback

- 定义：`include/bluetooth/services/ias.h:70`
- 遍历：`bluetooth/services/ias/ias.c:55`、`62`、`69`
- 实例：`bluetooth/services/ias/shell/ias.c:34`

补充：`bluetooth/services/ias/ias_client.c:174` 已存在显式注册 `bt_ias_client_cb_register()`。

---

### I. Shell 命令体系（基本已显式化）

- `bluetooth/common/bt_shell_private.h:75`：`BT_SHELL_CMD_ARG_REGISTER`
- `bluetooth/common/bt_shell_private.h:93`：`BT_SHELL_SUBCMD_ADD` 被明确定义为 no-op
- `bluetooth/common/bt_shell_private.c:262`：`bt_shell_init()` 显式调用各命令注册函数
- `bluetooth/mesh/shell/shell.c:1883` 之后：mesh shell 子命令手工聚合

结论：

- shell 主路径已经是“显式注册”风格
- 这是一个成功迁移的样板
- 但 `BT_SHELL_SUBCMD_ADD` 是 no-op，后续继续同步 Zephyr shell 代码时仍需人工介入

---

### J. Log backend / monitor

- `bluetooth/host/monitor.c:312`：`LOG_BACKEND_DEFINE(bt_monitor, ...)`
- `bluetooth/host/monitor.c:328`：`STACK_INIT(bt_monitor_init, ...)`

结论：

- 不仅受日志 backend 兼容层影响
- 还受 `STACK_INIT` 当前不遍历影响

---

### K. Tests 中仍保留 Zephyr section 假设

- `tests/bluetooth/audio/mocks/src/conn.c:15`
- `tests/bluetooth/audio/mocks/src/gatt.c:22`
- `tests/bluetooth/host/conn/mocks/mock-sections.ld:9`

说明：

- 测试侧仍然承认“iterable section 是原始语义”
- 与 OpenBlue 当前生产代码里“宏保留但语义移除”的状态存在明显背离

---

## 三、已经明确出问题或高风险的点

### 1. GAP service 校验失败

- 触发位置：`bluetooth/host/conn.c:4428`
- 校验实现：`bluetooth/host/gatt_gap_svc_validate.c:31`
- 默认 GAP service：`bluetooth/services/gap_svc.c:145`

当前现象：

- `CONFIG_BT_GATT_GAP_SVC_VALIDATE=y` 时，`gap_svc_count != 1`

根因判断：

- 默认 GAP service 走的是 `BT_GATT_SERVICE_DEFINE(...)`
- 这依赖 section 聚合
- 当前最终镜像中默认 GAP service 极可能没有被纳入 GATT DB

从现有代码来看，更大概率是 `gap_svc_count == 0`，而不是 `2`。

### 2. `STACK_INIT(...)` 注册项大面积失效

例如：

- `bluetooth/host/conn.c:4643`：`bt_conn_tx_workq_init`
- `bluetooth/host/hci_core.c:5167`：`bt_tx_processor_init`
- `bluetooth/host/long_wq.c:29`：`long_wq_init`
- `bluetooth/services/ans.c:234`：`ans_init`
- `bluetooth/services/bas/bas.c:89`：`bas_init`
- `bluetooth/services/hrs.c:123`：`hrs_init`
- `bluetooth/services/ots/ots.c:651`：`bt_gatt_ots_instances_prepare`
- `bluetooth/services/ots/ots_l2cap.c:203`：`bt_gatt_ots_l2cap_init`

### 3. `BT_CONN_CB_DEFINE` 静态回调总线失效

风险：

- 连接/断连后的资源清理不完整
- 安全状态回调不触发
- Audio / Mesh / Shell 的状态联动静默失效

### 4. L2CAP fixed channel 存在接口设计脱节

- LE fixed channel 宏本身已不再与 section 遍历协议匹配
- 这一项后续不能只靠“恢复遍历宏”解决

### 5. BR channel 同样受 section 机制影响

- 定义宏：`bluetooth/host/classic/l2cap_br_interface.h:16`
- 遍历位置：`bluetooth/host/classic/l2cap_br.c:1790`、`bluetooth/host/classic/l2cap_br.c:1871`
- 当前静态 BR fixed channel 实例：
  - `bluetooth/host/classic/l2cap_br.c:6184`：`BT_L2CAP_CID_BR_SIG`
  - `bluetooth/host/classic/l2cap_br.c:6530`：`BT_L2CAP_CID_CONNLESS`

风险表现：

- `get_fixed_channels_mask()` 可能得不到正确的 fixed channel mask
- `bt_l2cap_br_connected()` 可能无法自动接入 BR fixed channel
- BR signaling / connless 这种依赖固定 CID 的接入路径会不稳定，甚至完全失效

和 LE fixed channel 的区别：

- BR channel 的宏本身还保留了 `STRUCT_SECTION_ITERABLE(...)` 形式
- 所以它的问题更接近“section 机制整体失效”
- 而 LE fixed channel 是“section 机制失效 + 宏定义本身已经脱节”双重问题

### 6. Mesh callback buses 高风险

风险：

- AppKey/Subnet/Heartbeat/LPN/Friend/Beacon/Proxy 等事件广播失效
- 这类问题容易表现为“功能不完整”而非立即 crash，排障成本高

### 7. 静态库链接会放大所有 section 设计问题

即使 section 机制恢复：

- 若对象文件没有被最终可执行文件拉入
- 其中的静态注册对象仍然不会生效

所以不能只修 `STRUCT_SECTION_FOREACH`，还必须考虑最终链接模型。

---

## 四、重设计方案

这里给出 3 套方案，按推荐程度排序。

### 方案 A：全面改为显式注册 / 显式初始化表（推荐）

核心思路：

- 用显式 API 代替所有“静态定义即注册”语义
- 用中心化 init 表代替 `STACK_INIT` section 扫描
- 用显式注册链表/数组代替 `BT_CONN_CB_DEFINE`、mesh callback buses、IAS callback 等

建议映射：

- `BT_GATT_SERVICE_DEFINE` → 保留数据定义，统一在 init 流程中 `bt_gatt_service_register(...)`
- `BT_CONN_CB_DEFINE` → 模块 init 中调用 `bt_conn_cb_register()`
- mesh callback bus → 为每类 callback 增加 `*_register()` API
- `STACK_INIT` → 引入一个 always-linked 的注册表 `.c` 文件，按顺序显式调用初始化函数

优点：

- 最易调试
- 行为最明确
- 不再依赖 linker/section 魔法
- 对后续 openblue 独立演进最友好

缺点：

- 改动面大
- 需要系统性重构 registry 设计

### 方案 B：恢复完整 section 语义（不推荐优先采用）

核心思路：

- 真正实现 `STRUCT_SECTION_ITERABLE` / `STRUCT_SECTION_FOREACH`
- 补齐 linker script / section range / KEEP 逻辑
- 对承载静态注册对象的库使用 `--whole-archive` 或 object library

优点：

- 更接近 Zephyr 原始设计
- 某些上游代码迁移成本较低

缺点：

- 构建/链接复杂度高
- 与 openblue 当前去 Zephyr 化目标不一致
- 排障难度高

### 方案 C：混合方案（过渡方案）

核心思路：

- 先处理启动路径与高风险模块，其他部分保留过渡态
- 优先把最危险的 section 依赖替换成显式注册
- 低优先级模块逐步迁移

建议优先处理：

1. `STACK_INIT`
2. `BT_GATT_SERVICE_DEFINE` / GATT mandatory services / services/gap_svc.c
3. `BT_L2CAP_FIXED_CHANNEL_DEFINE`
4. `BT_MEM_POOL_DEFINE`
5. `BT_CONN_CB_DEFINE`

优点：

- 可分阶段推进
- 能先解决启动与核心功能问题

缺点：

- 中间态会同时存在多套机制
- 文档和规范必须跟上，否则容易继续扩散

---

## 五、推荐的整改顺序

### 首批整改建议（建议先做这三类）

结合当前启动/建链风险，建议第一优先级不是一次性铺开所有 registry，而是先把下面 3 类打通：

1. `STACK_INIT`
2. BR fixed channel
3. LE fixed channel

原因：

- `STACK_INIT` 是全局初始化总入口，很多后续模块是否能正常工作都取决于它先恢复
- BR / LE fixed channel 是 host 基础建链路径的一部分，属于比静态 callback 更底层的通信基础设施
- 这 3 类问题都不是“功能增强”，而是“框架地基”问题，越早显式化，后面迁移 GATT service / callback bus 越稳

### P0：先解决启动与基础运行时

1. 替换 `STACK_INIT`
   - 引入显式初始化顺序表
   - 不再依赖 `.stack_init` section

2. 重做 BR fixed channel 注册
   - 把 `BT_L2CAP_BR_CHANNEL_DEFINE(...)` 改成显式注册模型
   - 为 `BT_L2CAP_CID_BR_SIG`、`BT_L2CAP_CID_CONNLESS` 建立明确的注册入口
   - 避免 `get_fixed_channels_mask()` 和 `bt_l2cap_br_connected()` 再依赖 `STRUCT_SECTION_FOREACH(...)`

3. 重做 LE fixed channel 注册
   - 为 ATT / SMP / LE signaling 建立显式 fixed channel register 流程
   - 顺手修复 `BT_L2CAP_FIXED_CHANNEL_DEFINE(...)` 当前与遍历协议脱节的问题

4. 替换默认 GATT 静态服务
   - 先把 `gatt.c` 内的 `_1_gatt_svc`
   - `services/gap_svc.c`
   - 以及其他纯静态 service，改成显式注册

5. 重新梳理 mem pool 初始化
   - 改成显式 `bt_mem_pool_register()` 或静态表初始化

### P1：替换 callback 总线

1. `BT_CONN_CB_DEFINE`
2. mesh callback buses
3. IAS callback
4. classic SCO callback

### P2：清理兼容层与收口规范

1. 决定是否彻底删除 `STRUCT_SECTION_*` 宏
2. 若不再支持 section，明确把相关旧宏改成编译期报错或迁移提示
3. 为后续新增模块制定统一规则：
   - 不允许再引入新的 section-based 自动注册

---

## 六、建议的最终设计原则

重设计建议统一遵守以下规则：

1. **初始化路径显式化**
   - 所有模块初始化都必须能从调用栈直接看到入口

2. **注册行为显式化**
   - callback / service / fixed channel / mem pool / shell command 尽量通过函数注册，而不是链接副作用

3. **构建与运行语义解耦**
   - 不让“对象是否被链接进最终镜像”决定功能是否存在

4. **兼容宏不再伪装有语义**
   - 如果某个机制不再支持，就不要只保留空宏，应明确迁移路径

5. **优先复用已存在的显式 API**
   - 如 `bt_gatt_service_register()`、`bt_conn_cb_register()`、`bt_sco_conn_cb_register()` 等

---

## 七、后续逐项治理建议

后续建议按下面顺序一个一个解决：

1. `STACK_INIT` 设计重写
2. GATT mandatory/static service 显式注册
3. LE fixed channel 注册机制重写
4. `BT_MEM_POOL_DEFINE` 替换为显式注册/初始化
5. `BT_CONN_CB_DEFINE` 全量迁移
6. mesh callback bus 迁移
7. classic / IAS 等剩余总线迁移

每完成一类，都建议：

- 在本文件补充整改状态
- 在 `CHANGELOG_OPENBLUE.md` 记录落地改动
- 增加对应的回归验证项

---

## 八、当前已确认的直接症状

### 已确认 / 高度怀疑的问题

- `gatt_gap_svc_validate()` 中 `gap_svc_count != 1`
- `BT_GATT_SERVICE_DEFINE` 定义的默认 GAP service 未稳定进入最终 GATT DB
- `STACK_INIT` 注册点大面积不执行
- `BT_CONN_CB_DEFINE` 静态回调不工作
- LE fixed channel 机制与当前宏定义协议不一致
- BR fixed channel 依赖的 section 聚合当前同样不可靠
- mesh callback buses 大概率失效

这些问题不是彼此独立的零散 bug，而是同一类“section 机制被移除后仍残留旧设计”的系统性后果。

---

## 九、附录：关键文件索引

### 基础机制
- `base/utils.h`
- `include/bt_stack_init.h`
- `core/stack_init.c`

### GATT / GAP / ATT / L2CAP
- `include/bluetooth/gatt.h`
- `bluetooth/host/gatt.c`
- `bluetooth/host/gatt_gap_svc_validate.c`
- `bluetooth/services/gap_svc.c`
- `include/bluetooth/l2cap.h`
- `bluetooth/host/l2cap.c`
- `bluetooth/host/att.c`
- `bluetooth/host/smp.c`

### Callback buses
- `include/bluetooth/conn.h`
- `bluetooth/host/conn.c`
- `bluetooth/mesh/app_keys.h`
- `bluetooth/mesh/subnet.h`
- `include/bluetooth/mesh/heartbeat.h`
- `include/bluetooth/mesh/proxy.h`
- `include/bluetooth/mesh/main.h`
- `include/bluetooth/services/ias.h`
- `bluetooth/host/classic/sco_internal.h`
- `bluetooth/host/classic/sco.c`

### 资源与构建
- `base/bt_mem_pool.h`
- `base/bt_mem_pool.c`
- `CMakeLists.txt`
- `bluetooth/services/CMakeLists.txt`
- `samples/demo/CMakeLists.txt`
- `samples/cmds/CMakeLists.txt`
