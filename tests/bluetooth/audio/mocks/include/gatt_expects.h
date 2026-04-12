/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_GATT_EXPECTS_H_
#define MOCKS_GATT_EXPECTS_H_

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <zephyr/bluetooth/gatt.h>

#include "gatt.h"
#include "expects_util.h"

#define expect_bt_gatt_notify_cb_called_once(_conn, _uuid, _attr, _data, _len)                     \
do {                                                                                               \
	const char *func_name = "bt_gatt_notify_cb";                                               \
	struct bt_gatt_notify_params *params;                                                      \
	if ((_conn) != NULL) {                                                                     \
		assert_ptr_equal(_conn, mock_bt_gatt_notify_cb_fake.arg0_val);                     \
	}                                                                                          \
	params = mock_bt_gatt_notify_cb_fake.arg1_val;                                             \
	/* params->uuid is optional */                                                             \
	if (params->uuid) {                                                                        \
		if ((_uuid) != NULL) {                                                              \
			assert_true(bt_uuid_cmp(_uuid, params->uuid) == 0);                         \
		}                                                                                  \
	} else {                                                                                   \
		if ((_attr) != NULL) {                                                              \
			assert_ptr_equal(_attr, params->attr);                                      \
		}                                                                                  \
	}                                                                                          \
	/* assert if _data is valid, but _len is empty */                                          \
	if ((_len) == 0U && (_data) != NULL) {                                                     \
		fail();                                                                               \
	}                                                                                          \
	if ((_len) != 0U) {                                                                        \
		assert_int_equal(_len, params->len);                                                \
		expect_data(func_name, "params->data", _data, params->data, _len);              \
	}                                                                                          \
} while (0)

static inline void expect_bt_gatt_notify_cb_not_called(void)
{
	const char *func_name = "bt_gatt_notify_cb";

	assert_int_equal(0, mock_bt_gatt_notify_cb_fake.call_count);
}

#endif /* MOCKS_GATT_EXPECTS_H_ */
