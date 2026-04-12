/* main.c - Application main entry point */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <bluetooth/att.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/pacs.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>
#include <utils/bt_utils.h>

static int test_case_teardown(void **state)
{
	(void)state;

	/* attempt to clean up after any failures */
	(void)bt_pacs_unregister();

	return 0;
}

/* Helper macro to define parameters ignoring unsupported features */
#define PACS_REGISTER_PARAM(_snk_pac, _snk_loc, _src_pac, _src_loc)                                \
	(struct bt_pacs_register_param)                                                            \
	{                                                                                          \
		IF_ENABLED(CONFIG_BT_PAC_SNK, (.snk_pac = (_snk_pac),))                            \
		IF_ENABLED(CONFIG_BT_PAC_SNK_LOC, (.snk_loc = (_snk_loc),))                        \
		IF_ENABLED(CONFIG_BT_PAC_SRC, (.src_pac = (_src_pac),))                            \
		IF_ENABLED(CONFIG_BT_PAC_SRC_LOC, (.src_loc = (_src_loc),))                        \
	}

static void test_pacs_register(void **state)
{
	(void)state;
	const struct bt_pacs_register_param pacs_params[] = {
#if defined(CONFIG_BT_PAC_SNK)
		/* valid snk_pac combinations */
		PACS_REGISTER_PARAM(true, true, true, true),
		PACS_REGISTER_PARAM(true, true, true, false),
		PACS_REGISTER_PARAM(true, true, false, false),
		PACS_REGISTER_PARAM(true, false, true, true),
		PACS_REGISTER_PARAM(true, false, true, false),
		PACS_REGISTER_PARAM(true, false, false, false),
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
		/* valid src_pac combinations */
		PACS_REGISTER_PARAM(true, true, true, true),
		PACS_REGISTER_PARAM(true, false, true, true),
		PACS_REGISTER_PARAM(false, false, true, true),
		PACS_REGISTER_PARAM(true, true, true, false),
		PACS_REGISTER_PARAM(true, false, true, false),
		PACS_REGISTER_PARAM(false, false, true, false),
#endif /* CONFIG_BT_PAC_SRC */
	};

	for (size_t i = 0U; i < ARRAY_SIZE(pacs_params); i++) {
		struct bt_gatt_attr *attr;
		int err;

		err = bt_pacs_register(&pacs_params[i]);
		assert_int_equal(err, 0);

#if defined(CONFIG_BT_PAC_SNK)
		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK);
		if (pacs_params[i].snk_pac) {
			assert_non_null(attr);
		} else {
			assert_null(attr);
		}
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SNK_LOC)
		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK_LOC);
		if (pacs_params[i].snk_loc) {
			assert_non_null(attr);
		} else {
			assert_null(attr);
		}
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC)
		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC);
		if (pacs_params[i].src_pac) {
			assert_non_null(attr);
		} else {
			assert_null(attr);
		}
#endif /* CONFIG_BT_PAC_SRC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC_LOC);
		if (pacs_params[i].src_loc) {
			assert_non_null(attr);
		} else {
			assert_null(attr);
		}
#endif /* CONFIG_BT_PAC_SRC_LOC */

		err = bt_pacs_unregister();
		assert_int_equal(err, 0);

		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK);
		assert_null(attr);

		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK_LOC);
		assert_null(attr);

		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC);
		assert_null(attr);

		attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC_LOC);
		assert_null(attr);
	}
}

static void test_pacs_register_inval_null_param(void **state)
{
	(void)state;

	assert_int_equal(bt_pacs_register(NULL), -EINVAL);
}

static void test_pacs_register_inval_double_register(void **state)
{
	(void)state;
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);

	assert_int_equal(bt_pacs_register(&pacs_param), 0);
	assert_int_equal(bt_pacs_register(&pacs_param), -EALREADY);
}

static void test_pacs_register_inval_snk_loc_without_snk_pac(void **state)
{
	(void)state;
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(false, true, true, true);

	if (!(IS_ENABLED(CONFIG_BT_PAC_SNK) && IS_ENABLED(CONFIG_BT_PAC_SNK_LOC))) {
		skip();
	}

	assert_int_equal(bt_pacs_register(&pacs_param), -EINVAL);
}

static void test_pacs_register_inval_src_loc_without_src_pac(void **state)
{
	(void)state;
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, false, true);

	if (!(IS_ENABLED(CONFIG_BT_PAC_SRC) && IS_ENABLED(CONFIG_BT_PAC_SRC_LOC))) {
		skip();
	}

	assert_int_equal(bt_pacs_register(&pacs_param), -EINVAL);
}

static void test_pacs_register_inval_no_pac(void **state)
{
	(void)state;
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(false, false, false, false);

	if (!(IS_ENABLED(CONFIG_BT_PAC_SNK) && IS_ENABLED(CONFIG_BT_PAC_SNK_LOC))) {
		skip();
	}

	assert_int_equal(bt_pacs_register(&pacs_param), -EINVAL);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_teardown(test_pacs_register, test_case_teardown),
		cmocka_unit_test_teardown(test_pacs_register_inval_null_param, test_case_teardown),
		cmocka_unit_test_teardown(test_pacs_register_inval_double_register,
						 test_case_teardown),
		cmocka_unit_test_teardown(test_pacs_register_inval_snk_loc_without_snk_pac,
						 test_case_teardown),
		cmocka_unit_test_teardown(test_pacs_register_inval_src_loc_without_src_pac,
						 test_case_teardown),
		cmocka_unit_test_teardown(test_pacs_register_inval_no_pac, test_case_teardown),
	};

	return cmocka_run_group_tests_name("bt_audio_pacs", tests, NULL, NULL);
}
