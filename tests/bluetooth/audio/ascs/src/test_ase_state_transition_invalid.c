/* test_ase_state_transition_invalid.c - ASE state transition tests */

/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>
#include <string.h>

#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/util_macro.h>
#include <sys/types.h>

#include "bap_unicast_server.h"
#include "bap_stream.h"
#include "conn.h"
#include "gatt_expects.h"

#include "test_common.h"

#define fixture (get_fixture(state))

struct test_ase_state_transition_invalid_fixture {
	const struct bt_gatt_attr *ase_cp;
	const struct bt_gatt_attr *ase_snk;
	const struct bt_gatt_attr *ase_src;
	struct bt_bap_stream stream;
	struct bt_conn conn;
};

static void *test_ase_state_transition_invalid_setup(void)
{
	struct test_ase_state_transition_invalid_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	assert_non_null(fixture);


	return fixture;
}

static void test_ase_state_transition_invalid_before(void *f)
{
	struct test_ase_state_transition_invalid_fixture *fixture =
		(struct test_ase_state_transition_invalid_fixture *)f;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
	};
	int err;

	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, 0);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	memset(fixture, 0, sizeof(struct test_ase_state_transition_invalid_fixture));
	fixture->ase_cp = test_ase_control_point_get();
	test_conn_init(&fixture->conn);
	test_ase_snk_get(CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT, &fixture->ase_snk);
	test_ase_src_get(CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT, &fixture->ase_src);
}

static void test_ase_state_transition_invalid_after(void *f)
{
	int err;

	err = bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	/* Sleep to trigger any pending state changes from unregister_cb */
	k_sleep(K_SECONDS(1));

	err = bt_bap_unicast_server_unregister();
	assert_int_equal(err, 0);
}

static void test_ase_state_transition_invalid_teardown(void *f)
{
	free(f);
}



static void test_client_config_codec_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
							     const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x01,           /* Opcode = Config Codec */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_config_codec(conn, ase_id, NULL);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_config_qos_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
							   const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x02,           /* Opcode = Config QoS */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_config_qos(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_enable_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
						       const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x03,           /* Opcode = Enable */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_enable(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_receiver_start_ready_expect_transition_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x04,           /* Opcode = Receiver Start Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_receiver_start_ready(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_receiver_start_ready_expect_ase_direction_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x04,           /* Opcode = Receiver Start Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x05,           /* Response_Code[0] = Invalid ASE direction */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_receiver_start_ready(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_disable_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
							const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x05,           /* Opcode = Disable */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_disable(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_receiver_stop_ready_expect_transition_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x06,           /* Opcode = Receiver Stop Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_receiver_stop_ready(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_receiver_stop_ready_expect_ase_direction_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x06,           /* Opcode = Receiver Stop Ready */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x05,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_receiver_stop_ready(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_update_metadata_expect_transition_error(
	struct bt_conn *conn, uint8_t ase_id, const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x07,           /* Opcode = Update Metadata */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_update_metadata(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static void test_client_release_expect_transition_error(struct bt_conn *conn, uint8_t ase_id,
							const struct bt_gatt_attr *ase_cp)
{
	const uint8_t expected_error[] = {
		0x08,           /* Opcode = Release */
		0x01,           /* Number_of_ASEs */
		ase_id,         /* ASE_ID[0] */
		0x04,           /* Response_Code[0] = Invalid ASE State Machine Transition */
		0x00,           /* Reason[0] */
	};

	test_ase_control_client_release(conn, ase_id);
	expect_bt_gatt_notify_cb_called_once(conn, BT_UUID_ASCS_ASE_CP, ase_cp, expected_error,
					     sizeof(expected_error));
	test_mocks_reset();
}

static struct test_ase_state_transition_invalid_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

static int test_ase_state_transition_invalid_case_setup(void **state)
{
	void *fixture_local = NULL;

	test_mocks_init();

	fixture_local = test_ase_state_transition_invalid_setup();

	*state = fixture_local;

	test_ase_state_transition_invalid_before(fixture_local);

	return 0;
}

static int test_ase_state_transition_invalid_case_teardown(void **state)
{
	void *fixture_local = state != NULL ? *state : NULL;


	test_ase_state_transition_invalid_after(fixture_local);

	test_mocks_cleanup();

	test_ase_state_transition_invalid_teardown(fixture_local);

	if (state != NULL) {
		*state = NULL;
	}

	return 0;
}

static void test_client_sink_state_idle(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);

	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
	test_client_release_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_sink_state_codec_configured(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_sink_state_qos_configured(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_sink_state_enabling(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_enabling(conn, ase_id, stream);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
}

static void test_sink_client_state_streaming(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
}

static void expect_ase_state_releasing(struct bt_conn *conn, const struct bt_gatt_attr *ase)
{
	struct test_ase_chrc_value_hdr hdr = { 0xff };
	ssize_t ret;

	assert_non_null(conn);
	assert_non_null(ase);

	ret = ase->read(conn, ase, &hdr, sizeof(hdr), 0);
	assert_false(ret < 0);
	assert_int_equal(BT_BAP_EP_STATE_RELEASING, hdr.ase_state);
}

static void test_client_sink_state_releasing(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_releasing(conn, ase_id, stream, &chan, false);
	expect_ase_state_releasing(conn, fixture->ase_snk);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_ase_direction_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_source_state_idle(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);

	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
	test_client_release_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_source_state_codec_configured(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_source_state_qos_configured(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_source_state_enabling(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_enabling(conn, ase_id, stream);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_source_state_streaming(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_source_state_disabling(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_disabling(conn, ase_id, stream, &chan);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_client_source_state_releasing(void **state)
{
	const struct bt_gatt_attr *ase_cp = fixture->ase_cp;
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_releasing(conn, ase_id, stream, &chan, true);
	expect_ase_state_releasing(conn, fixture->ase_src);

	test_client_config_codec_expect_transition_error(conn, ase_id, ase_cp);
	test_client_config_qos_expect_transition_error(conn, ase_id, ase_cp);
	test_client_enable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_start_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_receiver_stop_ready_expect_transition_error(conn, ase_id, ase_cp);
	test_client_disable_expect_transition_error(conn, ase_id, ase_cp);
	test_client_update_metadata_expect_transition_error(conn, ase_id, ase_cp);
}

static void test_server_config_codec_expect_error(struct bt_bap_stream *stream)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int err;

	err = bt_bap_stream_reconfig(stream, &codec_cfg);
	assert_false(err == 0);
}

static void test_server_receiver_start_ready_expect_error(struct bt_bap_stream *stream)
{
	int err;

	err = bt_bap_stream_start(stream);
	assert_false(err == 0);
}

static void test_server_disable_expect_error(struct bt_bap_stream *stream)
{
	int err;

	err = bt_bap_stream_disable(stream);
	assert_false(err == 0);
}

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
#include "bap_endpoint.h"

static void test_server_config_qos_expect_error(struct bt_bap_stream *stream)
{
	struct bt_bap_unicast_group group;
	int err;

	sys_slist_init(&group.streams);
	sys_slist_append(&group.streams, &stream->_node);

	err = bt_bap_stream_qos(stream->conn, &group);
	assert_false(err == 0);
}

static void test_server_enable_expect_error(struct bt_bap_stream *stream)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_RINGTONE)),
	};
	int err;

	err = bt_bap_stream_enable(stream, meta, ARRAY_SIZE(meta));
	assert_false(err == 0);
}

static void test_server_receiver_stop_ready_expect_error(struct bt_bap_stream *stream)
{
	int err;

	err = bt_bap_stream_stop(stream);
	assert_false(err == 0);
}
#else
#define test_server_config_qos_expect_error(...)
#define test_server_enable_expect_error(...)
#define test_server_receiver_stop_ready_expect_error(...)
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

static void test_server_update_metadata_expect_error(struct bt_bap_stream *stream)
{
	const uint8_t meta[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_RINGTONE)),
	};
	int err;

	err = bt_bap_stream_metadata(stream, meta, ARRAY_SIZE(meta));
	assert_false(err == 0);
}

static void test_server_sink_state_codec_configured(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

static void test_server_sink_state_qos_configured(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

static void test_server_sink_state_enabling(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_enabling(conn, ase_id, stream);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
}

static void test_server_sink_state_streaming(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_streaming(conn, ase_id, stream, &chan, false);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
}

static void test_server_sink_state_releasing(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SNK)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_snk);
	test_preamble_state_releasing(conn, ase_id, stream, &chan, false);
	expect_ase_state_releasing(conn, fixture->ase_snk);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

static void test_server_source_state_codec_configured(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_codec_configured(conn, ase_id, stream);

	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

static void test_server_source_state_qos_configured(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_qos_configured(conn, ase_id, stream);

	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

static void test_server_source_state_enabling(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_enabling(conn, ase_id, stream);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
}

static void test_server_source_state_streaming(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_streaming(conn, ase_id, stream, &chan, true);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
}

static void test_server_source_state_disabling(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_disabling(conn, ase_id, stream, &chan);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

static void test_server_source_state_releasing(void **state)
{
	struct bt_bap_stream *stream = &fixture->stream;
	struct bt_conn *conn = &fixture->conn;
	struct bt_iso_chan *chan;
	uint8_t ase_id;

	if (!IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) { skip(); }

	ase_id = test_ase_id_get(fixture->ase_src);
	test_preamble_state_releasing(conn, ase_id, stream, &chan, true);
	expect_ase_state_releasing(conn, fixture->ase_src);

	test_server_config_codec_expect_error(stream);
	test_server_config_qos_expect_error(stream);
	test_server_enable_expect_error(stream);
	test_server_receiver_start_ready_expect_error(stream);
	test_server_disable_expect_error(stream);
	test_server_receiver_stop_ready_expect_error(stream);
	test_server_update_metadata_expect_error(stream);
}

static int run_test_ase_state_transition_invalid(void)
{
	const struct CMUnitTest test_ase_state_transition_invalid_tests[] = {
		cmocka_unit_test_setup_teardown(test_client_sink_state_idle, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_sink_state_codec_configured, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_sink_state_qos_configured, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_sink_state_enabling, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_sink_client_state_streaming, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_sink_state_releasing, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_source_state_idle, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_source_state_codec_configured, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_source_state_qos_configured, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_source_state_enabling, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_source_state_streaming, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_source_state_disabling, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_client_source_state_releasing, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_sink_state_codec_configured, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_sink_state_qos_configured, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_sink_state_enabling, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_sink_state_streaming, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_sink_state_releasing, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_source_state_codec_configured, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_source_state_qos_configured, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_source_state_enabling, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_source_state_streaming, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_source_state_disabling, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
		cmocka_unit_test_setup_teardown(test_server_source_state_releasing, test_ase_state_transition_invalid_case_setup, test_ase_state_transition_invalid_case_teardown),
	};

	return cmocka_run_group_tests_name("test_ase_state_transition_invalid", test_ase_state_transition_invalid_tests, NULL, NULL);
}

int run_test_ase_state_transition_invalid_tests(void)
{
	int result = 0;

	result |= run_test_ase_state_transition_invalid();

	return result;
}

