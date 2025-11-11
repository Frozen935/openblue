/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INCLUDE_BLUETOOTH_MESH_DELAYABLE_MSG_H___
#define __INCLUDE_BLUETOOTH_MESH_DELAYABLE_MSG_H___

int bt_mesh_delayable_msg_manage(struct bt_mesh_msg_ctx *ctx, struct bt_buf_simple *buf,
				 uint16_t src_addr, const struct bt_mesh_send_cb *cb,
				 void *cb_data);
void bt_mesh_delayable_msg_init(void);
void bt_mesh_delayable_msg_stop(void);

#endif /* __INCLUDE_BLUETOOTH_MESH_DELAYABLE_MSG_H__ */
