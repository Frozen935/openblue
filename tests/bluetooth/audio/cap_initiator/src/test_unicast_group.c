/* test_unicast_group.c - unit test for unicast group functions */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include <autoconf.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/bap.h>
#include <bluetooth/audio/bap_lc3_preset.h>
#include <bluetooth/audio/cap.h>
#include <bluetooth/hci_types.h>
#include <bluetooth/iso.h>
#include <base/utils.h>

#include "audio/bap_endpoint.h"
#include "test_common.h"

struct cap_initiator_test_unicast_group_fixture {
	struct bt_cap_unicast_group_param *group_param;
	struct bt_cap_unicast_group *unicast_group;
	struct bt_bap_qos_cfg *qos_cfg;
};

static struct cap_initiator_test_unicast_group_fixture *group_fixture;

static int cap_initiator_test_unicast_group_setup(void **state)
{
	group_fixture = calloc(1, sizeof(*group_fixture));
	assert_non_null(group_fixture);
	(void)state;

	return 0;
}

static int cap_initiator_test_unicast_group_before(void **state)
{
	struct cap_initiator_test_unicast_group_fixture *fixture = group_fixture;
	struct bt_cap_unicast_group_stream_pair_param *pair_params;
	struct bt_cap_unicast_group_stream_param *stream_params;
	struct bt_cap_stream *cap_streams;
	size_t pair_cnt = 0U;
	size_t str_cnt = 0U;

	assert_non_null(fixture);
	memset(fixture, 0, sizeof(*fixture));
	test_mocks_init();

	fixture->group_param = calloc(sizeof(struct bt_cap_unicast_group_param), 1);
	assert_non_null(fixture->group_param);
	pair_params = calloc(sizeof(struct bt_cap_unicast_group_stream_pair_param),
			     DIV_ROUND_UP(CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT, 2U));
	assert_non_null(pair_params);
	stream_params = calloc(sizeof(struct bt_cap_unicast_group_stream_param),
			       CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);
	assert_non_null(stream_params);
	cap_streams = calloc(sizeof(struct bt_cap_stream),
			     CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);
	assert_non_null(cap_streams);
	fixture->qos_cfg = calloc(sizeof(struct bt_bap_qos_cfg), 1);
	assert_non_null(fixture->qos_cfg);

	*fixture->qos_cfg = BT_BAP_QOS_CFG_UNFRAMED(10000u, 40u, 2u, 10u, 40000u); /* 16_2_1 */

	while (str_cnt < CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		stream_params[str_cnt].stream = &cap_streams[str_cnt];
		stream_params[str_cnt].qos_cfg = fixture->qos_cfg;

		if (str_cnt & 1) {
			pair_params[pair_cnt].tx_param = &stream_params[str_cnt];
		} else {
			pair_params[pair_cnt].rx_param = &stream_params[str_cnt];
		}

		str_cnt++;
		pair_cnt = str_cnt / 2U;
	}

	fixture->group_param->packing = BT_ISO_PACKING_SEQUENTIAL;
	fixture->group_param->params_count = pair_cnt;
	fixture->group_param->params = pair_params;
	*state = fixture;

	return 0;
}

static int cap_initiator_test_unicast_group_after(void **state)
{
	struct cap_initiator_test_unicast_group_fixture *fixture = *state;
	struct bt_cap_unicast_group_param *group_param;

	/* In the case of a test failing, we delete the group so that subsequent tests won't fail */
	if (fixture->unicast_group != NULL) {
		bt_cap_unicast_group_delete(fixture->unicast_group);
	}

	group_param = fixture->group_param;

	free(group_param->params[0].rx_param->stream);
	free(group_param->params[0].rx_param);
	free(group_param->params);
	free(group_param);
	free(fixture->qos_cfg);
	test_mocks_cleanup();

	return 0;
}

static int cap_initiator_test_unicast_group_teardown(void **state)
{
	free(group_fixture);
	group_fixture = NULL;
	(void)state;

	return 0;
}

static struct cap_initiator_test_unicast_group_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

#define fixture get_fixture(state)

static void test_initiator_unicast_group_create(void **state)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	assert_int_equal(err, 0);
}

static void test_initiator_unicast_group_create_inval_null_param(void **state)
{
	int err = 0;

	(void)state;
	err = bt_cap_unicast_group_create(NULL, &fixture->unicast_group);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_create_inval_null_rx_stream(void **state)
{
	int err = 0;

	if (fixture->group_param->params[0].rx_param->stream == NULL) {
		skip();
	}
	fixture->group_param->params[0].rx_param->stream = NULL;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_create_inval_null_tx_stream(void **state)
{
	int err = 0;

	if (fixture->group_param->params[0].tx_param->stream == NULL) {
		skip();
	}
	fixture->group_param->params[0].tx_param->stream = NULL;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_create_inval_too_many_streams(void **state)
{
	int err = 0;

	(void)state;
	fixture->group_param->params_count = CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT + 1;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_reconfig(void **state)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_reconfig(fixture->unicast_group, fixture->group_param);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	assert_int_equal(err, 0);
}

static void test_initiator_unicast_group_reconfig_inval_null_group(void **state)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_reconfig(NULL, fixture->group_param);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_reconfig_inval_null_param(void **state)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_reconfig(fixture->unicast_group, NULL);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_add_streams(void **state)
{
	struct bt_cap_stream stream = {0};
	struct bt_cap_unicast_group_stream_param stream_param = {
		.stream = &stream,
		.qos_cfg = fixture->qos_cfg,
	};
	const struct bt_cap_unicast_group_stream_pair_param pair_param = {
		.rx_param = &stream_param,
	};
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_add_streams(fixture->unicast_group, &pair_param, 1);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	assert_int_equal(err, 0);
}

static void test_initiator_unicast_group_add_streams_inval_null_group(void **state)
{
	struct bt_cap_stream stream = {0};
	struct bt_cap_unicast_group_stream_param stream_param = {
		.stream = &stream,
		.qos_cfg = fixture->qos_cfg,
	};
	const struct bt_cap_unicast_group_stream_pair_param pair_param = {
		.rx_param = &stream_param,
	};
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_add_streams(NULL, &pair_param, 1);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_add_streams_inval_null_param(void **state)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_add_streams(fixture->unicast_group, NULL, 1);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_add_streams_inval_0_param(void **state)
{
	struct bt_cap_stream stream = {0};
	struct bt_cap_unicast_group_stream_param stream_param = {
		.stream = &stream,
		.qos_cfg = fixture->qos_cfg,
	};
	const struct bt_cap_unicast_group_stream_pair_param pair_param = {
		.rx_param = &stream_param,
	};
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_add_streams(fixture->unicast_group, &pair_param, 0);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_delete(void **state)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	assert_int_equal(err, 0);
	fixture->unicast_group = NULL;
}

static void test_initiator_unicast_group_delete_inval_null_group(void **state)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_delete(NULL);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_delete_inval_double_delete(void **state)
{
	int err = 0;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_delete(fixture->unicast_group);
	assert_int_equal(err, -EINVAL);
	fixture->unicast_group = NULL;
}

static bool unicast_group_foreach_stream_cb(struct bt_cap_stream *cap_stream, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;

	return true;
}

static void test_initiator_unicast_group_foreach_stream(void **state)
{
	size_t expect_cnt = 0U;
	size_t cnt = 0U;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_foreach_stream(fixture->unicast_group,
					  unicast_group_foreach_stream_cb, &cnt);
	assert_int_equal(err, 0);

	for (size_t i = 0; i < fixture->group_param->params_count; i++) {
		if (fixture->group_param->params[i].rx_param != NULL) {
			expect_cnt++;
		}

		if (fixture->group_param->params[i].tx_param != NULL) {
			expect_cnt++;
		}
	}

	assert_int_equal(cnt, expect_cnt);
}

static bool unicast_group_foreach_stream_return_early_cb(struct bt_cap_stream *stream,
							 void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;

	return false;
}

static void test_initiator_unicast_group_foreach_stream_return_early(void **state)
{
	size_t cnt = 0U;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_foreach_stream(
		fixture->unicast_group, unicast_group_foreach_stream_return_early_cb, &cnt);
	assert_int_equal(err, -ECANCELED);
	assert_int_equal(cnt, 1U);
}

static void test_initiator_unicast_group_foreach_stream_inval_null_group(void **state)
{
	size_t expect_cnt = 0U;
	size_t cnt = 0U;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_foreach_stream(NULL, unicast_group_foreach_stream_cb, &cnt);
	assert_int_equal(err, -EINVAL);

	assert_int_equal(cnt, expect_cnt);
}

static void test_initiator_unicast_group_foreach_stream_inval_null_func(void **state)
{
	size_t expect_cnt = 0U;
	size_t cnt = 0U;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_foreach_stream(fixture->unicast_group, NULL, &cnt);
	assert_int_equal(err, -EINVAL);

	assert_int_equal(cnt, expect_cnt);
}

static void test_initiator_unicast_group_get_info(void **state)
{
	struct bt_cap_unicast_group_info cap_info;
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_get_info(fixture->unicast_group, &cap_info);
	assert_int_equal(err, 0);

	assert_non_null(cap_info.unicast_group);
}

static void test_initiator_unicast_group_get_info_inval_null_group(void **state)
{
	struct bt_cap_unicast_group_info cap_info;
	int err;

	(void)state;
	err = bt_cap_unicast_group_get_info(NULL, &cap_info);
	assert_int_equal(err, -EINVAL);
}

static void test_initiator_unicast_group_get_info_inval_null_info(void **state)
{
	int err;

	err = bt_cap_unicast_group_create(fixture->group_param, &fixture->unicast_group);
	assert_int_equal(err, 0);

	err = bt_cap_unicast_group_get_info(fixture->unicast_group, NULL);
	assert_int_equal(err, -EINVAL);
}

int cap_initiator_unicast_group_suite_run(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_create,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_create_inval_null_param,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_create_inval_null_rx_stream,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_create_inval_null_tx_stream,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_create_inval_too_many_streams,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_reconfig,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_reconfig_inval_null_group,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_reconfig_inval_null_param,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_add_streams,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_add_streams_inval_null_group,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_add_streams_inval_null_param,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_add_streams_inval_0_param,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_delete,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_delete_inval_null_group,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_delete_inval_double_delete,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_foreach_stream,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_foreach_stream_return_early,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_foreach_stream_inval_null_group,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_foreach_stream_inval_null_func,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_get_info,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_get_info_inval_null_group,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
		cmocka_unit_test_setup_teardown(test_initiator_unicast_group_get_info_inval_null_info,
					       cap_initiator_test_unicast_group_before,
					       cap_initiator_test_unicast_group_after),
	};

	return cmocka_run_group_tests_name("cap_initiator_test_unicast_group", tests,
					  cap_initiator_test_unicast_group_setup,
					  cap_initiator_test_unicast_group_teardown);
}
