/* bluetooth.c - Bluetooth smoke test */

/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdbool.h>

#include <cmocka.h>

#include <errno.h>

#include <bt_stack_init.h>
#include <bluetooth/bluetooth.h>
#include <drivers/bluetooth.h>

#define EXPECTED_ERROR -ENOSYS

struct driver_data {
	bool ready;
};

static struct driver_data test_driver_data;

static int driver_open(const struct bt_hci_transport *transport, bt_hci_recv_t recv)
{
	(void)transport;
	(void)recv;

	/* Indicate that there is no real Bluetooth device */
	return EXPECTED_ERROR;
}

static int driver_send(const struct bt_hci_transport *transport, struct bt_buf *buf)
{
	(void)transport;
	(void)buf;

	return 0;
}

static bool driver_is_ready(const struct bt_hci_transport *transport)
{
	const struct driver_data *data = transport->user_data;

	return data->ready;
}

static const struct bt_hci_driver_api driver_api = {
	.open = driver_open,
	.send = driver_send,
};

static const struct bt_hci_transport test_transport = {
	.name = "test-hci",
	.bus = BT_HCI_BUS_VIRTUAL,
	.api = &driver_api,
	.user_data = &test_driver_data,
	.is_ready = driver_is_ready,
};

static void test_bluetooth_entry(void **state)
{
	int err;

	(void)state;

	err = bt_stack_init_once();
	assert_int_equal(err, 0);

	test_driver_data.ready = false;
	err = bt_hci_transport_register(&test_transport);
	assert_int_equal(err, 0);

	test_driver_data.ready = true;
	err = bt_enable(NULL);
	assert_int_equal(err, EXPECTED_ERROR);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_bluetooth_entry),
	};

	return cmocka_run_group_tests_name("bt_bluetooth", tests, NULL, NULL);
}
