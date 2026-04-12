/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include <cmocka.h>

#include <base/bt_buf.h>
#include <bluetooth/host/classic/at.h>

#include <osdep/os.h>

static struct at_client at;

BT_BUF_POOL_DEFINE(at_pool, 1, 140, 0, NULL);

static const char example_data[] = "\r\n+ABCD:999\r\n";

static int at_handle(struct at_client *hf_at)
{
	uint32_t val;

	assert_int_equal(at_get_number(hf_at, &val), 0);

	assert_int_equal(val, 999);

	return 0;
}

static int at_resp(struct at_client *hf_at, struct bt_buf *buf)
{
	int err;

	err = at_parse_cmd_input(hf_at, buf, "ABCD", at_handle,
				 AT_CMD_TYPE_NORMAL);
	assert_int_equal(err, 0);

	return 0;
}

static void test_at(void **state)
{
	struct bt_buf *buf;
	size_t len;

	(void)state;
	memset(&at, 0, sizeof(at));

	buf = bt_buf_alloc(&at_pool, OS_TIMEOUT_FOREVER);
	assert_non_null(buf);

	at_register(&at, at_resp, NULL);
	len = strlen(example_data);

	assert_true(bt_buf_tailroom(buf) >= len);
	bt_buf_add_mem(buf, example_data, len);

	assert_int_equal(at_parse_input(&at, buf), 0);

	bt_buf_unref(buf);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_at),
	};

	return cmocka_run_group_tests_name("bt_at", tests, NULL, NULL);
}
