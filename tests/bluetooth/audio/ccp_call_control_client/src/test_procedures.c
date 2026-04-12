/* test_procedures.c - Testing of CCP procedures  */

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

#include <base/utils.h>
#include <bluetooth/audio/ccp.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci_types.h>
#include <utils/bt_utils.h>

#include "test_common.h"

struct ccp_call_control_client_procedures_test_suite_fixture {
	struct bt_ccp_call_control_client_cb client_cbs;
	struct bt_ccp_call_control_client *client;
	struct bt_conn conn;

	/* Callback values */
	struct bt_ccp_call_control_client_bearer
		*bearers[CONFIG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT];
	char bearer_name[CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH];
};

static struct ccp_call_control_client_procedures_test_suite_fixture *group_fixture;

static struct ccp_call_control_client_procedures_test_suite_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

static void discover_cb(struct bt_ccp_call_control_client *client, int err,
			struct bt_ccp_call_control_client_bearers *bearers, void *user_data)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = user_data;

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

static void bearer_provider_name_cb(struct bt_ccp_call_control_client_bearer *bearer, int err,
				    const char *name, void *user_data)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = user_data;

	assert_non_null(bearer);
	assert_int_equal(err, 0);
	assert_non_null(name);
	assert_non_null(user_data);
	assert_true(strlen(name) < CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH);
	assert_true(strlen(fixture->bearer_name) == 0U); /* expect only a single call */
	utf8_lcpy(fixture->bearer_name, name, CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH);
}

static int ccp_call_control_client_procedures_test_suite_group_setup(void **state)
{
	(void)state;

	group_fixture = calloc(1, sizeof(*group_fixture));
	if (group_fixture == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static int ccp_call_control_client_procedures_test_suite_before(void **state)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = group_fixture;
	int err;

	memset(fixture, 0, sizeof(*fixture));
	test_conn_init(&fixture->conn);

	fixture->client_cbs.discover = discover_cb;
	fixture->client_cbs.bearer_provider_name = bearer_provider_name_cb;
	fixture->client_cbs.user_data = fixture;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	assert_int_equal(0, err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, &fixture->client);
	assert_int_equal(0, err);

	assert_non_null(fixture->bearers[0]);
	*state = fixture;

	return 0;
}

static int ccp_call_control_client_procedures_test_suite_after(void **state)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = get_fixture(state);

	(void)bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	mock_bt_conn_disconnected(&fixture->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	fixture->client = NULL;

	return 0;
}

static int ccp_call_control_client_procedures_test_suite_group_teardown(void **state)
{
	(void)state;

	free(group_fixture);
	group_fixture = NULL;

	return 0;
}

static void test_ccp_call_control_client_read_bearer_provider_name(void **state)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_read_bearer_provider_name(fixture->bearers[0]);
	assert_int_equal(err, 0);

	assert_true(strlen(fixture->bearer_name) > 0U);
}

static void test_ccp_call_control_client_read_bearer_provider_name_inval_null_bearer(void **state)
{
	(void)get_fixture(state);
	int err;

	err = bt_ccp_call_control_client_read_bearer_provider_name(NULL);
	assert_int_equal(err, -EINVAL);
}

static void test_ccp_call_control_client_read_bearer_provider_name_inval_not_discovered(void **state)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = get_fixture(state);
	int err;

	/* Fake disconnection to clear the discovered value for the bearers*/
	mock_bt_conn_disconnected(&fixture->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	/* Mark as connected again but without discovering */
	test_conn_init(&fixture->conn);

	err = bt_ccp_call_control_client_read_bearer_provider_name(fixture->bearers[0]);
	assert_int_equal(err, -EFAULT);
}

static void test_ccp_call_control_client_read_bearer_provider_name_inval_bearer(void **state)
{
	(void)get_fixture(state);
	struct bt_ccp_call_control_client_bearer *invalid_bearer =
		(struct bt_ccp_call_control_client_bearer *)0xdeadbeefU;
	int err;

	err = bt_ccp_call_control_client_read_bearer_provider_name(invalid_bearer);
	assert_int_equal(err, -EEXIST);
}

int ccp_call_control_client_run_procedure_tests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_ccp_call_control_client_read_bearer_provider_name,
						      ccp_call_control_client_procedures_test_suite_before,
						      ccp_call_control_client_procedures_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_read_bearer_provider_name_inval_null_bearer,
			ccp_call_control_client_procedures_test_suite_before,
			ccp_call_control_client_procedures_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_read_bearer_provider_name_inval_not_discovered,
			ccp_call_control_client_procedures_test_suite_before,
			ccp_call_control_client_procedures_test_suite_after),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_client_read_bearer_provider_name_inval_bearer,
			ccp_call_control_client_procedures_test_suite_before,
			ccp_call_control_client_procedures_test_suite_after),
	};

	return cmocka_run_group_tests_name("ccp_call_control_client_procedures_test_suite", tests,
					 ccp_call_control_client_procedures_test_suite_group_setup,
					 ccp_call_control_client_procedures_test_suite_group_teardown);
}
