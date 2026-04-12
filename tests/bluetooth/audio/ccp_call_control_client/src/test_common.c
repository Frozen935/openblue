/* test_common.c - Common functions */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <bluetooth/conn.h>
#include <bluetooth/hci_types.h>

#include <base/utils.h>

#include "test_common.h"

uint8_t bt_conn_index(const struct bt_conn *conn)
{
	return conn->index;
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	*info = conn->info;

	return 0;
}

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
	(void)conn;
}

bool bt_conn_is_type(const struct bt_conn *conn, enum bt_conn_type type)
{
	return conn->info.type == type;
}

void mock_bt_conn_connected(struct bt_conn *conn, uint8_t err)
{
	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->connected != NULL) {
			cb->connected(conn, err);
		}
	}
}

void mock_bt_conn_disconnected(struct bt_conn *conn, uint8_t err)
{
	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->disconnected != NULL) {
			cb->disconnected(conn, err);
		}
	}
}

void test_conn_init(struct bt_conn *conn)
{
	conn->index = 0;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_CENTRAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L2;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;

	mock_bt_conn_connected(conn, BT_HCI_ERR_SUCCESS);
}
