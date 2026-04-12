/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Demant A/S
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/autoconf.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util_macro.h>

#include "bap_unicast_server.h"
#include "bap_stream.h"

#include "test_common.h"

static void ascs_register_test_suite_after(void *f)
{
	/* Attempt to clean up failing tests */
	(void)bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);

	/* Sleep to trigger any pending state changes */
	k_sleep(K_SECONDS(1));

	(void)bt_bap_unicast_server_unregister();
}



static int ascs_register_test_suite_case_setup(void **state)
{
	void *fixture_local = NULL;

	test_mocks_init();

	*state = fixture_local;

	return 0;
}

static int ascs_register_test_suite_case_teardown(void **state)
{
	void *fixture_local = state != NULL ? *state : NULL;


	ascs_register_test_suite_after(fixture_local);

	test_mocks_cleanup();

	if (state != NULL) {
		*state = NULL;
	}

	return 0;
}

static void test_cb_register_without_ascs_registered(void **state)
{
	int err;

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, -ENOTSUP);
}

static void test_ascs_register_with_null_param(void **state)
{
	int err;

	err = bt_bap_unicast_server_register(NULL);
	assert_int_equal(err, -EINVAL);
}

static void test_ascs_register_twice(void **state)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT,
	};

	/* Setup already registered once, so calling once here should be sufficient */
	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, 0);

	/* Setup already registered once, so calling once here should be sufficient */
	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, -EALREADY);

	err = bt_bap_unicast_server_unregister();
	assert_int_equal(err, 0);
}

static void test_ascs_register_too_many_sinks(void **state)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT + 1,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT,
	};

	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, -EINVAL);
}

static void test_ascs_register_too_many_sources(void **state)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT + 1,
	};

	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, -EINVAL);
}

static void test_ascs_register_zero_ases(void **state)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {0, 0};

	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, -EINVAL);
}

static void test_ascs_register_fewer_than_max_ases(void **state)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT > 0 ? CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT - 1 : 0,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT > 0 ? CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT - 1 : 0,
	};

	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, 0);
}

static void test_ascs_unregister_without_register(void **state)
{
	int err;

	err = bt_bap_unicast_server_unregister();
	assert_int_equal(err, -EALREADY);
}

static void test_ascs_unregister_with_cbs_registered(void **state)
{
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT,
	};
	int err;

	err = bt_bap_unicast_server_register(&param);
	assert_int_equal(err, 0);

	err = bt_bap_unicast_server_register_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	/* Not valid to unregister while callbacks are still registered */
	err = bt_bap_unicast_server_unregister();
	assert_int_equal(err, -EAGAIN);

	err = bt_bap_unicast_server_unregister_cb(&mock_bap_unicast_server_cb);
	assert_int_equal(err, 0);

	err = bt_bap_unicast_server_unregister();
	assert_int_equal(err, 0);
}

static int run_ascs_register_test_suite(void)
{
	const struct CMUnitTest ascs_register_test_suite_tests[] = {
		cmocka_unit_test_setup_teardown(test_cb_register_without_ascs_registered, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ascs_register_with_null_param, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ascs_register_twice, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ascs_register_too_many_sinks, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ascs_register_too_many_sources, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ascs_register_zero_ases, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ascs_register_fewer_than_max_ases, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ascs_unregister_without_register, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
		cmocka_unit_test_setup_teardown(test_ascs_unregister_with_cbs_registered, ascs_register_test_suite_case_setup, ascs_register_test_suite_case_teardown),
	};

	return cmocka_run_group_tests_name("ascs_register_test_suite", ascs_register_test_suite_tests, NULL, NULL);
}

int run_test_ase_register_tests(void)
{
	int result = 0;

	result |= run_ascs_register_test_suite();

	return result;
}

