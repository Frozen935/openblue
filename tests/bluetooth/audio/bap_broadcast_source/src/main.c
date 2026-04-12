/* main.c - Application main entry point */

/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include <base/bt_buf.h>
#include <bluetooth/assigned_numbers.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/bap.h>
#include <bluetooth/audio/lc3.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/byteorder.h>
#include <bluetooth/hci_types.h>
#include <bluetooth/iso.h>

#define check_equal(expected, actual, ...) assert_int_equal((expected), (actual))
#define check_not_equal(expected, actual, ...) assert_true((expected) != (actual))
#define check_true(value, ...) assert_true(value)
#define check_false(value, ...) assert_false(value)
#define check_not_null(value, ...) assert_non_null(value)
#define check_null(value, ...) assert_null(value)
#define expect_call_count(_func_name, expected, actual) assert_int_equal((expected), (actual))
#define BAP_TEST(_name) cmocka_unit_test_setup_teardown(_name, test_case_setup, test_case_teardown)
#define CALLBACK_TEST(_name)                                                                    \
	cmocka_unit_test_setup_teardown(_name, callback_test_case_setup, callback_test_case_teardown)

struct bt_le_ext_adv {
	uint8_t id;
	uint8_t handle;
};

struct bt_conn {
	struct bt_iso_chan *chan;
};

struct bt_iso_big {
	struct bt_iso_chan *bis[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	uint8_t num_bis;
};

struct mock_bap_stream_state {
	size_t connected_count;
	size_t started_count;
	size_t sent_count;
	size_t disconnected_count;
	size_t stopped_count;
	struct bt_bap_stream *started_history[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
};

struct mock_bap_broadcast_source_state {
	size_t started_count;
	size_t stopped_count;
};

static struct mock_bap_stream_state mock_bap_stream_state;
static struct mock_bap_broadcast_source_state mock_bap_broadcast_source_state;
static struct bt_iso_big_cb *registered_iso_big_cb;

static void mock_bap_stream_connected_cb(struct bt_bap_stream *stream)
{
	(void)stream;
	mock_bap_stream_state.connected_count++;
}

static void mock_bap_stream_started_cb(struct bt_bap_stream *stream)
{
	if (mock_bap_stream_state.started_count < ARRAY_SIZE(mock_bap_stream_state.started_history)) {
		mock_bap_stream_state.started_history[mock_bap_stream_state.started_count] = stream;
	}

	mock_bap_stream_state.started_count++;
}

static void mock_bap_stream_sent_cb(struct bt_bap_stream *stream)
{
	(void)stream;
	mock_bap_stream_state.sent_count++;
}

static void mock_bap_stream_disconnected_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	(void)stream;
	(void)reason;
	mock_bap_stream_state.disconnected_count++;
}

static void mock_bap_stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	(void)stream;
	(void)reason;
	mock_bap_stream_state.stopped_count++;
}

static void mock_bap_broadcast_source_started_cb(struct bt_bap_broadcast_source *source)
{
	(void)source;
	mock_bap_broadcast_source_state.started_count++;
}

static void mock_bap_broadcast_source_stopped_cb(struct bt_bap_broadcast_source *source,
					      uint8_t reason)
{
	(void)source;
	(void)reason;
	mock_bap_broadcast_source_state.stopped_count++;
}

static struct bt_bap_stream_ops mock_bap_stream_ops = {
	.started = mock_bap_stream_started_cb,
	.stopped = mock_bap_stream_stopped_cb,
	.sent = mock_bap_stream_sent_cb,
	.connected = mock_bap_stream_connected_cb,
	.disconnected = mock_bap_stream_disconnected_cb,
};

struct bt_bap_broadcast_source_cb mock_bap_broadcast_source_cb = {
	.started = mock_bap_broadcast_source_started_cb,
	.stopped = mock_bap_broadcast_source_stopped_cb,
};

void mock_bap_broadcast_source_init(void)
{
	memset(&mock_bap_broadcast_source_state, 0, sizeof(mock_bap_broadcast_source_state));
}

void mock_bap_stream_init(void)
{
	memset(&mock_bap_stream_state, 0, sizeof(mock_bap_stream_state));
}

void mock_bap_stream_cleanup(void)
{
}

static void mock_bt_iso_connected(struct bt_conn *iso)
{
	struct bt_iso_chan *chan = iso->chan;

	chan->state = BT_ISO_STATE_CONNECTED;
	chan->iso = iso;

	if (chan->ops != NULL && chan->ops->connected != NULL) {
		chan->ops->connected(chan);
	}
}

static int mock_bt_iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	chan->state = BT_ISO_STATE_DISCONNECTED;

	if (chan->ops != NULL && chan->ops->disconnected != NULL) {
		chan->ops->disconnected(chan, reason);
	}

	free(chan->iso);
	chan->iso = NULL;

	return 0;
}

int bt_iso_big_register_cb(struct bt_iso_big_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	registered_iso_big_cb = cb;
	return 0;
}

int bt_iso_big_create(struct bt_le_ext_adv *adv, struct bt_iso_big_create_param *param,
		      struct bt_iso_big **out_big)
{
	struct bt_iso_big *big;

	(void)adv;
	check_not_null(param);
	check_not_null(out_big);
	check_not_equal(0, param->num_bis);

	big = calloc(1, sizeof(*big));
	check_not_null(big);

	for (uint8_t i = 0U; i < param->num_bis; i++) {
		struct bt_iso_chan *bis = param->bis_channels[i];
		struct bt_conn *iso;

		check_not_null(bis);
		iso = calloc(1, sizeof(*iso));
		check_not_null(iso);

		big->bis[i] = bis;
		big->num_bis++;
		iso->chan = bis;
		mock_bt_iso_connected(iso);
	}

	*out_big = big;

	if (registered_iso_big_cb != NULL && registered_iso_big_cb->started != NULL) {
		registered_iso_big_cb->started(big);
	}

	return 0;
}

int bt_iso_big_terminate(struct bt_iso_big *big)
{
	if (big == NULL || big->num_bis == 0U) {
		return -EINVAL;
	}

	for (uint8_t i = 0U; i < big->num_bis; i++) {
		check_not_null(big->bis[i]);
		(void)mock_bt_iso_disconnected(big->bis[i], BT_HCI_ERR_LOCALHOST_TERM_CONN);
	}

	if (registered_iso_big_cb != NULL && registered_iso_big_cb->stopped != NULL) {
		registered_iso_big_cb->stopped(big, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	}

	free(big);
	return 0;
}

int bt_iso_chan_send(struct bt_iso_chan *chan, struct bt_buf *buf, uint16_t seq_num)
{
	(void)buf;
	(void)seq_num;

	if (chan->ops != NULL && chan->ops->sent != NULL) {
		chan->ops->sent(chan);
	}

	return 0;
}

int bt_iso_chan_send_ts(struct bt_iso_chan *chan, struct bt_buf *buf, uint16_t seq_num, uint32_t ts)
{
	(void)ts;
	return bt_iso_chan_send(chan, buf, seq_num);
}

int bt_iso_chan_get_tx_sync(const struct bt_iso_chan *chan, struct bt_iso_tx_info *info)
{
	(void)chan;

	if (info != NULL) {
		memset(info, 0, sizeof(*info));
	}

	return 0;
}

struct bap_broadcast_source_test_suite_fixture {
	struct bt_bap_broadcast_source_param *param;
	size_t stream_cnt;
	struct bt_bap_broadcast_source *source;
};

static struct bap_broadcast_source_test_suite_fixture *group_fixture;

static void bap_broadcast_source_test_suite_fixture_init(
	struct bap_broadcast_source_test_suite_fixture *fixture)
{
	const uint8_t bis_cfg_data[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
				    BT_BYTES_LIST_LE32(BT_AUDIO_LOCATION_FRONT_LEFT |
					       BT_AUDIO_LOCATION_FRONT_RIGHT)),
	};
	const size_t streams_per_subgroup = CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT /
					    CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT;
	const enum bt_audio_context ctx = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
	const enum bt_audio_location loc = BT_AUDIO_LOCATION_FRONT_LEFT;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_param;
	struct bt_bap_broadcast_source_stream_param *stream_params;
	struct bt_audio_codec_cfg *codec_cfg;
	struct bt_bap_qos_cfg *qos_cfg;
	struct bt_bap_stream *streams;
	const uint16_t latency = 10U;
	const uint32_t pd = 40000U;
	const uint16_t sdu = 40U;
	const uint8_t rtn = 2U;
	uint8_t *bis_data;

	check_true(streams_per_subgroup > 0U);
	check_true(sizeof(bis_cfg_data) <= CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE);

	fixture->param = malloc(sizeof(struct bt_bap_broadcast_source_param));
	subgroup_param = malloc(sizeof(struct bt_bap_broadcast_source_subgroup_param) *
				CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT);
	check_not_null(subgroup_param);
	stream_params = malloc(sizeof(struct bt_bap_broadcast_source_stream_param) *
			       CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	check_not_null(stream_params);
	codec_cfg = malloc(sizeof(struct bt_audio_codec_cfg));
	check_not_null(codec_cfg);
	qos_cfg = malloc(sizeof(struct bt_bap_qos_cfg));
	check_not_null(qos_cfg);
	streams = malloc(sizeof(struct bt_bap_stream) * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	check_not_null(streams);
	bis_data = malloc(CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE);
	check_not_null(bis_data);

	memset(fixture->param, 0, sizeof(*fixture->param));
	memset(subgroup_param, 0,
	       sizeof(struct bt_bap_broadcast_source_subgroup_param) *
		       CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT);
	memset(stream_params, 0,
	       sizeof(struct bt_bap_broadcast_source_stream_param) *
		       CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	memset(codec_cfg, 0, sizeof(struct bt_audio_codec_cfg));
	memset(qos_cfg, 0, sizeof(struct bt_bap_qos_cfg));
	memset(streams, 0, sizeof(struct bt_bap_stream) * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	memset(bis_data, 0, CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE);

	*codec_cfg = BT_AUDIO_CODEC_LC3_CONFIG(BT_AUDIO_CODEC_CFG_FREQ_16KHZ,
				       BT_AUDIO_CODEC_CFG_DURATION_10, loc, 40U, 1, ctx);
	*qos_cfg = BT_BAP_QOS_CFG_UNFRAMED(10000u, sdu, rtn, latency, pd);
	memcpy(bis_data, bis_cfg_data, sizeof(bis_cfg_data));

	for (size_t i = 0U; i < CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT; i++) {
		subgroup_param[i].params_count = streams_per_subgroup;
		subgroup_param[i].params = stream_params + i * streams_per_subgroup;
		subgroup_param[i].codec_cfg = codec_cfg;
	}

	for (size_t i = 0U; i < CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT; i++) {
		stream_params[i].stream = &streams[i];
		stream_params[i].data = bis_data;
		stream_params[i].data_len = sizeof(bis_cfg_data);
		bt_bap_stream_cb_register(stream_params[i].stream, &mock_bap_stream_ops);
	}

	fixture->param->params_count = CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT;
	fixture->param->params = subgroup_param;
	fixture->param->qos = qos_cfg;
	fixture->param->encryption = false;
	memset(fixture->param->broadcast_code, 0, sizeof(fixture->param->broadcast_code));
	fixture->param->packing = BT_ISO_PACKING_SEQUENTIAL;

	fixture->stream_cnt = fixture->param->params_count * streams_per_subgroup;
}

static struct bap_broadcast_source_test_suite_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);
	return *state;
}

static int test_group_setup(void **state)
{
	(void)state;
	group_fixture = calloc(1, sizeof(*group_fixture));
	assert_non_null(group_fixture);
	return 0;
}

static int test_case_setup(void **state)
{
	int err;

	assert_non_null(group_fixture);
	mock_bap_broadcast_source_init();
	mock_bap_stream_init();
	memset(group_fixture, 0, sizeof(*group_fixture));
	bap_broadcast_source_test_suite_fixture_init(group_fixture);

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	assert_int_equal(err, 0);

	*state = group_fixture;
	return 0;
}

static int test_case_teardown(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;

	if (fixture->source != NULL) {
		(void)bt_bap_broadcast_source_stop(fixture->source);
		assert_int_equal(bt_bap_broadcast_source_delete(fixture->source), 0);
		fixture->source = NULL;
	}

	if (param != NULL) {
		free(param->params[0].params[0].data);
		free(param->params[0].params[0].stream);
		free(param->params[0].params);
		free(param->params[0].codec_cfg);
		free(param->params);
		free(param->qos);
		free(param);
		fixture->param = NULL;
	}

	(void)bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
	mock_bap_stream_cleanup();
	return 0;
}

static int test_group_teardown(void **state)
{
	(void)state;
	free(group_fixture);
	group_fixture = NULL;
	return 0;
}

int callback_test_case_setup(void **state);
int callback_test_case_teardown(void **state);
void test_broadcast_source_register_cb(void **state);
void test_broadcast_source_register_cb_inval_param_null(void **state);
void test_broadcast_source_register_cb_inval_double_register(void **state);
void test_broadcast_source_unregister_cb(void **state);
void test_broadcast_source_unregister_cb_inval_param_null(void **state);
void test_broadcast_source_unregister_cb_inval_double_unregister(void **state);
static void test_broadcast_source_create_delete(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	for (size_t i = 0u; i < create_param->params_count; i++) {
		for (size_t j = 0u; j < create_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				create_param->params[i].params[j].stream;

			check_equal(create_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			check_equal(create_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			check_equal(create_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_create_start_send_stop_delete(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	check_equal(0, err, "Unable to start broadcast source: err %d", err);

	expect_call_count("bt_bap_stream_ops.connected", fixture->stream_cnt,
			   mock_bap_stream_state.connected_count);
	expect_call_count("bt_bap_stream_ops.started", fixture->stream_cnt,
			   mock_bap_stream_state.started_count);
	expect_call_count("bt_bap_broadcast_source_cb.started", 1,
			   mock_bap_broadcast_source_state.started_count);

	for (size_t i = 0U; i < create_param->params_count; i++) {
		for (size_t j = 0U; j < create_param->params[i].params_count; j++) {
			struct bt_bap_stream *bap_stream = create_param->params[i].params[j].stream;

			/* verify bap stream started cb stream parameter */
			check_equal(mock_bap_stream_state.started_history[i], bap_stream);
			struct bt_audio_codec_cfg *codec_cfg = bap_stream->codec_cfg;
			enum bt_audio_location chan_allocation;
			/* verify subgroup codec data */
			check_equal(bt_audio_codec_cfg_get_freq(codec_cfg),
				      BT_AUDIO_CODEC_CFG_FREQ_16KHZ);
			check_equal(bt_audio_codec_cfg_get_frame_dur(codec_cfg),
				      BT_AUDIO_CODEC_CFG_DURATION_10);
			/* verify bis specific codec data */
			bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &chan_allocation, false);
			check_equal(chan_allocation,
				      BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT);
			/* Since BAP doesn't care about the `buf` we can just provide NULL */
			err = bt_bap_stream_send(bap_stream, NULL, 0);
			check_equal(0, err,
				      "Unable to send on broadcast stream[%zu][%zu]: err %d", i, j,
				      err);
		}
	}

	expect_call_count("bt_bap_stream_ops.sent", fixture->stream_cnt,
			   mock_bap_stream_state.sent_count);

	err = bt_bap_broadcast_source_stop(fixture->source);
	check_equal(0, err, "Unable to stop broadcast source: err %d", err);

	expect_call_count("bt_bap_stream_ops.disconnected", fixture->stream_cnt,
			   mock_bap_stream_state.disconnected_count);
	expect_call_count("bt_bap_stream_ops.stopped", fixture->stream_cnt,
			   mock_bap_stream_state.stopped_count);
	expect_call_count("bt_bap_broadcast_source_cb.stopped", 1,
			   mock_bap_broadcast_source_state.stopped_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_create_inval_param_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_bap_broadcast_source_create(NULL, &fixture->source);
	check_not_equal(0, err, "Did not fail with null params");
}

static void test_broadcast_source_create_inval_source_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	err = bt_bap_broadcast_source_create(create_param, NULL);
	check_not_equal(0, err, "Did not fail with null source");
}

static void test_broadcast_source_create_inval_subgroup_params_count_0(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	create_param->params_count = 0U;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with params_count %u", create_param->params_count);
}

static void test_broadcast_source_create_inval_subgroup_params_count_above_max(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	create_param->params_count = CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with params_count %u", create_param->params_count);
}

static void test_broadcast_source_create_inval_subgroup_params_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	int err;

	create_param->params = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	create_param->params = subgroup_params;
	check_not_equal(0, err, "Did not fail with NULL subgroup params");
}

static void test_broadcast_source_create_inval_qos_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_qos_cfg *qos = create_param->qos;
	int err;

	create_param->qos = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	create_param->qos = qos;
	check_not_equal(0, err, "Did not fail with NULL qos");
}

static void test_broadcast_source_create_inval_packing(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	create_param->packing = 0x02;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with packing %u", create_param->packing);
}

static void test_broadcast_source_create_inval_subgroup_params_params_count_0(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	int err;

	subgroup_params->params_count = 0U;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with %u stream params",
			  subgroup_params->params_count);
}

static void test_broadcast_source_create_inval_subgroup_params_params_count_above_max(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	int err;

	subgroup_params->params_count = CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with %u stream params",
			  subgroup_params->params_count);
}

static void test_broadcast_source_create_inval_subgroup_params_stream_params_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	int err;

	subgroup_params->params = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	subgroup_params->params = stream_params;
	check_not_equal(0, err, "Did not fail with NULL stream params");
}

static void test_broadcast_source_create_inval_subgroup_params_codec_cfg_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	subgroup_params->codec_cfg = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	subgroup_params->codec_cfg = codec_cfg;
	check_not_equal(0, err, "Did not fail with NULL codec_cfg");
}

static void test_broadcast_source_create_inval_subgroup_params_codec_cfg_data_len(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	codec_cfg->data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with codec_cfg->data_len %zu", codec_cfg->data_len);
}

static void test_broadcast_source_create_inval_subgroup_params_codec_cfg_meta_len(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	codec_cfg->meta_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with codec_cfg->meta_len %zu", codec_cfg->meta_len);
}

static void test_broadcast_source_create_inval_subgroup_params_codec_cfg_cid(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	codec_cfg->id = BT_HCI_CODING_FORMAT_LC3;
	codec_cfg->cid = 0x01; /* Shall be 0 if id == 0x06 (LC3)*/
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with codec_cfg->cid %u", codec_cfg->cid);
}

static void test_broadcast_source_create_inval_subgroup_params_codec_cfg_vid(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	codec_cfg->id = BT_HCI_CODING_FORMAT_LC3;
	codec_cfg->vid = 0x01; /* Shall be 0 if id == 0x06 (LC3)*/
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with codec_cfg->vid %u", codec_cfg->vid);
}

static void test_broadcast_source_create_inval_stream_params_stream_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	struct bt_bap_stream *stream = stream_params->stream;
	int err;

	stream_params->stream = NULL;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	stream_params->stream = stream;
	check_not_equal(0, err, "Did not fail with NULL stream_params->stream");
}

static void test_broadcast_source_create_inval_stream_params_data_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	uint8_t *data = stream_params->data;
	int err;

	stream_params->data = NULL;
	stream_params->data_len = 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	/* Restore the params for the cleanup after function */
	stream_params->data = data;
	check_not_equal(
		0, err,
		"Did not fail with NULL stream_params->data and stream_params_>data_len %zu",
		stream_params->data_len);
}

static void test_broadcast_source_create_inval_stream_params_data_len(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &create_param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	int err;

	stream_params->data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_not_equal(0, err, "Did not fail with stream_params_>data_len %zu",
			  stream_params->data_len);
}

static void test_broadcast_source_start_inval_source_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(NULL, &ext_adv);
	check_not_equal(0, err, "Did not fail with null source");
}

static void test_broadcast_source_start_inval_ext_adv_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, NULL);
	check_not_equal(0, err, "Did not fail with null ext_adv");
}

static void test_broadcast_source_start_inval_double_start(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	check_equal(0, err, "Unable to start broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	check_not_equal(0, err, "Did not fail with starting already started source");
}

static void test_broadcast_source_reconfigure_single_subgroup(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *reconf_param = fixture->param;
	const size_t subgroup_cnt = reconf_param->params_count;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       reconf_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(reconf_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	for (size_t i = 0u; i < reconf_param->params_count; i++) {
		for (size_t j = 0u; j < reconf_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				reconf_param->params[i].params[j].stream;

			check_equal(reconf_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			check_equal(reconf_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			check_equal(reconf_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	reconf_param->params_count = 1U;
	reconf_param->qos->sdu = 100U;
	reconf_param->qos->rtn = 3U;
	reconf_param->qos->phy = 1U;

	err = bt_bap_broadcast_source_reconfig(fixture->source, reconf_param);
	check_equal(0, err, "Unable to reconfigure broadcast source: err %d", err);

	for (size_t i = 0u; i < subgroup_cnt; i++) {
		for (size_t j = 0u; j < reconf_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				reconf_param->params[i].params[j].stream;

			check_equal(reconf_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			check_equal(reconf_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			check_equal(reconf_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_all(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *reconf_param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       reconf_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(reconf_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	for (size_t i = 0u; i < reconf_param->params_count; i++) {
		for (size_t j = 0u; j < reconf_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				reconf_param->params[i].params[j].stream;

			check_equal(reconf_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			check_equal(reconf_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			check_equal(reconf_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	reconf_param->qos->sdu = 100U;
	reconf_param->qos->rtn = 3U;
	reconf_param->qos->phy = 1U;

	err = bt_bap_broadcast_source_reconfig(fixture->source, reconf_param);
	check_equal(0, err, "Unable to reconfigure broadcast source: err %d", err);

	for (size_t i = 0u; i < reconf_param->params_count; i++) {
		for (size_t j = 0u; j < reconf_param->params[i].params_count; j++) {
			const struct bt_bap_stream *stream =
				reconf_param->params[i].params[j].stream;

			check_equal(reconf_param->qos->sdu, stream->qos->sdu,
				      "Unexpected stream SDU");
			check_equal(reconf_param->qos->rtn, stream->qos->rtn,
				      "Unexpected stream RTN");
			check_equal(reconf_param->qos->phy, stream->qos->phy,
				      "Unexpected stream PHY");
		}
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_param_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_reconfig(fixture->source, NULL);
	check_not_equal(0, err, "Did not fail with null params");

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_source_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_reconfig(NULL, param);
	check_not_equal(0, err, "Did not fail with null source");

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_count_0(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->params_count = 0U;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with params_count %u", param->params_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_count_above_max(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->params_count = CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with params_count %u", param->params_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->params = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	param->params = subgroup_params;
	check_not_equal(0, err, "Did not fail with NULL subgroup params");

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_qos_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_qos_cfg *qos = param->qos;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->qos = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	param->qos = qos;
	check_not_equal(0, err, "Did not fail with NULL qos");

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_packing(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	param->packing = 0x02;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with packing %u", param->packing);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_params_count_0(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	subgroup_params->params_count = 0U;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with %u stream params",
			  subgroup_params->params_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_params_count_above_max(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	subgroup_params->params_count = CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with %u stream params",
			  subgroup_params->params_count);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_stream_params_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	subgroup_params->params = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	subgroup_params->params = stream_params;
	check_not_equal(0, err, "Did not fail with NULL stream params");

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	subgroup_params->codec_cfg = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	subgroup_params->codec_cfg = codec_cfg;
	check_not_equal(0, err, "Did not fail with NULL codec_cfg");

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_data_len(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	codec_cfg->data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with codec_cfg->data_len %zu", codec_cfg->data_len);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_meta_len(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	codec_cfg->meta_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with codec_cfg->meta_len %zu", codec_cfg->meta_len);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_cid(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	codec_cfg->id = 0x06;
	codec_cfg->cid = 0x01; /* Shall be 0 if id == 0x06 (LC3)*/
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with codec_cfg->cid %u", codec_cfg->cid);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_vid(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_audio_codec_cfg *codec_cfg = subgroup_params->codec_cfg;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	codec_cfg->id = 0x06;
	codec_cfg->vid = 0x01; /* Shall be 0 if id == 0x06 (LC3)*/
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with codec_cfg->vid %u", codec_cfg->vid);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_stream_params_stream_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	struct bt_bap_stream *stream = stream_params->stream;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	stream_params->stream = NULL;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	stream_params->stream = stream;
	check_not_equal(0, err, "Did not fail with NULL stream_params->stream");

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_stream_params_data_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	uint8_t *data = stream_params->data;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	stream_params->data = NULL;
	stream_params->data_len = 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	/* Restore the params for the cleanup after function */
	stream_params->data = data;
	check_not_equal(
		0, err,
		"Did not fail with NULL stream_params->data and stream_params_>data_len %zu",
		stream_params->data_len);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_stream_params_data_len(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source_subgroup_param *subgroup_params = &param->params[0];
	struct bt_bap_broadcast_source_stream_param *stream_params = &subgroup_params->params[0];
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	stream_params->data_len = CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE + 1;
	err = bt_bap_broadcast_source_reconfig(fixture->source, param);
	check_not_equal(0, err, "Did not fail with stream_params_>data_len %zu",
			  stream_params->data_len);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_reconfigure_inval_state(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *param = fixture->param;
	struct bt_bap_broadcast_source *source;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);
	source = fixture->source;

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;

	err = bt_bap_broadcast_source_reconfig(source, param);
	check_not_equal(0, err, "Did not fail with deleted broadcast source");
}

static void test_broadcast_source_stop_inval_source_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	check_equal(0, err, "Unable to start broadcast source: err %d", err);

	err = bt_bap_broadcast_source_stop(NULL);
	check_not_equal(0, err, "Did not fail with null source");
}

static void test_broadcast_source_stop_inval_state(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_le_ext_adv ext_adv = {0};
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_start(fixture->source, &ext_adv);
	check_equal(0, err, "Unable to start broadcast source: err %d", err);

	err = bt_bap_broadcast_source_stop(fixture->source);
	check_equal(0, err, "Unable to stop broadcast source: err %d", err);

	err = bt_bap_broadcast_source_stop(NULL);
	check_not_equal(0, err, "Did not fail with stopping already stopped source");
}


static void test_broadcast_source_delete_inval_source_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_bap_broadcast_source_delete(NULL);
	check_not_equal(0, err, "Did not fail with null source");
}

static void test_broadcast_source_delete_inval_double_start(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source *source;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to start broadcast source: err %d", err);

	source = fixture->source;
	/* Set to NULL to avoid deleting it in bap_broadcast_source_test_suite_after */
	fixture->source = NULL;

	err = bt_bap_broadcast_source_delete(source);
	check_not_equal(0, err, "Did not fail with deleting already deleting source");
}

static void test_broadcast_source_get_base_single_bis(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	const uint8_t expected_base[] = {
		0x51, 0x18,                   /* uuid */
		0x40, 0x9C, 0x00,             /* pd */
		0x01,                         /* subgroup count */
		0x01,                         /* bis count */
		0x06, 0x00, 0x00, 0x00, 0x00, /* LC3 codec_id*/
		0x10,                         /* cc length */
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00, /* cc */
		0x04,                                           /* meta length */
		0x03, 0x02, 0x01, 0x00,                         /* meta */
		0x01,                                           /* bis index */
		0x06,                                           /* bis cc length */
		0x05, 0x03, 0x03, 0x00, 0x00, 0x00              /* bis cc length */
	};

	BT_BUF_SIMPLE_DEFINE(base_buf, 64);

	/* Make the create param simpler for verification */
	create_param->params_count = 1U;
	create_param->params[0].params_count = 1U;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, &base_buf);
	check_equal(0, err, "Unable to get broadcast source BASE: err %d", err);

	check_equal(sizeof(expected_base), base_buf.len, "Incorrect base_buf.len %u, expected %u",
		      base_buf.len, sizeof(expected_base));

	/* Use memcmp to print the buffers if they are not identical as the equality helper does not
	 * do that
	 */
	if (memcmp(expected_base, base_buf.data, base_buf.len) != 0) {
		for (size_t i = 0U; i < base_buf.len; i++) {
			printf("[%zu]: 0x%02X %s 0x%02X\n", i, expected_base[i],
			       expected_base[i] == base_buf.data[i] ? "==" : "!=",
			       base_buf.data[i]);
		}

		assert_memory_equal(expected_base, base_buf.data, base_buf.len);
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_get_base(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	const uint8_t expected_base[] = {
		0x51, 0x18,                   /* uuid */
		0x40, 0x9C, 0x00,             /* pd */
		0x02,                         /* subgroup count */
		0x01,                         /* Subgroup 1: bis count */
		0x06, 0x00, 0x00, 0x00, 0x00, /* LC3 codec_id*/
		0x10,                         /* cc length */
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00, /* cc */
		0x04,                                           /* meta length */
		0x03, 0x02, 0x01, 0x00,                         /* meta */
		0x01,                                           /* bis index */
		0x06,                                           /* bis cc length */
		0x05, 0x03, 0x03, 0x00, 0x00, 0x00,             /* bis cc length */
		0x01,                                           /* Subgroup 1: bis count */
		0x06, 0x00, 0x00, 0x00, 0x00,                   /* LC3 codec_id*/
		0x10,                                           /* cc length */
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00, /* cc */
		0x04,                                           /* meta length */
		0x03, 0x02, 0x01, 0x00,                         /* meta */
		0x02,                                           /* bis index */
		0x06,                                           /* bis cc length */
		0x05, 0x03, 0x03, 0x00, 0x00, 0x00              /* bis cc length */
	};

	BT_BUF_SIMPLE_DEFINE(base_buf, 128);

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, &base_buf);
	check_equal(0, err, "Unable to get broadcast source BASE: err %d", err);

	check_equal(sizeof(expected_base), base_buf.len, "Incorrect base_buf.len %u, expected %u",
		      base_buf.len, sizeof(expected_base));

	/* Use memcmp to print the buffers if they are not identical as the equality helper does not
	 * do that
	 */
	if (memcmp(expected_base, base_buf.data, base_buf.len) != 0) {
		for (size_t i = 0U; i < base_buf.len; i++) {
			printf("[%zu]: 0x%02X %s 0x%02X\n", i, expected_base[i],
			       expected_base[i] == base_buf.data[i] ? "==" : "!=",
			       base_buf.data[i]);
		}

		assert_memory_equal(expected_base, base_buf.data, base_buf.len);
	}

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_get_base_inval_source_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	int err;

	BT_BUF_SIMPLE_DEFINE(base_buf, 64);

	err = bt_bap_broadcast_source_get_base(NULL, &base_buf);
	check_not_equal(0, err, "Did not fail with null source");
}

static void test_broadcast_source_get_base_inval_base_buf_null(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, NULL);
	check_not_equal(0, err, "Did not fail with null BASE buffer");

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static void test_broadcast_source_get_base_inval_state(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	struct bt_bap_broadcast_source *source;
	int err;

	BT_BUF_SIMPLE_DEFINE(base_buf, 64);

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	source = fixture->source;

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;

	err = bt_bap_broadcast_source_get_base(source, &base_buf);
	check_not_equal(0, err, "Did not fail with deleted broadcast source");
}

/** This tests that providing a buffer too small for _any_ BASE fails correctly */
static void test_broadcast_source_get_base_inval_very_small_buf(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	BT_BUF_SIMPLE_DEFINE(base_buf, 15); /* Too small to hold any BASE */

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, &base_buf);
	check_not_equal(0, err, "Did not fail with too small base_buf (%u)", base_buf.size);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

/** This tests that providing a buffer too small for the BASE we want to setup fails correctly */
static void test_broadcast_source_get_base_inval_small_buf(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	struct bt_bap_broadcast_source_param *create_param = fixture->param;
	int err;

	/* Can hold a base, but not large enough for this configuration */
	BT_BUF_SIMPLE_DEFINE(base_buf, 64);

	printf("Creating broadcast source with %zu subgroups with %zu streams\n",
	       create_param->params_count, fixture->stream_cnt);

	err = bt_bap_broadcast_source_create(create_param, &fixture->source);
	check_equal(0, err, "Unable to create broadcast source: err %d", err);

	err = bt_bap_broadcast_source_get_base(fixture->source, &base_buf);
	check_not_equal(0, err, "Did not fail with too small base_buf (%u)", base_buf.size);

	err = bt_bap_broadcast_source_delete(fixture->source);
	check_equal(0, err, "Unable to delete broadcast source: err %d", err);
	fixture->source = NULL;
}

static bool bap_broadcast_source_foreach_stream_cb(struct bt_bap_stream *stream, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;

	return true;
}

static void test_broadcast_source_foreach_stream(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	size_t cnt = 0U;
	int err;

	err = bt_bap_broadcast_source_create(fixture->param, &fixture->source);
	check_equal(err, 0, "Unexpected return value: %d", err);

	err = bt_bap_broadcast_source_foreach_stream(fixture->source,
						     bap_broadcast_source_foreach_stream_cb, &cnt);
	check_equal(err, 0, "Unexpected return value: %d", err);
	check_equal(cnt, CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT, "Got %zu, expected %d", cnt,
		      CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
}

static bool bap_broadcast_source_foreach_stream_return_early_cb(struct bt_bap_stream *stream,
								void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;

	return false;
}

static void test_broadcast_source_foreach_stream_return_early(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	size_t cnt = 0U;
	int err;

	err = bt_bap_broadcast_source_create(fixture->param, &fixture->source);
	check_equal(err, 0, "Unexpected return value: %d", err);

	err = bt_bap_broadcast_source_foreach_stream(
		fixture->source, bap_broadcast_source_foreach_stream_return_early_cb, &cnt);
	check_equal(err, -ECANCELED, "Unexpected return value: %d", err);
	check_equal(cnt, 1U, "Got %zu, expected %u", cnt, 1U);
}

static void test_broadcast_source_foreach_stream_inval_null_source(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	size_t cnt = 0U;
	int err;

	err = bt_bap_broadcast_source_create(fixture->param, &fixture->source);
	check_equal(err, 0, "Unexpected return value: %d", err);

	err = bt_bap_broadcast_source_foreach_stream(NULL, bap_broadcast_source_foreach_stream_cb,
						     &cnt);
	check_equal(err, -EINVAL, "Unexpected return value: %d", err);
	check_equal(cnt, 0U, "Got %zu, expected %u", cnt, 0U);
}

static void test_broadcast_source_foreach_stream_inval_null_func(void **state)
{
	struct bap_broadcast_source_test_suite_fixture *fixture = get_fixture(state);
	size_t cnt = 0U;
	int err;

	err = bt_bap_broadcast_source_create(fixture->param, &fixture->source);
	check_equal(err, 0, "Unexpected return value: %d", err);

	err = bt_bap_broadcast_source_foreach_stream(fixture->source, NULL, &cnt);
	check_equal(err, -EINVAL, "Unexpected return value: %d", err);
	check_equal(cnt, 0U, "Got %zu, expected %u", cnt, 0U);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		BAP_TEST(test_broadcast_source_create_delete),
		BAP_TEST(test_broadcast_source_create_start_send_stop_delete),
		BAP_TEST(test_broadcast_source_create_inval_param_null),
		BAP_TEST(test_broadcast_source_create_inval_source_null),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_count_0),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_count_above_max),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_null),
		BAP_TEST(test_broadcast_source_create_inval_qos_null),
		BAP_TEST(test_broadcast_source_create_inval_packing),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_params_count_0),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_params_count_above_max),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_stream_params_null),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_codec_cfg_null),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_codec_cfg_data_len),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_codec_cfg_meta_len),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_codec_cfg_cid),
		BAP_TEST(test_broadcast_source_create_inval_subgroup_params_codec_cfg_vid),
		BAP_TEST(test_broadcast_source_create_inval_stream_params_stream_null),
		BAP_TEST(test_broadcast_source_create_inval_stream_params_data_null),
		BAP_TEST(test_broadcast_source_create_inval_stream_params_data_len),
		BAP_TEST(test_broadcast_source_start_inval_source_null),
		BAP_TEST(test_broadcast_source_start_inval_ext_adv_null),
		BAP_TEST(test_broadcast_source_start_inval_double_start),
		BAP_TEST(test_broadcast_source_reconfigure_single_subgroup),
		BAP_TEST(test_broadcast_source_reconfigure_all),
		BAP_TEST(test_broadcast_source_reconfigure_inval_param_null),
		BAP_TEST(test_broadcast_source_reconfigure_inval_source_null),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_count_0),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_count_above_max),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_null),
		BAP_TEST(test_broadcast_source_reconfigure_inval_qos_null),
		BAP_TEST(test_broadcast_source_reconfigure_inval_packing),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_params_count_0),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_params_count_above_max),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_stream_params_null),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_null),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_data_len),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_meta_len),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_cid),
		BAP_TEST(test_broadcast_source_reconfigure_inval_subgroup_params_codec_cfg_vid),
		BAP_TEST(test_broadcast_source_reconfigure_inval_stream_params_stream_null),
		BAP_TEST(test_broadcast_source_reconfigure_inval_stream_params_data_null),
		BAP_TEST(test_broadcast_source_reconfigure_inval_stream_params_data_len),
		BAP_TEST(test_broadcast_source_reconfigure_inval_state),
		BAP_TEST(test_broadcast_source_stop_inval_source_null),
		BAP_TEST(test_broadcast_source_stop_inval_state),
		BAP_TEST(test_broadcast_source_delete_inval_source_null),
		BAP_TEST(test_broadcast_source_delete_inval_double_start),
		BAP_TEST(test_broadcast_source_get_base_single_bis),
		BAP_TEST(test_broadcast_source_get_base),
		BAP_TEST(test_broadcast_source_get_base_inval_source_null),
		BAP_TEST(test_broadcast_source_get_base_inval_base_buf_null),
		BAP_TEST(test_broadcast_source_get_base_inval_state),
		BAP_TEST(test_broadcast_source_get_base_inval_very_small_buf),
		BAP_TEST(test_broadcast_source_get_base_inval_small_buf),
		BAP_TEST(test_broadcast_source_foreach_stream),
		BAP_TEST(test_broadcast_source_foreach_stream_return_early),
		BAP_TEST(test_broadcast_source_foreach_stream_inval_null_source),
		BAP_TEST(test_broadcast_source_foreach_stream_inval_null_func),
		CALLBACK_TEST(test_broadcast_source_register_cb),
		CALLBACK_TEST(test_broadcast_source_register_cb_inval_param_null),
		CALLBACK_TEST(test_broadcast_source_register_cb_inval_double_register),
		CALLBACK_TEST(test_broadcast_source_unregister_cb),
		CALLBACK_TEST(test_broadcast_source_unregister_cb_inval_param_null),
		CALLBACK_TEST(test_broadcast_source_unregister_cb_inval_double_unregister),
	};

	return cmocka_run_group_tests(tests, test_group_setup, test_group_teardown);
}
