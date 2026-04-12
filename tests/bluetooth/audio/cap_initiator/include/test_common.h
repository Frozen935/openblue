/* test_common.h */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <autoconf.h>
#include <bluetooth/audio/bap.h>
#include <bluetooth/audio/bap_lc3_preset.h>
#include <bluetooth/audio/cap.h>
#include <bluetooth/conn.h>

#include "conn.h"

void test_mocks_init(void);
void test_mocks_cleanup(void);

void test_conn_init(struct bt_conn *conn, uint8_t index);

void test_unicast_set_state(struct bt_cap_stream *cap_stream, struct bt_conn *conn,
			    struct bt_bap_ep *ep, struct bt_bap_lc3_preset *preset,
			    enum bt_bap_ep_state state);
void mock_discover(
	struct bt_conn conns[CONFIG_BT_MAX_CONN],
	struct bt_bap_ep *snk_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT],
	struct bt_bap_ep *src_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT]);

int cap_initiator_main_suite_run(void);
int cap_initiator_unicast_group_suite_run(void);
int cap_initiator_unicast_start_suite_run(void);
int cap_initiator_unicast_stop_suite_run(void);
