/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <errno.h>

#include <cmocka.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			struct bt_l2cap_chan **chan)
{
	(void)conn;
	(void)server;
	(void)chan;

	return -ENOSYS;
}

static struct bt_l2cap_server test_server = {
	.accept		= l2cap_accept,
};

static struct bt_l2cap_server test_fixed_server = {
	.accept		= l2cap_accept,
	.psm		= 0x007f,
};

static struct bt_l2cap_server test_dyn_server = {
	.accept		= l2cap_accept,
	.psm		= 0x00ff,
};

static struct bt_l2cap_server test_inv_server = {
	.accept		= l2cap_accept,
	.psm		= 0xffff,
};

static void test_l2cap_register(void **state)
{
	(void)state;

	/* Attempt to register server with PSM auto allocation */
	assert_int_equal(bt_l2cap_server_register(&test_server), 0);

	/* Attempt to register server with fixed PSM */
	assert_int_equal(bt_l2cap_server_register(&test_fixed_server), 0);

	/* Attempt to register server with dynamic PSM */
	assert_int_equal(bt_l2cap_server_register(&test_dyn_server), 0);

	/* Attempt to register server with invalid PSM */
	assert_true(bt_l2cap_server_register(&test_inv_server) != 0);

	/* Attempt to re-register server with PSM auto allocation */
	assert_true(bt_l2cap_server_register(&test_server) != 0);

	/* Attempt to re-register server with fixed PSM */
	assert_true(bt_l2cap_server_register(&test_fixed_server) != 0);

	/* Attempt to re-register server with dynamic PSM */
	assert_true(bt_l2cap_server_register(&test_dyn_server) != 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_l2cap_register),
	};

	return cmocka_run_group_tests_name("bt_l2cap", tests, NULL, NULL);
}
