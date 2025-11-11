/* conn_br.c - Bluetooth Classic connection handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/iso.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/direction.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci_vs.h>
#include <bluetooth/att.h>

#include "common/assert.h"
#include "common/bt_str.h"

#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "host/keys.h"
#include "host/smp.h"
#include "ssp.h"
#include "sco_internal.h"

#define LOG_LEVEL CONFIG_BT_CONN_LOG_LEVEL

void bt_sco_cleanup(struct bt_conn *sco_conn)
{
	bt_sco_cleanup_acl(sco_conn);
	bt_conn_unref(sco_conn);
}

struct bt_conn *bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param)
{
	struct bt_hci_cp_connect *cp;
	struct bt_conn *conn;
	struct bt_buf *buf;

	conn = bt_conn_lookup_addr_br(peer);
	if (conn) {
		switch (conn->state) {
		case BT_CONN_INITIATING:
		case BT_CONN_CONNECTED:
			return conn;
		default:
			bt_conn_unref(conn);
			return NULL;
		}
	}

	conn = bt_conn_add_br(peer);
	if (!conn) {
		return NULL;
	}

	buf = bt_hci_cmd_alloc(OS_TIMEOUT_FOREVER);
	if (!buf) {
		bt_conn_unref(conn);
		return NULL;
	}

	cp = bt_buf_add(buf, sizeof(*cp));

	(void)memset(cp, 0, sizeof(*cp));

	memcpy(&cp->bdaddr, peer, sizeof(cp->bdaddr));
	cp->packet_type = sys_cpu_to_le16(0xcc18); /* DM1 DH1 DM3 DH5 DM5 DH5 */
	cp->pscan_rep_mode = 0x02; /* R2 */
	cp->allow_role_switch = param->allow_role_switch ? 0x01 : 0x00;
	cp->clock_offset = 0x0000; /* TODO used cached clock offset */

	if (bt_hci_cmd_send_sync(BT_HCI_OP_CONNECT, buf, NULL) < 0) {
		bt_conn_unref(conn);
		return NULL;
	}

	bt_conn_set_state(conn, BT_CONN_INITIATING);
	conn->role = BT_CONN_ROLE_CENTRAL;

	return conn;
}

int bt_hci_connect_br_cancel(struct bt_conn *conn)
{
	struct bt_hci_cp_connect_cancel *cp;
	struct bt_hci_rp_connect_cancel *rp;
	struct bt_buf *buf, *rsp;
	int err;

	buf = bt_hci_cmd_alloc(OS_TIMEOUT_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	memcpy(&cp->bdaddr, &conn->br.dst, sizeof(cp->bdaddr));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_CONNECT_CANCEL, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;

	err = rp->status ? -EIO : 0;

	bt_buf_unref(rsp);

	return err;
}

const bt_addr_t *bt_conn_get_dst_br(const struct bt_conn *conn)
{
	if (conn == NULL) {
		LOG_DBG("Invalid connect");
		return NULL;
	}

	if (!bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		LOG_DBG("Invalid connection type: %u for %p", conn->type, conn);
		return NULL;
	}

	return &conn->br.dst;
}

void bt_br_acl_recv(struct bt_conn *conn, struct bt_buf *buf, bool complete)
{
	uint16_t acl_total_len;
	struct bt_l2cap_hdr *hdr;
	struct bt_buf_simple_state state;

	do {
		bt_buf_simple_save(&buf->b, &state);

		hdr = (void *)buf->data;
		acl_total_len = sys_le16_to_cpu(hdr->len) + sizeof(*hdr);
		if (buf->len > acl_total_len) {
			LOG_DBG("Multiple L2CAP packet (%u > %u)", buf->len, acl_total_len);
			buf->len = acl_total_len;
		} else if (buf->len < acl_total_len) {
			LOG_ERR("Short packet (%u < %u)", buf->len, acl_total_len);
			break;
		}
		bt_l2cap_recv(conn, bt_buf_ref(buf), complete);

		bt_buf_simple_restore(&buf->b, &state);
		bt_buf_pull(buf, acl_total_len);
	} while (buf->len > 0);

	bt_buf_unref(buf);
}

int bt_conn_br_switch_role(const struct bt_conn *conn, uint8_t role)
{
	struct bt_buf *buf;
	struct bt_hci_cp_switch_role *cp;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");
		return -EINVAL;
	}

	if (!bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		LOG_DBG("Invalid connection type: %u for %p", conn->type, conn);
		return -EINVAL;
	}

	buf = bt_hci_cmd_alloc(OS_TIMEOUT_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, &conn->br.dst);
	cp->role = role;

	return bt_hci_cmd_send_sync(BT_HCI_OP_SWITCH_ROLE, buf, NULL);
}

static int bt_conn_br_read_link_policy_settings(const struct bt_conn *conn,
						uint16_t *link_policy_settings)
{
	int err;
	struct bt_buf *buf;
	struct bt_hci_cp_read_link_policy_settings *cp;
	struct bt_hci_rp_read_link_policy_settings *rp;
	struct bt_buf *rsp;

	buf = bt_hci_cmd_alloc(OS_TIMEOUT_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LINK_POLICY_SETTINGS, buf, &rsp);
	if (!err) {
		rp = (void *)rsp->data;
		*link_policy_settings = rp->link_policy_settings;
	}

	return err;
}

static int bt_conn_br_write_link_policy_settings(const struct bt_conn *conn,
						 uint16_t link_policy_settings)
{
	struct bt_buf *buf;
	struct bt_hci_cp_write_link_policy_settings *cp;

	buf = bt_hci_cmd_alloc(OS_TIMEOUT_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->link_policy_settings = link_policy_settings;

	return bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_LINK_POLICY_SETTINGS, buf, NULL);
}

int bt_conn_br_set_role_switch_enable(const struct bt_conn *conn, bool enable)
{
	int err;
	uint16_t link_policy_settings;
	bool is_enabled;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");
		return -EINVAL;
	}

	if (!bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		LOG_DBG("Invalid connection type: %u for %p", conn->type, conn);
		return -EINVAL;
	}

	err = bt_conn_br_read_link_policy_settings(conn, &link_policy_settings);
	if (err) {
		return err;
	}

	is_enabled = (link_policy_settings & BT_HCI_LINK_POLICY_SETTINGS_ENABLE_ROLE_SWITCH);
	if (enable == is_enabled) {
		return 0;
	}

	link_policy_settings ^= BT_HCI_LINK_POLICY_SETTINGS_ENABLE_ROLE_SWITCH;
	return bt_conn_br_write_link_policy_settings(conn, link_policy_settings);
}
