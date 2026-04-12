/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024 Demant A/S
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>
#include <string.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <sys/types.h>

#include "bap_unicast_server.h"
#include "bap_unicast_server_expects.h"
#include "bap_stream.h"
#include "bap_stream_expects.h"
#include "conn.h"
#include "gatt.h"
#include "gatt_expects.h"
#include "iso.h"

#include "test_common.h"

#define fixture (get_fixture(state))

DEFINE_FFF_GLOBALS;

struct ascs_test_suite_fixture {
	const struct bt_gatt_attr *ase_cp;
	struct bt_bap_stream stream;
	struct bt_conn conn;
	struct {
		uint8_t id;
		const struct bt_gatt_attr *attr;
	} ase_snk, ase_src;
};

static void ascs_test_suite_fixture_init(struct ascs_test_suite_fixture *fixture)
{
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
	};
	int err;

	memset(fixture, 0, sizeof(*fixture));

	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, 0);

	fixture->ase_cp = test_ase_control_point_get();

	test_conn_init(&fixture->conn);

	test_ase_snk_get(CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT, &fixture->ase_snk.attr);
	if (fixture->ase_snk.attr != NULL) {
		fixture->ase_snk.id = test_ase_id_get(fixture->ase_snk.attr);
	}

	test_ase_src_get(CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT, &fixture->ase_src.attr);
	if (fixture->ase_src.attr != NULL) {
		fixture->ase_src.id = test_ase_id_get(fixture->ase_src.attr);
	}
}

static void *ascs_test_suite_setup(void)
{
	struct ascs_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	assert_non_null(fixture);

	return fixture;
}

static void ascs_test_suite_before(void *f)
{
	memset(f, 0, sizeof(struct ascs_test_suite_fixture));
	ascs_test_suite_fixture_init(f);
}

static void ascs_test_suite_teardown(void *f)
{
	free(f);
}

static void ascs_test_suite_after(void *f)
{
	int err;

	/* If any of these fails, it's a fatal error for any tests running afterwards */
	err = bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
	assert_true(err == 0 || err == -EALREADY);

	/* Sleep to trigger any pending state changes from unregister_cb */
	k_sleep(K_SECONDS(1));

	err = bt_bap_unicast_server_unregister();
	assert_int_equal(err, 0);
}



static struct ascs_test_suite_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

static int ascs_test_suite_case_setup(void **state)
{
	void *fixture_local = NULL;

	test_mocks_init();

	fixture_local = ascs_test_suite_setup();

	*state = fixture_local;

	ascs_test_suite_before(fixture_local);

	return 0;
}

static int ascs_test_suite_case_teardown(void **state)
{
	void *fixture_local = state != NULL ? *state : NULL;


	ascs_test_suite_after(fixture_local);

	test_mocks_cleanup();

	ascs_test_suite_teardown(fixture_local);

	if (state != NULL) {
		*state = NULL;
	}

	return 0;
}

static void test_has_sink_ase_chrc(void **state)
{
	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	assert_non_null(fixture->ase_snk.attr);
}

static void test_has_source_ase_chrc(void **state)
{
	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	assert_non_null(fixture->ase_src.attr);
}

static void test_has_control_point_chrc(void **state)
{
	assert_non_null(fixture->ase_cp);
}

static void test_sink_ase_read_state_idle(void **state)
{
	const struct bt_gatt_attr *ase = fixture->ase_snk.attr;
	struct bt_conn *conn = &fixture->conn;
	struct test_ase_chrc_value_hdr hdr = { 0xff };
	ssize_t ret;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }
	assert_non_null(fixture->ase_snk.attr);

	ret = ase->read(conn, ase, &hdr, sizeof(hdr), 0);
	assert_false(ret < 0);
	assert_int_equal(0x00, hdr.ase_state);
}

static void test_release_ase_on_callback_unregister(void **state)
{
	const struct test_ase_chrc_value_hdr *hdr;
	const struct bt_gatt_attr *ase;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_gatt_notify_params *notify_params;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}

	assert_non_null(ase);
	assert_true(ase_id != 0x00);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	/* Set ASE to non-idle state */
	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Reset mock, as we expect ASE notification to be sent */
	bt_gatt_notify_cb_reset();

	/* Unregister the callbacks - which will clean up the ASCS */
	bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);

	test_drain_syswq(); /* Ensure that state transitions are completed */

	/* Expected to notify the upper layers */
	expect_bt_bap_unicast_server_cb_release_called(1, &stream);
	expect_bt_bap_stream_ops_released_called(1, (const struct bt_bap_stream **)&stream);

	/* Expected to notify the client */
	expect_bt_gatt_notify_cb_called_once(conn, ase->uuid, ase, EMPTY, sizeof(*hdr));

	notify_params = mock_bt_gatt_notify_cb_fake.arg1_val;
	hdr = (void *)notify_params->data;
	assert_int_equal(0x00, hdr->ase_state);
}

static void test_abort_client_operation_if_callback_not_registered(void **state)
{
	const struct test_ase_cp_chrc_value_param *param;
	const struct test_ase_cp_chrc_value_hdr *hdr;
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_gatt_notify_params *notify_params;
	uint8_t ase_id;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase_id = fixture->ase_snk.id;
	} else {
		ase_id = fixture->ase_src.id;
	}

	assert_non_null(ase_cp);
	assert_true(ase_id != 0x00);

	/* Set ASE to non-idle state */
	test_ase_control_client_config_codec(conn, ase_id, stream);

	/* Expected ASE Control Point notification with Unspecified Error was sent */
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp,
					     EMPTY, TEST_ASE_CP_CHRC_VALUE_SIZE(1));

	notify_params = mock_bt_gatt_notify_cb_fake.arg1_val;
	hdr = (void *)notify_params->data;
	assert_int_equal(0x01, hdr->opcode);
	assert_int_equal(0x01, hdr->number_of_ases);
	param = (void *)hdr->params;
	assert_int_equal(ase_id, param->ase_id);
	/* Expect Unspecified Error */
	assert_int_equal(0x0E, param->response_code);
	assert_int_equal(0x00, param->reason);
}

static void test_release_ase_on_acl_disconnection(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}

	assert_non_null(ase);
	assert_true(ase_id != 0x00);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	/* Set ASE to non-idle state */
	test_preamble_state_streaming(conn, ase_id, stream, &chan,
				      !IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK));

	/* Mock ACL disconnection */
	mock_bt_conn_disconnected(conn, BT_HCI_ERR_CONN_TIMEOUT);

	/* Expected to notify the upper layers */
	expect_bt_bap_stream_ops_released_called(1, (const struct bt_bap_stream **)&stream);

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);
}

static void test_release_ase_pair_on_acl_disconnection(void **state)
{
	const struct bt_gatt_attr *ase_snk, *ase_src;
	struct bt_bap_stream snk_stream, src_stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_snk_id, ase_src_id;
	struct bt_iso_chan *chan;
	int err;

	if (CONFIG_BT_ASCS_MAX_ACTIVE_ASES < 2) {
		skip();
	}

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }
	memset(&snk_stream, 0, sizeof(snk_stream));
	ase_snk = fixture->ase_snk.attr;
	assert_non_null(ase_snk);
	ase_snk_id = fixture->ase_snk.id;
	assert_true(ase_snk_id != 0x00);

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }
	memset(&src_stream, 0, sizeof(src_stream));
	ase_src = fixture->ase_src.attr;
	assert_non_null(ase_src);
	ase_src_id = fixture->ase_src.id;
	assert_true(ase_src_id != 0x00);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	test_ase_control_client_config_codec(conn, ase_snk_id, &snk_stream);
	test_ase_control_client_config_qos(conn, ase_snk_id);
	test_ase_control_client_enable(conn, ase_snk_id);

	test_ase_control_client_config_codec(conn, ase_src_id, &src_stream);
	test_ase_control_client_config_qos(conn, ase_src_id);
	test_ase_control_client_enable(conn, ase_src_id);

	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	assert_int_equal(0, err);

	test_ase_control_client_receiver_start_ready(conn, ase_src_id);

	err = bt_bap_stream_start(&snk_stream);
	assert_int_equal(0, err);

	test_mocks_reset();

	/* Mock ACL disconnection */
	mock_bt_conn_disconnected(conn, BT_HCI_ERR_CONN_TIMEOUT);

	/* Expected to notify the upper layers */
	const struct bt_bap_stream *streams[] = { &snk_stream, &src_stream };

	expect_bt_bap_stream_ops_released_called(ARRAY_SIZE(streams), streams);

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);
}

static void test_recv_in_streaming_state(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase_snk.id;
	struct bt_iso_recv_info info = {
		.seq_num = 1,
		.flags = BT_ISO_FLAGS_VALID,
	};
	struct bt_iso_chan *chan;
	struct net_buf buf;
	int err;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	chan->ops->recv(chan, &info, &buf);

	/* Verification */
	expect_bt_bap_stream_ops_recv_called(1, &stream, &info, &buf);
}

static void test_recv_in_enabling_state(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id = fixture->ase_snk.id;
	struct bt_iso_recv_info info = {
		.seq_num = 1,
		.flags = BT_ISO_FLAGS_VALID,
	};
	struct bt_iso_chan *chan;
	struct net_buf buf;
	int err;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	test_preamble_state_enabling(conn, ase_id, stream);

	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	assert_int_equal(0, err);

	test_mocks_reset();

	chan->ops->recv(chan, &info, &buf);

	/* Verification */
	expect_bt_bap_stream_ops_recv_called(0, NULL, NULL, NULL);
}

static void test_cis_link_loss_in_streaming_state(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}
	assert_non_null(ase);
	assert_true(ase_id != 0x00);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	test_preamble_state_streaming(conn, ase_id, stream, &chan,
				      !IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK));

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);

	test_drain_syswq(); /* Ensure that state transitions are completed */

	/* Expected to notify the upper layers */
	expect_bt_bap_stream_ops_qos_set_called(1, &stream);
	expect_bt_bap_stream_ops_disabled_called(1, &stream);
	expect_bt_bap_stream_ops_released_called(0, NULL);
	expect_bt_bap_stream_ops_disconnected_called(1, (const struct bt_bap_stream **)&stream);
}

static void test_cis_link_loss_in_disabling_state(struct ascs_test_suite_fixture *fixture,
						  bool streaming)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;
	int err;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase = fixture->ase_src.attr;
	ase_id = fixture->ase_src.id;
	assert_non_null(ase);
	assert_true(ase_id != 0x00);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	test_preamble_state_enabling(conn, ase_id, stream);
	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	assert_int_equal(0, err);

	if (streaming) {
		test_ase_control_client_receiver_start_ready(conn, ase_id);
	}

	test_ase_control_client_disable(conn, ase_id);

	expect_bt_bap_stream_ops_disabled_called(1, &stream);

	test_mocks_reset();

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);

	test_drain_syswq(); /* Ensure that state transitions are completed */

	/* Expected to notify the upper layers */
	expect_bt_bap_stream_ops_qos_set_called(1, &stream);
	expect_bt_bap_stream_ops_disabled_called(0, NULL);
	expect_bt_bap_stream_ops_released_called(0, NULL);
	expect_bt_bap_stream_ops_disconnected_called(1, (const struct bt_bap_stream **)&stream);
}

static void test_cis_link_loss_in_disabling_state_v1(void **state)
{
	/* Enabling -> Streaming -> Disabling */
	test_cis_link_loss_in_disabling_state(fixture, true);
}

static void test_cis_link_loss_in_disabling_state_v2(void **state)
{
	/* Enabling -> Disabling */
	test_cis_link_loss_in_disabling_state(fixture, false);
}

static void test_cis_link_loss_in_enabling_state(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}
	assert_non_null(ase);
	assert_true(ase_id != 0x00);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	test_preamble_state_enabling(conn, ase_id, stream);
	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	assert_int_equal(0, err);

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_TIMEOUT);

	test_drain_syswq(); /* Ensure that state transitions are completed */

	/* Expected no change in ASE state */
	expect_bt_bap_stream_ops_qos_set_called(0, NULL);
	expect_bt_bap_stream_ops_released_called(0, NULL);
	expect_bt_bap_stream_ops_disconnected_called(1, (const struct bt_bap_stream **)&stream);

	err = bt_bap_stream_disable(stream);
	assert_int_equal(0, err);

	test_drain_syswq(); /* Ensure that state transitions are completed */

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		expect_bt_bap_stream_ops_qos_set_called(1, &stream);
		expect_bt_bap_stream_ops_disabled_called(1, &stream);
	} else {
		/* Server-initiated disable operation that shall not cause transition to QoS */
		expect_bt_bap_stream_ops_qos_set_called(0, NULL);
	}
}

static void test_cis_link_loss_in_enabling_state_client_retries(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase;
	struct bt_iso_chan *chan;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}
	assert_non_null(ase);
	assert_true(ase_id != 0x00);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	test_preamble_state_enabling(conn, ase_id, stream);
	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	assert_int_equal(0, err);
	expect_bt_bap_stream_ops_connected_called(1, (const struct bt_bap_stream **)&stream);

	/* Mock CIS disconnection */
	mock_bt_iso_disconnected(chan, BT_HCI_ERR_CONN_FAIL_TO_ESTAB);

	test_drain_syswq(); /* Ensure that state transitions are completed */

	/* Expected to not notify the upper layers */
	expect_bt_bap_stream_ops_qos_set_called(0, NULL);
	expect_bt_bap_stream_ops_released_called(0, NULL);
	expect_bt_bap_stream_ops_disconnected_called(1, (const struct bt_bap_stream **)&stream);

	/* Client retries to establish CIS */
	err = mock_bt_iso_accept(conn, 0x01, 0x01, &chan);
	assert_int_equal(0, err);
	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		test_ase_control_client_receiver_start_ready(conn, ase_id);
	} else {
		err = bt_bap_stream_start(stream);
		assert_int_equal(0, err);
	}

	test_drain_syswq(); /* Ensure that state transitions are completed */

	const struct bt_bap_stream *streams[] = { stream, stream };

	expect_bt_bap_stream_ops_connected_called(ARRAY_SIZE(streams), streams);
	expect_bt_bap_stream_ops_started_called(1, &stream);
}

static struct bt_bap_stream *stream_allocated;
static const struct bt_bap_qos_cfg_pref qos_pref =
	BT_BAP_QOS_CFG_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000, 40000, 40000);

static int unicast_server_cb_config_custom_fake(struct bt_conn *conn, const struct bt_bap_ep *ep,
						enum bt_audio_dir dir,
						const struct bt_audio_codec_cfg *codec_cfg,
						struct bt_bap_stream **stream,
						struct bt_bap_qos_cfg_pref *const pref,
						struct bt_bap_ascs_rsp *rsp)
{
	*stream = stream_allocated;
	*pref = qos_pref;
	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS, BT_BAP_ASCS_REASON_NONE);

	bt_bap_stream_cb_register(*stream, &mock_bap_stream_ops);

	return 0;
}

static void test_ase_state_notification_retry(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	const struct bt_gatt_attr *ase, *cp;
	struct bt_conn_info info;
	uint8_t ase_id;
	int err;

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) {
		ase = fixture->ase_snk.attr;
		ase_id = fixture->ase_snk.id;
	} else {
		ase = fixture->ase_src.attr;
		ase_id = fixture->ase_src.id;
	}

	assert_non_null(ase);
	assert_int_not_equal(ase_id, 0x00);

	cp = test_ase_control_point_get();
	assert_non_null(cp);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	stream_allocated = stream;
	mock_bap_unicast_server_cb_config_fake.custom_fake = unicast_server_cb_config_custom_fake;

	/* Mock out of buffers case */
	mock_bt_gatt_notify_cb_fake.return_val = -ENOMEM;

	const uint8_t buf[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x01,           /* Target_Latency[0] = Target low latency */
		0x02,           /* Target_PHY[0] = LE 2M PHY */
		0x06,           /* Codec_ID[0].Coding_Format = LC3 */
		0x00, 0x00,     /* Codec_ID[0].Company_ID */
		0x00, 0x00,     /* Codec_ID[0].Vendor_Specific_Codec_ID */
		0x00,           /* Codec_Specific_Configuration_Length[0] */
	};

	cp->write(conn, cp, (void *)buf, sizeof(buf), 0, 0);

	/* Verification */
	expect_bt_bap_stream_ops_configured_called(0, NULL, NULL);

	mock_bt_gatt_notify_cb_fake.return_val = 0;

	err = bt_conn_get_info(conn, &info);
	assert_int_equal(err, 0);

	/* Wait for ASE state notification retry */
	k_sleep(K_USEC(info.le.interval_us));

	expect_bt_bap_stream_ops_configured_called(1, &stream, NULL);
}

int run_test_ase_control_params_tests(void);
int run_test_ase_register_tests(void);
int run_test_ase_state_transition_tests(void);
int run_test_ase_state_transition_invalid_tests(void);

static int run_ascs_test_suite(void)
{
	const struct CMUnitTest ascs_test_suite_tests[] = {
		cmocka_unit_test_setup_teardown(test_has_sink_ase_chrc, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_has_source_ase_chrc, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_has_control_point_chrc, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_sink_ase_read_state_idle, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_release_ase_on_callback_unregister, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_abort_client_operation_if_callback_not_registered, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_release_ase_on_acl_disconnection, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_release_ase_pair_on_acl_disconnection, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_recv_in_streaming_state, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_recv_in_enabling_state, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_cis_link_loss_in_streaming_state, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_cis_link_loss_in_disabling_state_v1, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_cis_link_loss_in_disabling_state_v2, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_cis_link_loss_in_enabling_state, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_cis_link_loss_in_enabling_state_client_retries, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ase_state_notification_retry, ascs_test_suite_case_setup, ascs_test_suite_case_teardown),
	};

	return cmocka_run_group_tests_name("ascs_test_suite", ascs_test_suite_tests, NULL, NULL);
}

int run_ascs_main_tests(void)
{
	int result = 0;

	result |= run_ascs_test_suite();

	return result;
}

int main(void)
{
	int result = 0;


	result |= run_ascs_main_tests();

	result |= run_test_ase_control_params_tests();

	result |= run_test_ase_register_tests();

	result |= run_test_ase_state_transition_tests();

	result |= run_test_ase_state_transition_invalid_tests();


	return result;
}
