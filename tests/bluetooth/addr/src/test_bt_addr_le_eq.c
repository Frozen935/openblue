/* Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>

#include <bluetooth/addr.h>

static void test_all_zero(void **state)
{
	(void)state;

	bt_addr_le_t a = {.type = 0, .a = {{0, 0, 0, 0, 0, 0}}};
	bt_addr_le_t b = a;

	assert_true(bt_addr_le_eq(&a, &b));
}

static void test_type_not_zero(void **state)
{
	(void)state;

	bt_addr_le_t a = {.type = 1, .a = {{1, 2, 3, 4, 5, 6}}};
	bt_addr_le_t b = a;

	assert_true(bt_addr_le_eq(&a, &b));
}

static void test_type_matters(void **state)
{
	(void)state;

	bt_addr_le_t a = {.type = 0, .a = {{1, 2, 3, 4, 5, 6}}};
	bt_addr_le_t b = a;

	assert_true(bt_addr_le_eq(&a, &b));
	a.type = 1;
	assert_false(bt_addr_le_eq(&a, &b));
}

static void test_address_matters_start(void **state)
{
	(void)state;

	bt_addr_le_t a = {.type = 0, .a = {{1, 2, 3, 4, 5, 6}}};
	bt_addr_le_t b = a;

	assert_true(bt_addr_le_eq(&a, &b));
	a.a.val[0] = 0;
	assert_false(bt_addr_le_eq(&a, &b));
}

static void test_address_matters_end(void **state)
{
	(void)state;

	bt_addr_le_t a = {.type = 0, .a = {{1, 2, 3, 4, 5, 6}}};
	bt_addr_le_t b = a;

	assert_true(bt_addr_le_eq(&a, &b));
	a.a.val[5] = 0;
	assert_false(bt_addr_le_eq(&a, &b));
}

static void test_only_type_and_address_matters(void **state)
{
	(void)state;

	bt_addr_le_t a;
	bt_addr_le_t b;

	/* Make anything that is not the type and address unequal bytes. */
	memset(&a, 0xaa, sizeof(a));
	memset(&b, 0xbb, sizeof(b));
	a.type = 1;
	b.type = 1;
	memset(a.a.val, 1, sizeof(a.a.val));
	memset(b.a.val, 1, sizeof(b.a.val));

	assert_true(bt_addr_le_eq(&a, &b));
}

static void test_same_object(void **state)
{
	(void)state;

	bt_addr_le_t a = {.type = 0, .a = {{1, 2, 3, 4, 5, 6}}};

	assert_true(bt_addr_le_eq(&a, &a));
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_all_zero),
		cmocka_unit_test(test_type_not_zero),
		cmocka_unit_test(test_type_matters),
		cmocka_unit_test(test_address_matters_start),
		cmocka_unit_test(test_address_matters_end),
		cmocka_unit_test(test_only_type_and_address_matters),
		cmocka_unit_test(test_same_object),
	};

	return cmocka_run_group_tests_name("bt_addr_le_eq", tests, NULL, NULL);
}
