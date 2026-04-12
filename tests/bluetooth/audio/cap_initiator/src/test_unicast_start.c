/* test_unicast_start.c - unit test for unicast start procedure */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
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
#include "audio/bap_iso.h"
#include "cap_initiator.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

static_assert(CONFIG_BT_MAX_CONN * (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
				    CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT) >=
	      CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT,
	      "insufficient endpoints for configured group stream count");

/* Either BT_AUDIO_DIR_SINK or BT_AUDIO_DIR_SOURCE */
#define INDEX_TO_DIR(_idx) (((_idx) & 1U) + 1U)

struct cap_initiator_test_unicast_start_fixture {
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_bap_ep *snk_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	struct bt_bap_ep *src_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	struct bt_cap_unicast_audio_start_stream_param
		audio_start_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_audio_start_param audio_start_param;
	struct bt_cap_unicast_group *unicast_group;
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
	struct bt_bap_lc3_preset preset;
};

static struct cap_initiator_test_unicast_start_fixture *group_fixture;

static void cap_initiator_test_unicast_start_fixture_init(
	struct cap_initiator_test_unicast_start_fixture *fixture)
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

static int cap_initiator_test_unicast_start_setup(void **state)
{
	group_fixture = calloc(1, sizeof(*group_fixture));
	assert_non_null(group_fixture);
	(void)state;

	return 0;
}

static void init_default_params(struct cap_initiator_test_unicast_start_fixture *fixture)
{
	ARRAY_FOR_EACH(fixture->audio_start_stream_params, i) {
		struct bt_cap_unicast_audio_start_stream_param *stream_param =
			&fixture->audio_start_stream_params[i];
		const size_t conn_index = (i / 2U) % ARRAY_SIZE(fixture->conns);
		const size_t ep_index = i / (ARRAY_SIZE(fixture->conns) * 2U);
		const enum bt_audio_dir dir = INDEX_TO_DIR(i);

		stream_param->stream = &fixture->cap_streams[i];
		stream_param->codec_cfg = &fixture->preset.codec_cfg;
		stream_param->member.member = &fixture->conns[conn_index];

		if (dir == BT_AUDIO_DIR_SINK) {
			stream_param->ep = fixture->snk_eps[conn_index][ep_index];
		} else {
			stream_param->ep = fixture->src_eps[conn_index][ep_index];
		}
	}

	fixture->audio_start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->audio_start_param.count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT;
	fixture->audio_start_param.stream_params = fixture->audio_start_stream_params;
}

static int cap_initiator_test_unicast_start_before(void **state)
{
	int err;

	assert_non_null(group_fixture);
	memset(group_fixture, 0, sizeof(*group_fixture));
	cap_initiator_test_unicast_start_fixture_init(group_fixture);
	test_mocks_init();

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);

	mock_discover(group_fixture->conns, group_fixture->snk_eps, group_fixture->src_eps);
	init_default_params(group_fixture);
	*state = group_fixture;

	return 0;
}

static int cap_initiator_test_unicast_start_after(void **state)
{
	struct cap_initiator_test_unicast_start_fixture *fixture = *state;

	bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	bt_cap_initiator_unicast_audio_cancel();

	if (fixture->unicast_group != NULL) {
		struct bt_cap_stream *cap_stream_ptrs[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
		const struct bt_cap_unicast_audio_stop_param param = {
			.type = BT_CAP_SET_TYPE_AD_HOC,
			.count = ARRAY_SIZE(fixture->cap_streams),
			.streams = cap_stream_ptrs,
			.release = true,
		};

		ARRAY_FOR_EACH(cap_stream_ptrs, idx) {
			cap_stream_ptrs[idx] = &fixture->cap_streams[idx];
		}

		(void)bt_cap_initiator_unicast_audio_stop(&param);
		(void)bt_cap_unicast_group_delete(fixture->unicast_group);
	}

	test_mocks_cleanup();

	return 0;
}

static int cap_initiator_test_unicast_start_teardown(void **state)
{
	free(group_fixture);
	group_fixture = NULL;
	(void)state;

	return 0;
}

static struct cap_initiator_test_unicast_start_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

#define fixture get_fixture(state)

static void assert_start_complete_count(unsigned int expected)
{
	expect_call_count("bt_cap_initiator_cb.unicast_start_complete_cb", expected,
				  mock_cap_initiator_unicast_start_complete_cb_fake.call_count);
}

static void assert_successful_start_callback(void)
{
	assert_start_complete_count(1);
	assert_int_equal(0, mock_cap_initiator_unicast_start_complete_cb_fake.arg0_history[0]);
	assert_ptr_equal(NULL, mock_cap_initiator_unicast_start_complete_cb_fake.arg1_history[0]);
}

static void assert_all_stream_states(struct cap_initiator_test_unicast_start_fixture *fixture,
				     enum bt_bap_ep_state expected_state)
{
	ARRAY_FOR_EACH(fixture->cap_streams, i) {
		const struct bt_bap_stream *bap_stream = &fixture->cap_streams[i].bap_stream;

		assert_int_equal(bap_stream->ep->state, expected_state);
	}
}

static void test_initiator_unicast_start(void **state)
{
	int err;

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, 0);
	assert_successful_start_callback();
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_STREAMING);
}

static void test_initiator_unicast_start_inval_param_null(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_unicast_audio_start(NULL);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_null_param(void **state)
{
	int err;

	fixture->audio_start_param.stream_params = NULL;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_null_member(void **state)
{
	int err;

	fixture->audio_start_stream_params[0].member.member = NULL;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_missing_cas(void **state)
{
	int err;

	fixture->audio_start_param.type = BT_CAP_SET_TYPE_CSIP;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_zero_count(void **state)
{
	int err;

	fixture->audio_start_param.count = 0U;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_inval_count(void **state)
{
	int err;

	fixture->audio_start_param.count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT + 1U;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_inval_stream_param_null_stream(void **state)
{
	int err;

	fixture->audio_start_stream_params[0].stream = NULL;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_inval_stream_param_null_codec_cfg(void **state)
{
	int err;

	fixture->audio_start_stream_params[0].codec_cfg = NULL;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_inval_stream_param_null_member(void **state)
{
	int err;

	fixture->audio_start_stream_params[0].member.member = NULL;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_inval_stream_param_null_ep(void **state)
{
	int err;

	fixture->audio_start_stream_params[0].ep = NULL;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_inval_param_inval_stream_param_invalid_meta(void **state)
{
	int err;

	memset(fixture->audio_start_stream_params[0].codec_cfg->meta, 0,
	       sizeof(fixture->audio_start_stream_params[0].codec_cfg->meta));
	fixture->audio_start_stream_params[0].codec_cfg->meta_len = 0U;
	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EINVAL);
	assert_start_complete_count(0);
}

static void test_initiator_unicast_start_state_codec_configured(void **state)
{
	int err;

	ARRAY_FOR_EACH_PTR(fixture->audio_start_stream_params, stream_param) {
		test_unicast_set_state(stream_param->stream, stream_param->member.member,
				       stream_param->ep, &fixture->preset,
				       BT_BAP_EP_STATE_CODEC_CONFIGURED);
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, 0);
	assert_successful_start_callback();
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_STREAMING);
}

static void test_initiator_unicast_start_state_qos_configured(void **state)
{
	int err;

	ARRAY_FOR_EACH_PTR(fixture->audio_start_stream_params, stream_param) {
		test_unicast_set_state(stream_param->stream, stream_param->member.member,
				       stream_param->ep, &fixture->preset,
				       BT_BAP_EP_STATE_QOS_CONFIGURED);
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, 0);
	assert_successful_start_callback();
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_STREAMING);
}

static void test_initiator_unicast_start_state_enabling(void **state)
{
	int err;

	ARRAY_FOR_EACH_PTR(fixture->audio_start_stream_params, stream_param) {
		test_unicast_set_state(stream_param->stream, stream_param->member.member,
				       stream_param->ep, &fixture->preset,
				       BT_BAP_EP_STATE_ENABLING);
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, 0);
	assert_successful_start_callback();
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_STREAMING);
}

static void test_initiator_unicast_start_state_streaming(void **state)
{
	int err;

	ARRAY_FOR_EACH_PTR(fixture->audio_start_stream_params, stream_param) {
		test_unicast_set_state(stream_param->stream, stream_param->member.member,
				       stream_param->ep, &fixture->preset,
				       BT_BAP_EP_STATE_STREAMING);
	}

	err = bt_cap_initiator_unicast_audio_start(&fixture->audio_start_param);
	assert_int_equal(err, -EALREADY);
	assert_start_complete_count(0);
	assert_all_stream_states(fixture, BT_BAP_EP_STATE_STREAMING);
}

int cap_initiator_unicast_start_suite_run(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_inval_param_null,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_inval_param_null_param,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_inval_param_null_member,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_inval_missing_cas,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_inval_param_zero_count,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_inval_param_inval_count,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(
			test_initiator_unicast_start_inval_param_inval_stream_param_null_stream,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(
			test_initiator_unicast_start_inval_param_inval_stream_param_null_codec_cfg,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(
			test_initiator_unicast_start_inval_param_inval_stream_param_null_member,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(
			test_initiator_unicast_start_inval_param_inval_stream_param_null_ep,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(
			test_initiator_unicast_start_inval_param_inval_stream_param_invalid_meta,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_state_codec_configured,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_state_qos_configured,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_state_enabling,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_start_state_streaming,
					       cap_initiator_test_unicast_start_before,
					       cap_initiator_test_unicast_start_after),
	};

	return cmocka_run_group_tests_name("cap_initiator_test_unicast_start", tests,
					  cap_initiator_test_unicast_start_setup,
					  cap_initiator_test_unicast_start_teardown);
}
