/* main.c - Application main entry point */

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

#include <bluetooth/hci.h>

static void test_bt_hci_err_to_str_case(void **state)
{
	(void)state;

	/* Test a couple of entries */
	assert_memory_equal(bt_hci_err_to_str(BT_HCI_ERR_CONN_TIMEOUT),
			    "BT_HCI_ERR_CONN_TIMEOUT", strlen("BT_HCI_ERR_CONN_TIMEOUT"));
	assert_memory_equal(bt_hci_err_to_str(BT_HCI_ERR_REMOTE_USER_TERM_CONN),
			    "BT_HCI_ERR_REMOTE_USER_TERM_CONN",
			    strlen("BT_HCI_ERR_REMOTE_USER_TERM_CONN"));
	assert_memory_equal(bt_hci_err_to_str(BT_HCI_ERR_TOO_EARLY),
			    "BT_HCI_ERR_TOO_EARLY", strlen("BT_HCI_ERR_TOO_EARLY"));

	/* Test a entries that is not used */
	assert_memory_equal(bt_hci_err_to_str(0x2b),
			    "(unknown)", strlen("(unknown)"));
	assert_memory_equal(bt_hci_err_to_str(0xFF),
			    "(unknown)", strlen("(unknown)"));

	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		assert_non_null(bt_hci_err_to_str(i));
	}
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_bt_hci_err_to_str_case),
	};

	return cmocka_run_group_tests_name("bt_hci", tests, NULL, NULL);
}
