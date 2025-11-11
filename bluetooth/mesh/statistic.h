/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLUETOOTH_MESH_STATISTIC_H__
#define __BLUETOOTH_MESH_STATISTIC_H__

void bt_mesh_stat_planned_count(struct bt_mesh_adv_ctx *ctx);
void bt_mesh_stat_succeeded_count(struct bt_mesh_adv_ctx *ctx);
void bt_mesh_stat_rx(enum bt_mesh_net_if net_if);

#endif /* __BLUETOOTH_MESH_STATISTIC_H__ */
