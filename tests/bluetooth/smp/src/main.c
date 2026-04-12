/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>

#include <bluetooth/conn.h>
#include <bluetooth/host/smp.h>

static void test_bt_smp_err_to_str(void **state)
{
	(void)state;

	/* Test a couple of entries */
	assert_string_equal(bt_smp_err_to_str(0x00), "BT_SMP_ERR_SUCCESS");
	assert_string_equal(bt_smp_err_to_str(0x0a), "BT_SMP_ERR_INVALID_PARAMS");
	assert_string_equal(bt_smp_err_to_str(0x0F), "BT_SMP_ERR_KEY_REJECTED");

	/* Test entries that are not used */
	assert_memory_equal(bt_smp_err_to_str(0x10), "(unknown)", strlen("(unknown)"));
	assert_memory_equal(bt_smp_err_to_str(0xFF), "(unknown)", strlen("(unknown)"));

	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		assert_non_null(bt_smp_err_to_str((uint8_t)i));
	}
}

static void test_bt_security_err_to_str(void **state)
{
	(void)state;

	/* Test a couple of entries */
	assert_string_equal(bt_security_err_to_str(BT_SECURITY_ERR_AUTH_FAIL),
			   "BT_SECURITY_ERR_AUTH_FAIL");
	assert_string_equal(bt_security_err_to_str(BT_SECURITY_ERR_KEY_REJECTED),
			   "BT_SECURITY_ERR_KEY_REJECTED");
	assert_string_equal(bt_security_err_to_str(BT_SECURITY_ERR_UNSPECIFIED),
			   "BT_SECURITY_ERR_UNSPECIFIED");

	/* Test outside range */
	assert_string_equal(bt_security_err_to_str(BT_SECURITY_ERR_UNSPECIFIED + 1),
			   "(unknown)");

	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		assert_non_null(bt_security_err_to_str((enum bt_security_err)i));
	}
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_bt_smp_err_to_str),
		cmocka_unit_test(test_bt_security_err_to_str),
	};

	return cmocka_run_group_tests_name("bt_smp", tests, NULL, NULL);
}
