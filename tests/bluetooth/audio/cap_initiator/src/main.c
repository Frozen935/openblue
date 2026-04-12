/* main.c - Application main entry point */

/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include <autoconf.h>
#include <bluetooth/audio/cap.h>
#include <bluetooth/hci_types.h>
#include <base/utils.h>

#include "cap_initiator.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

struct cap_initiator_test_suite_fixture {
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
};

static struct cap_initiator_test_suite_fixture *group_fixture;

static void cap_initiator_test_suite_fixture_init(struct cap_initiator_test_suite_fixture *fixture)
{
	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i], i);
	}
}

static int cap_initiator_test_suite_setup(void **state)
{
	group_fixture = calloc(1, sizeof(*group_fixture));
	assert_non_null(group_fixture);
	(void)state;

	return 0;
}

static int cap_initiator_test_suite_before(void **state)
{
	assert_non_null(group_fixture);
	memset(group_fixture, 0, sizeof(*group_fixture));
	cap_initiator_test_suite_fixture_init(group_fixture);
	test_mocks_init();
	*state = group_fixture;

	return 0;
}

static int cap_initiator_test_suite_after(void **state)
{
	struct cap_initiator_test_suite_fixture *fixture = *state;

	bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	test_mocks_cleanup();

	return 0;
}

static int cap_initiator_test_suite_teardown(void **state)
{
	free(group_fixture);
	group_fixture = NULL;
	(void)state;

	return 0;
}

static struct cap_initiator_test_suite_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

#define fixture get_fixture(state)

static void test_initiator_register_cb(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);
}

static void test_initiator_register_cb_inval_param_null(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_register_cb(NULL);
	assert_int_equal(-EINVAL, err);
}

static void test_initiator_register_cb_inval_double_register(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(-EALREADY, err);
}

static void test_initiator_unregister_cb(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);

	err = bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);
}

static void test_initiator_unregister_cb_inval_param_null(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_unregister_cb(NULL);
	assert_int_equal(-EINVAL, err);
}

static void test_initiator_unregister_cb_inval_double_unregister(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);

	err = bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);

	err = bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);
	assert_int_equal(-EINVAL, err);
}

static void test_initiator_discover(void **state)
{
	int err;

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		err = bt_cap_initiator_unicast_discover(&fixture->conns[i]);
		assert_int_equal(0, err);
	}

	expect_call_count("bt_cap_initiator_cb.discovery_complete", ARRAY_SIZE(fixture->conns),
				  mock_cap_initiator_unicast_discovery_complete_cb_fake.call_count);
}

static void test_initiator_discover_inval_param_null(void **state)
{
	int err;

	(void)state;
	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	assert_int_equal(0, err);

	err = bt_cap_initiator_unicast_discover(NULL);
	assert_int_equal(-EINVAL, err);
}

int cap_initiator_main_suite_run(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_initiator_register_cb,
					       cap_initiator_test_suite_before,
					       cap_initiator_test_suite_after),
		cmocka_unit_test_setup_teardown(test_initiator_register_cb_inval_param_null,
					       cap_initiator_test_suite_before,
					       cap_initiator_test_suite_after),
		cmocka_unit_test_setup_teardown(test_initiator_register_cb_inval_double_register,
					       cap_initiator_test_suite_before,
					       cap_initiator_test_suite_after),
		cmocka_unit_test_setup_teardown(test_initiator_unregister_cb,
					       cap_initiator_test_suite_before,
					       cap_initiator_test_suite_after),
		cmocka_unit_test_setup_teardown(test_initiator_unregister_cb_inval_param_null,
					       cap_initiator_test_suite_before,
					       cap_initiator_test_suite_after),
		cmocka_unit_test_setup_teardown(test_initiator_unregister_cb_inval_double_unregister,
					       cap_initiator_test_suite_before,
					       cap_initiator_test_suite_after),
		cmocka_unit_test_setup_teardown(test_initiator_discover,
					       cap_initiator_test_suite_before,
					       cap_initiator_test_suite_after),
		cmocka_unit_test_setup_teardown(test_initiator_discover_inval_param_null,
					       cap_initiator_test_suite_before,
					       cap_initiator_test_suite_after),
	};

	return cmocka_run_group_tests_name("cap_initiator_test_suite", tests,
					  cap_initiator_test_suite_setup,
					  cap_initiator_test_suite_teardown);
}

int main(void)
{
	int err = 0;

	err |= cap_initiator_main_suite_run();
	err |= cap_initiator_unicast_group_suite_run();
	err |= cap_initiator_unicast_start_suite_run();
	err |= cap_initiator_unicast_stop_suite_run();

	return err;
}
