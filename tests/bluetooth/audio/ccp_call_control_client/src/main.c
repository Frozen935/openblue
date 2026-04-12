/* main.c - Application main entry point */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include <bluetooth/audio/ccp.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci_types.h>
#include <utils/bt_utils.h>

#include "test_common.h"

struct ccp_call_control_client_test_suite_fixture {
	struct bt_ccp_call_control_client_cb client_cbs;
	struct bt_ccp_call_control_client *client;
	struct bt_conn conn;

	/* Callback values */
	/** Need 1 additional bearer than the max to trigger some corner cases */
	struct bt_ccp_call_control_client_bearer
		*bearers[CONFIG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT + 1];
};

static struct ccp_call_control_client_test_suite_fixture *group_fixture;

int ccp_call_control_client_run_procedure_tests(void);

static struct ccp_call_control_client_test_suite_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

static void discover_cb(struct bt_ccp_call_control_client *client, int err,
			struct bt_ccp_call_control_client_bearers *bearers, void *user_data)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = user_data;

	assert_non_null(client);
	assert_int_equal(err, 0);
	assert_non_null(bearers);
	assert_non_null(user_data);
	assert_null(fixture->bearers[0]); /* expect only a single call */

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	assert_non_null(bearers->gtbs_bearer);
	fixture->bearers[0] = bearers->gtbs_bearer;
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	assert_int_equal(CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES, bearers->tbs_count);
	assert_non_null(bearers->tbs_bearers);
	for (size_t i = 0U; i < bearers->tbs_count; i++) {
		assert_non_null(bearers->tbs_bearers[i]);
		fixture->bearers[i + IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS)] =
			bearers->tbs_bearers[i];
	}
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
}

static int ccp_call_control_client_test_suite_group_setup(void **state)
{
	(void)state;

	group_fixture = calloc(1, sizeof(*group_fixture));
	if (group_fixture == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static int ccp_call_control_client_test_suite_before(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = group_fixture;

	memset(fixture, 0, sizeof(*fixture));
	test_conn_init(&fixture->conn);

	fixture->client_cbs.discover = discover_cb;
	fixture->client_cbs.user_data = fixture;
	*state = fixture;

	return 0;
}

static int ccp_call_control_client_test_suite_after(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);

	(void)bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	mock_bt_conn_disconnected(&fixture->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	fixture->client = NULL;

	return 0;
}

static int ccp_call_control_client_test_suite_group_teardown(void **state)
{
	(void)state;

	free(group_fixture);
	group_fixture = NULL;

	return 0;
}

static void test_ccp_call_control_client_register_cb(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);
}

static void test_ccp_call_control_client_register_cb_inval_param_null(void **state)
{
	(void)get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_register_cb(NULL);
	assert_int_equal(-EINVAL, err);
}

static void test_ccp_call_control_client_register_cb_inval_double_register(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(-EEXIST, err);
}

static void test_ccp_call_control_client_unregister_cb(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	assert_int_equal(0, err);
}

static void test_ccp_call_control_client_unregister_cb_inval_param_null(void **state)
{
	(void)get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_unregister_cb(NULL);
	assert_int_equal(-EINVAL, err);
}

static void test_ccp_call_control_client_unregister_cb_inval_double_unregister(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	assert_int_equal(-EALREADY, err);
}

static void test_ccp_call_control_client_discover(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, &fixture->client);
	assert_int_equal(0, err);
}

static void test_ccp_call_control_client_discover_inval_param_null_conn(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_discover(NULL, &fixture->client);
	assert_int_equal(-EINVAL, err);
}

static void test_ccp_call_control_client_discover_inval_param_null_client(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, NULL);
	assert_int_equal(-EINVAL, err);
}

static void test_ccp_call_control_client_get_bearers(void **state)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = get_fixture(state);
	struct bt_ccp_call_control_client_bearers bearers;
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, &fixture->client);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_get_bearers(fixture->client, &bearers);
	assert_int_equal(0, err);

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	assert_non_null(bearers.gtbs_bearer);
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	assert_int_equal(CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES, bearers.tbs_count);
	assert_non_null(bearers.tbs_bearers);
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
}

static int ccp_call_control_client_run_main_suite(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_ccp_call_control_client_register_cb,
						      ccp_call_control_client_test_suite_before,
						      ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_register_cb_inval_param_null,
			ccp_call_control_client_test_suite_before,
			ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_register_cb_inval_double_register,
			ccp_call_control_client_test_suite_before,
			ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(test_ccp_call_control_client_unregister_cb,
						      ccp_call_control_client_test_suite_before,
						      ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_unregister_cb_inval_param_null,
			ccp_call_control_client_test_suite_before,
			ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_unregister_cb_inval_double_unregister,
			ccp_call_control_client_test_suite_before,
			ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(test_ccp_call_control_client_discover,
						      ccp_call_control_client_test_suite_before,
						      ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_discover_inval_param_null_conn,
			ccp_call_control_client_test_suite_before,
			ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_discover_inval_param_null_client,
			ccp_call_control_client_test_suite_before,
			ccp_call_control_client_test_suite_after),
		cmocka_unit_test_setup_teardown(test_ccp_call_control_client_get_bearers,
						      ccp_call_control_client_test_suite_before,
						      ccp_call_control_client_test_suite_after),
	};

	return cmocka_run_group_tests_name("ccp_call_control_client_test_suite", tests,
					 ccp_call_control_client_test_suite_group_setup,
					 ccp_call_control_client_test_suite_group_teardown);
}

int main(void)
{
	int ret = 0;

	ret |= ccp_call_control_client_run_main_suite();
	ret |= ccp_call_control_client_run_procedure_tests();

	return ret;
}
