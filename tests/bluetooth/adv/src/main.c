/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>

#include <cmocka.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include <osdep/os.h>
#include <utils/bt_utils.h>

#define DEVICE_NAME "Test Adv Data"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define TIMEOUT_MS 300000 /* 5 minutes */

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void test_adv_fast_ad_data_update(void **state)
{
	uint64_t deadline;
	int err;

	(void)state;

	printf("Starting Beacon Demo\n");

	err = bt_enable(NULL);
	assert_int_equal(err, 0);

	printf("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	assert_int_equal(err, 0);

	printf("Advertising started\n");

	deadline = os_time_get_ms() + TIMEOUT_MS;
	while (os_time_get_ms() < deadline) {
		err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		assert_int_equal(err, 0);
	}
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_adv_fast_ad_data_update),
	};

	return cmocka_run_group_tests_name("bt_adv", tests, NULL, NULL);
}
