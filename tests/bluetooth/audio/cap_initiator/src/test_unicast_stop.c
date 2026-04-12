/* test_unicast_stop.c - unit test for unicast stop procedure */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include <autoconf.h>
#include <bluetooth/assigned_numbers.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/bap.h>
#include <bluetooth/audio/bap_lc3_preset.h>
#include <bluetooth/audio/cap.h>
#include <bluetooth/hci_types.h>
#include <bluetooth/iso.h>

#include <base/utils.h>

#include "audio/bap_endpoint.h"
#include "cap_initiator.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

static_assert(CONFIG_BT_MAX_CONN * (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
				    CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT) >=
	      CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT,
	      "insufficient endpoints for configured group stream count");

#define INDEX_TO_DIR(_idx) (((_idx) & 1U) + 1U)

struct cap_initiator_test_unicast_stop_fixture {
	struct bt_bap_ep *snk_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	struct bt_bap_ep *src_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	struct bt_cap_stream *audio_stop_streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_audio_stop_param audio_stop_param;
	struct bt_cap_unicast_group *unicast_group;
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
	struct bt_bap_lc3_preset preset;
};

static struct cap_initiator_test_unicast_stop_fixture *group_fixture;

static void cap_initiator_test_unicast_stop_fixture_init(
	struct cap_initiator_test_unicast_stop_fixture *fixture)
{
	struct bt_cap_unicast_group_stream_pair_param
		group_pair_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	struct bt_cap_unicast_group_stream_param
		group_stream_param[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT] = {0};
	struct bt_cap_unicast_group_param group_param = {0};
	size_t stream_cnt = 0U;
	size_t pair_idx = 0U;
	int err;

	fixture->preset = (struct bt_bap_lc3_preset)BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i], i);
	}

	while (stream_cnt < ARRAY_SIZE(group_stream_param)) {
		const enum bt_audio_dir dir = INDEX_TO_DIR(stream_cnt);

		pair_idx = stream_cnt / 2U;
		group_stream_param[stream_cnt].stream = &fixture->cap_streams[stream_cnt];
		group_stream_param[stream_cnt].qos_cfg = &fixture->preset.qos;

		if (dir == BT_AUDIO_DIR_SINK) {
			group_pair_params[pair_idx].tx_param = &group_stream_param[stream_cnt];
		} else {
			group_pair_params[pair_idx].rx_param = &group_stream_param[stream_cnt];
		}

		stream_cnt++;
	}

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = pair_idx + 1U;
	group_param.params = group_pair_params;

	err = bt_cap_unicast_group_create(&group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);
}

static struct bt_conn *get_conn_from_index(struct cap_initiator_test_unicast_stop_fixture *fixture,
					   size_t index)
{
	const size_t conn_index = (index / 2U) % ARRAY_SIZE(fixture->conns);

	return &fixture->conns[conn_index];
}

static struct bt_bap_ep *get_ep_from_index(struct cap_initiator_test_unicast_stop_fixture *fixture,
					   size_t index)
{
	const size_t conn_index = (index / 2U) % ARRAY_SIZE(fixture->conns);
	const size_t ep_index = index / (ARRAY_SIZE(fixture->conns) * 2U);
	const enum bt_audio_dir dir = INDEX_TO_DIR(index);

	if (dir == BT_AUDIO_DIR_SINK) {
		return fixture->snk_eps[conn_index][ep_index];
	}

	return fixture->src_eps[conn_index][ep_index];
}

static void init_default_params(struct cap_initiator_test_unicast_stop_fixture *fixture)
{
	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		fixture->audio_stop_streams[i] = &fixture->cap_streams[i];
	}

	fixture->audio_stop_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->audio_stop_param.count = ARRAY_SIZE(fixture->cap_streams);
	fixture->audio_stop_param.streams = fixture->audio_stop_streams;
	fixture->audio_stop_param.release = false;
}

static int cap_initiator_test_unicast_stop_setup(void **state)
{
	group_fixture = calloc(1, sizeof(*group_fixture));
	assert_non_null(group_fixture);
	(void)state;

	return 0;
}

static int cap_initiator_test_unicast_stop_before(void **state)
{
	int err;

	assert_non_null(group_fixture);
	memset(group_fixture, 0, sizeof(*group_fixture));
	cap_initiator_test_unicast_stop_fixture_init(group_fixture);
	test_mocks_init();

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);

	mock_discover(group_fixture->conns, group_fixture->snk_eps, group_fixture->src_eps);
	init_default_params(group_fixture);
	*state = group_fixture;

	return 0;
}

static int cap_initiator_test_unicast_stop_after(void **state)
{
	struct cap_initiator_test_unicast_stop_fixture *fixture = *state;

	bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	bt_cap_initiator_unicast_audio_cancel();

	if (fixture->unicast_group != NULL) {
		const struct bt_cap_unicast_audio_stop_param param = {
			.type = BT_CAP_SET_TYPE_AD_HOC,
			.count = ARRAY_SIZE(fixture->cap_streams),
			.streams = fixture->audio_stop_streams,
			.release = true,
		};

		(void)bt_cap_initiator_unicast_audio_stop(&param);
		(void)bt_cap_unicast_group_delete(fixture->unicast_group);
	}

	test_mocks_cleanup();

	return 0;
}

static int cap_initiator_test_unicast_stop_teardown(void **state)
{
	free(group_fixture);
	group_fixture = NULL;
	(void)state;

	return 0;
}

static struct cap_initiator_test_unicast_stop_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

#define fixture get_fixture(state)

static void prepare_stream_states(struct cap_initiator_test_unicast_stop_fixture *fixture,
				  enum bt_bap_ep_state state)
{
	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		test_unicast_set_state(&fixture->cap_streams[i], get_conn_from_index(fixture, i),
				       get_ep_from_index(fixture, i), &fixture->preset, state);
	}
}

static void assert_stop_complete_count(unsigned int expected)
{
	expect_call_count("bt_cap_initiator_cb.unicast_stop_complete_cb", expected,
				  mock_cap_initiator_unicast_stop_complete_cb_fake.call_count);
}

static void assert_all_stream_states(struct cap_initiator_test_unicast_stop_fixture *fixture,
				     enum bt_bap_ep_state expected_state)
{
	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;

		assert_int_equal(bap_stream->ep->state, expected_state);
	}
}

static void assert_all_streams_released(struct cap_initiator_test_unicast_stop_fixture *fixture)
{
	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;

		assert_ptr_equal(bap_stream->ep, NULL);
	}
}

static void test_initiator_unicast_stop_disable_state_codec_configured(void **state)
{
	int err;

	prepare_stream_states(fixture, BT_BAP_EP_STATE_CODEC_CONFIGURED);
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, -EALREADY);
	assert_stop_complete_count(0);
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_CODEC_CONFIGURED);
}

static void test_initiator_unicast_stop_disable_state_qos_configured(void **state)
{
	int err;

	prepare_stream_states(fixture, BT_BAP_EP_STATE_QOS_CONFIGURED);
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, -EALREADY);
	assert_stop_complete_count(0);
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_QOS_CONFIGURED);
}

static void test_initiator_unicast_stop_disable_state_enabling(void **state)
{
	int err;

	prepare_stream_states(fixture, BT_BAP_EP_STATE_ENABLING);
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, 0);
	assert_stop_complete_count(1);
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_QOS_CONFIGURED);
}

static void test_initiator_unicast_stop_disable_state_streaming(void **state)
{
	int err;

	prepare_stream_states(fixture, BT_BAP_EP_STATE_STREAMING);
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, 0);
	assert_stop_complete_count(1);
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_QOS_CONFIGURED);
}

static void test_initiator_unicast_stop_release_state_codec_configured(void **state)
{
	int err;

	prepare_stream_states(fixture, BT_BAP_EP_STATE_CODEC_CONFIGURED);
	fixture->audio_stop_param.release = true;
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, 0);
	assert_stop_complete_count(1);
	assert_all_streams_released(fixture);
}

static void test_initiator_unicast_stop_release_state_qos_configured(void **state)
{
	int err;

	prepare_stream_states(fixture, BT_BAP_EP_STATE_QOS_CONFIGURED);
	fixture->audio_stop_param.release = true;
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, 0);
	assert_stop_complete_count(1);
	assert_all_streams_released(fixture);
}

static void test_initiator_unicast_stop_release_state_enabling(void **state)
{
	int err;

	prepare_stream_states(fixture, BT_BAP_EP_STATE_ENABLING);
	fixture->audio_stop_param.release = true;
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, 0);
	assert_stop_complete_count(1);
	assert_all_streams_released(fixture);
}

static void test_initiator_unicast_stop_release_state_streaming(void **state)
{
	int err;

	prepare_stream_states(fixture, BT_BAP_EP_STATE_STREAMING);
	fixture->audio_stop_param.release = true;
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, 0);
	assert_stop_complete_count(1);
	assert_all_streams_released(fixture);
}

static void test_initiator_unicast_stop_inval_param_null(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_unicast_audio_stop(NULL);
	assert_int_equal(err, -EINVAL);
	assert_stop_complete_count(0);
}

static void test_initiator_unicast_stop_inval_param_null_streams(void **state)
{
	int err;

	fixture->audio_stop_param.streams = NULL;
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, -EINVAL);
	assert_stop_complete_count(0);
}

static void test_initiator_unicast_stop_inval_missing_cas(void **state)
{
	int err;

	fixture->audio_stop_param.type = BT_CAP_SET_TYPE_CSIP;
	prepare_stream_states(fixture, BT_BAP_EP_STATE_STREAMING);
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, -EINVAL);
	assert_stop_complete_count(0);
}

static void test_initiator_unicast_stop_inval_param_zero_count(void **state)
{
	int err;

	fixture->audio_stop_param.count = 0U;
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, -EINVAL);
	assert_stop_complete_count(0);
}

static void test_initiator_unicast_stop_inval_param_inval_count(void **state)
{
	int err;

	fixture->audio_stop_param.count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT + 1U;
	err = bt_cap_initiator_unicast_audio_stop(&fixture->audio_stop_param);
	assert_int_equal(err, -EINVAL);
	assert_stop_complete_count(0);
}

int cap_initiator_unicast_stop_suite_run(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_disable_state_codec_configured,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_disable_state_qos_configured,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_disable_state_enabling,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_disable_state_streaming,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_release_state_codec_configured,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_release_state_qos_configured,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_release_state_enabling,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_release_state_streaming,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_inval_param_null,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_inval_param_null_streams,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_inval_missing_cas,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_inval_param_zero_count,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_stop_inval_param_inval_count,
					       cap_initiator_test_unicast_stop_before,
					       cap_initiator_test_unicast_stop_after),
	};

	return cmocka_run_group_tests_name("cap_initiator_test_unicast_stop", tests,
					  cap_initiator_test_unicast_stop_setup,
					  cap_initiator_test_unicast_stop_teardown);
}
