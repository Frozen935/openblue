/* test_callback_register.c - Test bt_bap_broadcast_source_register and unregister */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <bluetooth/audio/bap.h>

#define check_equal(expected, actual, ...) assert_int_equal((expected), (actual))

extern struct bt_bap_broadcast_source_cb mock_bap_broadcast_source_cb;
extern void mock_bap_broadcast_source_init(void);

int callback_test_case_setup(void **state)
{
	(void)state;
	mock_bap_broadcast_source_init();
	return 0;
}

int callback_test_case_teardown(void **state)
{
	(void)state;
	(void)bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
	return 0;
}
void test_broadcast_source_register_cb(void **state)
{
	(void)state;
	int err;

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	check_equal(0, err, "Unexpected return value %d", err);
}

void test_broadcast_source_register_cb_inval_param_null(void **state)
{
	(void)state;
	int err;

	err = bt_bap_broadcast_source_register_cb(NULL);
	check_equal(err, -EINVAL, "Unexpected return value %d", err);
}

void test_broadcast_source_register_cb_inval_double_register(void **state)
{
	(void)state;
	int err;

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	check_equal(err, 0, "Unexpected return value %d", err);

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	check_equal(err, -EEXIST, "Unexpected return value %d", err);
}

void test_broadcast_source_unregister_cb(void **state)
{
	(void)state;
	int err;

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	check_equal(err, 0, "Unexpected return value %d", err);

	err = bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
	check_equal(err, 0, "Unexpected return value %d", err);
}

void test_broadcast_source_unregister_cb_inval_param_null(void **state)
{
	(void)state;
	int err;

	err = bt_bap_broadcast_source_unregister_cb(NULL);
	check_equal(err, -EINVAL, "Unexpected return value %d", err);
}

void test_broadcast_source_unregister_cb_inval_double_unregister(void **state)
{
	(void)state;
	int err;

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	check_equal(err, 0, "Unexpected return value %d", err);

	err = bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
	check_equal(err, 0, "Unexpected return value %d", err);

	err = bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
	check_equal(err, -ENOENT, "Unexpected return value %d", err);
}
