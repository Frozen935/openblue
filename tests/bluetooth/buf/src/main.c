/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <bluetooth/buf.h>
#include <bluetooth/hci.h>

#include <base/bt_buf.h>
#include <osdep/os.h>

static enum bt_buf_type freed_buf_type;
static os_sem_t rx_sem;

static void bt_buf_rx_freed_cb(enum bt_buf_type type)
{
	freed_buf_type = type;
	assert_int_equal(os_sem_give(&rx_sem), 0);
}

static void expect_freed_notification(enum bt_buf_type type, const char *message)
{
	struct bt_buf *buf;
	int err;

	freed_buf_type = BT_BUF_TYPE_NONE;
	assert_int_equal(os_sem_reset(&rx_sem), 0);

	buf = bt_buf_get_rx(type, OS_TIMEOUT_NO_WAIT);
	assert_non_null(buf);

	bt_buf_unref(buf);

	/* The freed callback is triggered by bt_buf_unref(), so the semaphore must
	 * already be available here.
	 */
	err = os_sem_take(&rx_sem, OS_TIMEOUT_NO_WAIT);
	assert_int_equal(err, 0);
	if ((freed_buf_type & type) != type) {
		fail_msg("%s (mask=0x%x expected=0x%x)", message, freed_buf_type, type);
	}
}

static int group_setup(void **state)
{
	(void)state;
	return os_sem_init(&rx_sem, 0, 1);
}

static void test_buf_freed_cb(void **state)
{
	(void)state;

	bt_buf_rx_freed_cb_set(bt_buf_rx_freed_cb);

	/* Test that the callback is called for the BT_BUF_EVT type */
	expect_freed_notification(BT_BUF_EVT, "Event buffer wasn't freed");

	/* Test that the callback is called for the BT_BUF_ACL_IN type */
	expect_freed_notification(BT_BUF_ACL_IN, "ACL buffer wasn't freed");

	/* Test that the callback is called for the BT_BUF_ISO_IN type */
	expect_freed_notification(BT_BUF_ISO_IN, "ISO buffer wasn't freed");
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_buf_freed_cb),
	};

	return cmocka_run_group_tests_name("bt_buf", tests, group_setup, NULL);
}
