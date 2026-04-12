/* test_common.h */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENBLUE_TESTS_BLUETOOTH_AUDIO_CCP_CALL_CONTROL_CLIENT_TEST_COMMON_H_
#define OPENBLUE_TESTS_BLUETOOTH_AUDIO_CCP_CALL_CONTROL_CLIENT_TEST_COMMON_H_

#include <stdint.h>

#include <bluetooth/conn.h>

struct bt_conn {
	uint8_t index;
	struct bt_conn_info info;
	struct bt_iso_chan *chan;
};

void test_conn_init(struct bt_conn *conn);
void mock_bt_conn_connected(struct bt_conn *conn, uint8_t err);
void mock_bt_conn_disconnected(struct bt_conn *conn, uint8_t err);

#endif /* OPENBLUE_TESTS_BLUETOOTH_AUDIO_CCP_CALL_CONTROL_CLIENT_TEST_COMMON_H_ */
