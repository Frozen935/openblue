/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <bluetooth/cs.h>

/*
 *  Test uninitialized chmap buffer is populated correctly
 *
 *  Expected behaviour:
 *   - test_chmap matches correct_chmap
 */
static void test_uninitialized_chmap(void **state)
{
	(void)state;
	uint8_t test_chmap[10];

	bt_le_cs_set_valid_chmap_bits(test_chmap);

	uint8_t correct_chmap[10] = {0xFC, 0xFF, 0x7F, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F};

	assert_memory_equal(test_chmap, correct_chmap, 10);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_uninitialized_chmap),
	};

	return cmocka_run_group_tests_name("bt_le_cs_set_valid_chmap_bits", tests, NULL, NULL);
}
