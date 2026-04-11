/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <sys/types.h>

#include <cmocka.h>

#include <bluetooth/audio/ccid.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>
#include <base/utils.h>

#define MAX_CCID_CNT 256

static void test_bt_ccid_alloc_value(void **state)
{
	(void)state;
	const int ret = bt_ccid_alloc_value();

	assert_true(ret >= 0 && ret <= UINT8_MAX);
}

static void test_bt_ccid_alloc_value_more_than_max(void **state)
{
	(void)state;

	/* Verify that we can allocate more than max CCID if they are not registered */
	for (uint16_t i = 0U; i < MAX_CCID_CNT * 2; i++) {
		const int ret = bt_ccid_alloc_value();

		assert_true(ret >= 0 && ret <= UINT8_MAX);
	}
}

static ssize_t read_ccid(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	const unsigned int ccid = POINTER_TO_UINT(attr->user_data);
	const uint8_t ccid_u8 = (uint8_t)ccid;

	assert_true(ccid <= BT_CCID_MAX);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ccid_u8, sizeof(ccid_u8));
}

#define CCID_DEFINE(_n, ...)                                                                       \
	BT_GATT_CHARACTERISTIC(BT_UUID_CCID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_ccid,      \
			       NULL, UINT_TO_POINTER(_n))

/* BT_GATT_PRIMARY_SERVICE only works in the global scope */
static struct bt_gatt_attr test_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_TBS),
	LISTIFY(MAX_CCID_CNT, CCID_DEFINE, (,)),
};

static void test_bt_ccid_alloc_value_all_allocated(void **state)
{
	(void)state;
	struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);
	int ret;

	assert_int_equal(bt_gatt_service_register(&test_svc), 0);

	/* Verify that CCID allocation fails if we have 256 characteristics with it */
	ret = bt_ccid_alloc_value();

	assert_int_equal(bt_gatt_service_unregister(&test_svc), 0);
	assert_int_equal(ret, -ENOMEM);
}

static void test_bt_ccid_find_attr(void **state)
{
	(void)state;
	struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);

	/* Service not registered, shall fail */
	assert_null(bt_ccid_find_attr(0));

	assert_int_equal(bt_gatt_service_register(&test_svc), 0);

	/* Service registered, shall not fail */
	assert_non_null(bt_ccid_find_attr(0));

	assert_int_equal(bt_gatt_service_unregister(&test_svc), 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_bt_ccid_alloc_value),
		cmocka_unit_test(test_bt_ccid_alloc_value_more_than_max),
		cmocka_unit_test(test_bt_ccid_alloc_value_all_allocated),
		cmocka_unit_test(test_bt_ccid_find_attr),
	};

	return cmocka_run_group_tests_name("bt_audio_ccid", tests, NULL, NULL);
}
