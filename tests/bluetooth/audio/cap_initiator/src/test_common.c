/* test_common.c - common procedures for unit test of CAP initiator */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>

#include <cmocka.h>

#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/bap.h>
#include <bluetooth/audio/bap_lc3_preset.h>
#include <bluetooth/audio/cap.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci_types.h>
#include <zephyr/fff.h>

#include "audio/bap_endpoint.h"
#include "cap_initiator.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

DEFINE_FFF_GLOBALS;

void test_mocks_init(void)
{
	mock_cap_initiator_init();
}

void test_mocks_cleanup(void)
{
	mock_cap_initiator_cleanup();
}

void test_conn_init(struct bt_conn *conn, uint8_t index)
{
	conn->index = index;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_CENTRAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L2;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;
}

void test_unicast_set_state(struct bt_cap_stream *cap_stream, struct bt_conn *conn,
			    struct bt_bap_ep *ep, struct bt_bap_lc3_preset *preset,
			    enum bt_bap_ep_state state)
{
	struct bt_bap_stream *bap_stream = &cap_stream->bap_stream;
	int err;

	printk("Setting stream %p to state %d\n", bap_stream, state);

	if (state == BT_BAP_EP_STATE_IDLE) {
		return;
	}

	assert_non_null(cap_stream);
	assert_non_null(conn);
	assert_non_null(ep);
	assert_non_null(preset);

	err = bt_bap_stream_config(conn, &cap_stream->bap_stream, ep, &preset->codec_cfg);
	assert_int_equal(err, 0);

	bap_stream->conn = conn;
	bap_stream->ep = ep;
	bap_stream->qos = &preset->qos;
	bap_stream->codec_cfg = &preset->codec_cfg;
	bap_stream->ep->state = state;
}

DECLARE_FAKE_VOID_FUNC(mock_bap_discover_endpoint, struct bt_conn *, enum bt_audio_dir,
		       struct bt_bap_ep *);
DEFINE_FAKE_VOID_FUNC(mock_bap_discover_endpoint, struct bt_conn *, enum bt_audio_dir,
		      struct bt_bap_ep *);

void mock_discover(
	struct bt_conn conns[CONFIG_BT_MAX_CONN],
	struct bt_bap_ep *snk_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT],
	struct bt_bap_ep *src_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT])
{
	struct bt_bap_unicast_client_cb unicast_client_cb = {
		.endpoint = mock_bap_discover_endpoint,
	};
	int err;

	err = bt_bap_unicast_client_register_cb(&unicast_client_cb);
	assert_int_equal(0, err);

	for (int i = 0U; i < CONFIG_BT_MAX_CONN; i++) {
		RESET_FAKE(mock_bap_discover_endpoint);
		err = bt_bap_unicast_client_discover(&conns[i], BT_AUDIO_DIR_SINK);
		assert_int_equal(0, err);

		/* TODO: use callback to populate eps */

		zexpect_call_count("unicast_client_cb.bap_discover_endpoint",
				   CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT,
				   mock_bap_discover_endpoint_fake.call_count);
		for (size_t j = 0U; j < mock_bap_discover_endpoint_fake.call_count; j++) {
			/* Verify conn */
			assert_ptr_equal(mock_bap_discover_endpoint_fake.arg0_history[j], &conns[i]);

			/* Verify dir */
			assert_int_equal(mock_bap_discover_endpoint_fake.arg1_history[j],
					 BT_AUDIO_DIR_SINK);

			/* Verify and store ep */
			assert_non_null(mock_bap_discover_endpoint_fake.arg2_history[j]);

			snk_eps[conns[i].index][j] =
				mock_bap_discover_endpoint_fake.arg2_history[j];
		}

		RESET_FAKE(mock_bap_discover_endpoint);
		err = bt_bap_unicast_client_discover(&conns[i], BT_AUDIO_DIR_SOURCE);
		assert_int_equal(0, err);

		zexpect_call_count("unicast_client_cb.bap_discover_endpoint",
				   CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT,
				   mock_bap_discover_endpoint_fake.call_count);
		for (size_t j = 0U; j < mock_bap_discover_endpoint_fake.call_count; j++) {
			/* Verify conn */
			assert_ptr_equal(mock_bap_discover_endpoint_fake.arg0_history[j], &conns[i]);

			/* Verify dir */
			assert_int_equal(mock_bap_discover_endpoint_fake.arg1_history[j],
					 BT_AUDIO_DIR_SOURCE);

			/* Verify and store ep */
			assert_non_null(mock_bap_discover_endpoint_fake.arg2_history[j]);

			src_eps[conns[i].index][j] =
				mock_bap_discover_endpoint_fake.arg2_history[j];
		}
	}

	/* We don't need the callbacks anymore */
	err = bt_bap_unicast_client_unregister_cb(&unicast_client_cb);
	assert_int_equal(0, err);
}
